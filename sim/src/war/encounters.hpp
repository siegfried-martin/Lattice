#pragma once

#include "lattice/world.hpp"

// The coarse open-space step (TIM-6): who is where, a spatial hash per cell of
// space.detect_range, and what happens between ships that come within range:
//  - members of one faction share facts (SYS-15);
//  - a raider engages a member of a faction it believes it's at war with, and a patrol
//    engages a raider of such a faction (MIL-15, MIL-5), with chance space.engage_p a
//    step, unless a faster quarry slips away (space.evade x its speed edge);
//  - anyone who comes near an army records a sighting with its intent (INF-3, INF-9).
// Raiders and patrols add to their sector's risk (SYS-19).
namespace lattice::encounters {

void step(World& world, Observer* observer);

}  // namespace lattice::encounters
