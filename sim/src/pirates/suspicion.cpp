#include "pirates/suspicion.hpp"

#include <algorithm>
#include <cmath>

#include "chars/characters.hpp"
#include "pirates/pirates.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"
#include "war/diplomacy.hpp"

namespace lattice::suspicion {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

double faded(const World& w, const Suspicion& s) {
    const double weeks = static_cast<double>(w.now - s.updated) / static_cast<double>(7 * kDay);
    return s.value * std::pow(1 - w.config.sus.fade, std::max(0.0, weeks));
}

}  // namespace

double get(const World& w, const Character& c, Id faction) {
    for (const Suspicion& s : c.suspicion) {
        if (s.faction == faction) return faded(w, s);
    }
    return 0;
}

void add(World& w, Character& c, Id faction, double amount) {
    auto it = std::lower_bound(c.suspicion.begin(), c.suspicion.end(), faction,
                               [](const Suspicion& s, Id f) { return s.faction < f; });
    if (it == c.suspicion.end() || it->faction != faction) it = c.suspicion.insert(it, {.faction = faction, .updated = w.now});
    it->value = std::max(0.0, faded(w, *it) + amount);
    it->updated = w.now;
}

double intensity(const World& w, Id planet) {
    const Planet& p = w.planets[at(planet)];
    const auto& s = w.config.sus;
    const Id root = fealty::root_of(w, planet);
    return 1 + s.police_per_unit * p.policing / w.config.value.security_unit + (diplomacy::enemies(w, root).empty() ? 0 : s.war);
}

bool on_landing(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    const Id planet = c.planet;
    if (planet == kNone || w.planets[at(planet)].pirate || c.governs != kNone) return false;
    const auto& s = w.config.sus;
    const Id root = fealty::root_of(w, planet);
    const double relation = std::max(0.0, pirates::relation(w, c));
    const bool contraband = c.cargo_good == kStims && c.cargo_qty > 0;
    Rng& rng = w.rng[Stream::Missions];
    const bool inspected = rng.chance(std::min(1.0, s.inspect_base * intensity(w, planet)));
    if (inspected) ++w.ledger.inspections;
    const double find = std::min(1.0, s.find_base + (get(w, c, root) + s.pirate * relation) / s.scale);
    if (inspected && contraband && rng.chance(find)) {
        w.ledger.destroyed[kStims] += c.cargo_qty;
        w.ledger.confiscated += c.cargo_qty;
        c.cargo_qty = 0, c.cargo_cost = 0, c.cargo_good = kGoodCount;
        opinion::deed(w, w.planets[at(planet)].governor, id, DeedKind::Caught, observer);
        add(w, c, root, s.caught);
        ++w.ledger.caught;
        if (observer) observer->log(w.now, "smuggler.caught", id, "at " + std::to_string(planet));
        return true;
    }
    if (contraband) add(w, c, root, s.contraband);
    if (relation > 0) add(w, c, root, s.pirate * relation);
    return false;
}

}  // namespace lattice::suspicion
