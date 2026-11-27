#include <doctest.h>

#include <cmath>
#include <numeric>

#include "economy/economy.hpp"
#include "economy/events.hpp"
#include "lattice/happiness.hpp"
#include "lattice/market.hpp"
#include "helpers.hpp"
#include "lattice/save.hpp"
#include "lattice/world.hpp"
#include "trade/osmosis.hpp"

using namespace lattice;

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

// A world whose planet 0 makes food, water and oxygen at high, medium and low.
World lab() {
    World w = new_world(test::small(), 7);
    Planet& p = w.planets[0];
    p.makes = {kFood, kWater, kOxygen};
    p.tiers = {kHigh, kMedium, kLow};
    p.tier_shift = {0, 0, 0};
    p.festivals = 0;
    return w;
}

void fill_to(World& w, Planet& p, double fill) {
    for (int g = 0; g < kGoodCount; ++g) p.stock[at(g)] = fill * market::reserve(w.config, p, static_cast<Good>(g));
}

// Every credit anyone holds: treasuries, characters' wealth and the Authority's.
double total_money(const World& w) {
    double m = 0;
    for (const Planet& p : w.planets) m += p.treasury;
    for (const Character& c : w.characters) m += c.wealth;
    for (const Army& a : w.armies) m += a.alive ? a.purse : 0;  // held for a campaign (MIL-2)
    return m + w.authority.treasury;
}

}  // namespace

TEST_CASE("price comes from stock against the reserve (SYS-3, PRD-10)") {
    World w = lab();
    const Config& c = w.config;
    Planet& p = w.planets[0];
    fill_to(w, p, 1);
    CHECK(market::price(c, p, kFood) == doctest::Approx(c.goods[kFood].base_price));
    CHECK(market::price(c, p, kWine) == doctest::Approx(c.goods[kWine].base_price));
    CHECK(market::ask(c, p, kFood) > market::price(c, p, kFood));
    CHECK(market::bid(c, p, kFood) < market::price(c, p, kFood));

    p.stock[kFood] *= 0.5;
    const double half = market::price(c, p, kFood);
    CHECK(half == doctest::Approx(10 * std::pow(2.0, 1.5)));
    p.stock[kFood] = 0;
    CHECK(market::price(c, p, kFood) == doctest::Approx(c.goods[kFood].base_price * c.price.max));
    p.stock[kFood] = 1e12;
    CHECK(market::price(c, p, kFood) == doctest::Approx(c.goods[kFood].base_price * c.price.min));

    // Life support is more sensitive than luxuries.
    p.stock[kFood] = 0.5 * market::reserve(c, p, kFood);
    p.stock[kWine] = 0.5 * market::reserve(c, p, kWine);
    CHECK(market::price(c, p, kFood) / c.goods[kFood].base_price > market::price(c, p, kWine) / c.goods[kWine].base_price);

    // Lawful markets hold no contraband (PIR-14).
    CHECK(market::reserve(c, p, kStims) == 0);
}

TEST_CASE("the cost of making a unit follows the tier and event shifts (PRD-7, SYS-2, SYS-4)") {
    World w = lab();
    const Config& c = w.config;
    Planet& p = w.planets[0];
    const double food = c.goods[kFood].base_price, water = c.goods[kWater].base_price, oxygen = c.goods[kOxygen].base_price;
    CHECK(market::unit_cost(c, p, 0) == doctest::Approx(food * c.prod.cost_high));
    CHECK(market::unit_cost(c, p, 1) == doctest::Approx(water * c.prod.cost_medium));
    CHECK(market::unit_cost(c, p, 2) == doctest::Approx(oxygen * c.prod.cost_low));
    p.tier_shift = {-1, 1, 1};
    CHECK(market::unit_cost(c, p, 0) == doctest::Approx(food * c.prod.cost_high / c.event.bumper_mult));
    CHECK(market::unit_cost(c, p, 1) == doctest::Approx(water * c.prod.cost_low));
    CHECK(market::unit_cost(c, p, 2) == doctest::Approx(oxygen));
    // Contraband is cheap to make: its cost is the risk (PRD-6).
    p.makes[0] = kStims;
    p.tier_shift = {0, 0, 0};
    CHECK(market::unit_cost(c, p, 0) == doctest::Approx(c.goods[kStims].base_price * c.prod.cost_contraband));
}

