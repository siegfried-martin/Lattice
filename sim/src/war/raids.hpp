#pragma once

#include "lattice/world.hpp"

// Raids on the enemy's trade (MIL-15, TRD-7) and the war's other sorties.
namespace lattice::raids {

// The roots a governor believes it is at war with (DIP-5).
std::vector<Id> believed_enemies(const World& world, Id planet);

// Weekly, at war (proposed): each of a governor's commanders at home but one goes raiding
// with chance raid.share, out to raid.standoff short of the nearest enemy planet within
// dip.border_days, holding there raid.days, then home. raid.enabled switches it off.
void dispatch(World& world, Id planet, Observer* observer);

// A raider home again: it sells its loot (MIL-15) and goes back to its post.
void home(World& world, Id character, Observer* observer);

}  // namespace lattice::raids
