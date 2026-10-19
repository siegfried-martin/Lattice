#pragma once

#include "lattice/world.hpp"

// Osmosis (SYS-6, TRD-13): a slow trickle of goods between nearby markets, run as real
// trades by nobody in particular.
namespace lattice::osmosis {

// The market pairs within osmosis.range_days, with their falloff weights.
std::vector<Derived::Pair> pairs(const World& world);

// One day's flows. Returns the goods moved.
double run(World& world);

}  // namespace lattice::osmosis
