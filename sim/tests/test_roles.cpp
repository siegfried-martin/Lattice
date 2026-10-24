#include <doctest.h>

#include <algorithm>
#include <cmath>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/missions.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "politics/budget.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"
#include "politics/roles.hpp"
#include "war/armies.hpp"
#include "war/combat.hpp"
#include "war/diplomacy.hpp"

using namespace lattice;

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

// No one but the seeded governors, and nothing happening on its own.
Config quiet() {
    Config c;
    c.chars.target = 0;
    c.chars.spawn_per_day = 0;
    c.courier.mission_share = 0;
    c.dip.declare_threshold = 1e9;
    c.gen.pirate_planets = 0;
    c.value.invade_months = 0;
    c.raid.share = 0;
    c.rebel.scale = 0;
    return c;
}

Id liege_with_vassal(const World& w, Id& vassal) {
    for (Id v = 0; v < static_cast<Id>(w.planets.size()); ++v) {
        const Id l = w.planets[at(v)].liege;
        if (l != kNone && w.planets[at(l)].liege == kNone) {
            vassal = v;
            return l;
        }
    }
    return kNone;
}

// Gives a planet a month's worth of net income, so posts with salaries can be offered.
void income(World& w, Id planet, double per_day) {
    w.planets[at(planet)].net_avg = per_day;
    w.planets[at(planet)].income_avg = per_day;
}

// Kills a governor the way a battle does: no fleet, and the pod is lost.
void kill(World& w, Id id) {
    w.config.combat.pod_death_p = 1;
    std::vector<Id> side{id};
    double carried = 1e12;
    combat::destroy(w, side, 1e12, carried, nullptr);
}

}  // namespace

TEST_CASE("fitness levels off in power, cargo and reputation and reads the chooser's opinion (CHR-20, SYS-22)") {
    World w = new_world(quiet(), 201);
    const Id planet = 0;
    const Id a = chars::spawn(w, planet);
    Character& c = w.characters[at(a)];
    c.ships = {ships::variant(w.map, c.culture, ShipClass::Freighter, 1)};
    c.reputation = 10;
    const Id chooser = w.planets[at(planet)].governor;
    opinion::deed(w, chooser, a, DeedKind::Patronage, nullptr);
    const Config& k = w.config;
    const ships::Fleet f = ships::fleet(w, c);
    const double o = opinion::get(w, chooser, a);
    const auto expect = [&](const Config::FitWeights& wt) {
        return wt.power * f.combat / (f.combat + k.value.power_ref) + wt.trade * f.cargo / (f.cargo + k.fit.cargo_half) +
               wt.rep * 10 / (10 + k.hire.rep_half) + wt.opinion * o / 100 + wt.culture * (c.culture == w.planets[0].region ? 1 : 0);
    };
    CHECK(roles::fitness(w, roles::Post::Steward, chooser, a, planet) == doctest::Approx(expect(k.fit.steward)));
    CHECK(roles::fitness(w, roles::Post::Governor, chooser, a, planet) == doctest::Approx(expect(k.fit.governor)));
    // A merchant suits the steward's post better than a governor's (CHR-20's weights).
    CHECK(roles::fitness(w, roles::Post::Steward, chooser, a, planet) > roles::fitness(w, roles::Post::Governor, chooser, a, planet));
    // Twice the cargo is worth less than twice the trade term.
    const auto fleet = c.ships;
    c.ships.clear();
    const double none = roles::fitness(w, roles::Post::Steward, kNone, a, planet);
    c.ships = fleet;
    const double one = roles::fitness(w, roles::Post::Steward, kNone, a, planet) - none;
    c.ships.push_back(c.ships[0]);
    const double two = roles::fitness(w, roles::Post::Steward, kNone, a, planet) - none;
    CHECK(two > one);
    CHECK(two < 2 * one);
}

