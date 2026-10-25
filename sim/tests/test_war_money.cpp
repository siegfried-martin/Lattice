#include <doctest.h>

#include <algorithm>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/ships.hpp"
#include "helpers.hpp"
#include "politics/budget.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"
#include "politics/relief.hpp"
#include "war/armies.hpp"
#include "war/combat.hpp"
#include "war/diplomacy.hpp"
#include "war/prizes.hpp"

using namespace lattice;

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

// No one but the seeded governors and commanders, and nothing happening on its own.
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

Id gov_of(const World& w, Id planet) { return w.planets[at(planet)].governor; }

Id commander(World& w, Id planet, int n) {
    const Id id = chars::spawn(w, planet);
    Character& c = w.characters[at(id)];
    c.ships.assign(static_cast<std::size_t>(n), ships::variant(w.map, c.culture, ShipClass::Gunship, 1));
    c.wealth = 1e6;
    contracts::sign(w, ContractKind::Service, gov_of(w, planet), id, budget::stipend(w, c), 30 * kDay);
    c.activity = Activity::Serving;
    return id;
}

double price_of(const World& w, Id id) {
    double p = 0;
    for (Id v : w.characters[at(id)].ships) p += w.map.variants[at(v)].price;
    return p;
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

Id loan_of(const World& w, Id army, Id lender) {
    for (Id id = 0; id < static_cast<Id>(w.loans.size()); ++id) {
        if (w.loans[at(id)].army == army && w.loans[at(id)].lender == lender) return id;
    }
    return kNone;
}

bool in(const std::vector<Id>& list, Id id) { return std::find(list.begin(), list.end(), id) != list.end(); }

}  // namespace

TEST_CASE("the root owes a bounty for each enemy ship its commanders destroy, and pays what it can (MIL-18)") {
    World w = new_world(quiet(), 301);
    w.config.combat.pod_death_p = 0;
    const Front f = front(w);
    REQUIRE(f.root_a != kNone);
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    const Id killer = commander(w, f.a, 1);
    const Id victim = commander(w, f.b, 2);
    const double price = price_of(w, victim);
    // With static defence beside it as strong as its fleet, it gets half the credit.
    const auto by = prizes::shares(w, {killer}, ships::fleet(w, w.characters[at(killer)]).combat);
    std::vector<Id> side{victim};
    double carried = 0;
    combat::destroy(w, side, 1e12, carried, nullptr, &by);
    const double owed = w.config.bounty.share * price * 0.5;
    CHECK(prizes::owed(w, f.root_a) == doctest::Approx(owed));
    CHECK(w.ledger.bounties_owed == doctest::Approx(owed));

    // A ship of a faction it isn't at war with is no bounty.
    Id neutral = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && neutral == kNone; ++p) {
        const Id r = fealty::root_of(w, p);
        if (r != f.root_a && r != f.root_b) neutral = p;
    }
    REQUIRE(neutral != kNone);
    std::vector<Id> other{commander(w, neutral, 1)};
    carried = 0;
    combat::destroy(w, other, 1e12, carried, nullptr, &by);
    CHECK(prizes::owed(w, f.root_a) == doctest::Approx(owed));

    SUBCASE("paid at the root's payment") {
        Character& k = w.characters[at(killer)];
        const double before = k.wealth;
        w.planets[at(f.root_a)].treasury = 1e9;
        prizes::pay(w, f.root_a, nullptr);
        CHECK(k.wealth == doctest::Approx(before + owed));
        CHECK(prizes::owed(w, f.root_a) == 0);
        CHECK(w.ledger.bounties_unpaid == 0);
    }
    SUBCASE("a root that can't pay owes it still, and loses the commander's opinion") {
        w.planets[at(f.root_a)].treasury = 0;
        const Id root_gov = gov_of(w, f.root_a);
        const double was = opinion::get(w, killer, root_gov);
        prizes::pay(w, f.root_a, nullptr);
        CHECK(prizes::owed(w, f.root_a) == doctest::Approx(owed));
        CHECK(w.ledger.bounties_unpaid == 1);
        CHECK(opinion::get(w, killer, root_gov) < was);
    }
}

