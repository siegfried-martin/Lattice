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

// Monthly, at the vassal's payment (FAC-6): tribute_rate x (its month's income, plus the
// tribute it received if fealty.cascade) to its liege.
void pay_tribute(World& world, Id planet, Observer* observer);

// Weekly, the liege's tools (FAC-12) *(proposed)*: a liege in the red raises tribute on
// vassals loyal enough to bear it (fealty.raise_opinion); a wavering vassal (opinion
// below half fealty.defect_opinion) gets its tribute lowered. Each is a deed.
void adjust_tribute(World& world, Id liege, Observer* observer);

// Weekly (FAC-11, SYS-11): a governor whose opinion of its liege has been below
// fealty.defect_opinion for fealty.defect_weeks defects, to a neighbouring root that
// thinks well of it (above fealty.rival_opinion) or to independence.
void check_defection(World& world, Id planet, Observer* observer);
// Swears the planet to a new liege, or none; its vassals go with it. Swearing into its own
// branch would make a cycle, and does nothing.
void swear(World& world, Id planet, Id liege);

}  // namespace lattice::fealty