TEST_CASE("a slot makes goods only while its bid covers the cost, which isn't paid (PRD-7)") {
    World w = lab();
    const Config& c = w.config;
    Planet& p = w.planets[0];
    fill_to(w, p, 1);
    p.labour = {0, 0, 1};  // all on oxygen, made at low
    const double n = p.population, P = p.productivity;
    const double unit = market::unit_cost(c, p, 2);

    SUBCASE("the bid covers it: made, even in debt, and no money leaves") {
        p.treasury = -1e6;
        REQUIRE(market::bid(c, p, kOxygen) >= unit);
        const double made0 = w.ledger.produced[kOxygen], sunk0 = w.ledger.money_sunk;
        economy::planet_day(w, 0);
        const double made = w.ledger.produced[kOxygen] - made0;
        CHECK(made == doctest::Approx(n * c.prod.labour_per_capita * c.goods[kOxygen].yield * P));
        CHECK(w.ledger.production_cost == doctest::Approx(made * unit));
        CHECK(w.ledger.money_sunk == sunk0);
    }
    SUBCASE("a glut: the bid is below the cost, and the slot idles") {
        p.stock[kOxygen] = 100 * market::reserve(c, p, kOxygen);
        REQUIRE(market::bid(c, p, kOxygen) < unit);
        const double made0 = w.ledger.produced[kOxygen];
        economy::planet_day(w, 0);
        CHECK(w.ledger.produced[kOxygen] == made0);
        CHECK(w.ledger.production_cost == 0);
    }
}

TEST_CASE("a fed population grows; a starved one shrinks slowly (SYS-1, PRD-2)") {
    World w = lab();
    Planet& p = w.planets[0];
    const double n = p.population;

    SUBCASE("plenty of everything") {
        fill_to(w, p, 2);
        economy::planet_day(w, 0);
        CHECK(p.population > n);
        CHECK(p.population <= n * (1 + p.growth_factor * w.config.pop.growth_rate) + 1e-9);
    }
    SUBCASE("no oxygen") {
        fill_to(w, p, 2);
        p.stock[kOxygen] = 0;
        p.labour = {1, 0, 0};  // so none is made today
        economy::planet_day(w, 0);
        CHECK(p.population == doctest::Approx(n * (1 - w.config.pop.shrink_rate)));
    }
    SUBCASE("at reserve: no excess, no growth") {
        fill_to(w, p, 1);
        p.labour = {0, 0, 0};
        economy::planet_day(w, 0);
        CHECK(p.population == doctest::Approx(n));
    }
}

TEST_CASE("new money is the grant, the same to every planet; eating costs nothing (ECO-1, ECO-2)") {
    World w = lab();
    Planet& p = w.planets[0];
    fill_to(w, p, 1);
    const double before = p.treasury, created = w.ledger.money_created;
    economy::planet_day(w, 0);
    CHECK(w.ledger.money_created - created == doctest::Approx(w.config.econ.grant));
    CHECK(w.ledger.production_cost > 0);
    CHECK(p.treasury - before == doctest::Approx(w.config.econ.grant));  // making goods costs labour, not money (PRD-7)
    CHECK(w.ledger.consumed[kFood] > 0);
    // The grant doesn't scale with the population.
    World big = lab();
    Planet& q = big.planets[0];
    q.population *= 4;
    fill_to(big, q, 1);
    const double big_created = big.ledger.money_created;
    economy::planet_day(big, 0);
    CHECK(big.ledger.money_created - big_created == doctest::Approx(w.config.econ.grant));
}

TEST_CASE("a short stock is a shortfall, however much money the planet has (ECO-2)") {
    World w = lab();
    Planet& p = w.planets[0];
    fill_to(w, p, 1);
    p.labour = {0, 0, 0};
    p.treasury = 1e12;
    p.stock[kWater] = 0.5 * p.population * w.config.goods[kWater].consumption;
    const double n = p.population;
    economy::planet_day(w, 0);
    CHECK(p.shortfall == doctest::Approx(0.5));
    CHECK(p.population < n);
}