TEST_CASE("spoils leave a captured planet's treasury, split by power x days in the siege (MIL-19)") {
    World w = new_world(quiet(), 302);
    const Front f = front(w);
    REQUIRE(f.root_a != kNone);
    const Id m1 = commander(w, f.a, 1), m2 = commander(w, f.a, 1);
    w.armies.push_back({.commissioner = gov_of(w, f.a), .home = f.a, .target = f.b, .members = {m1, m2},
                        .action = ArmyAction::Besiege, .planet = f.b});
    const auto army = static_cast<Id>(w.armies.size() - 1);
    w.sieges.push_back({.planet = f.b, .army = army, .start = w.now, .power_days = {{m1, 30}, {m2, 10}}});
    const auto siege = static_cast<Id>(w.sieges.size() - 1);
    Planet& t = w.planets[at(f.b)];
    const double full = w.config.spoils.per_prosperity * t.population * t.productivity;
    REQUIRE(full > 0);
    const double w1 = w.characters[at(m1)].wealth, w2 = w.characters[at(m2)].wealth;

    SUBCASE("a rich planet yields its spoils") {
        t.treasury = full * 10;
        prizes::take_spoils(w, siege, nullptr);
        CHECK(t.treasury == doctest::Approx(full * 9));
        CHECK(w.characters[at(m1)].wealth - w1 == doctest::Approx(0.75 * full));
        CHECK(w.characters[at(m2)].wealth - w2 == doctest::Approx(0.25 * full));
        CHECK(w.ledger.spoils == doctest::Approx(full));
    }
    SUBCASE("a poor one yields what it has, and one in debt nothing") {
        t.treasury = full / 4;
        prizes::take_spoils(w, siege, nullptr);
        CHECK(t.treasury == doctest::Approx(0));
        t.treasury = -100;
        prizes::take_spoils(w, siege, nullptr);
        CHECK(t.treasury == -100);
        CHECK(w.ledger.spoils == doctest::Approx(full / 4));
    }
}

TEST_CASE("a liege borrows a vassal's commanders: fee and stipends move, and they go back when the army disbands (MIL-3, INF-16)") {
    World w = new_world(quiet(), 303);
    w.config.objective.choose = false;
    Id vassal = kNone;
    const Id liege = liege_with_vassal(w, vassal);
    REQUIRE(liege != kNone);
    const Id lgov = gov_of(w, liege), vgov = gov_of(w, vassal);
    const Id m = commander(w, vassal, 2);
    const Id k = contracts::held(w, m, ContractKind::Service);
    Id target = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()) && target == kNone; ++p) {
        if (!fealty::same_faction(w, p, liege)) target = p;
    }
    REQUIRE(target != kNone);
    const Id army = armies::commission(w, liege, target, false, nullptr);
    REQUIRE(army != kNone);
    REQUIRE(in(w.armies[at(army)].asked, vassal));

    SUBCASE("granted: the fleet is the reply") {
        opinion::deed(w, vgov, lgov, DeedKind::PlanetGranted, nullptr);  // loyal
        const double was = opinion::get(w, lgov, vgov);
        armies::loan_request(w, army, vassal, nullptr);
        const Id loan = loan_of(w, army, vassal);
        REQUIRE(loan != kNone);
        CHECK(w.loans[at(loan)].alive);
        CHECK(w.contracts[at(k)].employer == lgov);
        CHECK(w.contracts[at(k)].loan == loan);
        double pay = 0;
        for (Id c : w.loans[at(loan)].contracts) pay += w.contracts[at(c)].pay;
        CHECK(w.loans[at(loan)].fee == doctest::Approx(w.config.loan.fee_months * pay));
        CHECK(w.ledger.loan_fees == doctest::Approx(w.loans[at(loan)].fee));
        REQUIRE(in(w.armies[at(army)].called, m));
        // It arrives at the muster: the liege counts the call answered when the army
        // marches (all its requests answered, or the deadline).
        armies::member_arrived(w, m, nullptr);
        CHECK(in(w.armies[at(army)].answered, vassal));
        armies::muster_deadline(w, army, 0, nullptr);
        CHECK(opinion::get(w, lgov, vgov) > was);
        armies::disband(w, army, nullptr);
        CHECK(!w.loans[at(loan)].alive);
        CHECK(w.contracts[at(k)].employer == vgov);
        CHECK(w.contracts[at(k)].loan == kNone);
    }
    SUBCASE("refused: no fleet by the deadline is a refusal") {
        opinion::deed(w, vgov, lgov, DeedKind::Attacked, nullptr);
        opinion::deed(w, vgov, lgov, DeedKind::Attacked, nullptr);
        w.planets[at(vassal)].income_avg = 1e7;  // its home's lost defence outweighs the fee
        const double was = opinion::get(w, lgov, vgov);
        armies::loan_request(w, army, vassal, nullptr);
        CHECK(loan_of(w, army, vassal) == kNone);
        CHECK(w.contracts[at(k)].employer == vgov);
        CHECK(w.ledger.loans_refused >= 1);
        armies::muster_deadline(w, army, 0, nullptr);
        CHECK(opinion::get(w, lgov, vgov) < was);
    }
}

