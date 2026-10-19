#pragma once

#include "lattice/world.hpp"

// Suspicion and inspection (INF-14, SYS-17, PIR-14, PIR-15). One meter per character per
// faction (and the Authority), raised by behaviour, lowered by genuine trade, fading
// weekly by sus.fade toward 0.
namespace lattice::suspicion {

double get(const World& world, const Character& c, Id faction);
void add(World& world, Character& c, Id faction, double amount);

// How hard a planet looks (PIR-15): 1 + sus.police_per_unit per value.security_unit of
// monthly policing, + sus.war when its faction is at war.
double intensity(const World& world, Id planet);

// Landing at a lawful planet (INF-14):
//   P(inspect) = sus.inspect_base x intensity
//   P(caught)  = P(inspect) x min(1, sus.find_base + (suspicion + sus.pirate x pirate relation
//                above neutral) / sus.scale)
// Contraband aboard is what an inspection can find; caught, it's confiscated, the governor
// thinks worse of the smuggler (deed `caught`) and suspicion jumps. Not caught, carrying
// contraband or a good pirate relation adds suspicion. Returns whether it was caught.
bool on_landing(World& world, Id character, Observer* observer);

}  // namespace lattice::suspicion
