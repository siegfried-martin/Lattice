#include <doctest.h>

#include <algorithm>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "helpers.hpp"
#include "lattice/save.hpp"
#include "politics/budget.hpp"
#include "politics/fealty.hpp"
#include "politics/governor.hpp"
#include "politics/opinion.hpp"
#include "trade/osmosis.hpp"
#include "war/armies.hpp"
#include "war/combat.hpp"
#include "war/diplomacy.hpp"

using namespace lattice;

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

Config quiet() {
    Config c;
    c.chars.target = 0;
    c.chars.spawn_per_day = 0;
    c.courier.mission_share = 0;
    c.dip.declare_threshold = 1e9;  // no wars but the ones a test declares
    c.value.invade_months = 0;      // and no invasions but the ones it raises
    c.value.gate_months = 0;
    return c;
}

// A sworn commander for a planet's governor, with n gunships of a tier, at the planet.
Id commander(World& w, Id planet, int n, int tier = 1) {
    const Id id = chars::spawn(w, planet);
    Character& c = w.characters[at(id)];
    c.ships.assign(static_cast<std::size_t>(n), ships::variant(w.map, c.culture, ShipClass::Gunship, tier));
    c.wealth = 1e6;
    contracts::sign(w, ContractKind::Service, w.planets[at(planet)].governor, id, budget::stipend(w, c), 30 * kDay);
    c.activity = Activity::Serving;
    return id;
}

// Two roots near each other, and the nearest pair of their planets.
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

void strip_defences(World& w, Id planet) {
    Planet& p = w.planets[at(planet)];
    p.security = 0;
    p.defence = 0;
    const Id gov = p.governor;
    for (Id k : std::vector<Id>(w.characters[at(gov)].contracts)) {
        if (w.contracts[at(k)].employer == gov && w.contracts[at(k)].kind == ContractKind::Service) contracts::end(w, k);
    }
}

}  // namespace

TEST_CASE("war, ceasefire and peace between roots (DIP-1, DIP-2)") {
    World w = new_world(quiet(), 61);
    const Front f = front(w);
    REQUIRE(f.root_a != kNone);
    CHECK(diplomacy::neighbours(w, f.root_a, f.root_b));
    CHECK(diplomacy::state(w, f.root_a, f.root_b) == DipState::Peace);
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    CHECK(diplomacy::at_war(w, f.root_b, f.root_a));
    CHECK(diplomacy::planets_at_war(w, f.a, f.b));
    CHECK(diplomacy::enemies(w, f.root_a) == std::vector<Id>{f.root_b});

    diplomacy::make_ceasefire(w, f.root_a, f.root_b, 1000, nullptr);
    CHECK(diplomacy::state(w, f.root_a, f.root_b) == DipState::Ceasefire);
    const double payee = w.planets[at(f.root_b)].treasury;
    diplomacy::pay_ceasefire(w, f.root_a, nullptr);
    CHECK(w.planets[at(f.root_b)].treasury == doctest::Approx(payee + 1000));
    advance(w, (w.config.dip.ceasefire_days + 8) * kDay);
    CHECK(diplomacy::state(w, f.root_a, f.root_b) == DipState::Peace);
}

TEST_CASE("no war is declared during the warm-up (GEN-10)") {
    Config c = quiet();
    c.dip.declare_threshold = -1e9;  // everyone wants war
    World w = new_world(c, 62);
    advance(w, (c.gen.warmup_days - 1) * kDay);
    CHECK(w.ledger.wars == 0);
    advance(w, 14 * kDay);
    CHECK(w.ledger.wars > 0);
}

TEST_CASE("war stops osmosis between the factions (TRD-6)") {
    World w = new_world(quiet(), 63);
    const Front f = front(w);
    w.derived.osmosis_pairs = {{f.a, f.b, 1, 1}};
    for (int g = 0; g < kLawfulGoodCount; ++g) {
        w.planets[at(f.a)].stock[at(g)] *= 3;
        w.planets[at(f.b)].stock[at(g)] *= 0.2;
    }
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    CHECK(osmosis::run(w) == 0);
}