TEST_CASE("relief is a loan to the planet relieved, and spends its favour with the liege (MIL-11)") {
    World w = new_world(quiet(), 304);
    Id vassal = kNone;
    const Id liege = liege_with_vassal(w, vassal);
    REQUIRE(liege != kNone);
    const Id lgov = gov_of(w, liege), vgov = gov_of(w, vassal);
    const Id m = commander(w, liege, 2);
    const Id k = contracts::held(w, m, ContractKind::Service);
    const double was = opinion::get(w, lgov, vgov);
    const Id army = armies::commission(w, liege, vassal, true, nullptr);
    REQUIRE(army != kNone);
    REQUIRE(in(w.armies[at(army)].members, m));
    CHECK(w.contracts[at(k)].employer == vgov);
    CHECK(w.contracts[at(k)].loan != kNone);
    CHECK(w.ledger.relief_loans == 1);
    CHECK(opinion::get(w, lgov, vgov) < was);
    // The vassal pays its stipend while it serves; the vassal's budget doesn't let it go.
    w.planets[at(vassal)].net_avg = 0;
    budget::weekly(w, vassal, nullptr);
    CHECK(w.contracts[at(k)].alive);
    armies::disband(w, army, nullptr);
    CHECK(w.contracts[at(k)].employer == lgov);
}

