#include "politics/roles.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "pirates/pirates.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"
#include "trade/merchants.hpp"
#include "war/armies.hpp"
#include "war/diplomacy.hpp"

namespace lattice::roles {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

const Config::FitWeights& weights(const Config& c, Post post) {
    switch (post) {
        case Post::Governor: return c.fit.governor;
        case Post::Steward: return c.fit.steward;
        default: return c.fit.commander;
    }
}

double monthly_net(const World& w, Id planet) {
    return std::max(0.0, w.planets[at(planet)].net_avg) * static_cast<double>(w.config.time.payment_days);
}

// A governor's sworn commanders, free to take a seat (not with an army or in a fight).
std::vector<Id> sworn_free(const World& w, Id governor) {
    std::vector<Id> out;
    for (Id k : w.characters[at(governor)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.kind != ContractKind::Service || ct.employer != governor) continue;
        const Character& c = w.characters[at(ct.employee)];
        if (c.alive && c.planet != kNone && c.army == kNone && c.battle == kNone) out.push_back(ct.employee);
    }
    return out;
}

// The heir takes over what the dead governor ran: its contracts as employer (commanders,
// steward, spies, merchants), the missions it posted, the armies it raised, and what it
// knew (the planet's information).
void inherit(World& w, Id dead, Id heir) {
    Character& d = w.characters[at(dead)];
    Character& h = w.characters[at(heir)];
    for (Id k : std::vector<Id>(d.contracts)) {
        Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.employer != dead || ct.employee == heir) continue;
        ct.employer = heir;
        h.contracts.push_back(k);
        std::erase(d.contracts, k);
    }
    for (Mission& m : w.missions) {
        if (m.poster == dead && (m.state == MissionState::Open || m.state == MissionState::Taken)) m.poster = heir;
    }
    for (Army& a : w.armies) {
        if (a.alive && a.commissioner == dead) a.commissioner = heir;
    }
    info::merge_into(h.facts, d.facts);
}

// Fills a vacant seat and closes the vacancy.
void succeed(World& w, Id planet, Id heir, Observer* observer) {
    Planet& p = w.planets[at(planet)];
    const Id dead = p.governor;
    if (heir == kNone) heir = generate(w, planet);
    seat(w, planet, heir, observer);
    inherit(w, dead, heir);
    w.characters[at(dead)].governs = kNone;
    ++w.ledger.successions;
    w.ledger.vacant_days += static_cast<double>(w.now - p.vacant_since) / kDay;
    p.vacant_since = kNever;
    p.succession_at = kNever;
    if (observer) observer->log(w.now, "governor.succeeded", planet, "by " + std::to_string(heir) + " after " + std::to_string(dead));
}

// The pool a chooser fills a seat from (SYS-22): its sworn commanders and the planet's
// steward, else the fallback.
std::vector<Id> pool_for(const World& w, Id chooser, Id planet) {
    std::vector<Id> pool = sworn_free(w, chooser);
    const Planet& p = w.planets[at(planet)];
    if (p.steward != kNone) pool.push_back(p.steward);
    std::erase_if(pool, [&](Id id) { return w.characters[at(id)].pirate != p.pirate; });
    if (pool.empty()) pool = fallback_pool(w, planet);
    return pool;
}

}  // namespace

double fitness(const World& w, Post post, Id chooser, Id candidate, Id planet) {
    const Config& c = w.config;
    const Config::FitWeights& wt = weights(c, post);
    const Character& ch = w.characters[at(candidate)];
    const ships::Fleet f = ships::fleet(w, ch);
    const double rep = std::max(0.0, ch.reputation);
    const double opinion = chooser == kNone ? 0 : opinion::get(w, chooser, candidate);
    return wt.power * f.combat / (f.combat + c.value.power_ref) + wt.trade * f.cargo / (f.cargo + c.fit.cargo_half) +
           wt.rep * rep / (rep + c.hire.rep_half) + wt.opinion * opinion / 100 +
           wt.culture * (ch.culture == w.planets[at(planet)].region ? 1 : 0);
}

bool vacant(const World& w, Id planet) { return w.planets[at(planet)].vacant_since != kNever; }

bool home(const World& w, Id planet) {
    const Id gov = w.planets[at(planet)].governor;
    if (gov == kNone) return false;
    const Character& g = w.characters[at(gov)];
    return g.alive && g.planet == planet;
}

Id in_charge(const World& w, Id planet) {
    const Planet& p = w.planets[at(planet)];
    if (home(w, planet) || p.steward == kNone) return p.governor;
    return p.steward;
}

double governor_risk(const World& w, Id planet) {
    const Planet& p = w.planets[at(planet)];
    if (vacant(w, planet) && p.steward != kNone) return w.characters[at(p.steward)].risk;
    return w.characters[at(p.governor)].risk;
}

double steward_risk(const World& w, Id planet) {
    const Planet& p = w.planets[at(planet)];
    return w.characters[at(p.steward != kNone ? p.steward : p.governor)].risk;
}

