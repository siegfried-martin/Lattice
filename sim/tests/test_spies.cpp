#include <doctest.h>

#include <algorithm>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "pirates/suspicion.hpp"
#include "politics/budget.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"
#include "war/armies.hpp"
#include "war/diplomacy.hpp"
#include "war/spies.hpp"

using namespace lattice;

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

Config quiet() {
    Config c;
    c.chars.target = 0;
    c.chars.spawn_per_day = 0;
    c.courier.mission_share = 0;
    c.gen.pirate_planets = 0;
    c.dip.declare_threshold = 1e9;
    c.value.invade_months = 0;
    c.raid.share = 0;
    c.sus.inspect_base = 0;  // nobody looks, unless a test says so
    return c;
}

struct Front {
    Id root_a = kNone, root_b = kNone, a = kNone, b = kNone;
};
Front front(const World& w) {
    Front f;
    double best = 1e9;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
            const Id rp = fealty::root_of(w, p), rq = fealty::root_of(w, q);
            if (rp == rq || fealty::faction_planets(w, rp).size() < 3 || fealty::faction_planets(w, rq).size() < 3) continue;
            if (w.derived.travel(p, q) < best) best = w.derived.travel(p, q), f = {rp, rq, p, q};
        }
    }
    return f;
}

Id trader(World& w, Id planet) {
    const Id id = chars::spawn(w, planet);
    Character& c = w.characters[at(id)];
    c.ships = {ships::variant(w.map, c.culture, ShipClass::Freighter, 1)};
    c.wealth = 50000;
    return id;
}

void know_war(World& w, const Front& f, Id planet) {
    info::learn_state(w.characters[at(w.planets[at(planet)].governor)].facts, f.root_a, f.root_b, DipState::War, w.now);
}

}  // namespace

TEST_CASE("a spy's contract is secret: it reads as an independent (SYS-17, INF-13)") {
    World w = new_world(quiet(), 111);
    const Front f = front(w);
    const Id id = trader(w, f.a);
    spies::hire(w, f.a, id, nullptr);
    CHECK(spies::employer(w, id) == w.planets[at(f.a)].governor);
    CHECK(chars::faction_of(w, w.characters[at(id)]) == kNone);
}

TEST_CASE("a spy goes to an enemy market, trades for cover, and reports home (INF-13)") {
    World w = new_world(quiet(), 112);
    const Front f = front(w);
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    know_war(w, f, f.a);
    const Id id = trader(w, f.a);
    spies::hire(w, f.a, id, nullptr);
    advance(w, 60 * kDay);
    CHECK(w.ledger.spy_visits >= 1);
    // Home knows the enemy's market now.
    const Facts& home = w.characters[at(w.planets[at(f.a)].governor)].facts;
    bool knows_enemy = false;
    for (const PriceFact& p : home.prices) knows_enemy = knows_enemy || fealty::root_of(w, p.planet) == f.root_b;
    CHECK(knows_enemy);
}

TEST_CASE("a spy sees an army forming and its home raises the alarm (INF-13, MIL-11)") {
    World w = new_world(quiet(), 113);
    const Front f = front(w);
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    know_war(w, f, f.a);
    // An army forming at b against a.
    w.config.army.muster_days = 60;
    const Id cmd = chars::spawn(w, f.b);
    w.characters[at(cmd)].ships = {ships::variant(w.map, w.characters[at(cmd)].culture, ShipClass::Gunship, 2)};
    contracts::sign(w, ContractKind::Service, w.planets[at(f.b)].governor, cmd, 1, 30 * kDay);
    w.characters[at(cmd)].activity = Activity::Serving;
    const Id army = armies::commission(w, f.b, f.a, false, nullptr);
    REQUIRE(army != kNone);
    w.armies[at(army)].action = ArmyAction::Muster;  // still gathering
    w.armies[at(army)].planet = f.b;
    // A spy of a's lands at b, then at home.
    const Id id = trader(w, f.b);
    contracts::sign(w, ContractKind::Spy, w.planets[at(f.a)].governor, id, 1, 30 * kDay);
    w.characters[at(id)].activity = Activity::Spying;
    spies::landed(w, id, nullptr);
    CHECK(w.ledger.musters_seen == 1);
    w.characters[at(id)].planet = f.a;
    spies::landed(w, id, nullptr);
    const Facts& home = w.characters[at(w.planets[at(f.a)].governor)].facts;
    bool seen = false;
    for (const Sighting& sg : home.sightings) seen = seen || (sg.army == army && sg.target == f.a);
    CHECK(seen);
    CHECK(w.ledger.warnings >= 1);  // a is the target: it warns its liege, or decides itself
}

TEST_CASE("a spy whose cover is blown loses its contract and is attacked (INF-13)") {
    Config c = quiet();
    c.sus.inspect_base = 1;
    c.sus.find_base = 1;
    World w = new_world(c, 114);
    const Front f = front(w);
    const Id id = trader(w, f.b);
    contracts::sign(w, ContractKind::Spy, w.planets[at(f.a)].governor, id, 1, 30 * kDay);
    const Id guard = chars::spawn(w, f.b);
    w.characters[at(guard)].ships = {ships::variant(w.map, w.characters[at(guard)].culture, ShipClass::Gunship, 2)};
    contracts::sign(w, ContractKind::Service, w.planets[at(f.b)].governor, guard, 1, 30 * kDay);
    w.characters[at(guard)].activity = Activity::Serving;
    CHECK(suspicion::on_landing(w, id, nullptr));
    CHECK(spies::employer(w, id) == kNone);
    CHECK(w.ledger.spies_caught == 1);
    CHECK(w.characters[at(id)].battle != kNone);
    CHECK(opinion::get(w, w.planets[at(f.b)].governor, id) < 0);
}

TEST_CASE("governors hire spies at war, never at peace, and not with spies off (EXP-4)") {
    World w = new_world(quiet(), 115);
    const Front f = front(w);
    for (int i = 0; i < 5; ++i) trader(w, f.a);
    budget::weekly(w, f.a, nullptr);
    CHECK(spies::employed(w, w.planets[at(f.a)].governor).empty());
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    know_war(w, f, f.a);
    budget::weekly(w, f.a, nullptr);
    CHECK(!spies::employed(w, w.planets[at(f.a)].governor).empty());

    Config off = quiet();
    off.spy.enabled = false;
    World v = new_world(off, 115);
    for (int i = 0; i < 5; ++i) trader(v, f.a);
    diplomacy::declare_war(v, f.root_a, f.root_b, nullptr);
    know_war(v, f, f.a);
    budget::weekly(v, f.a, nullptr);
    CHECK(spies::employed(v, v.planets[at(f.a)].governor).empty());
}
