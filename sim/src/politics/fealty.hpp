#pragma once

#include "lattice/world.hpp"

// Fealty trees (FAC-2). A faction is a root and everything under it; it isn't stored.
// Every planet has its own governor (FAC-1), so fealty is kept per planet: the planet
// whose governor this planet's governor is sworn to.
namespace lattice::fealty {

Id root_of(const World& world, Id planet);
bool same_faction(const World& world, Id a, Id b);
std::vector<Id> faction_planets(const World& world, Id root);
std::vector<Id> vassals(const World& world, Id planet);

// GEN-8: grows a tree from each seat of power over the Lattice, each planet sworn to a
// nearby planet one step closer to its seat, then frees gen.independent_share of the
// planets with no vassals as their own roots. All factions start at peace.
void make_trees(World& world);

}  // namespace lattice::fealty
