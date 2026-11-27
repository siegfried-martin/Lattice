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
#include "pirates/smuggling.hpp"
#include "pirates/suspicion.hpp"
#include "politics/budget.hpp"
#include "politics/opinion.hpp"
#include "politics/authority.hpp"
#include "politics/fealty.hpp"
#include "politics/governor.hpp"
#include "war/armies.hpp"
#include "war/combat.hpp"
#include "war/diplomacy.hpp"
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
    c.rebel.scale = 0;  // rebellions only where a test raises them
    return c;
}

// A planet with vassals.
Id liege_planet(const World& w) {
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (!fealty::vassals(w, p).empty() && w.planets[at(p)].liege != kNone) return p;
    }
    return kNone;
}

// A sworn planet with no vassals.
Id leaf_planet(const World& w) {
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (fealty::vassals(w, p).empty() && w.planets[at(p)].liege != kNone) return p;
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

TEST_CASE("a failing planet with no vassals turns pirate (ECO-13, PIR-3)") {
    World w = new_world(quiet(), 91);
    w.config.rebel.scale = 1;  // deep enough in debt, certain
    const Id p = leaf_planet(w);
    REQUIRE(p != kNone);
    Planet& planet = w.planets[at(p)];
    planet.treasury = -10 * economy::monthly_costs(w.config, planet);
    const double lawful_costs = economy::monthly_costs(w.config, planet);
    pirates::check_rebellion(w, p, nullptr);
    REQUIRE(planet.pirate);
    CHECK(planet.liege == kNone);
    CHECK(planet.treasury == 0);                       // its debt is forgotten
    CHECK(planet.makes[0] == kStims);                  // only contraband
    CHECK(planet.labour[0] == 1);
    CHECK(economy::monthly_costs(w.config, planet) == doctest::Approx(lawful_costs * w.config.pirate.cost_mult));
    CHECK(market::reserve(w.config, planet, kStims) > 0);  // a pirate market holds contraband
    CHECK(w.characters[at(planet.governor)].pirate);
    CHECK(chars::faction_of(w, w.characters[at(planet.governor)]) == kPirates);
    // No dues for a pirate planet (ECO-14).
    const double before = planet.treasury, dues = w.ledger.dues;
    governor::monthly(w, p, nullptr);
    CHECK(w.ledger.dues == dues);
    CHECK(planet.treasury <= before);
}

TEST_CASE("a failing planet with vassals secedes with them, at war with its old faction (ECO-13)") {
    World w = new_world(quiet(), 91);
    const Id p = liege_planet(w);
    REQUIRE(p != kNone);
    const std::vector<Id> below = fealty::vassals(w, p);
    const Id old_root = fealty::root_of(w, p);
    Planet& planet = w.planets[at(p)];
    w.config.rebel.scale = 1;
    planet.treasury = -10 * economy::monthly_costs(w.config, planet);
    pirates::check_rebellion(w, p, nullptr);
    CHECK(!planet.pirate);
    CHECK(planet.liege == kNone);
    for (Id v : below) CHECK(fealty::root_of(w, v) == p);  // its branch goes with it
    CHECK(diplomacy::at_war(w, p, old_root));
    CHECK(w.ledger.secessions == 1);
}

TEST_CASE("starvation makes a population unhappy, and rebellion grows likelier with misery and debt (PRD-2, ECO-13, SYS-16)") {
    World w = new_world(quiet(), 92);
    w.config.rebel.scale = Config{}.rebel.scale;
    const Config& c = w.config;
    const Id p = leaf_planet(w);
    REQUIRE(p != kNone);
    Planet& planet = w.planets[at(p)];
    planet.happiness = 0;
    planet.treasury = 0;
    CHECK(pirates::rebellion_chance(w, p) == 0);  // content enough and solvent: never
    for (Good g : kLifeSupport) planet.stock[g] = 0;
    for (Good g : kLuxuries) planet.stock[g] = 0;  // nothing to soften it
    planet.productivity = 0;                        // and nothing made today
    economy::planet_day(w, p);
    CHECK(planet.happiness < (c.happy.base - c.happy.hunger * 0.5) / c.happy.settle_days);
    planet.happiness = -0.2;
    const double unhappy = pirates::rebellion_chance(w, p);
    CHECK(unhappy > 0);
    planet.happiness = -1;
    const double miserable = pirates::rebellion_chance(w, p);
    CHECK(miserable > unhappy);
    CHECK(miserable <= c.rebel.scale);
    planet.treasury = -0.5 * c.pirate.flip_debt * planet.income_avg * static_cast<double>(c.time.payment_days);
    CHECK(pirates::rebellion_chance(w, p) == doctest::Approx(miserable + c.rebel.scale * c.rebel.w_debt * 0.5));
    // Weekly draws: over enough weeks, it happens.
    w.config.rebel.scale = 0.5;
    for (int week = 0; week < 50 && !planet.pirate; ++week) pirates::check_rebellion(w, p, nullptr);
    CHECK(planet.pirate);
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
    // Tier 2, so not the bottom rung, which looks for work in debt and would earn its way
    // out first (CHR-16).
    const Id id = armed(w, start, 1, 2);
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

// Open space gets out of this planet: a lawful planet within passenger.max_days (TRD-18).
bool open_out(const World& w, Id p) {
    for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
        if (q != p && !w.planets[at(q)].pirate && w.derived.travel(p, q, false) <= w.config.passenger.max_days) return true;
    }
    return false;
}

TEST_CASE("a timid broke commander never turns pirate; bankrupt, it starts over on the bottom rung (PIR-4, CHR-16)") {
    Config c = quiet();
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
    const Id id = armed(w, start, 2, 2);
    Character& ch = w.characters[at(id)];
    ch.risk = c.pirate.turn_min_risk / 2;
    ch.wealth = -1e6;
    ch.activity = Activity::Exploring;
    pirates::observe(w, ch.facts, haven);
    advance(w, (c.chars.broke_days + 2) * kDay);
    const Character& after = w.characters[at(id)];
    CHECK(!after.pirate);
    CHECK(after.activity != Activity::Renouncing);
    CHECK(w.ledger.turned_pirate == 0);
    CHECK(w.ledger.bankruptcies == 1);
    CHECK(w.ledger.debt_written_off > 0);
    REQUIRE(after.ships.size() == 1);
    CHECK(w.map.variants[at(after.ships[0])].cls == ShipClass::Courier);
    CHECK(w.map.variants[at(after.ships[0])].tier == 1);
    // A shuttle (no standing, no record, TRD-18), with a drive only where open space doesn't get out.
    CHECK(w.map.variants[at(after.ships[0])].lattice_drive == !open_out(w, after.planet));
    CHECK(after.wealth > -1000);  // the debt is gone; only days of upkeep since
}

TEST_CASE("a ruined pirate with no salary gets a scout and goes raiding on its own (PIR-16)") {
    Config c = quiet();
    World w = new_world(c, 94);
    Id haven = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (w.planets[at(p)].fringe && fealty::vassals(w, p).empty()) haven = p;
    }
    REQUIRE(haven != kNone);
    pirates::flip(w, haven, nullptr);
    const Id id = chars::spawn(w, haven);
    Character& ch = w.characters[at(id)];
    ch.pirate = true;
    ch.ships.clear();
    ch.wealth = 0;
    chars::wake(w, id, w.now);
    advance(w, kDay);
    const Character& after = w.characters[at(id)];
    REQUIRE(after.ships.size() == 1);
    CHECK(w.map.variants[at(after.ships[0])].cls == ShipClass::Scout);
    CHECK(w.map.variants[at(after.ships[0])].tier == 1);
    CHECK(w.ledger.freelance_raids == 1);
    CHECK(after.activity == Activity::Raiding);
}

