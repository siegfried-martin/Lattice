#include <doctest.h>

#include "lattice/config.hpp"

using namespace lattice;

TEST_CASE("an empty config gives the defaults from tunables.md") {
    Config c = parse_config("");
    CHECK(c == Config{});
    CHECK(c.time.real_seconds_per_day == 60);
    CHECK(c.time.payment_days == 30);
}

TEST_CASE("keys override defaults; integers are accepted for numbers") {
    Config c = parse_config(R"(
[time]
real_seconds_per_day = 90
space_step_minutes = 5
[debug]
pings = 12
)");
    CHECK(c.time.real_seconds_per_day == 90.0);
    CHECK(c.time.space_step_minutes == 5);
    CHECK(c.debug.pings == 12);
    CHECK(c.time.labour_days == 7);
}

TEST_CASE("unknown keys, wrong types and bad values are errors") {
    CHECK_THROWS_WITH_AS(parse_config("[time]\nlabor_days = 7\n", "x.toml"),
                         doctest::Contains("unknown key time.labor_days"), ConfigError);
    CHECK_THROWS_WITH_AS(parse_config("[time]\nlabour_days = 7.5\n"),
                         doctest::Contains("must be an integer"), ConfigError);
    CHECK_THROWS_WITH_AS(parse_config("[time]\nreal_seconds_per_day = \"fast\"\n"),
                         doctest::Contains("must be a number"), ConfigError);
    CHECK_THROWS_WITH_AS(parse_config("[time]\npayment_days = 0\n"),
                         doctest::Contains("payment_days"), ConfigError);
    CHECK_THROWS_WITH_AS(parse_config("[time\n", "broken.toml"), doctest::Contains("broken.toml:1"), ConfigError);
}

TEST_CASE("the written config reads back to the same config (ARC-7)") {
    Config c;
    c.time.real_seconds_per_day = 61.25;
    c.debug.pings = 3;
    const std::string text = to_toml(c);
    CHECK(text.find("[time]") != std::string::npos);
    CHECK(parse_config(text) == c);
}

TEST_CASE("a missing file is an error") {
    CHECK_THROWS_AS(load_config_file("/nonexistent/lattice.toml"), ConfigError);
}

TEST_CASE("per-good keys and lists (MS-2)") {
    Config c = parse_config(R"(
[good.food]
base_price = 12
[good.amber]
base_price = 45
[fleet]
run_cost = [1, 2, 4]
)");
    CHECK(c.goods[kFood].base_price == 12);
    CHECK(c.goods[kAmber].base_price == 45);
    CHECK(c.fleet.run_cost[2] == 4);

    // Luxuries take their use from happy.need, so the key doesn't exist.
    CHECK_THROWS_WITH_AS(parse_config("[good.wine]\nconsumption = 1\n"), doctest::Contains("unknown key good.wine.consumption"),
                         ConfigError);
    CHECK_THROWS_WITH_AS(parse_config("[fleet]\nrun_cost = [0, 1]\n"), doctest::Contains("list of 3 numbers"), ConfigError);
    CHECK_THROWS_WITH_AS(parse_config("[fleet]\nrun_cost = [0, \"x\", 2]\n"), doctest::Contains("list of numbers"),
                         ConfigError);

    Config d;
    d.fleet.run_cost = {0.5, 0.25, 0.125};
    d.goods[kSilk].k = 2.5;
    CHECK(parse_config(to_toml(d)) == d);
}

TEST_CASE("the v1 config file loads") {
    const Config c = load_config_file(LATTICE_SOURCE_DIR "/configs/v1.toml");
    CHECK(c.gen.width == 10);
    CHECK(c.gen.planets == 100);
}
