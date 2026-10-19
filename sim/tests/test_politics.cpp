#include <doctest.h>

#include <cmath>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/ships.hpp"
#include "economy/economy.hpp"
#include "helpers.hpp"
#include "politics/budget.hpp"
#include "politics/fealty.hpp"
#include "politics/governor.hpp"
#include "politics/opinion.hpp"

using namespace lattice;

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

Config quiet() {
    Config c;
    c.chars.target = 0;
    c.courier.mission_share = 0;
    c.gen.pirate_planets = 0;
    return c;
}

// A planet with a liege, and that liege.
std::pair<Id, Id> sworn_pair(const World& w) {
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (w.planets[at(p)].liege != kNone) return {p, w.planets[at(p)].liege};
    }
    return {kNone, kNone};
}

Id gov_of(const World& w, Id planet) { return w.planets[at(planet)].governor; }

}  // namespace

TEST_CASE("deeds move opinion, which fades back toward rest (SYS-10, REL-3)") {
    World w = new_world(quiet(), 41);
    const Id a = gov_of(w, 0), b = gov_of(w, 1);
    CHECK(opinion::get(w, a, b) == opinion::rest(w, a, b));
    opinion::deed(w, a, b, DeedKind::Patronage);
    const double after = opinion::get(w, a, b);
    CHECK(after == doctest::Approx(opinion::rest(w, a, b) + w.config.deed.amount[static_cast<std::size_t>(DeedKind::Patronage)]));
    advance(w, 7 * kDay);
    const double rest = opinion::rest(w, a, b);
    CHECK(opinion::get(w, a, b) == doctest::Approx(rest + (after - rest) * (1 - w.config.rel.fade)));
    for (int i = 0; i < 20; ++i) opinion::deed(w, a, b, DeedKind::Attacked);
    CHECK(opinion::get(w, a, b) == -100);  // clamped
    CHECK(w.characters[at(a)].opinions[0].recent.size() == static_cast<std::size_t>(w.config.rel.recent_deeds));
}

TEST_CASE("a vassal's opinion of its liege rests higher in a faction of its own culture (FAC-11, WLD-19)") {
    World w = new_world(quiet(), 42);
    const auto [vassal, liege] = sworn_pair(w);
    REQUIRE(vassal != kNone);
    const std::vector<Id> faction = fealty::faction_planets(w, fealty::root_of(w, vassal));
    double same = 0;
    for (Id p : faction) same += w.planets[at(p)].region == w.characters[at(gov_of(w, vassal))].culture;
    const double expected = w.config.rel.culture_rest * (same / static_cast<double>(faction.size()) - 0.5);
    CHECK(opinion::rest(w, gov_of(w, vassal), gov_of(w, liege)) == doctest::Approx(expected));
    CHECK(opinion::rest(w, gov_of(w, liege), gov_of(w, vassal)) == 0);  // favor rests at neutral
}

TEST_CASE("tribute is a share of the month's income, cascading (FAC-6)") {
    World w = new_world(quiet(), 43);
    const auto [vassal, liege] = sworn_pair(w);
    Planet& v = w.planets[at(vassal)];
    v.income_month = 10000;
    v.tribute_month = 2000;
    const double before_v = v.treasury, before_l = w.planets[at(liege)].treasury;
    fealty::pay_tribute(w, vassal, nullptr);
    const double expected = v.tribute_rate * 12000;
    CHECK(before_v - v.treasury == doctest::Approx(expected));
    CHECK(w.planets[at(liege)].treasury - before_l == doctest::Approx(expected));
    CHECK(w.planets[at(liege)].tribute_month == doctest::Approx(expected));
    CHECK(v.income_month == 0);

    w.config.fealty.cascade = false;
    v.income_month = 10000;
    v.tribute_month = 2000;
    fealty::pay_tribute(w, vassal, nullptr);
    CHECK(v.tribute_paid == doctest::Approx(v.tribute_rate * 10000));
}

