#include "war/raids.hpp"

#include <algorithm>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/movement.hpp"
#include "info/facts.hpp"
#include "lattice/market.hpp"
#include "politics/fealty.hpp"

namespace lattice::raids {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

}  // namespace

std::vector<Id> believed_enemies(const World& w, Id planet) {
    std::vector<Id> out;
    const Id gov = w.planets[at(planet)].governor;
    if (gov == kNone) return out;
    const Id root = fealty::root_of(w, planet);
    for (const DipFact& f : w.characters[at(gov)].facts.dips) {
        if (static_cast<DipState>(f.state) != DipState::War || (f.a != root && f.b != root)) continue;
        const Id other = f.a == root ? f.b : f.a;
        if (w.planets[at(other)].liege == kNone) out.push_back(other);  // still a root
    }
    return out;
}

void dispatch(World& w, Id planet, Observer* observer) {
    if (!w.config.raid.enabled) return;
    const std::vector<Id> enemies = believed_enemies(w, planet);
    if (enemies.empty()) return;
    Id target = kNone;
    for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
        if (std::find(enemies.begin(), enemies.end(), fealty::root_of(w, q)) == enemies.end()) continue;
        if (w.derived.travel(planet, q) > w.config.dip.border_days) continue;
        if (target == kNone || w.derived.travel(planet, q) < w.derived.travel(planet, target)) target = q;
    }
    if (target == kNone) return;
    const Id gov = w.planets[at(planet)].governor;
    std::vector<Id> home;
    for (Id k : w.characters[at(gov)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.kind != ContractKind::Service || ct.employer != gov) continue;
        const Character& c = w.characters[at(ct.employee)];
        if (c.activity == Activity::Serving && c.planet == planet && c.army == kNone) home.push_back(ct.employee);
    }
    Rng& rng = w.rng[Stream::Combat];
    for (std::size_t i = 1; i < home.size(); ++i) {  // one stays
        if (!rng.chance(w.config.raid.share)) continue;
        const Id id = home[i];
        Character& c = w.characters[at(id)];
        std::vector<Leg> route = move::sortie(w, fealty::root_of(w, planet), planet, target, w.config.raid.standoff,
                                              w.config.raid.days * kDay);
        if (!move::start_route(w, id, std::move(route))) continue;
        c.activity = Activity::Raiding;
        if (observer) observer->log(w.now, "raid.sent", id, "from " + std::to_string(planet) + " toward " + std::to_string(target));
    }
}

void home(World& w, Id id, Observer*) {
    Character& c = w.characters[at(id)];
    if (c.cargo_qty > 0 && c.planet != kNone) {
        const market::Fill f = market::sell(w.planets[at(c.planet)], w.config, c.cargo_good, c.cargo_qty, 0);
        chars::earn(c, f.money, true);
        c.cargo_qty -= f.qty;
        if (c.cargo_qty <= 1e-9) c.cargo_qty = 0, c.cargo_good = kGoodCount;
        c.cargo_cost = 0;
    }
    c.activity = Activity::Idle;
    chars::wake(w, id, w.now);
}

}  // namespace lattice::raids
