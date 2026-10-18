#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "lattice/fields.hpp"
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
    PlanetDay = 2,     // a planet's daily economy, staggered over the day (SYS-1 to SYS-3)
    PlanetLabour = 3,  // a planet's weekly labour shift (SYS-2)
    PlanetPayment = 4, // a planet's monthly operating costs (SYS-5)
    WorldEventEnd = 5, // a world event ends (SYS-4); record is the event
    CharDecide = 6,    // a character decides what to do (TIM-3); a = its wake sequence
    CharArrive = 7,    // a character's leg ends; a = its travel sequence
    KinematicStep = 8, // the open-space step (TIM-6)
    CourierDue = 9,    // a liege's courier run to this vassal planet is due (SYS-15)
    CourierArrive = 10,  // an automated courier run arrives; record is the run
    MissionExpire = 11,  // record is the mission
    GovernorWeekly = 12,  // a governor's weekly decisions (SYS-5, SYS-11, SYS-12); record is the planet
    ArmyMuster = 13,   // an army's muster deadline (SYS-13); record is the army
    ArmyArrive = 14,   // an army's leg ends; a = its travel sequence
    BattleRound = 15,  // hourly (SYS-14); record is the battle
    SiegeRound = 16,   // daily; record is the siege
    CallForHelp = 17,  // a call for help reaches a liege; record is the siege, a the liege's planet
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

template <class Ar, record_of<Event> E>
void fields(Ar& ar, E& e) {
    ar(e.time, e.phase, e.seq, e.kind, e.record, e.period, e.a, e.b);
}

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
