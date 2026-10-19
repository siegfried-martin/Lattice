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
        double burn = 1;           // per sector width on the Lattice, per ship
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

    struct Char {
        std::int64_t target = 500;  // dev 500, v1 2,000
        std::int64_t spawn_per_day = 3;
        double start_wealth = 2000;
        double start_armed_p = 0.5;
        std::int64_t broke_days = 14;

        friend bool operator==(const Char&, const Char&) = default;
    } chars;

    // Preset ships (MOD-1, CHR-12, CHR-13): tier-1 stats per class, scaled per tier and
    // varied per culture. Content to be designed (OPN-35); these make the classes differ.
    struct ShipClassParams {
        double combat = 0;
        double cargo = 0;  // tons
        double speed = 0;  // sector widths per day in open space
        double price = 0;

        friend bool operator==(const ShipClassParams&, const ShipClassParams&) = default;
    };
    struct Ship {
        std::array<ShipClassParams, 4> classes = {{
            {0, 60, 0.3, 8000},       // courier
            {2, 20, 0.25, 10000},     // scout
            {0.5, 400, 0.12, 20000},  // freighter
            {5, 30, 0.18, 30000},     // gunship
        }};
        double tier_combat = 1.8;  // per tier above 1
        double tier_cargo = 2.0;
        double tier_speed = 1.05;
        double tier_price = 2.5;
        double culture_spread = 0.15;  // each culture's variant is within +- this
        double resale = 0.5;           // share of the price a sold ship returns

        friend bool operator==(const Ship&, const Ship&) = default;
    } ship;

    struct Fleet {
        std::array<double, 3> run_cost = {5, 15, 40};  // per ship per day, by tier (SYS-21)
        double munitions_per_power = 100;
        std::int64_t max_ships = 4;

        friend bool operator==(const Fleet&, const Fleet&) = default;
    } fleet;

    struct Merchant {
        std::int64_t candidates = 12;
        double min_score = 50;
        double loss_rate = 0.1;
        double threat_ref = 5;
        double info_value = 300;   // per month
        double salary = 600;       // per month, offered by every root (MS-4: the budget sets it)
        double per_planet = 1;     // salaried merchants a root wants per faction planet
        double stale_days = 30;    // older prices send an explorer to look again
        double dock_hours = 6;     // time spent at each stop (TRD-12)

        friend bool operator==(const Merchant&, const Merchant&) = default;
    } merchant;

    struct MissionParams {
        double pay_growth = 0.05;
        std::int64_t expiry_days = 30;
        double courier_pay = 150;  // per day of the expected trip
        double scout_pay = 300;

        friend bool operator==(const MissionParams&, const MissionParams&) = default;
    } mission;

    struct Skill {
        double gain = 1;
        double max = 20;
        double bonus = 0.02;

        friend bool operator==(const Skill&, const Skill&) = default;
    } skill;

    struct Spend {
        double threshold_base = 2;  // wealth over the next ship's price before buying:
        double threshold_risk = 1;  // base - risk x this
        double reserve_months = 3;  // running costs the cautious keep back: this x (1 - risk)

        friend bool operator==(const Spend&, const Spend&) = default;
    } spend;

    struct Courier {
        std::int64_t interval_days = 7;
        double run_cost = 50;
        double mission_share = 0.25;  // runs posted as missions instead of automated

        friend bool operator==(const Courier&, const Courier&) = default;
    } courier;

    struct Scout {
        double min_combat = 1;
        double stale_days = 60;  // a root scouts outside markets it hasn't heard of for this long
        std::int64_t threat_days = 7;

        friend bool operator==(const Scout&, const Scout&) = default;
    } scout;

    struct Space {
        double detect_range = 0.25;     // sector widths (the rendered 4 km)
        double meet_cooldown_hours = 6;  // a character merges facts in open space at most this often

        friend bool operator==(const Space&, const Space&) = default;
    } space;

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
        double independent_share = 0.1;  // planets that start as their own roots (GEN-8)

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
    f("fuel.burn", c.fuel.burn);

    f("travel.lattice_speed", c.travel.lattice_speed);
    f("travel.space_speed", c.travel.space_speed);
    f("travel.hop_days", c.travel.hop_days);

    f("char.target", c.chars.target);
    f("char.spawn_per_day", c.chars.spawn_per_day);
    f("char.start_wealth", c.chars.start_wealth);
    f("char.start_armed_p", c.chars.start_armed_p);
    f("char.broke_days", c.chars.broke_days);

    constexpr const char* classes[] = {"courier", "scout", "freighter", "gunship"};
    for (std::size_t i = 0; i < c.ship.classes.size(); ++i) {
        const std::string prefix = std::string("ship.") + classes[i] + ".";
        f(prefix + "combat", c.ship.classes[i].combat);
        f(prefix + "cargo", c.ship.classes[i].cargo);
        f(prefix + "speed", c.ship.classes[i].speed);
        f(prefix + "price", c.ship.classes[i].price);
    }
    f("ship.tier_combat", c.ship.tier_combat);
    f("ship.tier_cargo", c.ship.tier_cargo);
    f("ship.tier_speed", c.ship.tier_speed);
    f("ship.tier_price", c.ship.tier_price);
    f("ship.culture_spread", c.ship.culture_spread);
    f("ship.resale", c.ship.resale);

    f("fleet.run_cost", c.fleet.run_cost);
    f("fleet.munitions_per_power", c.fleet.munitions_per_power);
    f("fleet.max_ships", c.fleet.max_ships);

    f("merchant.candidates", c.merchant.candidates);
    f("merchant.min_score", c.merchant.min_score);
    f("merchant.loss_rate", c.merchant.loss_rate);
    f("merchant.threat_ref", c.merchant.threat_ref);
    f("merchant.info_value", c.merchant.info_value);
    f("merchant.salary", c.merchant.salary);
    f("merchant.per_planet", c.merchant.per_planet);
    f("merchant.stale_days", c.merchant.stale_days);
    f("merchant.dock_hours", c.merchant.dock_hours);

    f("mission.pay_growth", c.mission.pay_growth);
    f("mission.expiry_days", c.mission.expiry_days);
    f("mission.courier_pay", c.mission.courier_pay);
    f("mission.scout_pay", c.mission.scout_pay);

    f("skill.gain", c.skill.gain);
    f("skill.max", c.skill.max);
    f("skill.bonus", c.skill.bonus);

    f("spend.threshold_base", c.spend.threshold_base);
    f("spend.threshold_risk", c.spend.threshold_risk);
    f("spend.reserve_months", c.spend.reserve_months);

    f("courier.interval_days", c.courier.interval_days);
    f("courier.run_cost", c.courier.run_cost);
    f("courier.mission_share", c.courier.mission_share);

    f("scout.min_combat", c.scout.min_combat);
    f("scout.stale_days", c.scout.stale_days);
    f("scout.threat_days", c.scout.threat_days);

    f("space.detect_range", c.space.detect_range);
    f("space.meet_cooldown_hours", c.space.meet_cooldown_hours);

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
    f("gen.independent_share", c.gen.independent_share);
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
