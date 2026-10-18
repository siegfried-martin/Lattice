#pragma once

#include "lattice/world.hpp"

namespace lattice::travel {

// Reference travel times between every pair of planets (TIM-4): the fastest mix of
// open space at travel.space_speed, the Lattice at travel.lattice_speed and hop lanes
// at travel.hop_days, ignoring gate permissions. Row-major, planets x planets.
std::vector<double> planet_times(const World& world);

}  // namespace lattice::travel
