#include "lattice/config.hpp"

#include <array>
#include <fstream>
#include <set>
#include <sstream>
#include <type_traits>

#include <toml.hpp>

namespace lattice {

namespace {

std::string where(std::string_view source, const toml::node& node) {
    std::ostringstream out;
    out << source << ":" << node.source().begin.line;
    return out.str();
}

// Collects every leaf key, dotted, so unknown ones can be reported.
void collect_keys(const toml::table& table, const std::string& prefix,
                  std::set<std::string>& out) {
    for (auto&& [key, node] : table) {
        std::string path = prefix.empty() ? std::string(key.str()) : prefix + "." + std::string(key.str());
        if (const toml::table* sub = node.as_table()) {
            collect_keys(*sub, path, out);
        } else {
            out.insert(path);
        }
    }
}

template <class T>
struct is_std_array : std::false_type {};
template <std::size_t N>
struct is_std_array<std::array<double, N>> : std::true_type {};

// Inserts value at a dotted path, creating the tables on the way.
template <class T>
void insert_path(toml::table& root, std::string_view path, const T& value) {
    toml::table* table = &root;
    std::size_t start = 0;
    for (std::size_t dot; (dot = path.find('.', start)) != std::string_view::npos; start = dot + 1) {
        auto [it, _] = table->insert(path.substr(start, dot - start), toml::table{});
        table = it->second.as_table();
    }
    if constexpr (is_std_array<T>::value) {
        toml::array array;
        for (double v : value) array.push_back(v);
        table->insert_or_assign(path.substr(start), std::move(array));
    } else {
        table->insert_or_assign(path.substr(start), value);
    }
}

}  // namespace

Config parse_config(std::string_view toml_text, std::string_view source) {
    toml::table table;
    try {
        table = toml::parse(toml_text, source);
    } catch (const toml::parse_error& e) {
        std::ostringstream out;
        out << source << ":" << e.source().begin.line << ": " << e.description();
        throw ConfigError(out.str());
    }

    Config config;
    std::set<std::string> known;
    for_each_field(config, [&](std::string_view key, auto& field) {
        known.insert(std::string(key));
        toml::node_view node = table.at_path(key);
        if (!node) return;
        using T = std::remove_reference_t<decltype(field)>;
        if constexpr (std::is_same_v<T, double>) {
            if (auto f = node.as_floating_point()) {
                field = f->get();
            } else if (auto i = node.as_integer()) {
                field = static_cast<double>(i->get());
            } else {
                throw ConfigError(where(source, *node.node()) + ": " + std::string(key) + " must be a number");
            }
        } else if constexpr (is_std_array<T>::value) {
            const toml::array* array = node.as_array();
            if (!array || array->size() != field.size()) {
                throw ConfigError(where(source, *node.node()) + ": " + std::string(key) + " must be a list of " +
                                  std::to_string(field.size()) + " numbers");
            }
            for (std::size_t i = 0; i < field.size(); ++i) {
                const toml::node& item = *array->get(i);
                if (auto f = item.as_floating_point()) {
                    field[i] = f->get();
                } else if (auto n = item.as_integer()) {
                    field[i] = static_cast<double>(n->get());
                } else {
                    throw ConfigError(where(source, item) + ": " + std::string(key) + " must be a list of numbers");
                }
            }
        } else if constexpr (std::is_same_v<T, bool>) {
            auto b = node.as_boolean();
            if (!b) throw ConfigError(where(source, *node.node()) + ": " + std::string(key) + " must be true or false");
            field = b->get();
        } else {
            static_assert(std::is_same_v<T, std::int64_t>, "add parsing for this field type");
            auto i = node.as_integer();
            if (!i) throw ConfigError(where(source, *node.node()) + ": " + std::string(key) + " must be an integer");
            field = i->get();
        }
    });

    std::set<std::string> present;
    collect_keys(table, "", present);
    for (const std::string& key : present) {
        if (!known.contains(key)) {
            throw ConfigError(where(source, *table.at_path(key).node()) + ": unknown key " + key);
        }
    }

    validate(config);
    return config;
}

Config load_config_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw ConfigError("cannot open config " + path);
    std::ostringstream text;
    text << in.rdbuf();
    return parse_config(text.str(), path);
}