bool can_sail(const World& w, Id planet) {
    const Planet& p = w.planets[at(planet)];
    return !p.pirate && !vacant(w, planet) && p.steward != kNone && w.characters[at(p.steward)].planet == planet;
}

void handover(World& w, Id planet) {
    const Planet& p = w.planets[at(planet)];
    if (p.steward == kNone || p.governor == kNone) return;
    info::share(w.characters[at(p.governor)].facts, w.characters[at(p.steward)].facts);
}

void set_growth(World& w, Id planet) {
    Planet& p = w.planets[at(planet)];
    if (p.governor == kNone) return;
    const double risk = w.characters[at(p.steward != kNone ? p.steward : p.governor)].risk;
    p.growth_factor = w.config.pop.growth_factor_min + risk * (w.config.pop.growth_factor_max - w.config.pop.growth_factor_min);
}

void record_path(Character& c) {
    if (c.earned_cargo <= 0 && c.earned_combat <= 0) {
        c.path = static_cast<std::uint8_t>(EarnPath::None);
    } else {
        c.path = static_cast<std::uint8_t>(c.earned_cargo >= c.earned_combat ? EarnPath::Trade : EarnPath::Combat);
    }
}

void seat(World& w, Id planet, Id id, Observer* observer) {
    Planet& p = w.planets[at(planet)];
    Character& g = w.characters[at(id)];
    record_path(g);
    // Its own jobs end: a commander's service, a merchant's salary, a steward's post.
    for (Id k : std::vector<Id>(g.contracts)) {
        if (w.contracts[at(k)].employee == id) contracts::end(w, k);
    }
    if (g.army != kNone) armies::leave(w, g.army, id);
    g.army = kNone;
    g.trip.clear();
    g.governs = planet;
    p.governor = id;
    set_growth(w, planet);
    if (g.planet == planet) {
        g.activity = Activity::Governing;
    } else {
        g.activity = Activity::Idle;
        chars::wake(w, id, w.now);  // it goes home (SYS-22)
    }
    if (observer) observer->log(w.now, "governor.seated", planet, "character " + std::to_string(id));
}

Id best_governor(const World& w, const std::vector<Id>& pool, Id chooser, Id planet, Id bonus_for) {
    Id best = kNone;
    double best_score = -std::numeric_limits<double>::max();
    for (Id id : pool) {
        const Character& c = w.characters[at(id)];
        if (!c.alive || c.governs != kNone) continue;
        const double score = fitness(w, Post::Governor, chooser, id, planet) + (id == bonus_for ? w.config.fealty.capture_bonus : 0);
        if (score > best_score || (score == best_score && id < best)) best_score = score, best = id;
    }
    return best;
}

std::vector<Id> fallback_pool(const World& w, Id planet) {
    std::vector<Id> out;
    const bool pirate = w.planets[at(planet)].pirate;
    for (Id id = 0; id < static_cast<Id>(w.characters.size()); ++id) {
        const Character& c = w.characters[at(id)];
        if (!c.alive || c.governs != kNone || c.planet == kNone || c.pirate != pirate) continue;
        if (c.mission != kNone || c.army != kNone || c.battle != kNone || c.escorting != kNone || c.escort != kNone) continue;
        if (c.activity == Activity::Renouncing || c.activity == Activity::Spying) continue;
        if (w.derived.travel(c.planet, planet) > w.config.hire.range_days) continue;
        out.push_back(id);
    }
    return out;
}

Id generate(World& w, Id planet) {
    const Id id = chars::spawn_ruler(w, planet);
    if (w.planets[at(planet)].pirate) {
        Character& c = w.characters[at(id)];
        c.pirate = true;
        pirates::set_relation(w, c, 100);
    }
    return id;
}

void steward_weekly(World& w, Id planet, Observer* observer) {
    Planet& p = w.planets[at(planet)];
    const Config& c = w.config;
    const double salary = c.steward.salary_share * monthly_net(w, planet);
    if (p.steward != kNone) {
        const Id k = contracts::held(w, p.steward, ContractKind::Steward);
        if (k != kNone) w.contracts[at(k)].pay = salary;  // it follows income, as stipends follow power
        return;
    }
    if (!c.steward.enabled || p.pirate || vacant(w, planet) || salary <= 0) return;
    const Id gov = p.governor;
    std::vector<std::pair<double, Id>> scored;
    for (Id id = 0; id < static_cast<Id>(w.characters.size()); ++id) {
        const Character& ch = w.characters[at(id)];
        if (!ch.alive || ch.governs != kNone || ch.stewards != kNone || ch.pirate || ch.planet == kNone) continue;
        if (ch.mission != kNone || ch.army != kNone || ch.battle != kNone || ch.escorting != kNone || ch.escort != kNone ||
            ch.cargo_qty > 0) {
            continue;
        }
        if (ch.activity != Activity::Idle && ch.activity != Activity::Trading && ch.activity != Activity::Exploring) continue;
        if (w.derived.travel(ch.planet, planet) > c.hire.range_days) continue;
        bool employed = false;
        for (ContractKind k : {ContractKind::Service, ContractKind::MerchantSalary, ContractKind::Steward, ContractKind::Spy}) {
            employed = employed || contracts::held(w, id, k) != kNone;
        }
        if (employed) continue;
        scored.emplace_back(-fitness(w, Post::Steward, gov, id, planet), id);
    }
    std::sort(scored.begin(), scored.end());
    for (auto [score, id] : scored) {
        Character& ch = w.characters[at(id)];
        if (salary <= ch.earned_cargo + ch.earned_combat) continue;  // it makes more on its own
        record_path(ch);
        contracts::sign(w, ContractKind::Steward, gov, id, salary, c.time.payment_days * kDay);
        p.steward = id;
        ch.stewards = planet;
        ch.activity = Activity::Stewarding;
        ch.trip.clear();
        ch.salary_checked = w.now;
        set_growth(w, planet);
        if (home(w, planet)) handover(w, planet);
        ++w.ledger.stewards_appointed;
        chars::wake(w, id, w.now);
        if (observer) observer->log(w.now, "steward.appointed", planet, "character " + std::to_string(id));
        return;
    }
}

