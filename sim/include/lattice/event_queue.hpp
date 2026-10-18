#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "lattice/time.hpp"

namespace lattice {

// What runs first among events at the same time (TIM-2): scheduled events, then
// cadence decisions, then the open-space step.
enum class Phase : std::uint8_t {
    Scheduled = 0,
    Cadence = 1,
    Kinematic = 2,
};

// Append only: the values are stored in saves.
enum class EventKind : std::uint16_t {
    DailyTick = 0,  // the daily economy cadence (TIM-3)
    DebugPing = 1,  // scaffolding that exercises the queue and RNG (debug.pings)
};

// One entry in the scheduled-event queue (MOD-11).
struct Event {
    Time time = 0;
    Phase phase = Phase::Scheduled;
    std::uint64_t seq = 0;  // breaks ties, so the order is total (ARC-6)
    EventKind kind = EventKind::DailyTick;
    std::int64_t record = -1;  // the record it concerns, or -1
    Time period = 0;           // above 0: rescheduled this far ahead after it runs
    std::int64_t a = 0;        // payload, meaning set by the kind
    std::int64_t b = 0;

    friend bool operator==(const Event&, const Event&) = default;
};

// A min-heap on (time, phase, seq). Every key is unique, so the pop order is fully
// determined by what was pushed, whatever the heap layout.
class EventQueue {
public:
    // Assigns the next sequence number and returns it.
    std::uint64_t push(Event e);
    Event pop();
    const Event& top() const { return heap_.front(); }
    bool empty() const { return heap_.empty(); }
    std::size_t size() const { return heap_.size(); }

    // For saves (ARC-9).
    const std::vector<Event>& raw() const { return heap_; }
    std::uint64_t next_seq() const { return next_seq_; }
    void restore(std::vector<Event> events, std::uint64_t next_seq);

private:
    std::vector<Event> heap_;
    std::uint64_t next_seq_ = 0;
};

}  // namespace lattice