TEST_CASE("a steward is appointed, paid, sets the growth factor and keeps the planet's information (FAC-16, SYS-22)") {
    World w = new_world(quiet(), 202);
    const Id planet = 0;
    income(w, planet, 2000);
    const Id m = chars::spawn(w, planet);
    Character& s = w.characters[at(m)];
    s.ships = {ships::variant(w.map, s.culture, ShipClass::Freighter, 1)};
    s.risk = 0.9;
    roles::steward_weekly(w, planet, nullptr);
    Planet& p = w.planets[at(planet)];
    REQUIRE(p.steward == m);
    CHECK(s.stewards == planet);
    CHECK(s.activity == Activity::Stewarding);
    const Id k = contracts::held(w, m, ContractKind::Steward);
    REQUIRE(k != kNone);
    CHECK(w.contracts[at(k)].pay == doctest::Approx(w.config.steward.salary_share * 2000 * w.config.time.payment_days));
    CHECK(w.ledger.stewards_appointed == 1);
    // PRD-3 follows the steward now.
    const auto& pop = w.config.pop;
    CHECK(p.growth_factor == doctest::Approx(pop.growth_factor_min + 0.9 * (pop.growth_factor_max - pop.growth_factor_min)));
    // A steward isn't for hire as a commander, and is paid with the stipends.
    const double before = s.wealth;
    budget::pay_stipends(w, planet, nullptr);
    CHECK(s.wealth == doctest::Approx(before + w.contracts[at(k)].pay));
    // With the governor away, the planet's information is the steward's.
    Character& g = w.characters[at(p.governor)];
    CHECK(roles::in_charge(w, planet) == p.governor);
    g.planet = kNone;
    CHECK(roles::in_charge(w, planet) == m);
    CHECK(&info::planet_facts(std::as_const(w), planet) == &s.facts);
    g.planet = planet;
    // Its contract ending empties the post.
    contracts::end(w, k);
    CHECK(p.steward == kNone);
    CHECK(s.stewards == kNone);
}

TEST_CASE("a governor's fleet defends at home and marches only with a steward (MIL-20)") {
    World w = new_world(quiet(), 203);
    const Id planet = 0;
    const Id gov = w.planets[at(planet)].governor;
    REQUIRE(!w.characters[at(gov)].ships.empty());
    auto has = [&](const std::vector<Id>& v) { return std::find(v.begin(), v.end(), gov) != v.end(); };
    CHECK(has(armies::ready(w, planet)));
    CHECK(!has(armies::ready(w, planet, true)));
    CHECK(!roles::can_sail(w, planet));
    income(w, planet, 2000);
    const Id m = chars::spawn(w, planet);
    roles::steward_weekly(w, planet, nullptr);
    REQUIRE(w.planets[at(planet)].steward == m);
    CHECK(roles::can_sail(w, planet));
    CHECK(has(armies::ready(w, planet, true)));
    // Its own fleet is a commander unit in its budget (SYS-5).
    w.planets[at(planet)].treasury = 1e7;
    w.config.value.defence = 1e3;
    budget::weekly(w, planet, nullptr);
    CHECK(w.planets[at(planet)].governor_stipend == doctest::Approx(budget::stipend(w, w.characters[at(gov)])));
}

TEST_CASE("a vassal governor's death waits for its liege to hear, then a successor takes everything over (FAC-17)") {
    World w = new_world(quiet(), 204);
    Id vassal = kNone;
    const Id liege = liege_with_vassal(w, vassal);
    REQUIRE(liege != kNone);
    const Id dead = w.planets[at(vassal)].governor;
    // A commander sworn to the dead governor, and one sworn to the liege.
    const Id kept = chars::spawn(w, vassal);
    w.characters[at(kept)].ships = {ships::variant(w.map, 0, ShipClass::Gunship, 1)};
    const Id contract = contracts::sign(w, ContractKind::Service, dead, kept, 100, 30 * kDay);
    w.characters[at(kept)].activity = Activity::Serving;
    const Id heir = chars::spawn(w, liege);
    w.characters[at(heir)].ships = {ships::variant(w.map, 0, ShipClass::Gunship, 2)};
    contracts::sign(w, ContractKind::Service, w.planets[at(liege)].governor, heir, 100, 30 * kDay);
    w.characters[at(heir)].activity = Activity::Serving;

    kill(w, dead);
    REQUIRE(!w.characters[at(dead)].alive);
    CHECK(roles::vacant(w, vassal));
    CHECK(w.ledger.governors_killed == 1);
    CHECK(w.contracts[at(contract)].alive);  // waiting for the successor
    CHECK(roles::settle(w, vassal, nullptr));  // the liege hasn't heard yet
    w.now = w.planets[at(vassal)].succession_at;
    CHECK(!roles::settle(w, vassal, nullptr));
    CHECK(!roles::vacant(w, vassal));
    CHECK(w.planets[at(vassal)].governor == heir);
    CHECK(w.characters[at(heir)].governs == vassal);
    CHECK(contracts::held(w, heir, ContractKind::Service) == kNone);  // its own service ended
    CHECK(w.contracts[at(contract)].alive);
    CHECK(w.contracts[at(contract)].employer == heir);
    CHECK(w.ledger.successions == 1);
    CHECK(w.ledger.vacant_days > 0);
}

