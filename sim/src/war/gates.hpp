#pragma once

#include "lattice/world.hpp"

// Gate attacks and blockades (MIL-17). A gate is a group's interchange; its state is
// discrete: open (perhaps under attack), or blockaded by a captor's faction.
namespace lattice::gates {

// The planet holding an interchange's gates (WLD-5), or none for a junction.
Id holder(const World& world, Id node);
// The gate a planet's group gets on and off at, if this planet holds it.
Id held_by(const World& world, Id planet);

// The faction (root) that holds a blockade, or none if the gate is open.
Id captor_faction(const World& world, Id node);
// Whether a blockade shuts the gate to a faction: it does to all but the captor's.
bool shut_to(const World& world, Id node, Id faction);
// As a traveller believes it (INF-2).
bool believed_shut(const World& world, const Facts& facts, Id node, Id faction);

// What a gate's state is now, as a fact; learnt by those who see it.
void observe(const World& world, Facts& facts, Id node);
const GateFact* find(const Facts& facts, Id node);

// The static defence a gate rebuilds toward: blockade.defence_share of what its holder's
// security spending buys (MIL-5). A blockaded gate has none.
double max_defence(const World& world, Id node);
// Daily, from the holder's day: the defence rebuilds unless attacked or blockaded.
void regen(World& world, Id node);

// The monthly income of the planets the gate serves.
double group_income(const World& world, Id node);

// The army took the gate: a blockade, for its faction (MIL-17).
void capture(World& world, Id node, Id army, Observer* observer);
// The holder's faction took it back.
void retake(World& world, Id node, Observer* observer);
// Weekly: a blockade lifts once the two factions are no longer at war, or the holder is
// the captor's own.
void check(World& world, Id node, Observer* observer);

}  // namespace lattice::gates
