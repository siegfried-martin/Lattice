#include "war/spies.hpp"

#include <algorithm>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "pirates/suspicion.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"
#include "trade/merchants.hpp"
#include "war/armies.hpp"
#include "war/combat.hpp"
#include "war/raids.hpp"
#include "war/threats.hpp"

namespace lattice::spies {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

Id home_of(const World& w, Id id) {
    const Id gov = employer(w, id);
    return gov == kNone ? kNone : w.characters[at(gov)].governs;
}

}  // namespace

Id employer(const World& w, Id id) {
    const Id k = contracts::held(w, id, ContractKind::Spy);
    return k == kNone ? kNone : w.contracts[at(k)].employer;
}

std::vector<Id> candidates(const World& w, Id planet) {
    std::vector<Id> out;
    for (Id id = 0; id < static_cast<Id>(w.characters.size()); ++id) {
        const Character& c = w.characters[at(id)];
        if (!c.alive || c.pirate || c.governs != kNone || c.planet == kNone || c.ships.empty() || c.mission != kNone) continue;
        if (c.activity != Activity::Idle || !c.contracts.empty() || c.offer != kNone) continue;
        if (ships::fleet(w, c).cargo <= 0 || w.derived.travel(c.planet, planet) > w.config.hire.range_days) continue;
        out.push_back(id);
    }
    return out;
}

std::vector<Id> employed(const World& w, Id governor) {
    std::vector<Id> out;
    for (Id k : w.characters[at(governor)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (ct.alive && ct.kind == ContractKind::Spy && ct.employer == governor) out.push_back(ct.employee);
    }
    return out;
}

void hire(World& w, Id planet, Id id, Observer* observer) {
    contracts::sign(w, ContractKind::Spy, w.planets[at(planet)].governor, id, w.config.spy.salary, w.config.time.payment_days * kDay);
    w.characters[at(id)].activity = Activity::Spying;
    ++w.ledger.spies_hired;
    chars::wake(w, id, w.now);
    if (observer) observer->log(w.now, "spy.hired", id, "by " + std::to_string(planet));
}

void release(World& w, Id id) {
    contracts::end(w, contracts::held(w, id, ContractKind::Spy));
    Character& c = w.characters[at(id)];
    if (c.activity == Activity::Spying) c.activity = Activity::Idle;
}

void next(World& w, Id id, Observer*) {
    Character& c = w.characters[at(id)];
    const Id home = home_of(w, id);
    if (home == kNone) {
        c.activity = Activity::Idle;
        chars::wake(w, id, w.now);
        return;
    }
    c.activity = Activity::Spying;
    if (c.planet != home) {
        if (!move::travel_to(w, id, home)) chars::wake(w, id, w.now + kDay);
        return;
    }
    // The enemy market it has seen least recently, as its employer believes the war.
    const std::vector<Id> enemies = raids::believed_enemies(w, home);
    Id target = kNone;
    Time oldest = 0;
    for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
        if (std::find(enemies.begin(), enemies.end(), fealty::root_of(w, q)) == enemies.end()) continue;
        if (w.derived.travel(home, q) > w.config.spy.range_days) continue;
        const PriceFact* f = info::find(c.facts, q);
        const Time seen = f ? f->time : kNever;
        if (target == kNone || seen < oldest) target = q, oldest = seen;
    }
    if (target == kNone || !move::travel_to(w, id, target)) chars::wake(w, id, w.now + 7 * kDay);
}

void landed(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    const Id home = home_of(w, id);
    if (home == kNone) {
        c.activity = Activity::Idle;
        chars::wake(w, id, w.now);
        return;
    }
    if (c.planet == home) {
        merchant::sell_here(w, id);
        Facts& known = info::governor_facts(w, home);
        info::share(c.facts, known);  // the secret contract: it reports home (INF-4)
        threats::check(w, home, observer);
        chars::wake(w, id, w.now + static_cast<Time>(w.config.merchant.dock_hours * kHour));
        return;
    }
    ++w.ledger.spy_visits;
    const Id root = fealty::root_of(w, c.planet);
    // Cover: a genuine trade, buying for the market at home (INF-14: good spies are good
    // traders).
    const double before = c.cargo_qty;
    merchant::sell_here(w, id);
    merchant::buy_for(w, id, home);
    if (c.cargo_qty <= before) suspicion::add(w, c, root, w.config.sus.no_trade);
    // What it sees: armies forming or stationed here.
    for (Id a = 0; a < static_cast<Id>(w.armies.size()); ++a) {
        const Army& army = w.armies[at(a)];
        if (!army.alive || army.planet != c.planet) continue;
        info::add_sighting(c.facts, {.army = a, .time = w.now, .x = w.planets[at(c.planet)].x, .y = w.planets[at(c.planet)].y,
                                     .target = army.target, .faction = fealty::root_of(w, army.home), .power = armies::power(w, army)});
        if (army.action == ArmyAction::Muster) {
            ++w.ledger.musters_seen;
            suspicion::add(w, c, root, w.config.sus.muster);
        }
    }
    chars::wake(w, id, w.now + static_cast<Time>(w.config.merchant.dock_hours * kHour));
}

void caught(World& w, Id id, Id planet, Observer* observer) {
    const Id gov = w.planets[at(planet)].governor;
    release(w, id);
    opinion::deed(w, gov, id, DeedKind::Caught, observer);
    suspicion::add(w, w.characters[at(id)], fealty::root_of(w, planet), w.config.sus.caught);
    ++w.ledger.spies_caught;
    if (observer) observer->log(w.now, "spy.caught", id, "at " + std::to_string(planet));
    // Attacked by the planet's commanders at home, if it has any (INF-13).
    std::vector<Id> guards;
    for (Id k : w.characters[at(gov)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.kind != ContractKind::Service || ct.employer != gov) continue;
        const Character& g = w.characters[at(ct.employee)];
        if (g.activity == Activity::Serving && g.planet == planet && g.battle == kNone && g.army == kNone) guards.push_back(ct.employee);
    }
    if (!guards.empty()) combat::start(w, guards, {id}, planet, false, kNone, kNone, observer);
}

void pay(World& w, Id planet, Observer*) {
    const Id gov = w.planets[at(planet)].governor;
    if (gov == kNone) return;
    for (Id id : employed(w, gov)) {
        const double salary = w.config.spy.salary;
        w.planets[at(planet)].treasury -= salary;
        chars::earn(w.characters[at(id)], salary, false);
        w.ledger.spy_pay += salary;
    }
}

}  // namespace lattice::spies