TEST_CASE("a root's steward succeeds at once; without one its most loyal vassal becomes root (FAC-17)") {
    SUBCASE("with a steward") {
        World w = new_world(quiet(), 205);
        Id vassal = kNone;
        const Id root = liege_with_vassal(w, vassal);
        REQUIRE(root != kNone);
        income(w, root, 2000);
        const Id m = chars::spawn(w, root);
        roles::steward_weekly(w, root, nullptr);
        REQUIRE(w.planets[at(root)].steward == m);
        kill(w, w.planets[at(root)].governor);
        CHECK(w.planets[at(root)].governor == m);
        CHECK(w.planets[at(root)].steward == kNone);
        CHECK(w.planets[at(root)].liege == kNone);
        CHECK(!roles::vacant(w, root));
    }
    SUBCASE("without") {
        World w = new_world(quiet(), 205);
        Id vassal = kNone;
        const Id root = liege_with_vassal(w, vassal);
        REQUIRE(root != kNone);
        const Id dead = w.planets[at(root)].governor;
        const std::vector<Id> below = fealty::vassals(w, root);
        REQUIRE(!below.empty());
        // The last vassal loves it most.
        const Id loyal = below.back();
        for (int i = 0; i < 3; ++i) opinion::deed(w, w.planets[at(loyal)].governor, dead, DeedKind::PlanetGranted, nullptr);
        kill(w, dead);
        CHECK(w.planets[at(loyal)].liege == kNone);
        CHECK(w.planets[at(root)].liege == loyal);
        for (Id v : below) {
            if (v != loyal) CHECK(w.planets[at(v)].liege == loyal);
        }
        CHECK(!roles::vacant(w, root));
        CHECK(w.characters[at(w.planets[at(root)].governor)].alive);
    }
}

TEST_CASE("a mission past its deadline fails: a bad deed, no pay, reputation unchanged (MSN-3, SYS-8)") {
    World w = new_world(quiet(), 206);
    Id vassal = kNone;
    const Id liege = liege_with_vassal(w, vassal);
    REQUIRE(liege != kNone);
    missions::post_courier(w, liege, vassal, nullptr);
    const Id mid = w.planets[at(liege)].missions.back();
    const Id taker = chars::spawn(w, liege);
    missions::take(w, taker, mid, nullptr);
    Mission& m = w.missions[at(mid)];
    REQUIRE(m.state == MissionState::Taken);
    CHECK(m.deadline > w.now);
    CHECK(w.ledger.issuer_taken[static_cast<std::size_t>(Issuer::Root)] == 1);
    const double rep = w.characters[at(taker)].reputation;
    const double before = opinion::get(w, m.poster, taker);
    const double wealth = w.characters[at(taker)].wealth;
    missions::deadline(w, mid, nullptr);  // not yet
    CHECK(m.state == MissionState::Taken);
    w.now = m.deadline;
    missions::deadline(w, mid, nullptr);
    CHECK(m.state == MissionState::Failed);
    CHECK(w.characters[at(taker)].mission == kNone);
    CHECK(contracts::held(w, taker, ContractKind::Courier) == kNone);
    CHECK(opinion::get(w, m.poster, taker) < before);
    CHECK(w.characters[at(taker)].reputation == rep);
    CHECK(w.characters[at(taker)].wealth == wealth);
    CHECK(w.ledger.issuer_failed[static_cast<std::size_t>(Issuer::Root)] == 1);
}

TEST_CASE("losing the fleet a mission needed fails it (MSN-3)") {
    World w = new_world(quiet(), 207);
    Id vassal = kNone;
    const Id liege = liege_with_vassal(w, vassal);
    REQUIRE(liege != kNone);
    missions::post_courier(w, liege, vassal, nullptr);
    const Id mid = w.planets[at(liege)].missions.back();
    const Id taker = chars::spawn(w, liege);
    missions::take(w, taker, mid, nullptr);
    w.config.combat.pod_death_p = 0;
    std::vector<Id> side{taker};
    double carried = 1e12;
    combat::destroy(w, side, 1e12, carried, nullptr);
    CHECK(w.missions[at(mid)].state == MissionState::Failed);
    CHECK(w.characters[at(taker)].mission == kNone);
}

TEST_CASE("a world with stewards and sailing governors runs, and every planet keeps a governor (CHK-29)") {
    Config c;
    c.chars.target = 150;
    World w = new_world(c, 208);
    advance(w, 400 * kDay);
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        const Planet& pl = w.planets[at(p)];
        REQUIRE(pl.governor != kNone);
        const Character& g = w.characters[at(pl.governor)];
        CHECK((g.alive || roles::vacant(w, p)));
        if (g.alive) CHECK(g.governs == p);
        if (pl.steward != kNone) {
            CHECK(w.characters[at(pl.steward)].stewards == p);
            CHECK(contracts::held(w, pl.steward, ContractKind::Steward) != kNone);
        }
    }
    CHECK(w.ledger.stewards_appointed > 0);
}
