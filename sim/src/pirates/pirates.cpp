#include "pirates/pirates.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "economy/economy.hpp"
#include "lattice/market.hpp"
#include "politics/budget.hpp"
#include "politics/fealty.hpp"
#include "war/diplomacy.hpp"

namespace lattice::pirates {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

bool by_planet(const PirateFact& f, Id p) { return f.planet < p; }

// A sworn pirate commander for a pirate planet's governor: a service contract at
// pirate.salary of the lawful stipend (PIR-5).
void hire(World& w, Id planet, Id id) {
    Character& c = w.characters[at(id)];
    c.pirate = true;
    set_relation(w, c, 100);
    contracts::sign(w, ContractKind::Service, w.planets[at(planet)].governor, id,
                    budget::stipend(w, c) * w.config.pirate.salary, w.config.time.payment_days * kDay);
    c.activity = Activity::Serving;
}

}  // namespace

void flip(World& w, Id id, Observer* observer) {
    Planet& p = w.planets[at(id)];
    if (p.pirate) return;
    // Its vassals become roots, each at peace with everyone (PIR-3, MOD-5).
    for (Id v : fealty::vassals(w, id)) {
        fealty::swear(w, v, kNone);
        diplomacy::forget(w, v);
    }
    fealty::swear(w, id, kNone);
    diplomacy::forget(w, id);
    p.pirate = true;
    p.lawful_makes = p.makes;
    p.lawful_tiers = p.tiers;
    // Its whole labour goes to contraband (PIR-3), in its first slot.
    p.makes[0] = kStims;
    p.tiers[0] = kHigh;
    for (int s = 1; s < 3; ++s) {
        if (p.makes[at(s)] == kStims) p.makes[at(s)] = p.lawful_makes[0];
    }
    p.labour = {1, 0, 0};
    // Debt left behind is ignored (ECO-13): writing it off puts money back in the world.
    if (p.treasury < 0) {
        w.ledger.money_created -= p.treasury;
        p.treasury = 0;
    }
    p.deficit_days = 0;
    p.tribute_rate = 0;
    // The governor turns with it; those it paid lose their contracts.
    Character& gov = w.characters[at(p.governor)];
    for (Id k : std::vector<Id>(gov.contracts)) {
        if (w.contracts[at(k)].employer == p.governor) contracts::end(w, k);
    }
    gov.pirate = true;
    set_relation(w, gov, 100);
    for (Id m : std::vector<Id>(p.missions)) w.missions[at(m)].expires = w.now;  // its old jobs lapse
    // Its neighbours see it happen; the rest hear by courier and sharing (MOD-8).
    for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
        const Planet& n = w.planets[at(q)];
        if (n.governor != kNone && q != id && w.derived.travel(q, id) <= w.config.dip.neighbour_days) {
            observe(w, w.characters[at(n.governor)].facts, id);
        }
    }
    ++w.ledger.flips;
    if (observer) observer->log(w.now, "planet.pirate", id, "turned pirate");
}

void check_flip(World& w, Id id, Observer* observer) {
    const Planet& p = w.planets[at(id)];
    if (p.pirate || p.deficit_days < w.config.pirate.flip_days) return;
    if (p.treasury >= -w.config.pirate.flip_debt * economy::monthly_costs(w.config, p)) return;
    flip(w, id, observer);
}

void restore(World& w, Id id) {
    Planet& p = w.planets[at(id)];
    if (!p.pirate) return;
    p.pirate = false;
    p.makes = p.lawful_makes;
    p.tiers = p.lawful_tiers;
    p.labour = {1.0 / 3, 1.0 / 3, 1.0 / 3};
    w.ledger.destroyed[kStims] += p.stock[kStims];  // a lawful market holds none (PIR-14)
    p.stock[kStims] = 0;
    ++w.ledger.recaptures;
}

void seed(World& w) {
    std::vector<Id> fringe;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (w.planets[at(p)].fringe && fealty::vassals(w, p).empty()) fringe.push_back(p);
    }
    Rng& rng = w.rng[Stream::WorldGen];
    rng.shuffle(fringe);
    const auto count = std::min<std::size_t>(fringe.size(), static_cast<std::size_t>(w.config.gen.pirate_planets));
    for (std::size_t i = 0; i < count; ++i) {
        const Id p = fringe[i];
        flip(w, p, nullptr);
        w.ledger.flips = 0;  // starting conditions, not events
        for (std::int64_t k = 0; k < w.config.pirate.commanders; ++k) {
            const Id id = chars::spawn(w, p);
            Character& c = w.characters[at(id)];
            c.ships = {ships::variant(w.map, c.culture, ShipClass::Gunship, 1)};
            hire(w, p, id);
        }
    }
}

