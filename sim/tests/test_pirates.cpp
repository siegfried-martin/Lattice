#include <doctest.h>

#include <algorithm>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/missions.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "economy/economy.hpp"
#include "helpers.hpp"
#include "info/facts.hpp"
#include "lattice/market.hpp"
#include "pirates/pirates.hpp"
#include "politics/authority.hpp"
#include "politics/fealty.hpp"
#include "politics/governor.hpp"
#include "war/armies.hpp"
#include "war/combat.hpp"
#include "war/encounters.hpp"

using namespace lattice;

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

Config quiet() {
    Config c;
    c.chars.target = 0;
    c.chars.spawn_per_day = 0;
    c.courier.mission_share = 0;
    c.dip.declare_threshold = 1e9;
    c.gen.pirate_planets = 0;
    c.value.invade_months = 0;
    c.raid.share = 0;
    return c;
}

// A planet with vassals.
Id liege_planet(const World& w) {
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (!fealty::vassals(w, p).empty() && w.planets[at(p)].liege != kNone) return p;
    }
    return kNone;
}

Id armed(World& w, Id planet, int ships_n, int tier) {
    const Id id = chars::spawn(w, planet);
    Character& c = w.characters[at(id)];
    c.ships.assign(static_cast<std::size_t>(ships_n), ships::variant(w.map, c.culture, ShipClass::Gunship, tier));
    return id;
}

}  // namespace

TEST_CASE("a planet long in deficit turns pirate (ECO-13, PIR-3)") {
    World w = new_world(quiet(), 91);
    const Id p = liege_planet(w);
    REQUIRE(p != kNone);
    const std::vector<Id> below = fealty::vassals(w, p);
    Planet& planet = w.planets[at(p)];
    planet.deficit_days = w.config.pirate.flip_days;
    planet.treasury = -10 * economy::monthly_costs(w.config, planet);
    const double lawful_costs = economy::monthly_costs(w.config, planet);
    pirates::check_flip(w, p, nullptr);
    REQUIRE(planet.pirate);
    CHECK(planet.liege == kNone);
    CHECK(planet.treasury == 0);                       // its debt is forgotten
    CHECK(planet.makes[0] == kStims);                  // only contraband
    CHECK(planet.labour[0] == 1);
    CHECK(economy::monthly_costs(w.config, planet) == doctest::Approx(lawful_costs * w.config.pirate.cost_mult));
    CHECK(market::reserve(w.config, planet, kStims) > 0);  // a pirate market holds contraband
    CHECK(w.characters[at(planet.governor)].pirate);
    CHECK(chars::faction_of(w, w.characters[at(planet.governor)]) == kPirates);
    for (Id v : below) CHECK(w.planets[at(v)].liege == kNone);  // its vassals become roots
    // No dues for a pirate planet (ECO-14).
    const double before = planet.treasury, dues = w.ledger.dues;
    governor::monthly(w, p, nullptr);
    CHECK(w.ledger.dues == dues);
    CHECK(planet.treasury <= before);
}

TEST_CASE("starting pirate planets sit on the fringe with pirate commanders (GEN-8, GEN-9)") {
    Config c = quiet();
    c.gen.pirate_planets = 2;
    for (std::uint64_t seed = 1; seed <= 6; ++seed) {
        const World w = new_world(c, seed);
        int pirate_planets = 0;
        for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
            if (!w.planets[at(p)].pirate) continue;
            ++pirate_planets;
            CHECK(w.planets[at(p)].fringe);
            int crew = 0;
            for (Id k : w.characters[at(w.planets[at(p)].governor)].contracts) {
                const Contract& ct = w.contracts[at(k)];
                if (ct.kind == ContractKind::Service && ct.alive) {
                    ++crew;
                    CHECK(w.characters[at(ct.employee)].pirate);
                }
            }
            CHECK(crew == c.pirate.commanders);
        }
        CHECK(pirate_planets <= c.gen.pirate_planets);
    }
}

