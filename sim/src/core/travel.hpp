#pragma once

#include "lattice/world.hpp"

namespace lattice::travel {

// Reference routes between every pair of planets (TIM-4): the fastest mix of open space
// at travel.space_speed, the Lattice at travel.lattice_speed and hop lanes at
// travel.hop_days, ignoring gate permissions. Vertices are planets (0 to P-1) then
// Lattice nodes (P onward).
struct Routes {
    std::vector<double> days;           // P x P
    std::vector<std::int32_t> pred;     // P x V: the vertex before v on the way from s
    std::vector<std::uint8_t> via;      // P x V: how v is reached: a LegKind
};
Routes planet_routes(const World& world);

// The vertices from planet a to planet b along the reference route, a first.
std::vector<std::int32_t> path(const World& world, Id a, Id b);

}  // namespace lattice::travel