TEST_CASE("a new character is born with standing in its faction, and courier runs need it (REL-5, MSN-2)") {
    Config c = quiet();
    World w = new_world(c, 94);
    Id home = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && home == kNone; ++p) {
        if (!w.planets[at(p)].pirate && w.planets[at(p)].governor != kNone && w.planets[at(p)].liege != kNone) home = p;
    }
    REQUIRE(home != kNone);
    const Id root = fealty::root_of(w, home);
    const Id born = chars::spawn(w, home);
    CHECK(opinion::standing(w, root, born) == doctest::Approx(c.chars.start_standing));
    // A courier run down the tree from its root goes only to those the faction trusts.
    Id vassal = kNone;
    for (Id v : fealty::vassals(w, root)) vassal = v;
    REQUIRE(vassal != kNone);
    missions::post_courier(w, root, vassal, nullptr);
    Id run = kNone;
    for (Id m : w.planets[at(root)].missions) {
        if (w.missions[at(m)].kind == MissionKind::Courier) run = m;
    }
    REQUIRE(run != kNone);
    const Id stranger = chars::spawn(w, root);
    w.characters[at(stranger)].ships = {ships::variant(w.map, w.characters[at(stranger)].culture, ShipClass::Courier, 1)};
    CHECK(missions::qualifies(w, stranger, w.missions[at(run)]));
    opinion::seed(w, w.planets[at(root)].governor, stranger, c.mission.courier_standing - 1);
    CHECK(!missions::qualifies(w, stranger, w.missions[at(run)]));
    // Its standing fades unless it works for the faction: here it's gone and can't.
    opinion::seed(w, w.planets[at(root)].governor, born, c.chars.start_standing);
    w.characters[at(born)].alive = false;
    advance(w, 52 * 7 * kDay);
    CHECK(opinion::standing(w, root, born) < c.mission.courier_standing);
}

