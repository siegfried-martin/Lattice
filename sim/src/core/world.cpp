#include "lattice/world.hpp"

#include <cassert>
#include <numeric>

namespace lattice {

namespace {

constexpr Time kPingMin = kHour;
constexpr Time kPingMax = 2 * kDay;

void on_daily_tick(World& world, Observer* observer) {
    if (!observer) return;
    observer->sample(world.now, "world.events_processed", -1, static_cast<double>(world.events_processed));
    observer->sample(world.now, "world.queue_size", -1, static_cast<double>(world.events.size()));
    if (!world.ping_fires.empty()) {
        const auto total = std::accumulate(world.ping_fires.begin(), world.ping_fires.end(), std::int64_t{0});
        observer->sample(world.now, "debug.ping_fires", -1, static_cast<double>(total));
    }
}

void on_debug_ping(World& world, const Event& e) {
    ++world.ping_fires[static_cast<std::size_t>(e.record)];
    Event next = e;
    next.time = world.now + world.rng[Stream::Debug].uniform_int(kPingMin, kPingMax);
    world.events.push(next);
}

void dispatch(World& world, const Event& e, Observer* observer) {
    switch (e.kind) {
        case EventKind::DailyTick:
            on_daily_tick(world, observer);
            break;
        case EventKind::DebugPing:
            on_debug_ping(world, e);
            break;
    }
}

}  // namespace

World new_world(const Config& config, std::uint64_t seed) {
    validate(config);
    World world;
    world.config = config;
    world.seed = seed;
    world.rng = RngStreams::from_seed(seed);

    world.events.push({.time = 0, .phase = Phase::Cadence, .kind = EventKind::DailyTick, .period = kDay});

    world.ping_fires.assign(static_cast<std::size_t>(config.debug.pings), 0);
    for (std::int64_t i = 0; i < config.debug.pings; ++i) {
        world.events.push({.time = world.rng[Stream::Debug].uniform_int(0, kPingMax - 1),
                           .kind = EventKind::DebugPing,
                           .record = i});
    }
    return world;
}

void advance(World& world, Time dt, Observer* observer) {
    assert(dt >= 0);
    const Time end = world.now + dt;
    while (!world.events.empty() && world.events.top().time < end) {
        Event e = world.events.pop();
        assert(e.time >= world.now);
        world.now = e.time;
        dispatch(world, e, observer);
        ++world.events_processed;
        if (e.period > 0) {
            e.time += e.period;
            world.events.push(e);
        }
    }
    world.now = end;
}

}  // namespace lattice
