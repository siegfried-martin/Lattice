#include <doctest.h>

#include <cstdio>

#include "lattice/save.hpp"

using namespace lattice;

namespace {

World busy_world() {
    Config c;
    c.debug.pings = 50;
    c.time.real_seconds_per_day = 45.5;
    World w = new_world(c, 1234);
    advance(w, 10 * kDay + 3 * kHour);
    return w;
}

}  // namespace

TEST_CASE("a saved world loads back to the same bytes (ARC-9)") {
    World w = busy_world();
    const auto bytes = save_world(w);
    World loaded = load_world(bytes);
    CHECK(save_world(loaded) == bytes);
    CHECK(loaded.config == w.config);
    CHECK(loaded.now == w.now);
    CHECK(loaded.rng == w.rng);
    CHECK(loaded.ping_fires == w.ping_fires);
}

TEST_CASE("a loaded world carries on exactly as the original") {
    World a = busy_world();
    World b = load_world(save_world(a));
    advance(a, 30 * kDay);
    advance(b, 30 * kDay);
    CHECK(state_hash(a) == state_hash(b));
}

TEST_CASE("bad saves are rejected") {
    auto bytes = save_world(busy_world());

    SUBCASE("not a save") {
        std::vector<std::uint8_t> junk{1, 2, 3};
        CHECK_THROWS_WITH_AS(load_world(junk), "not a Lattice save", SaveError);
    }
    SUBCASE("another version") {
        bytes[8] = static_cast<std::uint8_t>(kSaveVersion + 1);
        CHECK_THROWS_WITH_AS(load_world(bytes), doctest::Contains("this build reads version"), SaveError);
    }
    SUBCASE("truncated") {
        bytes.resize(bytes.size() - 5);
        CHECK_THROWS_WITH_AS(load_world(bytes), "save is truncated", SaveError);
    }
    SUBCASE("corrupted") {
        bytes[bytes.size() / 2] ^= 0xff;
        CHECK_THROWS_WITH_AS(load_world(bytes), doctest::Contains("hash mismatch"), SaveError);
    }
}

TEST_CASE("saves round-trip through a file") {
    World w = busy_world();
    const std::string path = "lattice-test-save.bin";
    save_world_file(w, path);
    CHECK(state_hash(load_world_file(path)) == state_hash(w));
    std::remove(path.c_str());
    CHECK_THROWS_AS(load_world_file("/nonexistent/save.bin"), SaveError);
}

TEST_CASE("a save whose indices point nowhere is rejected (ARC-9)") {
    World w = busy_world();
    w.planets[0].group = 100000;
    CHECK_THROWS_WITH_AS(load_world(save_world(w)), doctest::Contains("inconsistent"), SaveError);

    World v = busy_world();
    v.events.push({.time = v.now + kDay, .kind = EventKind::PlanetDay, .record = 100000});
    CHECK_THROWS_WITH_AS(load_world(save_world(v)), doctest::Contains("inconsistent"), SaveError);
}
