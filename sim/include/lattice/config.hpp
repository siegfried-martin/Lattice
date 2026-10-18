#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

#include "lattice/goods.hpp"

namespace lattice {

// Every tunable the simulation reads (ARC-7). Defaults are the starting values in
// docs/spec/sim/tunables.md; keep the two in step. Keys are added as milestones need
// them. The defaults build a development-size world (GEN-1); sim/configs/v1.toml
// overrides the sizes for a v1 world.
struct Config {
    struct Clock {
        double real_seconds_per_day = 60;     // rendered clock (WLD-15)
        std::int64_t space_step_minutes = 10;  // open-space step (TIM-6)
        std::int64_t battle_round_hours = 1;   // SYS-14
        std::int64_t labour_days = 7;          // cadences (TIM-3)
        std::int64_t budget_days = 7;
        std::int64_t diplomacy_days = 7;
        std::int64_t payment_days = 30;

        friend bool operator==(const Clock&, const Clock&) = default;
    } time;

    struct Debug {
        // Records that ping at random intervals, to exercise the event queue and the
        // RNG streams. Scaffolding from MS-1; 0 in real runs.
        std::int64_t pings = 0;

        friend bool operator==(const Debug&, const Debug&) = default;
    } debug;

    // Per good (MOD-1). Which keys a good has depends on its kind; see for_each_field.
    struct GoodParams {
        double base_price = 10;    // price at reserve
        double reserve_days = 30;  // reserve target in days of consumption
        double k = 1.5;            // price exponent
        double consumption = 0;    // per thousand per day (life support, contraband)
        double happiness = 0;      // per unit per thousand (luxuries, contraband)

        friend bool operator==(const GoodParams&, const GoodParams&) = default;
    };
    std::array<GoodParams, kGoodCount> goods = {{
        {10, 30, 1.5, 1.0, 0},    // food
        {10, 30, 1.5, 1.0, 0},    // water
        {10, 30, 1.5, 1.0, 0},    // oxygen
        {30, 15, 1.0, 0, 0.02},   // wine
        {30, 15, 1.0, 0, 0.02},   // spice
        {30, 15, 1.0, 0, 0.02},   // silk
        {60, 15, 1.0, 0.2, 0.06}, // stims (contraband)
    }};

    struct Price {
        double min = 0.2;      // clamp on price / base
        double max = 5;
        double spread = 0.06;  // ask-bid spread
        double chunk = 0.05;   // trade chunk as a share of reserve (slippage)

        friend bool operator==(const Price&, const Price&) = default;
    } price;

    struct Econ {
        double tax = 0.05;           // transaction tax (ECO-3)
        double wage = 40;            // purchasing power per thousand per day (ECO-1)
        double credit_limit = 2;     // months of income a planet may owe and still buy
        double op_base = 2000;       // operating cost per month (ECO-4)
        double op_per_capita = 300;  // per thousand per month

        friend bool operator==(const Econ&, const Econ&) = default;
    } econ;

    struct Pop {
        double growth_rate = 0.0005;  // daily growth at full excess
        double shrink_rate = 0.002;   // daily shrink at total shortfall
        double growth_factor_min = 0.5;
        double growth_factor_max = 1.5;
        double happiness_drain = 0.01;
        double p_base = 0.6;
        double p_happy = 0.6;
        double contraband_penalty = 4;
        std::int64_t contraband_window = 30;

        friend bool operator==(const Pop&, const Pop&) = default;
    } pop;

    struct Lux {
        double consumption = 0.5;  // luxury use per thousand at full fill
        double max_fill = 1.5;
        double variety_min = 0.2;  // share of a luxury's full use that counts for variety
        std::array<double, 4> variety_bonus = {0, 0, 0.003, 0.006};  // by luxuries used

        friend bool operator==(const Lux&, const Lux&) = default;
    } lux;

    struct Prod {
        double efficiency_high = 1.0;
        double efficiency_medium = 0.6;
        double efficiency_low = 0.3;
        double labour_per_capita = 3.5;
        double labour_shift = 0.05;