TEST_CASE("a character with nothing to do for a day moves on to a nearby planet (TRD-4)") {
    Config c = quiet();
    c.passenger.per_planet = 0;
    World w = new_world(c, 94);
    Id home = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && home == kNone; ++p) {
        if (w.planets[at(p)].pirate || w.planets[at(p)].governor == kNone) continue;
        for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
            if (q != p && !w.planets[at(q)].pirate && w.derived.travel(p, q, false) <= c.merchant.wander_days) home = p;
        }
    }
    REQUIRE(home != kNone);
    const Id id = chars::spawn(w, home);
    Character& ch = w.characters[at(id)];
    ch.wealth = 0;  // nothing to trade with; no drive, so open space costs it no fuel
    // Tier 2: above the bottom rung, which is posted errands instead (MSN-14).
    ch.ships = {ships::variant(w.map, ch.culture, ShipClass::Courier, 2, false)};
    // It knows every market, freshly: nothing stale to go and look at.
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) info::observe_market(w, ch.facts, p);
    advance(w, 3 * kDay);
    CHECK(w.ledger.wanders >= 1);
}

TEST_CASE("a ruined character's free hull follows who it was; standing wins (CHR-16, TRD-18)") {
    Config c = quiet();
    World w = new_world(c, 94);
    Id home = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && home == kNone; ++p) {
        if (!w.planets[at(p)].pirate && w.planets[at(p)].governor != kNone && w.planets[at(p)].siege == kNone && open_out(w, p)) home = p;
    }
    REQUIRE(home != kNone);
    const Id ruler = w.planets[at(fealty::root_of(w, home))].governor;
    auto ruined = [&](double record, bool trusted) {
        const Id id = chars::spawn(w, home);
        Character& ch = w.characters[at(id)];
        ch.ships.clear();
        ch.wealth = 0;
        ch.career_cargo = record;
        for (int i = 0; trusted && i < 200 && opinion::get(w, ruler, id) < c.ship.trusted_opinion; ++i) {
            opinion::deed(w, ruler, id, DeedKind::JobDone);
        }
        return id;
    };
    const Id nobody = ruined(0, false);
    const Id merchant = ruined(c.ship.merchant_record, false);
    const Id trusted = ruined(c.ship.merchant_record, true);  // standing wins over the record
    REQUIRE(opinion::get(w, ruler, trusted) >= c.ship.trusted_opinion);
    advance(w, kHour);
    auto hull = [&](Id id) -> const ShipVariant& {
        REQUIRE(w.characters[at(id)].ships.size() == 1);
        return w.map.variants[at(w.characters[at(id)].ships[0])];
    };
    CHECK(hull(nobody).cls == ShipClass::Courier);
    CHECK(!hull(nobody).lattice_drive);
    CHECK(hull(merchant).cls == ShipClass::Freighter);
    CHECK(!hull(merchant).lattice_drive);
    CHECK(hull(trusted).cls == ShipClass::Courier);
    CHECK(hull(trusted).lattice_drive);
    for (Id id : {nobody, merchant, trusted}) CHECK(hull(id).tier == 1);
    CHECK(w.characters[at(nobody)].cover == 0);  // free hulls are uninsured (CHR-21)
    // Where open space doesn't get out, the free hull has a drive: it can always leave.
    Id shut = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && shut == kNone; ++p) {
        if (!w.planets[at(p)].pirate && w.planets[at(p)].siege == kNone && !open_out(w, p)) shut = p;
    }
    if (shut != kNone) {
        const Id id = chars::spawn(w, shut);
        w.characters[at(id)].ships.clear();
        w.characters[at(id)].wealth = 0;
        advance(w, kHour);
        CHECK(hull(id).lattice_drive);
    }
}

