#include <doctest.h>

#include "lattice/rng.hpp"

using namespace lattice;

TEST_CASE("the same seed gives the same streams; different seeds differ") {
    auto a = RngStreams::from_seed(42);
    auto b = RngStreams::from_seed(42);
    auto c = RngStreams::from_seed(43);
    CHECK(a == b);
    CHECK(a[Stream::Debug].next() == b[Stream::Debug].next());
    CHECK(a[Stream::Debug].next() != c[Stream::Debug].next());
}

TEST_CASE("streams are independent (ARC-6)") {
    auto a = RngStreams::from_seed(5);
    auto b = RngStreams::from_seed(5);
    CHECK(a[Stream::WorldGen] != a[Stream::Debug]);
    for (int i = 0; i < 100; ++i) a[Stream::WorldGen].next();  // extra draws in one stream
    CHECK(a[Stream::Debug].next() == b[Stream::Debug].next());
}

TEST_CASE("uniform01 stays in [0, 1) and uniform_int in its bounds") {
    Rng rng(1);
    bool saw_lo = false, saw_hi = false;
    for (int i = 0; i < 20000; ++i) {
        const double u = rng.uniform01();
        CHECK(u >= 0.0);
        CHECK(u < 1.0);
        const auto n = rng.uniform_int(-3, 3);
        CHECK(n >= -3);
        CHECK(n <= 3);
        saw_lo |= n == -3;
        saw_hi |= n == 3;
    }
    CHECK(saw_lo);
    CHECK(saw_hi);
    CHECK(rng.uniform_int(4, 4) == 4);
    rng.uniform_int(INT64_MIN, INT64_MAX);  // full range must not overflow
}

TEST_CASE("draws are fixed for a seed, on every platform") {
    // The engine and these distributions are fully specified, so these values must not
    // change. If one does, every saved world and recorded run changes with it.
    Rng rng(42);
    CHECK(rng.next() == 13930160852258120406ULL);
    Rng rng2(42);
    CHECK(rng2.uniform01() == 0.755155532954539);
}

TEST_CASE("state round-trips through text") {
    Rng a(9);
    for (int i = 0; i < 10; ++i) a.next();
    Rng b;
    b.set_state(a.state());
    CHECK(a == b);
    CHECK(a.next() == b.next());
    CHECK_THROWS(b.set_state("not a state"));
}
