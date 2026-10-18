#pragma once

#include "lattice/world.hpp"

// Pirate missions and smugglers (SYS-16, PIR-3, PIR-6, PIR-10, PIR-12, PIR-13).
namespace lattice::smuggling {

// Weekly, at a pirate planet: a legal-goods run for each life-support good whose fill is
// below pirate.need_fill (to bring it back to its reserve), and a smuggling run when its
// contraband fill passes pirate.surplus_fill (the surplus).
void post(World& world, Id planet, Observer* observer);

// On contact in open space (PIR-13): a pirate offers its home's best open run to an
// independent with risk tolerance above pirate.smuggler_risk. Accepting sets the pirate
// relation to pirel.accept, so the pirate doesn't attack, and the run is taken when the
// character is next free. Returns whether it was offered.
bool offer(World& world, Id pirate, Id character, Observer* observer);

// Who can run for pirates: lawful (not full pirates), empty-holded, with cargo space, and
// bold enough or already on good terms.
bool qualifies(const World& world, Id character, const Mission& mission);

// Taking a run. A smuggling run goes to the pirate planet, buys its contraband and sells
// it to a lawful population; a legal-goods run buys the good in lawful space and sells it
// to the pirate market, with pirate.legal_bonus a unit on top.
void take(World& world, Id character, Id mission, Observer* observer);
// The character landed while on a run.
void landed(World& world, Id character, Observer* observer);

// Selling contraband to a lawful population (SYS-16): untaxed, from its budget, at
// pirate.contraband_price x base, as much as it wants today: N x consumption x (1 - H),
// up to pirate.demand_cap per thousand.
double sell_to_population(World& world, Id character);

}  // namespace lattice::smuggling