TEST_CASE("a character climbing back first swaps its shuttle for a courier with a drive (TRD-18, SYS-8)") {
    Config c = quiet();
    World w = new_world(c, 94);
    Id home = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && home == kNone; ++p) {
        if (!w.planets[at(p)].pirate && w.planets[at(p)].governor != kNone && w.planets[at(p)].siege == kNone) home = p;
    }
    REQUIRE(home != kNone);
    const Id id = chars::spawn(w, home);
    Character& ch = w.characters[at(id)];
    ch.ships = {ships::variant(w.map, w.planets[at(home)].region, ShipClass::Courier, 1, false)};
    ch.wealth = 1e6;
    ch.earned_cargo = 1000;
    advance(w, kHour);
    const Character& after = w.characters[at(id)];
    REQUIRE(!after.ships.empty());
    // The first step up is the same hull with a drive; the fleet then grows as usual.
    CHECK(w.map.variants[at(after.ships[0])].cls == ShipClass::Courier);
    CHECK(w.map.variants[at(after.ships[0])].lattice_drive);
    CHECK(move::uses_lattice(w, after));
}

TEST_CASE("a hull without a Lattice Drive keeps off the Lattice, slower and burning no fuel (TRD-18)") {
    Config c = quiet();
    World w = new_world(c, 94);
    Id a = kNone, b = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && a == kNone; ++p) {
        for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
            if (p != q && !w.planets[at(p)].pirate && !w.planets[at(q)].pirate && w.derived.route(p, q).lattice > 0) {
                a = p, b = q;
                break;
            }
        }
    }
    REQUIRE(a != kNone);
    const Id id = chars::spawn(w, a);
    Character& ch = w.characters[at(id)];
    ch.ships = {ships::variant(w.map, ch.culture, ShipClass::Courier, 1, true)};
    CHECK(move::uses_lattice(w, ch));
    const double with_drive = move::trip_days(w, ch, a, b);
    CHECK(move::fuel_cost(w, ch, a, b) > 0);
    ch.ships = {ships::variant(w.map, ch.culture, ShipClass::Courier, 1, false)};
    CHECK(!move::uses_lattice(w, ch));
    CHECK(move::trip_days(w, ch, a, b) > with_drive);
    CHECK(move::fuel_cost(w, ch, a, b) == 0);
    const std::vector<Leg> route = move::plan(w, chars::faction_of(w, ch), a, b, &ch.facts, false);
    REQUIRE(!route.empty());
    CHECK(std::none_of(route.begin(), route.end(), [](const Leg& l) { return l.kind == LegKind::Lattice; }));
    CHECK(route.back().to_planet == b);
    // One ship without a drive holds the whole fleet off it.
    ch.ships.push_back(ships::variant(w.map, ch.culture, ShipClass::Freighter, 1, true));
    CHECK(!move::uses_lattice(w, ch));
}