TEST_CASE("an enemy's gates are shut: the route gets on and off elsewhere (WLD-12, WLD-13)") {
    World w = new_world(quiet(), 64);
    const Front f = front(w);
    // Somewhere deep in b's faction, reached on the Lattice.
    Id deep = kNone;
    for (Id p : fealty::faction_planets(w, f.root_b)) {
        if (w.derived.route(f.a, p).lattice > 0 && (deep == kNone || w.derived.travel(f.a, p) > w.derived.travel(f.a, deep))) deep = p;
    }
    REQUIRE(deep != kNone);
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    const std::vector<Leg> legs = move::plan(w, f.root_a, f.a, deep);
    REQUIRE(!legs.empty());
    CHECK(legs.back().to_planet == deep);
    for (std::size_t i = 0; i < legs.size(); ++i) {
        // No open-space leg to or from an interchange held by the enemy.
        auto enemy_gate = [&](Id node) {
            if (node == kNone || w.map.nodes[at(node)].kind != NodeKind::Interchange) return false;
            return fealty::root_of(w, w.map.groups[at(w.map.nodes[at(node)].group)].holder) == f.root_b;
        };
        if (legs[i].kind == LegKind::Space) CHECK(!enemy_gate(legs[i].to_node));
        if (legs[i].kind == LegKind::Lattice && i + 1 < legs.size()) CHECK(!enemy_gate(legs[i].to_node));
    }
}

TEST_CASE("battles remove whole ships, weakest first, flagships last (SYS-14)") {
    World w = new_world(quiet(), 65);
    const Id x = commander(w, 0, 1, 1);
    Character& c = w.characters[at(x)];
    c.ships = {ships::variant(w.map, c.culture, ShipClass::Gunship, 3), ships::variant(w.map, c.culture, ShipClass::Courier, 1),
               ships::variant(w.map, c.culture, ShipClass::Gunship, 1)};
    std::vector<Id> side{x};
    double carried = 0;
    // Enough to take the courier and the small gunship, not the flagship.
    const double small = w.map.variants[at(c.ships[2])].combat;
    combat::destroy(w, side, small + 0.01, carried, nullptr);
    REQUIRE(w.characters[at(x)].ships.size() == 1);
    CHECK(w.map.variants[at(w.characters[at(x)].ships[0])].tier == 3);
    // Losing the last ship ends its service (CHR-16).
    combat::destroy(w, side, 1e9, carried, nullptr);
    CHECK(w.characters[at(x)].ships.empty());
    CHECK(contracts::held(w, x, ContractKind::Service) == kNone);
    CHECK(side.empty());
}

TEST_CASE("the stronger side wins a battle in rounds (SYS-14)") {
    World w = new_world(quiet(), 66);
    const Id strong = commander(w, 0, 4, 2), weak = commander(w, 0, 1, 1);
    combat::start(w, {strong}, {weak}, kNone, false, kNone, kNone, nullptr);
    advance(w, 3 * kDay);
    CHECK(!w.battles[0].alive);
    CHECK(w.characters[at(strong)].ships.size() >= 3);
    CHECK(w.characters[at(weak)].battle == kNone);
    CHECK(w.battles[0].rounds >= 1);
}

TEST_CASE("security spending buys static defence, more at war (MIL-5)") {
    World w = new_world(quiet(), 67);
    const Front f = front(w);
    budget::weekly(w, f.a, nullptr);
    const double peace = w.planets[at(f.a)].security;
    CHECK(peace > 0);
    advance(w, 30 * kDay);
    CHECK(w.planets[at(f.a)].defence == doctest::Approx(w.config.defence.per_spend * w.planets[at(f.a)].security));
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    budget::weekly(w, f.a, nullptr);
    CHECK(w.planets[at(f.a)].security > peace);
}

TEST_CASE("an invasion besieges, wins and captures (SYS-13, FAC-8, FAC-9, FAC-10)") {
    World w = new_world(quiet(), 68);
    const Front f = front(w);
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    for (int i = 0; i < 4; ++i) commander(w, f.a, 3, 2);
    strip_defences(w, f.b);
    w.planets[at(f.b)].defence = 5;
    w.planets[at(f.b)].security = 1000;
    for (Id p : fealty::faction_planets(w, f.root_b)) w.planets[at(p)].liege = p == f.root_b ? kNone : w.planets[at(p)].liege;
    const Id old_gov = w.planets[at(f.b)].governor;
    const double pop = w.planets[at(f.b)].population;
    const Id army = armies::commission(w, f.a, f.b, false, nullptr);
    REQUIRE(army != kNone);
    advance(w, 60 * kDay);
    CHECK(w.ledger.sieges >= 1);
    REQUIRE(w.ledger.captures == 1);
    CHECK(fealty::root_of(w, f.b) == f.root_a);
    CHECK(w.planets[at(f.b)].governor != old_gov);
    CHECK(w.characters[at(old_gov)].governs == kNone);
    CHECK(w.characters[at(w.planets[at(f.b)].governor)].governs == f.b);
    CHECK(w.planets[at(f.b)].population < pop);
    CHECK(!w.armies[at(army)].alive);
}

