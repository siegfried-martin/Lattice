#include "lattice/world.hpp"

#include <cassert>
#include <cmath>
#include <numeric>
#include <string>

#include "core/travel.hpp"
#include "economy/economy.hpp"
#include "economy/events.hpp"
#include "lattice/market.hpp"
#include "gen/worldgen.hpp"
#include "trade/osmosis.hpp"

namespace lattice {

namespace {

constexpr Time kPingMin = kHour;
constexpr Time kPingMax = 2 * kDay;

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

// The day's metrics (ARC-8), sampled at the start of each day, before anything runs.
void sample_metrics(const World& w, Observer& o) {
    const Config& c = w.config;
    const Time t = w.now;
    o.sample(t, "world.events_processed", -1, static_cast<double>(w.events_processed));
    o.sample(t, "world.queue_size", -1, static_cast<double>(w.events.size()));
    if (!w.ping_fires.empty()) {
        const auto total = std::accumulate(w.ping_fires.begin(), w.ping_fires.end(), std::int64_t{0});
        o.sample(t, "debug.ping_fires", -1, static_cast<double>(total));
    }
    if (w.planets.empty()) return;

    static const auto price_series = [] {
        std::array<std::string, kGoodCount> s;
        for (int g = 0; g < kGoodCount; ++g) s[at(g)] = "planet.price." + std::string(good_name(static_cast<Good>(g)));
        return s;
    }();
    static const auto labour_series = [] {
        std::array<std::string, kGoodCount> s;
        for (int g = 0; g < kGoodCount; ++g) s[at(g)] = "planet.labour." + std::string(good_name(static_cast<Good>(g)));
        return s;
    }();

    double population = 0, money = 0, index = 0;
    std::array<double, kGoodCount> stock{}, price_sum{}, price_sq{};
    for (Id id = 0; id < static_cast<Id>(w.planets.size()); ++id) {
        const Planet& p = w.planets[at(id)];
        o.sample(t, "planet.population", id, p.population);
        o.sample(t, "planet.happiness", id, p.happiness);
        o.sample(t, "planet.productivity", id, p.productivity);
        o.sample(t, "planet.treasury", id, p.treasury);
        for (int gi = 0; gi < kLawfulGoodCount; ++gi) {
            const auto g = static_cast<Good>(gi);
            const double price = market::price(c, p, g);
            o.sample(t, price_series[g], id, price);
            stock[g] += p.stock[g];
            price_sum[g] += price;
            price_sq[g] += price * price;
            index += price / c.goods[g].base_price;
        }
        for (int s = 0; s < 3; ++s) o.sample(t, labour_series[p.makes[at(s)]], id, p.labour[at(s)]);
        population += p.population;
        money += p.treasury;
    }
    const double n = static_cast<double>(w.planets.size());
    o.sample(t, "world.population", -1, population);
    o.sample(t, "world.money", -1, money);
    o.sample(t, "world.money_created", -1, w.ledger.money_created);
    o.sample(t, "world.money_sunk", -1, w.ledger.money_sunk);
    o.sample(t, "world.osmosis_volume", -1, w.ledger.osmosis_volume);
    o.sample(t, "world.osmosis_margin", -1, w.ledger.osmosis_margin);
    o.sample(t, "world.price_index", -1, index / (n * kLawfulGoodCount));
    std::int64_t active = 0;
    for (const WorldEvent& e : w.world_events) active += e.alive;
    o.sample(t, "world.events_active", -1, static_cast<double>(active));
    for (int g = 0; g < kLawfulGoodCount; ++g) {
        const double mean = price_sum[at(g)] / n;
        const double var = std::max(0.0, price_sq[at(g)] / n - mean * mean);
        o.sample(t, "good.price_cv", g, mean > 0 ? std::sqrt(var) / mean : 0);  // dispersion (CHK-14)
        o.sample(t, "good.stock", g, stock[at(g)]);
        o.sample(t, "good.produced", g, w.ledger.produced[at(g)]);
        o.sample(t, "good.consumed", g, w.ledger.consumed[at(g)]);
    }
}

// The world's daily cadence (TIM-3): metrics, then the event roll (SYS-4), then
// osmosis (SYS-6). Each planet's own day runs from its PlanetDay event.
void on_daily_tick(World& w, Observer* observer) {
    if (observer) sample_metrics(w, *observer);
    events::roll(w, observer);
    osmosis::run(w);
}

void on_debug_ping(World& w, const Event& e) {
    ++w.ping_fires[at(static_cast<Id>(e.record))];
    Event next = e;
    next.time = w.now + w.rng[Stream::Debug].uniform_int(kPingMin, kPingMax);
    w.events.push(next);
}

void dispatch(World& w, const Event& e, Observer* observer) {
    const auto record = static_cast<Id>(e.record);
    switch (e.kind) {
        case EventKind::DailyTick:
            on_daily_tick(w, observer);
            break;
        case EventKind::DebugPing:
            on_debug_ping(w, e);
            break;
        case EventKind::PlanetDay:
            economy::planet_day(w, record);
            break;
        case EventKind::PlanetLabour:
            economy::planet_labour(w, record);
            break;
        case EventKind::PlanetPayment:
            economy::planet_payment(w, record);
            break;
        case EventKind::WorldEventEnd:
            events::end(w, record, observer);
            break;
    }
}

}  // namespace

void rebuild_derived(World& w) {
    Derived& d = w.derived;
    d = Derived{};
    d.planet_count = w.planets.size();
    d.travel_days = travel::planet_times(w);
    d.osmosis_pairs = osmosis::pairs(w);
    d.region_planets.assign(w.map.regions.size(), {});
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) d.region_planets[at(w.planets[at(p)].region)].push_back(p);
}

World new_world(const Config& config, std::uint64_t seed) {
    validate(config);
    World world;
    world.config = config;
    world.seed = seed;
    world.rng = RngStreams::from_seed(seed);

    gen::generate(world);
    economy::init_state(world);
    rebuild_derived(world);

    world.events.push({.time = 0, .phase = Phase::Cadence, .kind = EventKind::DailyTick, .period = kDay});
    economy::schedule(world);

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