TEST_CASE("pirates stay off the Lattice, and pirate gates are shut to everyone (WLD-12, PIR-2)") {
    World w = new_world(quiet(), 92);
    Id a = kNone, b = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && b == kNone; ++p) {
        for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
            if (w.derived.route(p, q).lattice > 2) {
                a = p, b = q;
                break;
            }
        }
    }
    REQUIRE(b != kNone);
    for (const Leg& l : move::plan(w, kPirates, a, b)) CHECK(l.kind != LegKind::Lattice);
    // Make the holder of the first interchange on the way a pirate planet: lawful traffic
    // may not get on there.
    const std::vector<Leg> legs = move::route_legs(w, a, b);
    Id entry = kNone;
    for (const Leg& l : legs) {
        if (l.kind == LegKind::Space && l.to_node != kNone) {
            entry = l.to_node;
            break;
        }
    }
    REQUIRE(entry != kNone);
    const Id holder = w.map.groups[at(w.map.nodes[at(entry)].group)].holder;
    if (holder == a || holder == b) return;  // can't shut the ends
    pirates::flip(w, holder, nullptr);
    for (const Leg& l : move::plan(w, kNone, a, b)) {
        if (l.kind == LegKind::Space) CHECK(l.to_node != entry);
    }
}

TEST_CASE("the pirate relation fades toward hostile; pirates spare those at neutral (PIR-9)") {
    World w = new_world(quiet(), 93);
    const Id x = armed(w, 0, 1, 1), y = armed(w, 0, 1, 1);
    Character& pirate = w.characters[at(x)];
    Character& other = w.characters[at(y)];
    pirate.pirate = true;
    CHECK(pirates::relation(w, other) == w.config.pirel.rest);
    CHECK(pirates::attacks(w, pirate, other));
    pirates::set_relation(w, other, w.config.pirel.accept);
    CHECK(!pirates::attacks(w, pirate, other));
    pirates::set_relation(w, other, 40);
    advance(w, 7 * kDay);
    const auto& r = w.config.pirel;
    CHECK(pirates::relation(w, other) == doctest::Approx(r.rest + (40 - r.rest) * (1 - r.fade)));
}

TEST_CASE("a broke commander who knows a pirate planet joins it (PIR-4)") {
    Config c = quiet();
    c.pirate.turn_all = true;
    World w = new_world(c, 94);
    Id haven = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (w.planets[at(p)].fringe && fealty::vassals(w, p).empty()) haven = p;
    }
    REQUIRE(haven != kNone);
    pirates::flip(w, haven, nullptr);
    Id start = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (!w.planets[at(p)].pirate && w.planets[at(p)].system != w.planets[at(haven)].system) start = p;
    }
    const Id id = armed(w, start, 1, 1);
    Character& ch = w.characters[at(id)];
    ch.wealth = -100;
    ch.activity = Activity::Exploring;
    pirates::observe(w, ch.facts, haven);
    advance(w, (c.chars.broke_days + 2) * kDay);
    REQUIRE((w.characters[at(id)].activity == Activity::Renouncing || w.characters[at(id)].pirate));
    advance(w, 200 * kDay);
    CHECK(w.characters[at(id)].pirate);
    CHECK(w.ledger.turned_pirate == 1);
    const Id k = contracts::held(w, id, ContractKind::Service);
    REQUIRE(k != kNone);
    CHECK(w.contracts[at(k)].employer == w.planets[at(haven)].governor);
}