TEST_CASE("a besieged planet calls for help and its liege relieves it (MIL-11)") {
    World w = new_world(quiet(), 69);
    const Front f = front(w);
    // The defender: a vassal planet whose liege has a strong defence force.
    Id target = kNone;
    for (Id p : fealty::faction_planets(w, f.root_b)) {
        const Id liege = w.planets[at(p)].liege;
        if (liege != kNone && liege != p && w.derived.travel(f.a, p) < 6 && (target == kNone || w.derived.travel(f.a, p) < w.derived.travel(f.a, target))) target = p;
    }
    REQUIRE(target != kNone);
    const Id liege = w.planets[at(target)].liege;
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    commander(w, f.a, 4, 2);  // strong enough to keep up a siege
    strip_defences(w, target);
    w.planets[at(target)].defence = 30;
    w.planets[at(target)].security = 6000;
    for (int i = 0; i < 4; ++i) commander(w, liege, 3, 3);
    REQUIRE(armies::commission(w, f.a, target, false, nullptr) != kNone);
    advance(w, 60 * kDay);
    CHECK(w.ledger.sieges >= 1);
    CHECK(w.ledger.reliefs >= 1);
    CHECK(w.ledger.captures == 0);
    CHECK(fealty::root_of(w, target) == f.root_b);
    // HelpSent (REL-3): the planet's governor thinks better of its liege.
    const Id asking = w.planets[at(target)].governor, helper = w.planets[at(liege)].governor;
    CHECK(opinion::get(w, asking, helper) > opinion::rest(w, asking, helper));
}

TEST_CASE("taking a root passes its wars to the faction's new root (MOD-5)") {
    World w = new_world(quiet(), 70);
    const Front f = front(w);
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    std::vector<Id> below = fealty::vassals(w, f.root_b);
    REQUIRE(!below.empty());
    for (int i = 0; i < 6; ++i) commander(w, f.root_a, 3, 3);
    strip_defences(w, f.root_b);
    w.planets[at(f.root_b)].defence = 1;
    // March straight on the enemy's root.
    REQUIRE(armies::commission(w, f.root_a, f.root_b, false, nullptr) != kNone);
    advance(w, 60 * kDay);
    REQUIRE(w.ledger.captures >= 1);
    CHECK(fealty::root_of(w, f.root_b) == f.root_a);
    // One of its vassals leads what's left, and is still at war.
    Id heir = kNone;
    for (Id v : below) {
        if (w.planets[at(v)].liege == kNone) heir = v;
    }
    REQUIRE(heir != kNone);
    CHECK(diplomacy::at_war(w, f.root_a, heir));
}

TEST_CASE("keep or grant: a vassal much nearer the prize, in favour, is granted it (FAC-8)") {
    World w = new_world(quiet(), 71);
    const Front f = front(w);
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    // The commissioner is the root; its vassal nearest the target will be granted it.
    Id near = kNone;
    for (Id v : fealty::vassals(w, f.root_a)) {
        if (near == kNone || w.derived.travel(v, f.b) < w.derived.travel(near, f.b)) near = v;
    }
    REQUIRE(near != kNone);
    w.config.value.protect = 1;  // distance dominates
    for (int i = 0; i < 4; ++i) opinion::deed(w, w.planets[at(f.root_a)].governor, w.planets[at(near)].governor, DeedKind::PlanetGranted);
    for (int i = 0; i < 5; ++i) commander(w, f.root_a, 3, 3);
    strip_defences(w, f.b);
    w.planets[at(f.b)].defence = 1;
    if (w.derived.travel(near, f.b) >= w.derived.travel(f.root_a, f.b)) return;  // the root is nearest: nothing to grant
    REQUIRE(armies::commission(w, f.root_a, f.b, false, nullptr) != kNone);
    advance(w, 60 * kDay);
    REQUIRE(w.ledger.captures == 1);
    CHECK(w.planets[at(f.b)].liege != f.root_a);
    CHECK(fealty::root_of(w, f.b) == f.root_a);
}

TEST_CASE("a world at war is still deterministic and saveable") {
    Config c = test::small();
    c.dip.declare_threshold = -1e9;
    World a = new_world(c, 72);
    advance(a, (c.gen.warmup_days + 40) * kDay);
    World b = load_world(save_world(a));
    advance(a, 40 * kDay);
    advance(b, 40 * kDay);
    CHECK(state_hash(a) == state_hash(b));
    CHECK(a.ledger.wars > 0);
}
