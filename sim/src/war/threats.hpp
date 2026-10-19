#pragma once

#include "lattice/world.hpp"

// Seeing an army coming (INF-9, INF-12, MIL-11): a governor that learns from a sighting
// that a hostile army means to take its planet warns its liege at once, by courier; a
// liege that learns its vassal is threatened decides relief itself.
namespace lattice::threats {

// After a governor's facts change.
void check(World& world, Id planet, Observer* observer);

// A warning reaches a liege: relief to garrison the planet before the siege, or the
// warning goes on up the tree.
void warning(World& world, Id army, Id liege, Id planet, Observer* observer);

}  // namespace lattice::threats
