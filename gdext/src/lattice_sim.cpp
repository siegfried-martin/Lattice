#include "lattice_sim.hpp"

#include <cmath>

#include <string>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "lattice/market.hpp"
#include "lattice/config.hpp"

namespace godot {

bool LatticeSim::start(int64_t seed, const String& config_toml) {
    try {
        const CharString text = config_toml.utf8();
        lattice::Config config = lattice::parse_config(text.get_data(), "config");
        world_ = std::make_unique<lattice::World>(lattice::new_world(config, static_cast<uint64_t>(seed)));
        carry_ = 0.0;
        return true;
    } catch (const std::exception& e) {
        UtilityFunctions::push_error(String("LatticeSim.start: ") + e.what());
        return false;
    }
}

void LatticeSim::advance(double real_seconds) {
    if (!require_world("advance") || !(real_seconds > 0)) return;
    carry_ += real_seconds * static_cast<double>(lattice::kDay) / world_->config.time.real_seconds_per_day;
    const double whole = std::floor(carry_);
    carry_ -= whole;
    lattice::advance(*world_, static_cast<lattice::Time>(whole));
}

void LatticeSim::advance_game_seconds(int64_t game_seconds) {
    if (!require_world("advance_game_seconds") || game_seconds <= 0) return;
    lattice::advance(*world_, game_seconds);
}

Dictionary LatticeSim::snapshot() const {
    Dictionary d;
    if (!world_) return d;
    d["time"] = world_->now;
    d["day"] = lattice::day_of(world_->now);
    d["seed"] = static_cast<int64_t>(world_->seed);
    d["events_processed"] = static_cast<int64_t>(world_->events_processed);
    d["queue_size"] = static_cast<int64_t>(world_->events.size());
    double population = 0, money = 0;
    for (const lattice::Planet& p : world_->planets) population += p.population, money += p.treasury;
    d["planets"] = static_cast<int64_t>(world_->planets.size());
    d["population"] = population;
    d["money"] = money;
    return d;
}

Dictionary LatticeSim::planet(int64_t id) const {
    Dictionary d;
    if (!world_ || id < 0 || id >= static_cast<int64_t>(world_->planets.size())) return d;
    const lattice::Planet& p = world_->planets[static_cast<std::size_t>(id)];
    d["sector"] = p.sector;
    d["region"] = p.region;
    d["fringe"] = p.fringe;
    d["position"] = Vector2(static_cast<real_t>(p.x), static_cast<real_t>(p.y));
    d["population"] = p.population;
    d["happiness"] = p.happiness;
    d["treasury"] = p.treasury;
    d["fuel_price"] = p.fuel_price;
    Dictionary prices;
    for (int g = 0; g < lattice::kLawfulGoodCount; ++g) {
        const auto good = static_cast<lattice::Good>(g);
        prices[String(std::string(lattice::good_name(good)).c_str())] = lattice::market::price(world_->config, p, good);
    }
    d["prices"] = prices;
    return d;
}

bool LatticeSim::require_world(const char* method) const {
    if (world_) return true;
    UtilityFunctions::push_error(String("LatticeSim.") + method + ": call start() first");
    return false;
}

void LatticeSim::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start", "seed", "config_toml"), &LatticeSim::start, DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("advance", "real_seconds"), &LatticeSim::advance);
    ClassDB::bind_method(D_METHOD("advance_game_seconds", "game_seconds"), &LatticeSim::advance_game_seconds);
    ClassDB::bind_method(D_METHOD("snapshot"), &LatticeSim::snapshot);
    ClassDB::bind_method(D_METHOD("planet", "id"), &LatticeSim::planet);
}

}  // namespace godot
