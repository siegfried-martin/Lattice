#include "lattice/event_queue.hpp"

#include <algorithm>
#include <tuple>
#include <utility>

namespace lattice {

namespace {

// std heaps put the greatest element first, so "greater" here means "runs later".
bool runs_later(const Event& x, const Event& y) {
    return std::tie(x.time, x.phase, x.seq) > std::tie(y.time, y.phase, y.seq);
}

}  // namespace

std::uint64_t EventQueue::push(Event e) {
    e.seq = next_seq_++;
    heap_.push_back(e);
    std::push_heap(heap_.begin(), heap_.end(), runs_later);
    return e.seq;
}

Event EventQueue::pop() {
    std::pop_heap(heap_.begin(), heap_.end(), runs_later);
    Event e = heap_.back();
    heap_.pop_back();
    return e;
}

void EventQueue::restore(std::vector<Event> events, std::uint64_t next_seq) {
    heap_ = std::move(events);
    // A saved heap is already valid, under any standard library. Keeping its layout
    // means a reloaded world saves to the same bytes.
    if (!std::is_heap(heap_.begin(), heap_.end(), runs_later)) {
        std::make_heap(heap_.begin(), heap_.end(), runs_later);
    }
    next_seq_ = next_seq;
}

}  // namespace lattice
