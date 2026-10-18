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
        double starve_happiness = 0.03;  // daily happiness lost at total shortfall (PRD-2)
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
        double labour_per_capita = 8;  // 3.5 until Phase A round 1: the world couldn't feed itself
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
        double engage_p = 0.5;          // chance per step that a hostile in range is engaged
        double evade = 0.5;             // a faster quarry's chance to slip away, x its speed edge
        double engage_cooldown_hours = 12;  // after a fight, neither side is engaged again for this long

        friend bool operator==(const Space&, const Space&) = default;
    } space;

    struct Rel {
        double fade = 0.03;          // weekly share of opinion lost toward rest (SYS-10)
        double culture_rest = 20;    // a vassal's resting opinion of its liege per culture share
        std::int64_t recent_deeds = 3;  // kept per relationship for the UI (REL-12)

        friend bool operator==(const Rel&, const Rel&) = default;
    } rel;

    // Opinion change per deed (REL-3), by DeedKind.
    struct Deeds {
        std::array<double, 13> amount = {15, -20, 25, 10, -10, -10, 10, -15, 5, 5, -30, 5, -20};

        friend bool operator==(const Deeds&, const Deeds&) = default;
    } deed;

    struct Fealty {
        bool cascade = true;         // tribute on tribute received (FAC-6)
        double tribute_rate = 0.15;  // starting rate
        double rate_step = 0.05;     // a liege moves a vassal's rate by this
        double rate_min = 0;
        double rate_max = 0.5;
        double raise_opinion = 20;   // a liege short of money raises tribute only on vassals this loyal
        double defect_opinion = -30;
        std::int64_t defect_weeks = 8;
        std::int64_t threat_days = 10;
        double rival_opinion = 20;
        double w_favor = 1;
        double capture_bonus = 30;

        friend bool operator==(const Fealty&, const Fealty&) = default;
    } fealty;

    struct Budget {
        double credit_risk = 1;     // months of income of credit, x risk tolerance (SYS-5)
        double diminishing = 0.6;   // each further unit of an option, at risk 0
        double debt_pressure = 0.1; // cost multiplier per month in the red

        friend bool operator==(const Budget&, const Budget&) = default;
    } budget;

    // Option values for the budget model (SYS-5): first guesses for Phase A.
    struct Value {
        double defence = 0.01;        // a value.power_ref fleet is worth this share of monthly income
        double power_ref = 10;
        double patronage_months = 12; // patronage is worth this many months of the tribute it saves
        double war_mult = 2;          // defence is worth this much more at war
        double border_mult = 1;       // and this much more with an enemy planet within dip.border_days
        double security_unit = 1000;  // static defence is bought in units of this much a month
        double invade_months = 12;    // a planet taken is worth this many months of its income
        double gate_months = 6;       // more for one that holds gates (WLD-5)
        double aftermath_months = 2;  // rebuilding after capture costs this many months of its income
        double blockade_months = 4;   // blockading a gate is worth this many months of its group's income (MIL-17)
        double retake_months = 8;     // and retaking one's own, this many
        double protect = 0.1;         // keep or grant: protection cost per day of travel from the keeper

        friend bool operator==(const Value&, const Value&) = default;
    } value;

    struct Dip {
        double neighbour_days = 3;  // factions this close border each other (DIP-1)
        double border_days = 5;     // a governor's war vote counts enemy planets this close
        // Phase A round 1: 0.2, 0.3, 1 and a threshold of 1 had every faction at war all the time.
        double w_idle = 0.1, w_targets = 0.05, w_strength = 0.1, w_peace = 0.5, w_wars = 1.5, w_losses = 0.5;
        double declare_threshold = 1.5;
        // Money beyond reserve_months of costs votes for war, up to treasury_cap (DIP-4).
        double w_treasury = 0.3, reserve_months = 6, treasury_cap = 3;
        // Suing (DIP-3): losses, a treasury in the red, and weariness a month at war. The
        // number of wars isn't a term of its own; several wars make each go worse.
        double s_planets = 3, s_power = 2, s_treasury = 1, s_months = 0.4;
        double sue_threshold = 1;
        // The winner hears no offer before min_war_days; after, it wants what it hoped
        // to take (hope_share of the enemy's income near its border, by its share of the
        // two sides' power), a hope that halves every hope_halflife_months.
        std::int64_t min_war_days = 30;
        double hope_share = 0.5, hope_halflife_months = 2;
        double tribute_rate = 0.1;  // ceasefire tribute per month, x severity x the loser's income
        std::int64_t ceasefire_days = 90;

        friend bool operator==(const Dip&, const Dip&) = default;
    } dip;

    struct ArmyParams {
        std::int64_t muster_days = 14;
        double detour_ratio = 0.15;

        friend bool operator==(const ArmyParams&, const ArmyParams&) = default;
    } army;

    struct Combat {
        double lethality = 0.05;     // power lost per round per enemy power
        double noise = 0.3;
        double retreat_base = 0.6;   // retreat when own / enemy falls below base - risk x this
        double retreat_risk = 0.4;
        double retreat_loss = 0.2;
        double cargo_destroyed = 0.3;
        double pod_death_p = 0.05;

        friend bool operator==(const Combat&, const Combat&) = default;
    } combat;

    // Gate attacks and blockades (MIL-17).
    struct Blockade {
        bool enabled = true;
        double defence_share = 0.7;   // a gate's static defence against its holder's
        std::int64_t hold_days = 30;  // how long a captor's army garrisons the gate

        friend bool operator==(const Blockade&, const Blockade&) = default;
    } blockade;

    struct Defence {
        double per_spend = 0.005;  // static defence power per credit of monthly security spend
        double regen = 0.05;       // share of its maximum restored a day, when not besieged

        friend bool operator==(const Defence&, const Defence&) = default;
    } defence;

    struct Risk {
        double window = 30;    // days in the average (SYS-19)
        double presence = 0.2; // per hostile ship-day in the sector
        double clear = 0.5;    // per patrol day in the sector

        friend bool operator==(const Risk&, const Risk&) = default;
    } risk;

    struct Raid {
        bool enabled = true;   // EXP-1 switches it off for a faction
        double share = 0.3;    // chance a spare commander is sent each week at war
        std::int64_t days = 10;
        double standoff = 0.15;  // sector widths from the enemy planet

        friend bool operator==(const Raid&, const Raid&) = default;
    } raid;

    struct EscortParams {
        double rate = 20;  // what an escort asks per combat power per day

        friend bool operator==(const EscortParams&, const EscortParams&) = default;
    } escort;

    struct Security {
        double risk_post = 0.5;  // a planet whose sector's risk passes this posts a patrol
        std::int64_t days = 10;
        double pay = 200;        // per day

        friend bool operator==(const Security&, const Security&) = default;
    } security;

    // Rebellion (ECO-13): a planet unhappy and starving for `days` in a row rebels.
    struct Rebel {
        double happiness = 0.15;  // below this
        double shortfall = 0.5;   // and a life-support shortfall above this
        std::int64_t days = 90;

        friend bool operator==(const Rebel&, const Rebel&) = default;
    } rebel;

    struct Pirate {
        std::int64_t flip_days = 60;  // days in deficit before a planet turns (SYS-16)
        double flip_debt = 3;         // and a debt of this many months of costs
        double cost_mult = 0.3;       // a pirate planet's operating costs (ECO-14)
        bool turn_all = false;        // EXP-9: every broke commander turns pirate, not risk-weighted
        double salary = 0.6;          // pirate stipend / lawful stipend (PIR-5)
        double range_days = 20;       // how far, in open space, pirates go raiding (about 3 sectors)
        std::int64_t commanders = 2;  // pirate commanders at each starting pirate planet (GEN-9)
        double need_fill = 0.5, surplus_fill = 1.2;
        double smuggler_risk = 0.7;
        double contraband_price = 1.5;  // x base, what a lawful population pays a smuggler
        double demand_cap = 0.3;        // per thousand a day
        double legal_bonus = 5;         // a legal-goods run's pay per unit delivered, on top of the sale

        friend bool operator==(const Pirate&, const Pirate&) = default;
    } pirate;

    struct PirateRelation {
        double accept = 0, success = 15, fail = -50, rest = -50, fade = 0.02;  // PIR-9

        friend bool operator==(const PirateRelation&, const PirateRelation&) = default;
    } pirel;

    // Suspicion and inspection (INF-14, SYS-17).
    struct Sus {
        double inspect_base = 0.05;   // chance a landing is inspected, before intensity
        double war = 0.5;             // intensity added at war
        double police_per_unit = 0.5; // intensity per value.security_unit a month of policing
        double find_base = 0.3;       // chance an inspection catches a smuggler it knows nothing of
        double scale = 100;           // suspicion that makes a catch certain
        double contraband = 10;       // suspicion added per landing with contraband aboard, not caught
        double pirate = 0.2;          // per point of pirate relation above neutral, at each landing
        double trade = 2;             // removed per genuine trade at a planet of the faction
        double caught = 30;
        double fade = 0.05;           // weekly
        double police_effect = 2;     // the budget's value of policing (SYS-5)
        double no_trade = 15;         // a visit to a planet without trading there (INF-14)
        double muster = 10;           // lingering where an army is forming

        friend bool operator==(const Sus&, const Sus&) = default;
    } sus;

    struct SpyParams {
        bool enabled = true;      // EXP-4 switches spies off
        double salary = 500;      // a month, secret
        double range_days = 10;   // how far from home a spy works
        double value = 0.005;     // the budget's value of a spy: this share of monthly income, x threat

        friend bool operator==(const SpyParams&, const SpyParams&) = default;
    } spy;

    struct AuthorityParams {
        double risk_post = 0.3;           // a sector this risky near the network gets a clearing contract
        std::int64_t clear_days = 30;
        double pay = 300;                 // a day
        std::int64_t contracts = 3;       // most posted a week
        double opinion_per_job = 5;       // its opinion of a mercenary who completes one

        friend bool operator==(const AuthorityParams&, const AuthorityParams&) = default;
    } authority;

    struct Capture {
        double happiness_loss = 0.4;
        double stock_loss = 0.5;
        double pop_loss = 0.05;

        friend bool operator==(const Capture&, const Capture&) = default;
    } capture;

    struct Hire {
        double range_days = 5;
        double stipend_per_power = 300;  // per month
        double w_power = 1;
        double w_opinion = 0.05;
        double w_culture = 0.5;

        friend bool operator==(const Hire&, const Hire&) = default;
    } hire;

    struct Dues {
        double access = 500;   // per month, every lawful planet (ECO-10)
        double gate = 1500;    // per month, more for gate holders
        double maintenance = 0.6;  // share the Authority spends on the network (a sink)

        friend bool operator==(const Dues&, const Dues&) = default;
    } dues;

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
        double commanders_per_income = 1e-5;  // starting combat power per credit of monthly income (GEN-9)
        std::int64_t pirate_planets = 2;      // fringe planets that start as pirate planets (GEN-8)

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
    f("pop.starve_happiness", c.pop.starve_happiness);
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
    f("space.engage_p", c.space.engage_p);
    f("space.evade", c.space.evade);
    f("space.engage_cooldown_hours", c.space.engage_cooldown_hours);
    f("risk.window", c.risk.window);
    f("risk.presence", c.risk.presence);
    f("risk.clear", c.risk.clear);
    f("raid.enabled", c.raid.enabled);
    f("raid.share", c.raid.share);
    f("raid.days", c.raid.days);
    f("raid.standoff", c.raid.standoff);
    f("escort.rate", c.escort.rate);
    f("security.risk_post", c.security.risk_post);
    f("security.days", c.security.days);
    f("security.pay", c.security.pay);
    f("rebel.happiness", c.rebel.happiness);
    f("rebel.shortfall", c.rebel.shortfall);
    f("rebel.days", c.rebel.days);
    f("pirate.flip_days", c.pirate.flip_days);
    f("pirate.flip_debt", c.pirate.flip_debt);
    f("pirate.cost_mult", c.pirate.cost_mult);
    f("pirate.turn_all", c.pirate.turn_all);
    f("pirate.salary", c.pirate.salary);
    f("pirate.range_days", c.pirate.range_days);
    f("pirate.commanders", c.pirate.commanders);
    f("pirate.need_fill", c.pirate.need_fill);
    f("pirate.surplus_fill", c.pirate.surplus_fill);
    f("pirate.smuggler_risk", c.pirate.smuggler_risk);
    f("pirate.contraband_price", c.pirate.contraband_price);
    f("pirate.demand_cap", c.pirate.demand_cap);
    f("pirate.legal_bonus", c.pirate.legal_bonus);
    f("sus.inspect_base", c.sus.inspect_base);
    f("sus.war", c.sus.war);
    f("sus.police_per_unit", c.sus.police_per_unit);
    f("sus.find_base", c.sus.find_base);
    f("sus.scale", c.sus.scale);
    f("sus.contraband", c.sus.contraband);
    f("sus.pirate", c.sus.pirate);
    f("sus.trade", c.sus.trade);
    f("sus.caught", c.sus.caught);
    f("sus.fade", c.sus.fade);
    f("sus.police_effect", c.sus.police_effect);
    f("sus.no_trade", c.sus.no_trade);
    f("sus.muster", c.sus.muster);
    f("spy.enabled", c.spy.enabled);
    f("spy.salary", c.spy.salary);
    f("spy.range_days", c.spy.range_days);
    f("spy.value", c.spy.value);
    f("pirel.accept", c.pirel.accept);
    f("pirel.success", c.pirel.success);
    f("pirel.fail", c.pirel.fail);
    f("pirel.rest", c.pirel.rest);
    f("pirel.fade", c.pirel.fade);
    f("authority.risk_post", c.authority.risk_post);
    f("authority.clear_days", c.authority.clear_days);
    f("authority.pay", c.authority.pay);
    f("authority.contracts", c.authority.contracts);
    f("authority.opinion_per_job", c.authority.opinion_per_job);

    f("rel.fade", c.rel.fade);
    f("rel.culture_rest", c.rel.culture_rest);
    f("rel.recent_deeds", c.rel.recent_deeds);

    constexpr const char* deeds[] = {"help_sent", "help_missed", "planet_granted", "patronage", "tribute_raised",
                                     "passed_over", "call_answered", "call_refused", "border_held", "job_done",
                                     "attacked", "tribute_lowered", "caught"};
    for (std::size_t i = 0; i < c.deed.amount.size(); ++i) f(std::string("deed.") + deeds[i], c.deed.amount[i]);

    f("fealty.cascade", c.fealty.cascade);
    f("fealty.tribute_rate", c.fealty.tribute_rate);
    f("fealty.rate_step", c.fealty.rate_step);
    f("fealty.rate_min", c.fealty.rate_min);
    f("fealty.rate_max", c.fealty.rate_max);
    f("fealty.raise_opinion", c.fealty.raise_opinion);
    f("fealty.defect_opinion", c.fealty.defect_opinion);
    f("fealty.defect_weeks", c.fealty.defect_weeks);
    f("fealty.threat_days", c.fealty.threat_days);
    f("fealty.rival_opinion", c.fealty.rival_opinion);
    f("fealty.w_favor", c.fealty.w_favor);
    f("fealty.capture_bonus", c.fealty.capture_bonus);

    f("budget.credit_risk", c.budget.credit_risk);
    f("budget.diminishing", c.budget.diminishing);
    f("budget.debt_pressure", c.budget.debt_pressure);

    f("value.defence", c.value.defence);
    f("value.power_ref", c.value.power_ref);
    f("value.patronage_months", c.value.patronage_months);

    f("value.war_mult", c.value.war_mult);
    f("value.border_mult", c.value.border_mult);
    f("value.security_unit", c.value.security_unit);
    f("value.invade_months", c.value.invade_months);
    f("value.blockade_months", c.value.blockade_months);
    f("value.retake_months", c.value.retake_months);
    f("value.gate_months", c.value.gate_months);
    f("value.aftermath_months", c.value.aftermath_months);
    f("value.protect", c.value.protect);

    f("dip.neighbour_days", c.dip.neighbour_days);
    f("dip.border_days", c.dip.border_days);
    f("dip.w_idle", c.dip.w_idle);
    f("dip.w_targets", c.dip.w_targets);
    f("dip.w_strength", c.dip.w_strength);
    f("dip.w_peace", c.dip.w_peace);
    f("dip.w_wars", c.dip.w_wars);
    f("dip.w_losses", c.dip.w_losses);
    f("dip.declare_threshold", c.dip.declare_threshold);
    f("dip.w_treasury", c.dip.w_treasury);
    f("dip.reserve_months", c.dip.reserve_months);
    f("dip.treasury_cap", c.dip.treasury_cap);
    f("dip.s_planets", c.dip.s_planets);
    f("dip.s_power", c.dip.s_power);
    f("dip.s_treasury", c.dip.s_treasury);
    f("dip.s_months", c.dip.s_months);
    f("dip.sue_threshold", c.dip.sue_threshold);
    f("dip.min_war_days", c.dip.min_war_days);
    f("dip.hope_share", c.dip.hope_share);
    f("dip.hope_halflife_months", c.dip.hope_halflife_months);
    f("dip.tribute_rate", c.dip.tribute_rate);
    f("dip.ceasefire_days", c.dip.ceasefire_days);

    f("army.muster_days", c.army.muster_days);
    f("army.detour_ratio", c.army.detour_ratio);

    f("combat.lethality", c.combat.lethality);
    f("combat.noise", c.combat.noise);
    f("combat.retreat_base", c.combat.retreat_base);
    f("combat.retreat_risk", c.combat.retreat_risk);
    f("combat.retreat_loss", c.combat.retreat_loss);
    f("combat.cargo_destroyed", c.combat.cargo_destroyed);
    f("pod.death_p", c.combat.pod_death_p);

    f("defence.per_spend", c.defence.per_spend);
    f("defence.regen", c.defence.regen);
    f("blockade.enabled", c.blockade.enabled);
    f("blockade.defence_share", c.blockade.defence_share);
    f("blockade.hold_days", c.blockade.hold_days);

    f("capture.happiness_loss", c.capture.happiness_loss);
    f("capture.stock_loss", c.capture.stock_loss);
    f("capture.pop_loss", c.capture.pop_loss);

    f("hire.range_days", c.hire.range_days);
    f("hire.stipend_per_power", c.hire.stipend_per_power);
    f("hire.w_power", c.hire.w_power);
    f("hire.w_opinion", c.hire.w_opinion);
    f("hire.w_culture", c.hire.w_culture);

    f("dues.access", c.dues.access);
    f("dues.gate", c.dues.gate);
    f("authority.maintenance", c.dues.maintenance);

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
    f("gen.commanders_per_income", c.gen.commanders_per_income);
    f("gen.pirate_planets", c.gen.pirate_planets);
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