void steward_decide(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    const Id planet = c.stewards;
    if (c.planet != planet) {
        if (!move::travel_to(w, id, planet)) chars::wake(w, id, w.now + kDay);
        return;
    }
    c.activity = Activity::Stewarding;
    const Id k = contracts::held(w, id, ContractKind::Steward);
    if (k == kNone) {  // its contract ended under it
        c.stewards = kNone;
        c.activity = Activity::Idle;
        chars::wake(w, id, w.now);
        return;
    }
    if (w.now - c.salary_checked >= w.config.time.payment_days * kDay && !c.ships.empty()) {
        c.salary_checked = w.now;
        const double per_day = w.contracts[at(k)].pay / static_cast<double>(w.config.time.payment_days);
        const double trade = merchant::best_trip(w, c, kNone).score;
        if (std::isfinite(trade) && trade > per_day) {
            contracts::end(w, k);
            ++w.ledger.steward_quits;
            if (observer) observer->log(w.now, "steward.quit", planet, "character " + std::to_string(id));
            chars::wake(w, id, w.now);
            return;
        }
    }
    chars::wake(w, id, w.now + 7 * kDay);
}

void governor_died(World& w, Id planet, Observer* observer) {
    Planet& p = w.planets[at(planet)];
    const Id dead = p.governor;
    p.vacant_since = w.now;
    p.governor_stipend = 0;
    ++w.ledger.governors_killed;
    if (observer) observer->log(w.now, "governor.died", planet, "character " + std::to_string(dead));
    if (p.liege != kNone) {
        // A vassal: its liege hears by courier, and chooses at its next weekly decision.
        const double trip = w.derived.travel(planet, p.liege) + w.derived.travel(p.liege, planet);
        p.succession_at = w.now + static_cast<Time>(trip * kDay);
        return;
    }
    // A root: its steward takes over at once, and so does the faction (FAC-17).
    if (p.steward != kNone) {
        succeed(w, planet, p.steward, observer);
        return;
    }
    const std::vector<Id> below = fealty::vassals(w, planet);
    if (!below.empty()) {
        // The vassal most loyal to the dead root becomes root, as at capture (SYS-13), and
        // fills the seat from its own pool, keeping the planet under itself.
        Id heir = below[0];
        double best = -std::numeric_limits<double>::max();
        for (Id v : below) {
            const double o = opinion::get(w, w.planets[at(v)].governor, dead);
            if (o > best) best = o, heir = v;
        }
        fealty::swear(w, heir, kNone);
        for (Id v : below) {
            if (v != heir) fealty::swear(w, v, heir);
        }
        diplomacy::move_root(w, planet, heir);
        fealty::swear(w, planet, heir);
        const Id chooser = w.planets[at(heir)].governor;
        succeed(w, planet, best_governor(w, pool_for(w, chooser, planet), chooser, planet), observer);
        return;
    }
    // Alone: its best commander, else the fallback (SYS-22).
    std::vector<Id> pool = sworn_free(w, dead);
    if (pool.empty()) pool = fallback_pool(w, planet);
    succeed(w, planet, best_governor(w, pool, dead, planet), observer);
}

bool settle(World& w, Id planet, Observer* observer) {
    const Planet& p = w.planets[at(planet)];
    if (!vacant(w, planet)) return false;
    if (w.now < p.succession_at) return true;
    if (p.liege == kNone) {
        // It became a root while it waited (its liege was taken or fell): fill it now.
        std::vector<Id> pool = sworn_free(w, p.governor);
        if (p.steward != kNone) pool.push_back(p.steward);
        if (pool.empty()) pool = fallback_pool(w, planet);
        succeed(w, planet, best_governor(w, pool, p.governor, planet), observer);
        return false;
    }
    if (vacant(w, p.liege)) return true;  // nobody to choose yet
    const Id chooser = w.planets[at(p.liege)].governor;
    succeed(w, planet, best_governor(w, pool_for(w, chooser, planet), chooser, planet), observer);
    return false;
}

}  // namespace lattice::roles