TEST_CASE("a liege in the red raises tribute on the loyal and lowers it for the wavering (FAC-12)") {
    World w = new_world(quiet(), 44);
    const auto [vassal, liege] = sworn_pair(w);
    const double rate = w.planets[at(vassal)].tribute_rate;
    w.planets[at(liege)].treasury = -1000;
    for (int i = 0; i < 3; ++i) opinion::deed(w, gov_of(w, vassal), gov_of(w, liege), DeedKind::PlanetGranted);
    fealty::adjust_tribute(w, liege, nullptr);
    CHECK(w.planets[at(vassal)].tribute_rate == doctest::Approx(rate + w.config.fealty.rate_step));
    const double loyalty = opinion::get(w, gov_of(w, vassal), gov_of(w, liege));

    for (int i = 0; i < 4; ++i) opinion::deed(w, gov_of(w, vassal), gov_of(w, liege), DeedKind::Attacked);
    fealty::adjust_tribute(w, liege, nullptr);
    CHECK(w.planets[at(vassal)].tribute_rate == doctest::Approx(rate));
    CHECK(opinion::get(w, gov_of(w, vassal), gov_of(w, liege)) < loyalty);
}

TEST_CASE("sustained low opinion and an alternative make a governor defect (FAC-11)") {
    World w = new_world(quiet(), 45);
    // A vassal with vassals of its own: its branch leaves with it.
    Id branch = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (w.planets[at(p)].liege != kNone && !fealty::vassals(w, p).empty()) branch = p;
    }
    REQUIRE(branch != kNone);
    const Id liege = w.planets[at(branch)].liege;
    const std::vector<Id> below = fealty::vassals(w, branch);
    for (int i = 0; i < 3; ++i) opinion::deed(w, gov_of(w, branch), gov_of(w, liege), DeedKind::Attacked);

    fealty::check_defection(w, branch, nullptr);
    CHECK(w.planets[at(branch)].liege == liege);  // not yet: it has to last
    advance(w, (w.config.fealty.defect_weeks * 7 - 1) * kDay);
    w.planets[at(branch)].liege = liege;  // in case the weekly cadence already moved it
    for (int i = 0; i < 3; ++i) opinion::deed(w, gov_of(w, branch), gov_of(w, liege), DeedKind::Attacked);
    advance(w, 2 * 7 * kDay);
    CHECK(fealty::root_of(w, branch) == branch);
    for (Id v : below) CHECK(fealty::root_of(w, v) == branch);
    CHECK(w.ledger.defections >= 1);
}

TEST_CASE("a defector joins a neighbouring root that thinks well of it") {
    World w = new_world(quiet(), 46);
    const auto [vassal, liege] = sworn_pair(w);
    Id rival = kNone;
    for (Id r = 0; r < static_cast<Id>(w.planets.size()); ++r) {
        if (w.planets[at(r)].liege == kNone && r != fealty::root_of(w, vassal) &&
            w.derived.travel(vassal, r) <= static_cast<double>(w.config.fealty.threat_days)) {
            rival = r;
        }
    }
    REQUIRE(rival != kNone);
    for (int i = 0; i < 3; ++i) opinion::deed(w, gov_of(w, vassal), gov_of(w, liege), DeedKind::Attacked);
    for (int i = 0; i < 2; ++i) opinion::deed(w, gov_of(w, rival), gov_of(w, vassal), DeedKind::PlanetGranted);
    w.planets[at(vassal)].low_since = w.now - w.config.fealty.defect_weeks * 7 * kDay;
    fealty::check_defection(w, vassal, nullptr);
    CHECK(w.planets[at(vassal)].liege == rival);
    CHECK(w.planets[at(vassal)].tribute_rate == w.config.fealty.tribute_rate);
}

TEST_CASE("the starting commanders match each planet's income (GEN-9)") {
    Config c = quiet();
    c.chars.target = 250;  // room for the starting commanders
    c.chars.spawn_per_day = 0;
    const World w = new_world(c, 47);
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (w.planets[at(p)].pirate) continue;
        double power = 0;
        for (Id k : w.characters[at(gov_of(w, p))].contracts) {
            const Contract& ct = w.contracts[at(k)];
            if (ct.kind == ContractKind::Service) power += ships::fleet(w, w.characters[at(ct.employee)]).combat;
        }
        const double target = c.gen.commanders_per_income * w.planets[at(p)].income_avg * c.time.payment_days;
        CHECK(power >= target);
        CHECK(power < target + 10);
    }
}

