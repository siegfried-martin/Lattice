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

TEST_CASE("efficiency follows the tier and event shifts (PRD-7, SYS-4)") {
    World w = lab();
    const Config& c = w.config;
    Planet& p = w.planets[0];
    CHECK(market::efficiency(c, p, 0) == c.prod.efficiency_high);
    CHECK(market::efficiency(c, p, 1) == c.prod.efficiency_medium);
    CHECK(market::efficiency(c, p, 2) == c.prod.efficiency_low);
    p.tier_shift = {-1, 1, 1};
    CHECK(market::efficiency(c, p, 0) == doctest::Approx(c.prod.efficiency_high * c.event.bumper_mult));
    CHECK(market::efficiency(c, p, 1) == c.prod.efficiency_low);
    CHECK(market::efficiency(c, p, 2) < c.prod.efficiency_low);
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

TEST_CASE("the population's spending is new money paid to the treasury (ECO-1, ECO-2)") {
    World w = lab();
    Planet& p = w.planets[0];
    fill_to(w, p, 1);
    const double before = p.treasury, created = w.ledger.money_created;
    economy::planet_day(w, 0);
    const double spent = w.ledger.money_created - created;
    CHECK(spent > 0);
    CHECK(p.treasury - before == doctest::Approx(spent));
    CHECK(spent <= p.population * w.config.econ.wage * (1 + w.config.happy.p_up) + 1e-6);
}

TEST_CASE("a short budget buys less life support, and that is a shortfall") {
    World w = lab();
    w.config.econ.wage = 1;
    Planet& p = w.planets[0];
    fill_to(w, p, 1);
    p.labour = {0, 0, 0};
    const double n = p.population;
    economy::planet_day(w, 0);
    CHECK(p.population < n);
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
    SUBCASE("full luxury stocks and money to spend: up") {
        w.config.econ.wage = 1000;
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
        w.config.econ.wage = 1000;
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
        w.config.econ.wage = 1000;
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
    // The poverty floor still buys life support at reserve prices.
    double life = 0;
    for (Good g : kLifeSupport) life += c.goods[g].consumption * c.goods[g].base_price;
    CHECK(c.econ.wage * (1 - c.happy.p_down) >= life);
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

TEST_CASE("osmosis moves goods toward the emptier market and never makes money (SYS-6)") {
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
    CHECK(a.treasury + b.treasury <= money_before);  // the margin is a sink
    CHECK(money_before - (a.treasury + b.treasury) == doctest::Approx(w.ledger.osmosis_margin));

    // It closes only part of the gap.
    CHECK(a.stock[kFood] / market::reserve(c, a, kFood) > b.stock[kFood] / market::reserve(c, b, kFood));

    // No flow where the sale wouldn't cover the purchase.
    fill_to(w, a, 1.02);
    fill_to(w, b, 1.0);
    CHECK(osmosis::run(w) == 0);
}

TEST_CASE("money and goods add up over a year (CHK-7)") {
    World w = new_world(test::small(), 11);
    const double money0 = total_money(w), spawned0 = w.ledger.money_spawned;
    auto goods = [&](int g) {
        double stock = 0;
        for (const Planet& p : w.planets) stock += p.stock[at(g)];
        for (const Character& c : w.characters) stock += c.cargo_good == g ? c.cargo_qty : 0;  // cargo aboard
        return stock;
    };
    std::array<double, kGoodCount> stock0{};
    for (int g = 0; g < kGoodCount; ++g) stock0[at(g)] = goods(g);
    advance(w, 365 * kDay);
    const double expected = w.ledger.money_created + (w.ledger.money_spawned - spawned0) - w.ledger.money_sunk;
    CHECK(total_money(w) - money0 == doctest::Approx(expected).epsilon(1e-9));
    for (int g = 0; g < kLawfulGoodCount; ++g) {
        const double stock = goods(g);
        const auto& l = w.ledger;
        CHECK(stock - stock0[at(g)] == doctest::Approx(l.produced[at(g)] - l.consumed[at(g)] - l.destroyed[at(g)]).epsilon(1e-9));
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
            CHECK(ratio <= c.price.max + 1e-12);
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