        friend bool operator==(const Prod&, const Prod&) = default;
    } prod;

    struct Events {
        double drought_p = 0.0005;
        std::int64_t drought_days = 30;
        double bumper_p = 0.0005;
        std::int64_t bumper_days = 15;
        double bumper_mult = 1.3;
        double festival_p = 0.0005;
        std::int64_t festival_days = 14;
        double festival_mult = 1.5;
        double region_drought_p = 0.0002;

        friend bool operator==(const Events&, const Events&) = default;
    } event;

    struct Osmosis {
        double range_days = 3;
        double rate = 0.01;
        double falloff = 2;
        double max_close = 0.5;

        friend bool operator==(const Osmosis&, const Osmosis&) = default;
    } osmosis;

    struct Fuel {
        double base = 2;
        double fringe = 1.0;       // extra price at the farthest point from a seat
        double fringe_mult = 1.5;  // multiplier for planets off the network

        friend bool operator==(const Fuel&, const Fuel&) = default;
    } fuel;

    // Reference travel speeds (TIM-4), in sector widths per game day. Ships get their
    // own open-space speeds with ship variants (MS-3); these time osmosis and planning.
    struct Travel {
        double lattice_speed = 2;
        double space_speed = 0.15;
        double hop_days = 0.25;

        friend bool operator==(const Travel&, const Travel&) = default;
    } travel;

    struct Gen {
        std::int64_t width = 6;  // sectors
        std::int64_t height = 8;
        std::int64_t planets = 30;  // target, moons included
        std::int64_t regions = 3;
        std::int64_t seats = 3;
        double fringe_share = 0.15;
        double lattice_loops = 0.3;
        std::int64_t min_producers = 2;
        double start_treasury_months = 3;
        std::int64_t warmup_days = 180;
        double moon_share = 0.25;      // planets with moons (WLD-4)
        std::int64_t split_planets = 4;  // a system with this many planets is two groups
        double pop_median = 1000;      // thousands
        double pop_spread = 0.8;       // log-normal sigma per planet
        double region_pop_spread = 0.3;  // log-normal sigma per region
        double moon_pop = 0.3;         // a moon's population against a planet's
        double start_happiness = 0.5;
        double weight_spread = 0.5;  // region production weights in 1 +- this
        double favour_mult = 3;      // weight of each region's favoured good

        friend bool operator==(const Gen&, const Gen&) = default;
    } gen;