TEST_CASE("a vassal in need asks for tribute relief; it's granted or refused, and ends on its own terms (FAC-15)") {
    World w = new_world(quiet(), 305);
    Id vassal = kNone;
    const Id liege = liege_with_vassal(w, vassal);
    REQUIRE(liege != kNone);
    const Id lgov = gov_of(w, liege), vgov = gov_of(w, vassal);
    Planet& v = w.planets[at(vassal)];
    v.tribute_rate = 0.2;
    v.tribute_paid = 1e8;  // worth keeping, valued as patronage
    v.net_avg = 1e6;
    v.shortfall = 0;
    CHECK(!relief::in_need(w, vassal));
    relief::weekly(w, vassal, nullptr);
    CHECK(w.ledger.relief_asked == 0);  // only a planet that passes a test may ask
    v.shortfall = 0.5;
    REQUIRE(relief::in_need(w, vassal));
    relief::weekly(w, vassal, nullptr);
    CHECK(w.ledger.relief_asked == 1);
    REQUIRE(v.relief_pending != kNever);
    relief::weekly(w, vassal, nullptr);
    CHECK(w.ledger.relief_asked == 1);  // one at a time
    const auto answer_of = [&] {
        int granted = -1;
        for (const Event& e : w.events.raw()) {
            if (e.kind == EventKind::ReliefAnswer && e.record == vassal) granted = static_cast<int>(e.a);
        }
        return granted;
    };

    SUBCASE("granted, then ended once it no longer needs it") {
        w.planets[at(liege)].treasury = 1e9;
        relief::request(w, vassal, liege, nullptr);
        REQUIRE(answer_of() == 1);
        const double was = opinion::get(w, vgov, lgov);
        relief::answer(w, vassal, true, liege, nullptr);
        CHECK(v.tribute_rate == doctest::Approx(w.config.relief.rate));
        CHECK(v.relief_from == doctest::Approx(0.2));
        CHECK(opinion::get(w, vgov, lgov) > was);
        // The liege in the red doesn't raise a vassal on relief.
        w.planets[at(liege)].treasury = -1e6;
        opinion::deed(w, vgov, lgov, DeedKind::PlanetGranted, nullptr);
        fealty::adjust_tribute(w, liege, nullptr);
        CHECK(v.tribute_rate == doctest::Approx(w.config.relief.rate));
        // Still in need: still on relief. Then clear for relief.clear_weeks: back to 0.2.
        relief::weekly(w, vassal, nullptr);
        CHECK(v.relief_from >= 0);
        v.shortfall = 0;
        v.relief_needed = w.now - (w.config.relief.clear_weeks * 7 + 1) * kDay;
        relief::weekly(w, vassal, nullptr);
        CHECK(v.tribute_rate == doctest::Approx(0.2));
        CHECK(v.relief_from < 0);
        CHECK(w.ledger.relief_ended == 1);
    }
    SUBCASE("refused by a liege that can't afford it: a bad deed") {
        w.planets[at(liege)].treasury = -1e12;
        relief::request(w, vassal, liege, nullptr);
        REQUIRE(answer_of() == 0);
        const double was = opinion::get(w, vgov, lgov);
        relief::answer(w, vassal, false, liege, nullptr);
        CHECK(v.tribute_rate == doctest::Approx(0.2));
        CHECK(opinion::get(w, vgov, lgov) < was);
        CHECK(w.ledger.relief_refused == 1);
    }
    SUBCASE("no answer by the deadline is a refusal (INF-16)") {
        v.relief_pending = w.now - 1;
        relief::weekly(w, vassal, nullptr);
        CHECK(v.relief_pending == kNever);
        CHECK(w.ledger.relief_refused == 1);
    }
}

TEST_CASE("a stipend rises with reputation, levelling off at hire.rep_mult more (MIL-2)") {
    World w = new_world(quiet(), 306);
    const Id id = commander(w, 0, 2);
    Character& c = w.characters[at(id)];
    const double base = w.config.hire.stipend_per_power * ships::fleet(w, c).combat;
    c.reputation = 0;
    CHECK(budget::stipend(w, c) == doctest::Approx(base));
    c.reputation = w.config.hire.rep_half;
    CHECK(budget::stipend(w, c) == doctest::Approx(base * (1 + w.config.hire.rep_mult / 2)));
    c.reputation = 1e9;
    CHECK(budget::stipend(w, c) == doctest::Approx(base * (1 + w.config.hire.rep_mult)).epsilon(1e-6));
}

TEST_CASE("commanders choose an objective by what it pays against what it risks (MIL-2, SYS-9)") {
    World w = new_world(quiet(), 307);
    const Front f = front(w);
    REQUIRE(f.root_a != kNone);
    diplomacy::declare_war(w, f.root_a, f.root_b, nullptr);
    const Id m = commander(w, f.a, 2);
    Planet& t = w.planets[at(f.b)];
    for (Id k : std::vector<Id>(w.characters[at(t.governor)].contracts)) contracts::end(w, k);
    w.characters[at(t.governor)].ships.clear();

    SUBCASE("a strong, poor target with no purse: it declines, which costs it nothing") {
        t.defence = 1e7;
        t.treasury = 0;
        const double was = opinion::get(w, gov_of(w, f.a), m);
        const Id army = armies::commission(w, f.a, f.b, false, nullptr);
        CHECK(w.characters[at(m)].army == kNone);
        if (army != kNone) CHECK(!in(w.armies[at(army)].members, m));
        CHECK(w.ledger.objectives_declined >= 1);
        CHECK(opinion::get(w, gov_of(w, f.a), m) == was);
        if (army == kNone) {
            // Nobody came: the planet waits objective.retry_days before another campaign.
            CHECK(w.planets[at(f.a)].campaign_after == w.now + w.config.objective.retry_days * kDay);
            CHECK(armies::best_invasion(w, f.a).target == kNone);
        }
    }
    SUBCASE("the same target with a purse worth the risk: it joins, and the purse rises only as far as it must") {
        t.defence = 50;
        t.treasury = 0;
        const Id army = armies::commission(w, f.a, f.b, false, nullptr, kNone, 1e9);
        REQUIRE(army != kNone);
        CHECK(in(w.armies[at(army)].members, m));
        const auto& o = w.config.objective;
        CHECK(w.armies[at(army)].purse >= o.reward_share * 1e9);
        CHECK(w.armies[at(army)].purse <= o.reward_max * 1e9);
        CHECK(w.armies[at(army)].purse == doctest::Approx(o.reward_share * 1e9));  // the first offer was enough
    }
    SUBCASE("an undefended, rich target: it joins") {
        t.defence = 0;
        t.treasury = 1e9;
        const Id army = armies::commission(w, f.a, f.b, false, nullptr);
        REQUIRE(army != kNone);
        CHECK(in(w.armies[at(army)].members, m));
        CHECK(w.ledger.objectives_taken >= 1);
        CHECK(armies::weigh(w, w.armies[at(army)], m).join > 0);
    }
    SUBCASE("with objective.choose off, everyone at the muster joins") {
        w.config.objective.choose = false;
        t.defence = 1e7;
        const Id army = armies::commission(w, f.a, f.b, false, nullptr);
        REQUIRE(army != kNone);
        CHECK(in(w.armies[at(army)].members, m));
        CHECK(w.ledger.objectives_declined == 0);
    }
}