TEST_CASE("passenger runs are short open-space hops a shuttle can fly; other missions need a drive (MSN-13, TRD-18)") {
    Config c = quiet();
    World w = new_world(c, 94);
    // A planet with a neighbour in open-space reach (most have one in the same system).
    Id home = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && home == kNone; ++p) {
        if (w.planets[at(p)].pirate || w.planets[at(p)].governor == kNone) continue;
        for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
            if (q != p && !w.planets[at(q)].pirate && w.derived.travel(p, q, false) <= c.passenger.max_days) home = p;
        }
    }
    REQUIRE(home != kNone);
    missions::post_passengers(w, home, nullptr);
    const Id id = chars::spawn(w, home);
    Character& ch = w.characters[at(id)];
    ch.ships = {ships::variant(w.map, ch.culture, ShipClass::Courier, 1, false)};
    int passengers = 0;
    for (Id m : w.planets[at(home)].missions) {
        const Mission& mi = w.missions[at(m)];
        if (mi.kind == MissionKind::Passenger) {
            ++passengers;
            CHECK(w.derived.travel(home, mi.target, false) <= c.passenger.max_days);
            CHECK(missions::qualifies(w, id, mi));
        } else {
            CHECK(!missions::qualifies(w, id, mi));
        }
    }
    CHECK(passengers > 0);
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
    w.characters[at(w.planets[at(haven)].governor)].ships.clear();  // no governor's fleet to defend it (MIL-20)
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

// ---- MS-6 part 2: smuggling, suspicion, policing ----

TEST_CASE("suspicion fades and genuine trade lowers it (INF-14)") {
    World w = new_world(quiet(), 101);
    const Id id = armed(w, 0, 1, 1);
    Character& c = w.characters[at(id)];
    suspicion::add(w, c, 3, 40);
    CHECK(suspicion::get(w, c, 3) == 40);
    advance(w, 7 * kDay);
    CHECK(suspicion::get(w, w.characters[at(id)], 3) == doctest::Approx(40 * (1 - w.config.sus.fade)));
    suspicion::add(w, w.characters[at(id)], 3, -1000);
    CHECK(suspicion::get(w, w.characters[at(id)], 3) == 0);  // never below zero
}

TEST_CASE("policing and war make a planet look harder (PIR-15)") {
    World w = new_world(quiet(), 102);
    const double base = suspicion::intensity(w, 0);
    CHECK(base == 1);
    w.planets[0].policing = 2 * w.config.value.security_unit;
    CHECK(suspicion::intensity(w, 0) == doctest::Approx(1 + 2 * w.config.sus.police_per_unit));
}

TEST_CASE("an inspection finds contraband and confiscates it (INF-14, PIR-14)") {
    Config c = quiet();
    c.sus.inspect_base = 1;
    c.sus.find_base = 1;
    World w = new_world(c, 103);
    const Id id = armed(w, 0, 1, 1);
    Character& ch = w.characters[at(id)];
    ch.cargo_good = kStims;
    ch.cargo_qty = 10;
    CHECK(suspicion::on_landing(w, id, nullptr));
    CHECK(w.characters[at(id)].cargo_qty == 0);
    CHECK(w.ledger.confiscated == 10);
    CHECK(suspicion::get(w, w.characters[at(id)], fealty::root_of(w, 0)) == doctest::Approx(c.sus.caught));
    CHECK(opinion::get(w, w.planets[0].governor, id) < 0);
}