void validate(const Config& c) {
    auto require = [](bool ok, const std::string& message) {
        if (!ok) throw ConfigError(message);
    };
    auto positive = [&](double v, const char* key) { require(v > 0, std::string(key) + " must be above 0"); };
    auto non_negative = [&](double v, const char* key) { require(v >= 0, std::string(key) + " can't be negative"); };
    auto share = [&](double v, const char* key) { require(v >= 0 && v <= 1, std::string(key) + " must be 0 to 1"); };

    positive(c.time.real_seconds_per_day, "time.real_seconds_per_day");
    positive(static_cast<double>(c.time.space_step_minutes), "time.space_step_minutes");
    positive(static_cast<double>(c.time.battle_round_hours), "time.battle_round_hours");
    positive(static_cast<double>(c.time.labour_days), "time.labour_days");
    positive(static_cast<double>(c.time.budget_days), "time.budget_days");
    positive(static_cast<double>(c.time.diplomacy_days), "time.diplomacy_days");
    positive(static_cast<double>(c.time.payment_days), "time.payment_days");
    require(c.debug.pings >= 0 && c.debug.pings <= 1'000'000, "debug.pings must be 0 to 1,000,000");

    for (int i = 0; i < kGoodCount; ++i) {
        const auto& g = c.goods[static_cast<std::size_t>(i)];
        const std::string name = "good." + std::string(good_name(static_cast<Good>(i)));
        require(g.base_price > 0, name + ".base_price must be above 0");
        require(g.reserve_days > 0, name + ".reserve_days must be above 0");
        require(g.k > 0, name + ".k must be above 0");
        require(g.consumption >= 0, name + ".consumption can't be negative");
        require(g.happiness >= 0, name + ".happiness can't be negative");
    }

    positive(c.price.min, "price.min");
    require(c.price.max >= c.price.min, "price.max must be at least price.min");
    require(c.price.spread >= 0 && c.price.spread < 2, "price.spread must be 0 to 2");
    positive(c.price.chunk, "price.chunk");

    require(c.econ.tax >= 0 && c.econ.tax < 1, "econ.tax must be 0 to 1");
    non_negative(c.econ.wage, "econ.wage");
    non_negative(c.econ.credit_limit, "econ.credit_limit");
    non_negative(c.econ.op_base, "econ.op_base");
    non_negative(c.econ.op_per_capita, "econ.op_per_capita");

    non_negative(c.pop.growth_rate, "pop.growth_rate");
    share(c.pop.shrink_rate, "pop.shrink_rate");
    non_negative(c.pop.growth_factor_min, "pop.growth_factor_min");
    require(c.pop.growth_factor_max >= c.pop.growth_factor_min, "pop.growth_factor_max must be at least pop.growth_factor_min");
    non_negative(c.pop.happiness_drain, "pop.happiness_drain");
    non_negative(c.pop.p_base, "pop.p_base");
    non_negative(c.pop.p_happy, "pop.p_happy");
    non_negative(c.pop.contraband_penalty, "pop.contraband_penalty");
    positive(static_cast<double>(c.pop.contraband_window), "pop.contraband_window");

    non_negative(c.lux.consumption, "lux.consumption");
    positive(c.lux.max_fill, "lux.max_fill");
    non_negative(c.lux.variety_min, "lux.variety_min");

    positive(c.prod.efficiency_high, "prod.efficiency_high");
    positive(c.prod.efficiency_medium, "prod.efficiency_medium");
    positive(c.prod.efficiency_low, "prod.efficiency_low");
    non_negative(c.prod.labour_per_capita, "prod.labour_per_capita");
    share(c.prod.labour_shift, "prod.labour_shift");

    share(c.event.drought_p, "event.drought_p");
    positive(static_cast<double>(c.event.drought_days), "event.drought_days");
    share(c.event.bumper_p, "event.bumper_p");
    positive(static_cast<double>(c.event.bumper_days), "event.bumper_days");
    positive(c.event.bumper_mult, "event.bumper_mult");
    share(c.event.festival_p, "event.festival_p");
    positive(static_cast<double>(c.event.festival_days), "event.festival_days");
    positive(c.event.festival_mult, "event.festival_mult");
    share(c.event.region_drought_p, "event.region_drought_p");

    non_negative(c.osmosis.range_days, "osmosis.range_days");
    non_negative(c.osmosis.rate, "osmosis.rate");
    positive(c.osmosis.falloff, "osmosis.falloff");
    share(c.osmosis.max_close, "osmosis.max_close");

    non_negative(c.fuel.base, "fuel.base");
    non_negative(c.fuel.fringe, "fuel.fringe");
    positive(c.fuel.fringe_mult, "fuel.fringe_mult");

    positive(c.travel.lattice_speed, "travel.lattice_speed");
    positive(c.travel.space_speed, "travel.space_speed");
    non_negative(c.travel.hop_days, "travel.hop_days");

    require(c.chars.target >= 0 && c.chars.target <= 1'000'000, "char.target must be 0 to 1,000,000");
    require(c.chars.spawn_per_day >= 0, "char.spawn_per_day can't be negative");
    non_negative(c.chars.start_wealth, "char.start_wealth");
    share(c.chars.start_armed_p, "char.start_armed_p");
    require(c.chars.broke_days >= 0, "char.broke_days can't be negative");
    for (const auto& k : c.ship.classes) {
        require(k.combat >= 0 && k.cargo >= 0, "ship combat and cargo can't be negative");
        require(k.speed > 0 && k.price > 0, "ship speed and price must be above 0");
    }
    positive(c.ship.tier_combat, "ship.tier_combat");
    positive(c.ship.tier_cargo, "ship.tier_cargo");
    positive(c.ship.tier_speed, "ship.tier_speed");
    positive(c.ship.tier_price, "ship.tier_price");
    require(c.ship.culture_spread >= 0 && c.ship.culture_spread < 1, "ship.culture_spread must be 0 to 1");
    share(c.ship.resale, "ship.resale");
    for (double r : c.fleet.run_cost) non_negative(r, "fleet.run_cost");
    non_negative(c.fleet.munitions_per_power, "fleet.munitions_per_power");
    require(c.fleet.max_ships >= 1, "fleet.max_ships must be at least 1");
    require(c.merchant.candidates >= 1, "merchant.candidates must be at least 1");
    non_negative(c.merchant.info_value, "merchant.info_value");
    non_negative(c.merchant.salary, "merchant.salary");
    non_negative(c.merchant.per_planet, "merchant.per_planet");
    positive(c.merchant.stale_days, "merchant.stale_days");
    non_negative(c.merchant.dock_hours, "merchant.dock_hours");
    non_negative(c.mission.pay_growth, "mission.pay_growth");
    positive(static_cast<double>(c.mission.expiry_days), "mission.expiry_days");
    non_negative(c.mission.courier_pay, "mission.courier_pay");
    non_negative(c.mission.scout_pay, "mission.scout_pay");
    non_negative(c.skill.gain, "skill.gain");
    non_negative(c.skill.max, "skill.max");
    non_negative(c.skill.bonus, "skill.bonus");
    non_negative(c.spend.threshold_base, "spend.threshold_base");
    non_negative(c.spend.reserve_months, "spend.reserve_months");
    positive(static_cast<double>(c.courier.interval_days), "courier.interval_days");
    non_negative(c.courier.run_cost, "courier.run_cost");
    share(c.courier.mission_share, "courier.mission_share");
    non_negative(c.scout.min_combat, "scout.min_combat");
    positive(c.scout.stale_days, "scout.stale_days");
    positive(c.space.detect_range, "space.detect_range");
    non_negative(c.space.meet_cooldown_hours, "space.meet_cooldown_hours");
    non_negative(c.fuel.burn, "fuel.burn");
    share(c.gen.independent_share, "gen.independent_share");
    non_negative(c.gen.commanders_per_income, "gen.commanders_per_income");
    share(c.rel.fade, "rel.fade");
    require(c.rel.recent_deeds >= 0, "rel.recent_deeds can't be negative");
    share(c.fealty.tribute_rate, "fealty.tribute_rate");
    share(c.fealty.rate_step, "fealty.rate_step");
    require(c.fealty.rate_min >= 0 && c.fealty.rate_max <= 1 && c.fealty.rate_min <= c.fealty.rate_max,
            "fealty.rate_min and rate_max must be 0 <= min <= max <= 1");
    require(c.fealty.defect_weeks >= 0, "fealty.defect_weeks can't be negative");
    non_negative(c.budget.credit_risk, "budget.credit_risk");
    require(c.budget.diminishing > 0 && c.budget.diminishing < 1, "budget.diminishing must be between 0 and 1");
    non_negative(c.budget.debt_pressure, "budget.debt_pressure");
    non_negative(c.value.defence, "value.defence");
    positive(c.value.power_ref, "value.power_ref");
    non_negative(c.value.patronage_months, "value.patronage_months");
    non_negative(c.hire.range_days, "hire.range_days");
    positive(c.hire.stipend_per_power, "hire.stipend_per_power");
    non_negative(c.dues.access, "dues.access");
    non_negative(c.dues.gate, "dues.gate");
    share(c.dues.maintenance, "authority.maintenance");
    non_negative(c.value.war_mult, "value.war_mult");
    non_negative(c.value.border_mult, "value.border_mult");
    positive(c.value.security_unit, "value.security_unit");
    non_negative(c.value.invade_months, "value.invade_months");
    non_negative(c.value.gate_months, "value.gate_months");
    non_negative(c.value.aftermath_months, "value.aftermath_months");
    non_negative(c.value.protect, "value.protect");
    non_negative(c.dip.neighbour_days, "dip.neighbour_days");
    non_negative(c.dip.border_days, "dip.border_days");
    non_negative(c.dip.tribute_rate, "dip.tribute_rate");
    positive(static_cast<double>(c.dip.ceasefire_days), "dip.ceasefire_days");
    positive(static_cast<double>(c.army.muster_days), "army.muster_days");
    non_negative(c.army.detour_ratio, "army.detour_ratio");
    non_negative(c.combat.lethality, "combat.lethality");
    require(c.combat.noise >= 0 && c.combat.noise < 1, "combat.noise must be 0 to 1");
    non_negative(c.combat.retreat_base, "combat.retreat_base");
    share(c.combat.retreat_loss, "combat.retreat_loss");
    share(c.combat.cargo_destroyed, "combat.cargo_destroyed");
    share(c.combat.pod_death_p, "pod.death_p");
    non_negative(c.defence.per_spend, "defence.per_spend");
    share(c.defence.regen, "defence.regen");
    share(c.capture.happiness_loss, "capture.happiness_loss");
    share(c.capture.stock_loss, "capture.stock_loss");
    share(c.capture.pop_loss, "capture.pop_loss");
    share(c.space.engage_p, "space.engage_p");
    share(c.space.evade, "space.evade");
    positive(c.risk.window, "risk.window");
    non_negative(c.risk.presence, "risk.presence");
    non_negative(c.risk.clear, "risk.clear");
    share(c.raid.share, "raid.share");
    positive(static_cast<double>(c.raid.days), "raid.days");
    non_negative(c.raid.standoff, "raid.standoff");
    non_negative(c.escort.rate, "escort.rate");
    non_negative(c.security.risk_post, "security.risk_post");
    positive(static_cast<double>(c.security.days), "security.days");
    non_negative(c.security.pay, "security.pay");

    require(c.gen.width >= 2 && c.gen.width <= 200, "gen.width must be 2 to 200");
    require(c.gen.height >= 2 && c.gen.height <= 200, "gen.height must be 2 to 200");
    require(c.gen.planets >= 2 && c.gen.planets <= 10'000, "gen.planets must be 2 to 10,000");
    require(c.gen.regions >= 1 && c.gen.regions <= c.gen.width * c.gen.height, "gen.regions must be 1 to the number of sectors");
    require(c.gen.seats >= 1, "gen.seats must be at least 1");
    share(c.gen.fringe_share, "gen.fringe_share");
    non_negative(c.gen.lattice_loops, "gen.lattice_loops");
    require(c.gen.min_producers >= 0, "gen.min_producers can't be negative");
    non_negative(c.gen.start_treasury_months, "gen.start_treasury_months");
    require(c.gen.warmup_days >= 0, "gen.warmup_days can't be negative");
    share(c.gen.moon_share, "gen.moon_share");
    require(c.gen.split_planets >= 2, "gen.split_planets must be at least 2");
    positive(c.gen.pop_median, "gen.pop_median");
    non_negative(c.gen.pop_spread, "gen.pop_spread");
    non_negative(c.gen.region_pop_spread, "gen.region_pop_spread");
    positive(c.gen.moon_pop, "gen.moon_pop");
    share(c.gen.start_happiness, "gen.start_happiness");
    require(c.gen.weight_spread >= 0 && c.gen.weight_spread < 1, "gen.weight_spread must be 0 to 1");
    positive(c.gen.favour_mult, "gen.favour_mult");
}

std::string to_toml(const Config& config) {
    toml::table table;
    for_each_field(config, [&](std::string_view key, const auto& field) { insert_path(table, key, field); });
    std::ostringstream out;
    out << table << "\n";
    return out.str();
}

}  // namespace lattice
