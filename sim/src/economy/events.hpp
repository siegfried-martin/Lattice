#pragma once

#include "lattice/world.hpp"

// World events that keep prices moving (SYS-4, PRD-11).
namespace lattice::events {

// The daily roll: droughts, bumper harvests, festivals and regional droughts start
// by chance, each scheduling its own end. A planet already under an event of the same
// kind (on the same good) isn't given another.
void roll(World& world, Observer* observer);

// Ends an event and undoes its effect.
void end(World& world, Id event, Observer* observer);

// Undoes (sign -1) or reapplies (+1) the tier shifts (production cost, PRD-7) of the events covering a
// planet, around a change to what it makes (turning pirate, recapture), so an event
// that ends later undoes its shift on the right slot.
void reshift(World& world, Id planet, int sign);

}  // namespace lattice::events