    friend bool operator==(const Config&, const Config&) = default;
};

// Calls f(key, field) for every field, in a fixed order. Loading, writing and saving
// all go through this list, so a new field needs adding only here (and in validate).
// Field types: std::int64_t, double, bool, std::array<double, N>.
template <class C, class F>
void for_each_field(C& c, F&& f) {
    f("time.real_seconds_per_day", c.time.real_seconds_per_day);
    f("time.space_step_minutes", c.time.space_step_minutes);
    f("time.battle_round_hours", c.time.battle_round_hours);
    f("time.labour_days", c.time.labour_days);
    f("time.budget_days", c.time.budget_days);
    f("time.diplomacy_days", c.time.diplomacy_days);
    f("time.payment_days", c.time.payment_days);
    f("debug.pings", c.debug.pings);

    for (int i = 0; i < kGoodCount; ++i) {
        const Good g = static_cast<Good>(i);
        auto& p = c.goods[static_cast<std::size_t>(i)];
        const std::string prefix = "good." + std::string(good_name(g)) + ".";
        f(prefix + "base_price", p.base_price);
        f(prefix + "reserve_days", p.reserve_days);
        f(prefix + "k", p.k);
        if (kind_of(g) != GoodKind::Luxury) f(prefix + "consumption", p.consumption);
        if (kind_of(g) != GoodKind::LifeSupport) f(prefix + "happiness", p.happiness);
    }

    f("price.min", c.price.min);
    f("price.max", c.price.max);
    f("price.spread", c.price.spread);
    f("price.chunk", c.price.chunk);

    f("econ.tax", c.econ.tax);
    f("econ.wage", c.econ.wage);
    f("econ.credit_limit", c.econ.credit_limit);
    f("econ.op_base", c.econ.op_base);
    f("econ.op_per_capita", c.econ.op_per_capita);

    f("pop.growth_rate", c.pop.growth_rate);
    f("pop.shrink_rate", c.pop.shrink_rate);
    f("pop.growth_factor_min", c.pop.growth_factor_min);
    f("pop.growth_factor_max", c.pop.growth_factor_max);
    f("pop.happiness_drain", c.pop.happiness_drain);
    f("pop.p_base", c.pop.p_base);
    f("pop.p_happy", c.pop.p_happy);
    f("pop.contraband_penalty", c.pop.contraband_penalty);
    f("pop.contraband_window", c.pop.contraband_window);

    f("lux.consumption", c.lux.consumption);
    f("lux.max_fill", c.lux.max_fill);
    f("lux.variety_min", c.lux.variety_min);
    f("lux.variety_bonus", c.lux.variety_bonus);

    f("prod.efficiency_high", c.prod.efficiency_high);
    f("prod.efficiency_medium", c.prod.efficiency_medium);
    f("prod.efficiency_low", c.prod.efficiency_low);
    f("prod.labour_per_capita", c.prod.labour_per_capita);
    f("prod.labour_shift", c.prod.labour_shift);

    f("event.drought_p", c.event.drought_p);
    f("event.drought_days", c.event.drought_days);
    f("event.bumper_p", c.event.bumper_p);
    f("event.bumper_days", c.event.bumper_days);
    f("event.bumper_mult", c.event.bumper_mult);
    f("event.festival_p", c.event.festival_p);
    f("event.festival_days", c.event.festival_days);
    f("event.festival_mult", c.event.festival_mult);
    f("event.region_drought_p", c.event.region_drought_p);

    f("osmosis.range_days", c.osmosis.range_days);
    f("osmosis.rate", c.osmosis.rate);
    f("osmosis.falloff", c.osmosis.falloff);
    f("osmosis.max_close", c.osmosis.max_close);

    f("fuel.base", c.fuel.base);
    f("fuel.fringe", c.fuel.fringe);
    f("fuel.fringe_mult", c.fuel.fringe_mult);

    f("travel.lattice_speed", c.travel.lattice_speed);
    f("travel.space_speed", c.travel.space_speed);
    f("travel.hop_days", c.travel.hop_days);

    f("gen.width", c.gen.width);
    f("gen.height", c.gen.height);
    f("gen.planets", c.gen.planets);
    f("gen.regions", c.gen.regions);
    f("gen.seats", c.gen.seats);
    f("gen.fringe_share", c.gen.fringe_share);
    f("gen.lattice_loops", c.gen.lattice_loops);
    f("gen.min_producers", c.gen.min_producers);
    f("gen.start_treasury_months", c.gen.start_treasury_months);
    f("gen.warmup_days", c.gen.warmup_days);
    f("gen.moon_share", c.gen.moon_share);
    f("gen.split_planets", c.gen.split_planets);
    f("gen.pop_median", c.gen.pop_median);
    f("gen.pop_spread", c.gen.pop_spread);
    f("gen.region_pop_spread", c.gen.region_pop_spread);
    f("gen.moon_pop", c.gen.moon_pop);
    f("gen.start_happiness", c.gen.start_happiness);
    f("gen.weight_spread", c.gen.weight_spread);
    f("gen.favour_mult", c.gen.favour_mult);
}

class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Missing keys keep their defaults. Unknown keys, wrong types and invalid values throw
// ConfigError, so a typo in a config file can't pass silently.
Config parse_config(std::string_view toml_text, std::string_view source = "config");
Config load_config_file(const std::string& path);
void validate(const Config& config);

// The full config, defaults included, as TOML (written into each run's output, ARC-7).
std::string to_toml(const Config& config);

}  // namespace lattice
