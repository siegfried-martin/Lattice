#pragma once

#include "lattice/world.hpp"

namespace lattice::gen {

// Fills world.map and world.planets' static fields from the config and the WorldGen
// stream (GEN-1 to GEN-7). Population and growth factor are drawn here too; the
// market and treasury start in economy::init_state (GEN-10).
void generate(World& world);

// Offset-coordinate hex neighbours of a sector, in a fixed order.
std::vector<Id> neighbours(const Map& map, Id sector);

}  // namespace lattice::gen
