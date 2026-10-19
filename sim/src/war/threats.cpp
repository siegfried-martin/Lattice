#include "war/threats.hpp"

#include <algorithm>

#include "chars/contracts.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"
#include "war/armies.hpp"

namespace lattice::threats {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

const Sighting* sighting_of(const Facts& f, Id army) {
    for (const Sighting& s : f.sightings) {
        if (s.army == army) return &s;
    }
    return nullptr;
}

double ready_power(const World& w, Id planet) {
    double p = 0;
    const Id gov = w.planets[at(planet)].governor;
    for (Id k : w.characters[at(gov)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.kind != ContractKind::Service || ct.employer != gov) continue;
        const Character& c = w.characters[at(ct.employee)];
        if (c.activity == Activity::Serving && c.planet == planet && c.army == kNone) p += ships::fleet(w, c).combat;
    }
    return p;
}

// Relief to garrison a threatened planet, when it stands a fair chance against what was
// seen (as for a siege's call, SYS-13). Returns whether it went.
bool relieve(World& w, Id liege, Id planet, const Sighting& s, Observer* observer) {
    if (!fealty::same_faction(w, liege, planet)) return false;
    const double mine = ready_power(w, liege);
    const double a = mine * mine, d = s.power * s.power;
    if (mine <= 0 || a / (a + d) < 1.0 / 3) return false;
    if (armies::commission(w, liege, planet, true, observer) == kNone) return false;
    opinion::deed(w, w.planets[at(planet)].governor, w.planets[at(liege)].governor, DeedKind::HelpSent, observer);
    ++w.ledger.garrisons;
    return true;
}

}  // namespace

void check(World& w, Id planet, Observer* observer) {
    Planet& p = w.planets[at(planet)];
    if (p.governor == kNone) return;
    const Facts& facts = w.characters[at(p.governor)].facts;
    const Id root = fealty::root_of(w, planet);
    const Time recent = w.now - w.config.scout.threat_days * kDay;
    for (const Sighting& s : facts.sightings) {
        if (s.time < recent || s.target == kNone || s.army == p.warned_army) continue;
        if (!info::believes_war(facts, root, s.faction)) continue;
        const bool mine = s.target == planet;
        const bool vassal = w.planets[at(s.target)].liege == planet;
        if (!mine && !vassal) continue;
        p.warned_army = s.army;
        ++w.ledger.warnings;
        if (observer) observer->log(w.now, "threat.seen", planet, "army " + std::to_string(s.army) + " for " + std::to_string(s.target));
        if (vassal) {
            relieve(w, planet, s.target, s, observer);
        } else if (p.liege != kNone) {
            const Time trip = std::max<Time>(1, static_cast<Time>(w.derived.travel(planet, p.liege) * kDay));
            w.events.push({.time = w.now + trip, .kind = EventKind::ThreatWarning, .record = s.army, .a = p.liege, .b = planet});
        }
    }
}

void warning(World& w, Id army, Id liege, Id planet, Observer* observer) {
    // The courier brings what the planet knew about the army.
    const Facts& from = w.characters[at(w.planets[at(planet)].governor)].facts;
    const Sighting* s = sighting_of(from, army);
    if (!s || !w.armies[at(army)].alive) return;
    info::add_sighting(w.characters[at(w.planets[at(liege)].governor)].facts, *s);
    if (relieve(w, liege, planet, *s, observer)) return;
    const Id up = w.planets[at(liege)].liege;
    if (up == kNone) return;
    const Time trip = std::max<Time>(1, static_cast<Time>(w.derived.travel(liege, up) * kDay));
    w.events.push({.time = w.now + trip, .kind = EventKind::ThreatWarning, .record = army, .a = up, .b = planet});
}

}  // namespace lattice::threats