TEST_CASE("a world with war money runs, and loans and bounties stay consistent (MIL-3, MIL-18)") {
    for (const bool choose : {true, false}) {
        CAPTURE(choose);
        Config c = test::small();
        c.objective.choose = choose;
        World w = new_world(c, 308);
        advance(w, 400 * kDay);
        for (Id id = 0; id < static_cast<Id>(w.loans.size()); ++id) {
            const Loan& l = w.loans[at(id)];
            if (!l.alive) continue;
            for (Id k : l.contracts) {
                CHECK(w.contracts[at(k)].alive);
                CHECK(w.contracts[at(k)].loan == id);
                CHECK(w.contracts[at(k)].employer == gov_of(w, l.borrower));
            }
        }
        for (const Contract& k : w.contracts) {
            if (k.alive && k.loan != kNone) CHECK(w.loans[at(k.loan)].alive);
        }
        for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
            for (const Owed& o : w.planets[at(p)].bounties) CHECK(o.amount > 0);
            if (!w.planets[at(p)].bounties.empty()) CHECK(w.planets[at(p)].liege == kNone);
        }
    }
}

TEST_CASE("an objective done pays its purse from the army's home, split by days in the siege (MIL-2)") {
    World w = new_world(quiet(), 309);
    const Front f = front(w);
    REQUIRE(f.root_a != kNone);
    const Id m1 = commander(w, f.a, 1), m2 = commander(w, f.a, 1);
    w.armies.push_back({.commissioner = gov_of(w, f.a), .home = f.a, .target = f.b, .members = {m1, m2},
                        .action = ArmyAction::Besiege, .planet = f.b, .purse = 1000});
    const auto army = static_cast<Id>(w.armies.size() - 1);
    w.sieges.push_back({.planet = f.b, .army = army, .start = w.now, .power_days = {{m1, 3}, {m2, 1}}});
    const auto siege = static_cast<Id>(w.sieges.size() - 1);
    const double home = w.planets[at(f.a)].treasury;
    const double w1 = w.characters[at(m1)].wealth, w2 = w.characters[at(m2)].wealth;
    prizes::pay_purse(w, army, siege, nullptr);
    CHECK(w.planets[at(f.a)].treasury == doctest::Approx(home - 1000));
    CHECK(w.characters[at(m1)].wealth - w1 == doctest::Approx(750));
    CHECK(w.characters[at(m2)].wealth - w2 == doctest::Approx(250));
    CHECK(w.ledger.objective_rewards == doctest::Approx(1000));
    CHECK(w.armies[at(army)].purse == 0);
    prizes::pay_purse(w, army, siege, nullptr);  // paid once
    CHECK(w.ledger.objective_rewards == doctest::Approx(1000));
}