TEST_CASE("pirates raid lawful ships in open space, not pirates' friends (PIR-9, SYS-16)") {
    Config c = quiet();
    c.space.engage_p = 1;
    c.space.evade = 0;
    World w = new_world(c, 95);
    Id far = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (w.planets[at(p)].system != w.planets[0].system) far = p;
    }
    const Id raider = armed(w, 0, 2, 2), merchant = chars::spawn(w, 0), friend_ = chars::spawn(w, 0);
    w.characters[at(raider)].pirate = true;
    pirates::set_relation(w, w.characters[at(friend_)], 10);
    REQUIRE(move::start_route(w, raider, move::sortie(w, kPirates, 0, far, 0.15, kDay)));
    w.characters[at(raider)].activity = Activity::Raiding;
    w.characters[at(friend_)].activity = w.characters[at(merchant)].activity = Activity::Exploring;
    REQUIRE(move::travel_to(w, friend_, far));
    encounters::step(w, nullptr);
    CHECK(w.characters[at(friend_)].battle == kNone);
    REQUIRE(move::travel_to(w, merchant, far));
    encounters::step(w, nullptr);
    CHECK(w.characters[at(merchant)].battle != kNone);
    CHECK(w.ledger.raids == 1);
}

TEST_CASE("taking a pirate planet restores its old production (PIR-3, MIL-12)") {
    World w = new_world(quiet(), 96);
    Id haven = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (fealty::vassals(w, p).empty() && w.planets[at(p)].liege != kNone) haven = p;
    }
    REQUIRE(haven != kNone);
    const auto makes = w.planets[at(haven)].makes;
    pirates::flip(w, haven, nullptr);
    w.planets[at(haven)].defence = 1;
    // The nearest lawful planet's governor knows and attacks.
    Id home = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (p != haven && !w.planets[at(p)].pirate && (home == kNone || w.derived.travel(p, haven) < w.derived.travel(home, haven))) home = p;
    }
    for (int i = 0; i < 3; ++i) {
        const Id id = armed(w, home, 3, 2);
        contracts::sign(w, ContractKind::Service, w.planets[at(home)].governor, id, 1, 30 * kDay);
        w.characters[at(id)].activity = Activity::Serving;
    }
    REQUIRE(armies::commission(w, home, haven, false, nullptr) != kNone);
    advance(w, 60 * kDay);
    REQUIRE(w.ledger.captures == 1);
    CHECK(!w.planets[at(haven)].pirate);
    CHECK(w.planets[at(haven)].makes == makes);
    CHECK(w.planets[at(haven)].stock[kStims] == 0);
    CHECK(w.ledger.recaptures == 1);
}

TEST_CASE("the Authority pays mercenaries to clear risky sectors (FAC-14, SYS-20)") {
    World w = new_world(quiet(), 97);
    Id p = kNone;
    for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
        if (!w.planets[at(q)].fringe) p = q;
    }
    w.risk[at(w.planets[at(p)].sector)].risk = 3;
    w.authority.treasury = 1e7;
    authority::weekly(w, nullptr);
    REQUIRE(w.ledger.authority_contracts >= 1);
    Id job = kNone;
    for (Id m : w.planets[at(p)].missions) {
        if (w.missions[at(m)].authority) job = m;
    }
    REQUIRE(job != kNone);
    const Id merc = armed(w, p, 1, 1);
    missions::take(w, merc, job, nullptr);
    const double treasury = w.authority.treasury;
    advance(w, (w.config.authority.clear_days + 3) * kDay);
    CHECK(w.missions[at(job)].state == MissionState::Done);
    CHECK(w.authority.treasury < treasury);
    CHECK(w.characters[at(merc)].authority_opinion >= w.config.authority.opinion_per_job);
}

TEST_CASE("a pirate population buys its own contraband and pays for it in productivity (PRD-6)") {
    World w = new_world(quiet(), 98);
    const Id p = 0;
    pirates::flip(w, p, nullptr);
    Planet& planet = w.planets[at(p)];
    planet.stock[kStims] = market::reserve(w.config, planet, kStims);
    planet.happiness = 0.2;
    const double c0 = planet.contraband_avg;
    economy::planet_day(w, p);
    CHECK(planet.contraband_avg > c0);
}