double relation(const World& w, const Character& c) {
    const auto& r = w.config.pirel;
    const double weeks = static_cast<double>(w.now - c.pirate_relation_at) / static_cast<double>(7 * kDay);
    return r.rest + (c.pirate_relation - r.rest) * std::pow(1 - r.fade, std::max(0.0, weeks));
}

void set_relation(World& w, Character& c, double value) {
    c.pirate_relation = std::clamp(value, -100.0, 100.0);
    c.pirate_relation_at = w.now;
}

bool attacks(const World& w, const Character& pirate, const Character& other) {
    return pirate.pirate && !other.pirate && relation(w, other) < 0;
}

void dispatch(World& w, Id planet, Observer* observer) {
    const Planet& p = w.planets[at(planet)];
    if (!p.pirate || p.governor == kNone) return;
    // Hunger drives raids: the scarcest life support's shortfall against its reserve.
    double hunger = 0;
    for (Good g : kLifeSupport) {
        const double r = market::reserve(w.config, p, g);
        if (r > 0) hunger = std::max(hunger, 1 - std::min(1.0, p.stock[g] / r));
    }
    // Targets: lawful planets within reach in open space, weighted by population.
    std::vector<Id> targets;
    std::vector<double> weights;
    const double reach = w.config.pirate.range_days * w.config.travel.space_speed;
    for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
        const Planet& t = w.planets[at(q)];
        if (t.pirate || std::hypot(t.x - p.x, t.y - p.y) > reach) continue;
        targets.push_back(q);
        weights.push_back(t.population);
    }
    if (targets.empty()) return;
    Rng& rng = w.rng[Stream::Combat];
    const Id gov = p.governor;
    for (Id k : std::vector<Id>(w.characters[at(gov)].contracts)) {
        const Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.kind != ContractKind::Service || ct.employer != gov) continue;
        Character& c = w.characters[at(ct.employee)];
        if (c.activity != Activity::Serving || c.planet != planet) continue;
        if (!rng.chance(std::min(1.0, w.config.raid.share * (1 + hunger)))) continue;
        const Id target = targets[rng.weighted(weights)];
        if (!move::start_route(w, ct.employee, move::sortie(w, kPirates, planet, target, w.config.raid.standoff, w.config.raid.days * kDay))) {
            continue;
        }
        c.activity = Activity::Raiding;
        ++w.ledger.pirate_raids;
        if (observer) observer->log(w.now, "pirate.raid", ct.employee, "from " + std::to_string(planet) + " toward " + std::to_string(target));
    }
}

bool try_turn(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    if (c.pirate || c.planet == kNone || c.governs != kNone || c.ships.empty()) return false;
    // It picks from what it knows (PIR-4): the nearest pirate planet it has heard of.
    Id best = kNone;
    double best_d = std::numeric_limits<double>::max();
    for (const PirateFact& f : c.facts.pirates) {
        if (!f.pirate || !w.planets[at(f.planet)].pirate) continue;
        const double d = w.derived.travel(c.planet, f.planet);
        if (d < best_d) best_d = d, best = f.planet;
    }
    if (best == kNone) return false;
    const double p = w.config.pirate.turn_all ? 1 : c.risk;  // pirate.turn_p(risk) (EXP-9)
    if (!w.rng[Stream::Characters].chance(p)) return false;
    if (!move::travel_to(w, id, best)) return false;
    c.activity = Activity::Renouncing;
    c.renounce_to = best;
    if (observer) observer->log(w.now, "pirate.turning", id, "toward " + std::to_string(best));
    return true;
}

void renounce(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    const Id p = c.renounce_to;
    c.renounce_to = kNone;
    if (p == kNone || c.planet != p || !w.planets[at(p)].pirate) {
        c.activity = Activity::Idle;
        chars::wake(w, id, w.now);
        return;
    }
    for (Id k : std::vector<Id>(c.contracts)) contracts::end(w, k);
    hire(w, p, id);
    ++w.ledger.turned_pirate;
    if (observer) observer->log(w.now, "pirate.joined", id, "at " + std::to_string(p));
    chars::wake(w, id, w.now + kDay);
}

void observe_system(const World& w, Facts& facts, Id planet) {
    for (Id q : w.map.systems[at(w.planets[at(planet)].system)].planets) observe(w, facts, q);
}

void observe(const World& w, Facts& facts, Id planet) {
    const PirateFact f{.planet = planet, .time = w.now, .pirate = w.planets[at(planet)].pirate};
    auto it = std::lower_bound(facts.pirates.begin(), facts.pirates.end(), planet, by_planet);
    if (it != facts.pirates.end() && it->planet == planet) {
        *it = f;
    } else {
        facts.pirates.insert(it, f);
    }
}

}  // namespace lattice::pirates