TEST_CASE("a planet won't sell life support to traders below the floor (ECO-2)") {
    World w = lab();
    const Config& c = w.config;
    Planet& p = w.planets[0];
    fill_to(w, p, 1);
    const double r = market::reserve(c, p, kFood);
    const market::Fill f = market::buy(p, c, kFood, 10 * r, 1e15, 1e15);
    CHECK(p.stock[kFood] == doctest::Approx(c.price.sell_floor * r));
    CHECK(f.qty == doctest::Approx((1 - c.price.sell_floor) * r));
    // A luxury has no floor.
    const double rl = market::reserve(c, p, kWine);
    market::buy(p, c, kWine, 10 * rl, 1e15, 1e15);
    CHECK(p.stock[kWine] == doctest::Approx(0).epsilon(1e-6));
}

TEST_CASE("happiness settles toward a target set by luxuries and hunger (PRD-4, SYS-1)") {
    World w = lab();
    const Config& c = w.config;
    Planet& p = w.planets[0];
    p.happiness = 0;
    SUBCASE("no luxuries: toward happy.base, a step of 1 / settle_days of the gap") {
        fill_to(w, p, 1);
        for (Good g : kLuxuries) p.stock[g] = 0;
        economy::planet_day(w, 0);
        CHECK(p.happiness == doctest::Approx(c.happy.base / c.happy.settle_days));
        for (int i = 0; i < 400; ++i) {
            fill_to(w, p, 1);
            for (Good g : kLuxuries) p.stock[g] = 0;
            economy::planet_day(w, 0);
        }
        CHECK(p.happiness == doctest::Approx(c.happy.base).epsilon(1e-3));  // settles; no drift to an end
    }
    SUBCASE("full luxury stocks: up") {
        fill_to(w, p, 1.5);
        economy::planet_day(w, 0);
        CHECK(p.happiness > 0);
    }
    SUBCASE("hunger pulls the target down") {
        fill_to(w, p, 1);
        for (Good g : kLuxuries) p.stock[g] = 0;
        p.stock[kFood] = 0;
        p.labour = {0, 0, 0};
        economy::planet_day(w, 0);
        CHECK(p.happiness < c.happy.base / c.happy.settle_days - 1e-9);
    }
}

TEST_CASE("variety pays: the same luxury use spread over more luxuries is happier (PRD-4)") {
    auto settle = [](int kinds) {
        World w = lab();
        Planet& p = w.planets[0];
        for (int i = 0; i < 300; ++i) {
            fill_to(w, p, 1);
            for (Good g : kLuxuries) p.stock[g] = 0;
            // The same total stock, against reserve, over `kinds` luxuries.
            for (int k = 0; k < kinds; ++k) p.stock[kLuxuries[at(k)]] = 1.5 * market::reserve(w.config, p, kLuxuries[at(k)]) * 2 / kinds;
            p.labour = {1.0 / 3, 1.0 / 3, 1.0 / 3};
            economy::planet_day(w, 0);
        }
        return p.happiness;
    };
    CHECK(settle(2) > settle(1));
}

TEST_CASE("a contented population uses fewer luxuries (PRD-4)") {
    auto used = [](double h) {
        World w = lab();
        Planet& p = w.planets[0];
        fill_to(w, p, 1);
        p.happiness = h;
        const double before = w.ledger.consumed[kWine];
        economy::planet_day(w, 0);
        return w.ledger.consumed[kWine] - before;
    };
    CHECK(used(0.5) < used(0));
}

