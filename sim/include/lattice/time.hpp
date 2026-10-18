#pragma once

#include <cstdint>

namespace lattice {

// Simulation time is an integer count of game seconds (TIM-1), so comparisons are exact.
using Time = std::int64_t;

inline constexpr Time kSecond = 1;
inline constexpr Time kMinute = 60 * kSecond;
inline constexpr Time kHour = 60 * kMinute;
inline constexpr Time kDay = 24 * kHour;

// The day a time falls in, counting from day 0 at time 0.
constexpr std::int64_t day_of(Time t) { return t / kDay; }

// Offset of the index-th of count records within one period, so recurring decisions
// spread across the period instead of landing on the same tick (TIM-3).
constexpr Time stagger(Time period, std::int64_t index, std::int64_t count) {
    return count <= 0 ? 0 : period * (index % count) / count;
}

}  // namespace lattice