TEST_CASE("the budget keeps commanders while they're worth it and lets them go when not (SYS-5, SYS-9)") {
    Config c = quiet();
    c.chars.target = 250;  // room for the starting commanders
    c.chars.spawn_per_day = 0;
    World w = new_world(c, 48);
    Id rich = 0;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (w.planets[at(p)].income_avg > w.planets[at(rich)].income_avg) rich = p;
    }
    const Id gov = gov_of(w, rich);
    auto count = [&] {
        int n = 0;
        for (Id k : w.characters[at(gov)].contracts) n += w.contracts[at(k)].alive && w.contracts[at(k)].kind == ContractKind::Service;
        return n;
    };
    REQUIRE(count() > 0);
    // Plenty of candidates at the planet: the budget hires up to where value stops
    // covering cost.
    for (int i = 0; i < 10; ++i) {
        const Id id = chars::spawn(w, rich);
        w.characters[at(id)].ships = {ships::variant(w.map, w.characters[at(id)].culture, ShipClass::Gunship, 1)};
    }
    budget::weekly(w, rich, nullptr);
    const int hired = count();
    CHECK(hired > 0);
    CHECK(hired < 11 + 4);
    // Income gone: nobody is worth a stipend.
    w.planets[at(rich)].net_avg = 0;  // the budget reads net income (OPN-39)
    budget::weekly(w, rich, nullptr);
    CHECK(count() == 0);
    CHECK(w.ledger.fires >= hired);
}

TEST_CASE("a liege pays patronage to a vassal in the red (FAC-7)") {
    World w = new_world(quiet(), 49);
    const auto [vassal, liege] = sworn_pair(w);
    Planet& v = w.planets[at(vassal)];
    v.tribute_paid = 50000;
    v.treasury = -2 * economy::monthly_costs(w.config, v);
    const double before = w.planets[at(liege)].treasury;
    budget::weekly(w, liege, nullptr);
    CHECK(v.treasury >= 0);
    CHECK(w.planets[at(liege)].treasury < before);
    CHECK(w.ledger.patronage > 0);
    CHECK(opinion::get(w, v.governor, gov_of(w, liege)) > opinion::rest(w, v.governor, gov_of(w, liege)));
}

TEST_CASE("dues fund the Authority, which spends a share on the network (SYS-20, ECO-10)") {
    World w = new_world(quiet(), 50);
    Id holder = kNone, other = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        (governor::holds_gates(w, p) ? holder : other) = p;
    }
    REQUIRE(holder != kNone);
    REQUIRE(other != kNone);
    const double h0 = w.planets[at(holder)].treasury, o0 = w.planets[at(other)].treasury;
    const auto& d = w.config.dues;
    w.planets[at(holder)].liege = w.planets[at(other)].liege = kNone;  // no tribute in the way
    governor::monthly(w, holder, nullptr);
    governor::monthly(w, other, nullptr);
    CHECK(h0 - w.planets[at(holder)].treasury >= d.access + d.gate - 1e-6);
    CHECK(o0 - w.planets[at(other)].treasury >= d.access - 1e-6);
    CHECK(w.authority.treasury == doctest::Approx((2 * d.access + d.gate) * (1 - d.maintenance)));
    CHECK(w.ledger.maintenance == doctest::Approx((2 * d.access + d.gate) * d.maintenance));
}

TEST_CASE("sworn commanders are paid monthly and serve at home (MIL-2)") {
    Config c = quiet();
    c.chars.target = 250;  // room for the starting commanders
    c.chars.spawn_per_day = 0;
    World w = new_world(c, 51);
    advance(w, 35 * kDay);
    CHECK(w.ledger.stipends > 0);
    for (const Contract& k : w.contracts) {
        if (!k.alive || k.kind != ContractKind::Service) continue;
        const Character& ch = w.characters[at(k.employee)];
        if (ch.planet != kNone) CHECK(ch.planet == w.characters[at(k.employer)].governs);
    }
}

TEST_CASE("swearing into one's own branch is refused: trees never cycle (FAC-2)") {
    World w = new_world(quiet(), 52);
    Id parent = kNone, child = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        const std::vector<Id> below = fealty::vassals(w, p);
        if (!below.empty()) parent = p, child = below[0];
    }
    REQUIRE(parent != kNone);
    const Id before = w.planets[at(parent)].liege;
    fealty::swear(w, parent, child);
    CHECK(w.planets[at(parent)].liege == before);
}