TEST_CASE("productivity: 1 at neutral, diminishing toward a ceiling and a floor (PRD-5)") {
    const Config c;
    CHECK(happiness::productivity(c, 0) == 1);
    CHECK(happiness::productivity(c, 0.2) > 1);
    CHECK(happiness::productivity(c, 5) <= 1 + c.happy.p_up);
    CHECK(happiness::productivity(c, -5) >= 1 - c.happy.p_down);
    // Diminishing: the first step from neutral counts most.
    CHECK(happiness::productivity(c, 0.2) - 1 > happiness::productivity(c, 0.4) - happiness::productivity(c, 0.2));
    CHECK(1 - happiness::productivity(c, -0.2) > happiness::productivity(c, -0.2) - happiness::productivity(c, -0.4));
    // Unhappiness cuts growth; happiness doesn't raise it to start.
    CHECK(happiness::growth(c, -0.3) < 1);
    CHECK(happiness::growth(c, 0.3) == 1);
}

TEST_CASE("an unhappy planet stocks more luxuries, so it bids more (PRD-10, SYS-3)") {
    World w = lab();
    const Config& c = w.config;
    Planet& p = w.planets[0];
    fill_to(w, p, 1);
    p.happiness = 0;
    const double reserve = market::reserve(c, p, kWine), bid = market::bid(c, p, kWine);
    p.happiness = -0.5;
    CHECK(market::reserve(c, p, kWine) > reserve);
    CHECK(market::bid(c, p, kWine) > bid);
    CHECK(market::reserve(c, p, kFood) == doctest::Approx(c.goods[kFood].reserve_days * p.population * c.goods[kFood].consumption));
}

TEST_CASE("labour moves toward what pays and stays a valid split (SYS-2, PRD-9)") {
    World w = lab();
    Planet& p = w.planets[0];
    fill_to(w, p, 1);
    p.stock[kOxygen] = 0.2 * market::reserve(w.config, p, kOxygen);  // scarce, so dear
    const double before = p.labour[2];
    economy::planet_labour(w, 0);
    CHECK(p.labour[2] == doctest::Approx(before + w.config.prod.labour_shift));
    for (int i = 0; i < 100; ++i) economy::planet_labour(w, 0);
    CHECK(p.labour[0] + p.labour[1] + p.labour[2] == doctest::Approx(1));
    for (double l : p.labour) CHECK(l >= 0);
}

TEST_CASE("imports come out of net income, not gross (SYS-5, OPN-39)") {
    World w = lab();
    const Config& c = w.config;
    Planet& p = w.planets[0];
    fill_to(w, p, 1);
    p.income_avg = p.net_avg = 1000;
    const double month0 = p.income_month;
    const market::Fill f = market::sell(p, c, kWine, 0.2 * market::reserve(c, p, kWine), 0);
    REQUIRE(f.money > 0);
    economy::planet_day(w, 0);
    const auto days = static_cast<double>(c.time.payment_days);
    CHECK(p.income_avg - p.net_avg == doctest::Approx(f.money / days));
    // The month's income, which tribute is paid on, is net too (SYS-11).
    CHECK(p.income_month - month0 == doctest::Approx(c.econ.grant - f.money));
}

TEST_CASE("a planet in the red buys life support on credit but no luxuries (ECO-8, money round)") {
    World w = lab();
    const Config& c = w.config;
    Planet& p = w.planets[0];
    fill_to(w, p, 0.5);
    p.income_avg = p.net_avg = 1000;
    p.treasury = 0;  // in the black, just: both sell
    CHECK(market::can_buy(c, p, kWine));
    CHECK(market::can_buy(c, p, kFood));
    p.treasury = -1;  // in the red, well inside the credit limit
    CHECK_FALSE(market::can_buy(c, p, kWine));
    CHECK(market::can_buy(c, p, kFood));
    CHECK(market::sell(p, c, kWine, 10, 0).qty == 0);
    CHECK(market::sell(p, c, kFood, 10, 0).qty == doctest::Approx(10));
    p.treasury = -c.econ.credit_limit * p.income_avg * static_cast<double>(c.time.payment_days) - 1;  // past it
    CHECK_FALSE(market::can_buy(c, p, kFood));
}

