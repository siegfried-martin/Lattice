#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "lattice/config.hpp"
#include "lattice/event_queue.hpp"
#include "lattice/model.hpp"
#include "lattice/rng.hpp"
#include "lattice/time.hpp"

namespace lattice {

// Receives what a run reports (ARC-8). The runner writes it to CSV; the binding and the
// tests pass their own or none. Never stored in the world, so saves don't depend on it.
class Observer {
public:
    virtual ~Observer() = default;
    // One value of a tracked series; id is the record (planet, faction) or -1 for totals.
    virtual void sample(Time t, std::string_view series, std::int64_t id, double value) {
        (void)t, (void)series, (void)id, (void)value;
    }
    // One entry in the event log.
    virtual void log(Time t, std::string_view kind, std::int64_t id, std::string_view detail) {
        (void)t, (void)kind, (void)id, (void)detail;
    }
};

// Values computed from the static map, rebuilt after generation and after loading
// rather than saved.
struct Derived {
    // Reference travel time between planets in days (TIM-4), row-major by planet.
    std::vector<double> travel_days;
    std::size_t planet_count = 0;
    double travel(Id a, Id b) const {
        return travel_days[static_cast<std::size_t>(a) * planet_count + static_cast<std::size_t>(b)];
    }

    // Market pairs close enough for osmosis (SYS-6), a < b, with their falloff weight.
    struct Pair {
        Id a, b;
        double days, weight;
    };
    std::vector<Pair> osmosis_pairs;

    std::vector<std::vector<Id>> region_planets;
};

// The whole simulation state. Everything but `derived` goes into saves (ARC-9).
struct World {
    Config config;
    std::uint64_t seed = 0;
    Time now = 0;
    EventQueue events;
    RngStreams rng;
    std::uint64_t events_processed = 0;

    // debug.pings scaffolding: how often each ping record has fired.
    std::vector<std::int64_t> ping_fires;

    Map map;
    std::vector<Planet> planets;
    std::vector<WorldEvent> world_events;  // never removed; ended ones have alive = false
    Ledger ledger;

    Derived derived;
};

// Generates a world (GEN) and schedules its cadences. Doesn't run the warm-up
// (GEN-10); the runner does that.
World new_world(const Config& config, std::uint64_t seed);

// Recomputes world.derived from the map and config.
void rebuild_derived(World& world);

// Runs every event due in [now, now + dt), in (time, phase, seq) order, then sets now to
// now + dt (TIM-2). Splitting an interval into smaller advances gives the same result.
void advance(World& world, Time dt, Observer* observer = nullptr);

}  // namespace lattice
