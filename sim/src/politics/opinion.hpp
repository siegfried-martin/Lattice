#pragma once

#include "lattice/world.hpp"

// Opinion and deeds (REL-2, REL-3, SYS-10). One character's opinion of another, in
// [-100, 100], moved only by deeds and drifting back toward a resting point.
namespace lattice::opinion {

// Where an opinion rests: 0, or for a governor's opinion of its liege,
// rel.culture_rest x (share of the faction's planets of its culture - 0.5) (FAC-11, WLD-19).
double rest(const World& world, Id holder, Id about);

// The holder's opinion now. With no history it is the resting point.
double get(const World& world, Id holder, Id about);

// A deed: moves the holder's opinion by deed.<kind> and remembers it.
void deed(World& world, Id holder, Id about, DeedKind kind, Observer* observer = nullptr);

}  // namespace lattice::opinion