TEST_CASE("a smuggling run carries contraband from a haven to a lawful population (SYS-16, PIR-6)") {
    Config c = quiet();
    c.sus.inspect_base = 0;  // nobody looks
    World w = new_world(c, 104);
    Id haven = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (fealty::vassals(w, p).empty() && w.planets[at(p)].liege != kNone) haven = p;
    }
    pirates::flip(w, haven, nullptr);
    Planet& h = w.planets[at(haven)];
    h.stock[kStims] = 3 * market::reserve(c, h, kStims);
    smuggling::post(w, haven, nullptr);
    Id run = kNone;
    for (Id m : h.missions) {
        if (w.missions[at(m)].kind == MissionKind::Smuggle) run = m;
    }
    REQUIRE(run != kNone);
    const Id id = chars::spawn(w, haven);
    Character& s = w.characters[at(id)];
    s.ships = {ships::variant(w.map, s.culture, ShipClass::Freighter, 1)};
    s.risk = 0.9;
    s.wealth = 1e5;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) info::observe_market(w, s.facts, p);
    const double created = w.ledger.money_created;
    smuggling::take(w, id, run, nullptr);
    CHECK(w.characters[at(id)].cargo_good == kStims);
    CHECK(w.characters[at(id)].cargo_qty > 0);
    advance(w, 60 * kDay);
    CHECK(w.missions[at(run)].state == MissionState::Done);
    CHECK(w.ledger.smuggled > 0);
    CHECK(w.ledger.smuggled_runs == 1);
    CHECK(w.ledger.money_created > created);
    CHECK(pirates::relation(w, w.characters[at(id)]) > w.config.pirel.accept);
}

TEST_CASE("a legal-goods run feeds a hungry haven (PIR-3)") {
    Config c = quiet();
    c.sus.inspect_base = 0;
    World w = new_world(c, 105);
    Id haven = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (fealty::vassals(w, p).empty() && w.planets[at(p)].liege != kNone) haven = p;
    }
    pirates::flip(w, haven, nullptr);
    Planet& h = w.planets[at(haven)];
    h.stock[kFood] = 0.1 * market::reserve(c, h, kFood);
    smuggling::post(w, haven, nullptr);
    Id run = kNone;
    for (Id m : h.missions) {
        if (w.missions[at(m)].kind == MissionKind::LegalGoods && w.missions[at(m)].good == kFood) run = m;
    }
    REQUIRE(run != kNone);
    Id start = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (!w.planets[at(p)].pirate && w.planets[at(p)].stock[kFood] > 1000) start = p;
    }
    const Id id = chars::spawn(w, start);
    Character& s = w.characters[at(id)];
    s.ships = {ships::variant(w.map, s.culture, ShipClass::Freighter, 1)};
    s.risk = 0.9;
    s.wealth = 1e5;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) info::observe_market(w, s.facts, p);
    smuggling::take(w, id, run, nullptr);
    advance(w, 90 * kDay);
    CHECK(w.missions[at(run)].state == MissionState::Done);
    CHECK(w.ledger.legal_runs == 1);
    CHECK(pirates::relation(w, w.characters[at(id)]) > w.config.pirel.accept);  // the haven's food was delivered (and eaten)
}

TEST_CASE("a pirate offers a bold independent a run instead of attacking it (PIR-10, PIR-13)") {
    Config c = quiet();
    c.space.engage_p = 1;
    c.space.evade = 0;
    World w = new_world(c, 106);
    Id haven = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (fealty::vassals(w, p).empty() && w.planets[at(p)].liege != kNone && w.planets[at(p)].system != w.planets[0].system) haven = p;
    }
    pirates::flip(w, haven, nullptr);
    w.planets[at(haven)].stock[kStims] = 3 * market::reserve(c, w.planets[at(haven)], kStims);
    smuggling::post(w, haven, nullptr);
    const Id raider = armed(w, 0, 2, 2);
    w.characters[at(raider)].pirate = true;
    w.characters[at(raider)].last_planet = haven;
    const Id bold = chars::spawn(w, 0);
    w.characters[at(bold)].risk = 0.95;
    w.characters[at(bold)].activity = Activity::Exploring;
    REQUIRE(move::travel_to(w, bold, haven));
    // The raider waits on station on the independent's first stretch of open space.
    const std::vector<Leg>& path = w.characters[at(bold)].route;
    REQUIRE(path.front().kind == LegKind::Space);  // it leaves planet 0 in open space
    Leg station = path.front();
    station.x1 = station.x0, station.y1 = station.y0, station.to_planet = 0, station.to_node = kNone, station.wait = 2 * kDay;
    REQUIRE(move::start_route(w, raider, {station}));
    w.characters[at(raider)].activity = Activity::Raiding;
    w.characters[at(raider)].last_planet = haven;  // where it's from
    encounters::step(w, nullptr);
    CHECK(w.ledger.offers == 1);
    CHECK(w.characters[at(bold)].offer != kNone);
    CHECK(pirates::relation(w, w.characters[at(bold)]) >= c.pirel.accept);
    CHECK(w.characters[at(bold)].battle == kNone);  // spared
}

