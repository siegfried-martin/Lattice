#pragma once

#include <memory>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include "lattice/world.hpp"

namespace godot {

// One simulated world, as GDScript sees it (ARC-4). Commands go in, snapshots come
// out, and the simulation never calls the engine.
class LatticeSim : public RefCounted {
    GDCLASS(LatticeSim, RefCounted)

public:
    // Builds a new world from a seed and a TOML config (empty for the defaults).
    // Reports a bad config with push_error and returns false.
    bool start(int64_t seed, const String& config_toml);
    // Advances by real seconds at the rendered clock rate (WLD-15). Fractions of a game
    // second carry over to the next call.
    void advance(double real_seconds);
    // Advances by whole game seconds.
    void advance_game_seconds(int64_t game_seconds);
    // The world's clock, counters and totals.
    Dictionary snapshot() const;
    // One planet's state, or an empty Dictionary for a bad ID.
    Dictionary planet(int64_t id) const;

protected:
    static void _bind_methods();

private:
    bool require_world(const char* method) const;

    std::unique_ptr<lattice::World> world_;
    double carry_ = 0.0;  // game seconds not yet advanced
};

}  // namespace godot
