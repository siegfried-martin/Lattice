#include <doctest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "lattice/save.hpp"
#include "lattice/world.hpp"

using namespace lattice;

namespace {

struct Recorder : Observer {
    std::vector<std::pair<Time, std::string>> samples;
    void sample(Time t, std::string_view series, std::int64_t, double) override {
        samples.emplace_back(t, std::string(series));
    }
};

Config with_pings(std::int64_t n) {
    Config c;
    c.debug.pings = n;
    return c;
}

}  // namespace

TEST_CASE("advance runs events in [now, now + dt) (TIM-2)") {
    World w = new_world(Config{}, 1);
    Recorder r;
    advance(w, kDay, &r);
    CHECK(w.now == kDay);
    REQUIRE(r.samples.size() == 2);  // one daily tick, at time 0
    CHECK(r.samples[0].first == 0);
    advance(w, kDay, &r);
    CHECK(r.samples.size() == 4);
    CHECK(r.samples[2].first == kDay);
    advance(w, 0, &r);
    CHECK(r.samples.size() == 4);
}

TEST_CASE("a year has 365 daily ticks") {
    World w = new_world(Config{}, 1);
    advance(w, 365 * kDay);
    CHECK(w.events_processed == 365);
    CHECK(w.events.size() == 1);
    CHECK(w.events.top().time == 365 * kDay);
}

TEST_CASE("the same seed and config give the same world (ARC-5)") {
    World a = new_world(with_pings(100), 77);
    World b = new_world(with_pings(100), 77);
    advance(a, 60 * kDay);
    advance(b, 60 * kDay);
    CHECK(state_hash(a) == state_hash(b));

    World c = new_world(with_pings(100), 78);
    advance(c, 60 * kDay);
    CHECK(state_hash(a) != state_hash(c));
}

TEST_CASE("splitting an advance into pieces changes nothing") {
    World whole = new_world(with_pings(40), 5);
    advance(whole, 20 * kDay);

    World pieces = new_world(with_pings(40), 5);
    Rng steps(99);
    while (pieces.now < 20 * kDay) {
        advance(pieces, std::min<Time>(steps.uniform_int(0, 7 * kHour), 20 * kDay - pieces.now));
    }
    CHECK(state_hash(whole) == state_hash(pieces));
}

TEST_CASE("debug pings fire at random intervals of one hour to two days") {
    World w = new_world(with_pings(10), 3);
    advance(w, 100 * kDay);
    for (std::int64_t fires : w.ping_fires) {
        CHECK(fires >= 50);   // at most 2 days apart
        CHECK(fires <= 2400);  // at least an hour apart
    }
}

TEST_CASE("stagger spreads records across a period (TIM-3)") {
    CHECK(stagger(7 * kDay, 0, 7) == 0);
    CHECK(stagger(7 * kDay, 3, 7) == 3 * kDay);
    CHECK(stagger(7 * kDay, 10, 7) == 3 * kDay);
    CHECK(stagger(kDay, 1, 0) == 0);
}