TEST_CASE("contraband bought from smugglers costs a lawful population productivity (PRD-6, SYS-1)") {
    World w = new_world(quiet(), 107);
    World control = w;  // the same day without the contraband
    Planet& p = w.planets[0];
    p.contraband_today = 0.2 * p.population;
    economy::planet_day(w, 0);
    economy::planet_day(control, 0);
    CHECK(p.contraband_avg > 0);
    CHECK(p.productivity < control.planets[0].productivity);
    // And its governor answers with policing (PIR-15).
    p.contraband_avg = 0.05;
    budget::weekly(w, 0, nullptr);
    CHECK(p.policing > 0);
}

TEST_CASE("a bottom-rung character with nothing to do is posted an errand within two days (MSN-14)") {
    Config c = quiet();
    c.passenger.per_planet = 0;
    World w = new_world(c, 94);
    Id home = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && home == kNone; ++p) {
        if (w.planets[at(p)].pirate || w.planets[at(p)].governor == kNone) continue;
        for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
            if (q != p && !w.planets[at(q)].pirate && w.derived.travel(p, q, false) <= c.passenger.max_days) home = p;
        }
    }
    REQUIRE(home != kNone);
    const Id id = chars::spawn(w, home);
    Character& ch = w.characters[at(id)];
    ch.wealth = 0;
    ch.ships = {ships::variant(w.map, ch.culture, ShipClass::Courier, 1, false)};
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) info::observe_market(w, ch.facts, p);
    // No errand before the shortest wait...
    advance(w, static_cast<Time>(c.passenger.wait_min_days * kDay) - kHour);
    CHECK(w.ledger.errands == 0);
    CHECK(w.ledger.wanders == 0);  // it waits for paid work instead of wandering
    // ...and one by the longest, which it flies and is paid for as free money.
    advance(w, static_cast<Time>((c.passenger.wait_max_days - c.passenger.wait_min_days) * kDay) + 2 * kHour);
    CHECK(w.ledger.errands >= 1);
    advance(w, static_cast<Time>(c.passenger.max_days * 2 * kDay));
    CHECK(w.ledger.passenger_runs >= 1);
    CHECK(w.characters[at(id)].wealth > 0);
}

TEST_CASE("the broke clock runs at landing, not only in port at the daily tick (PIR-4, CHR-16)") {
    Config c = quiet();
    c.pirate.turn_p = 0;
    World w = new_world(c, 95);
    Id home = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && home == kNone; ++p) {
        if (!w.planets[at(p)].pirate && w.planets[at(p)].governor != kNone) home = p;
    }
    const Id id = chars::spawn(w, home);
    Character& ch = w.characters[at(id)];
    ch.wealth = -500;
    ch.broke_since = w.now - (c.chars.broke_days + 1) * kDay;
    ch.planet = home;
    chars::landed(w, id, nullptr);
    CHECK(w.ledger.bankruptcies == 1);
    CHECK(w.ledger.bankrupt_chars == 1);
    CHECK(w.characters[at(id)].wealth >= 0);  // solvent: the hull sold or the debt written off
}
