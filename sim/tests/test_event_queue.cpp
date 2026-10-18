#include <doctest.h>

#include <algorithm>
#include <tuple>
#include <vector>

#include "lattice/event_queue.hpp"
#include "lattice/rng.hpp"

using namespace lattice;

TEST_CASE("events pop by time, then phase, then sequence (TIM-2, MOD-11)") {
    EventQueue q;
    q.push({.time = 10, .phase = Phase::Kinematic, .record = 0});
    q.push({.time = 10, .phase = Phase::Scheduled, .record = 1});
    q.push({.time = 5, .phase = Phase::Kinematic, .record = 2});
    q.push({.time = 10, .phase = Phase::Cadence, .record = 3});
    q.push({.time = 10, .phase = Phase::Scheduled, .record = 4});

    std::vector<std::int64_t> order;
    while (!q.empty()) order.push_back(q.pop().record);
    CHECK(order == std::vector<std::int64_t>{2, 1, 4, 3, 0});
}

TEST_CASE("sequence numbers increase from zero") {
    EventQueue q;
    CHECK(q.push({}) == 0);
    CHECK(q.push({}) == 1);
    CHECK(q.next_seq() == 2);
}

TEST_CASE("many random events come out sorted") {
    Rng rng(7);
    EventQueue q;
    for (int i = 0; i < 5000; ++i) {
        q.push({.time = rng.uniform_int(0, 100), .phase = static_cast<Phase>(rng.uniform_int(0, 2))});
    }
    Event prev = q.pop();
    while (!q.empty()) {
        Event e = q.pop();
        CHECK(std::tie(prev.time, prev.phase, prev.seq) < std::tie(e.time, e.phase, e.seq));
        prev = e;
    }
}

TEST_CASE("restoring a saved heap keeps its layout and order") {
    Rng rng(3);
    EventQueue q;
    for (int i = 0; i < 200; ++i) q.push({.time = rng.uniform_int(0, 50)});
    EventQueue copy;
    copy.restore(q.raw(), q.next_seq());
    CHECK(copy.raw() == q.raw());
    CHECK(copy.next_seq() == q.next_seq());
    while (!q.empty()) CHECK(q.pop() == copy.pop());
}

TEST_CASE("restoring an unordered list rebuilds the heap") {
    EventQueue q;
    q.restore({{.time = 9, .seq = 0}, {.time = 1, .seq = 1}, {.time = 5, .seq = 2}}, 3);
    CHECK(q.pop().time == 1);
    CHECK(q.pop().time == 5);
    CHECK(q.pop().time == 9);
}
