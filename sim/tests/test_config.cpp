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