TEST_CASE("at the same price, labour moves to the cheapest slot to work (PRD-9)") {
    World w = lab();
    Planet& p = w.planets[0];
    p.makes = {kFood, kWater, kWine};
    p.tiers = {kLow, kHigh, kMedium};
    p.labour = {1.0 / 3, 1.0 / 3, 1.0 / 3};
    fill_to(w, p, 1);  // every price at base
    w.config.goods[kWine].yield = 1;
    for (Good g : {kFood, kWater, kWine}) w.config.goods[g].base_price = 10;
    economy::planet_labour(w, 0);
    CHECK(p.labour[1] == doctest::Approx(1.0 / 3 + w.config.prod.labour_shift));
    CHECK(p.labour[0] == doctest::Approx(1.0 / 3 - w.config.prod.labour_shift));
}

TEST_CASE("operating costs are paid monthly and sunk (ECO-4, ECO-5)") {
    World w = lab();
    Planet& p = w.planets[0];
    const double before = p.treasury;
    economy::planet_payment(w, 0);
    CHECK(before - p.treasury == doctest::Approx(w.config.econ.op_base + w.config.econ.op_per_capita * p.population));
    CHECK(w.ledger.money_sunk == doctest::Approx(before - p.treasury));
}

TEST_CASE("events shift efficiency and undo it when they end (SYS-4)") {
    World w = lab();
    w.config.event.drought_p = 1;  // every food producer, every day
    w.config.event.bumper_p = 0;
    w.config.event.festival_p = 1;
    w.config.event.region_drought_p = 0;
    events::roll(w, nullptr);
    Planet& p = w.planets[0];
    CHECK(p.tier_shift[0] == 1);
    CHECK(p.festivals == 1);
    events::roll(w, nullptr);  // already covered: nothing stacks
    CHECK(p.tier_shift[0] == 1);
    CHECK(p.festivals == 1);

    advance(w, (w.config.event.drought_days + 1) * kDay);
    // Rolls keep happening at p = 1, so check the bookkeeping instead: the shift equals
    // the number of alive droughts covering the planet.
    int alive = 0;
    for (const WorldEvent& e : w.world_events) {
        if (e.alive && e.kind == WorldEventKind::Drought && e.planets[0] == 0) ++alive;
    }
    CHECK(p.tier_shift[0] == alive);
    CHECK(alive <= 1);

    // With no new rolls, everything ends and every shift returns to 0.
    w.config.event.drought_p = 0;
    w.config.event.festival_p = 0;
    advance(w, 40 * kDay);
    for (const Planet& q : w.planets) {
        for (auto s : q.tier_shift) CHECK(s == 0);
        CHECK(q.festivals == 0);
    }
}

TEST_CASE("osmosis moves goods toward the emptier market and neither makes nor sinks money (SYS-6)") {
    World w = lab();
    const Config& c = w.config;
    Planet& a = w.planets[0];
    Planet& b = w.planets[1];
    fill_to(w, a, 2);
    fill_to(w, b, 0.3);
    w.derived.osmosis_pairs = {{0, 1, 1.0, std::exp(-1.0 / c.osmosis.falloff)}};

    const double goods_before = a.stock[kFood] + b.stock[kFood];
    const double money_before = a.treasury + b.treasury;
    const double b_food = b.stock[kFood];
    const double moved = osmosis::run(w);
    CHECK(moved > 0);
    CHECK(b.stock[kFood] > b_food);
    CHECK(a.stock[kFood] + b.stock[kFood] == doctest::Approx(goods_before));
    // The margin is split between the two (money round): what b paid, a received.
    CHECK(a.treasury + b.treasury == doctest::Approx(money_before));
    CHECK(w.ledger.osmosis_margin > 0);
    CHECK(w.ledger.money_sunk == 0);

    // It closes only part of the gap.
    CHECK(a.stock[kFood] / market::reserve(c, a, kFood) > b.stock[kFood] / market::reserve(c, b, kFood));

    // No flow where the sale wouldn't cover the purchase.
    fill_to(w, a, 1.02);
    fill_to(w, b, 1.0);
    CHECK(osmosis::run(w) == 0);
}

TEST_CASE("money and goods add up over a year (CHK-7)") {
    World w = new_world(test::small(), 11);
    const double money0 = total_money(w), spawned0 = w.ledger.money_spawned, free0 = w.ledger.free_money;
    auto goods = [&](int g) {
        double stock = 0;
        for (const Planet& p : w.planets) stock += p.stock[at(g)];
        for (const Character& c : w.characters) stock += c.cargo_good == g ? c.cargo_qty : 0;  // cargo aboard
        return stock;
    };
    std::array<double, kGoodCount> stock0{};
    for (int g = 0; g < kGoodCount; ++g) stock0[at(g)] = goods(g);
    advance(w, 365 * kDay);
    // Free money (MSN-5: insurance payouts, passenger fares) is new money too.
    const double expected =
        w.ledger.money_created + (w.ledger.money_spawned - spawned0) + (w.ledger.free_money - free0) - w.ledger.money_sunk;
    CHECK(total_money(w) - money0 == doctest::Approx(expected).epsilon(1e-9));
    for (int g = 0; g < kLawfulGoodCount; ++g) {
        const double stock = goods(g);
        const auto& l = w.ledger;
        // Rounding is relative to the goods that flowed, not to the (often small) net change.
        const double residual = (stock - stock0[at(g)]) - (l.produced[at(g)] - l.consumed[at(g)] - l.destroyed[at(g)]);
        CHECK(std::abs(residual) <= 1e-9 * std::max(1.0, l.produced[at(g)] + l.consumed[at(g)]));
    }
    CHECK(w.ledger.osmosis_volume > 0);
}

TEST_CASE("500 days, wars included, stay finite and in range") {
    World w = new_world(test::small(), 12);
    advance(w, 500 * kDay);
    const Config& c = w.config;
    for (const Planet& p : w.planets) {
        CHECK(std::isfinite(p.population));
        CHECK(p.population > 0);
        CHECK(std::isfinite(p.happiness));
        CHECK(p.productivity <= 1 + c.happy.p_up + 1e-12);
        CHECK(std::isfinite(p.treasury));
        CHECK(p.labour[0] + p.labour[1] + p.labour[2] == doctest::Approx(1));
        for (int g = 0; g < kLawfulGoodCount; ++g) {
            CHECK(p.stock[at(g)] >= -1e-9);
            const double ratio = market::price(c, p, static_cast<Good>(g)) / c.goods[at(g)].base_price;
            CHECK(ratio >= c.price.min - 1e-12);
            CHECK(ratio <= c.price.max * c.price.wealth_max + 1e-12);  // a rich planet pays more (PRD-10)
        }
    }
}

TEST_CASE("a loaded world rebuilds its derived tables") {
    World w = new_world(test::small(), 13);
    advance(w, 50 * kDay);
    World loaded = load_world(save_world(w));
    CHECK(loaded.derived.travel_days == w.derived.travel_days);
    CHECK(loaded.derived.osmosis_pairs.size() == w.derived.osmosis_pairs.size());
    advance(w, 100 * kDay);
    advance(loaded, 100 * kDay);
    CHECK(state_hash(w) == state_hash(loaded));
}

TEST_CASE("a rich planet pays more: spare money raises every price it posts (PRD-10)") {
    World w = lab();
    const Config& c = w.config;
    Planet& p = w.planets[0];
    fill_to(w, p, 1);
    const double cost = economy::monthly_costs(c, p);
    p.treasury = c.budget.reserve_months * cost;  // what it keeps back: no change
    const double plain = market::price(c, p, kFood);
    CHECK(market::wealth(c, p) == 1);
    p.treasury = -5 * cost;  // a broke planet isn't discounted; its credit limits it
    CHECK(market::price(c, p, kFood) == doctest::Approx(plain));
    p.treasury = std::exp(1.0) * c.budget.reserve_months * cost;  // one e-fold over
    CHECK(market::wealth(c, p) == doctest::Approx(1 + c.price.wealth));
    CHECK(market::bid(c, p, kWine) / market::ask(c, p, kWine) == doctest::Approx((1 - c.price.spread / 2) / (1 + c.price.spread / 2)));
    p.treasury = 1e15;
    CHECK(market::wealth(c, p) == doctest::Approx(c.price.wealth_max));
    CHECK(market::price(c, p, kFood) == doctest::Approx(plain * c.price.wealth_max));
}
