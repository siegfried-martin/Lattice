#pragma once

#include "lattice/world.hpp"

// Spies (INF-13, SYS-17). A spy holds a secret contract with a governor: to everyone else
// it reads as an independent trader, with neutral standing and the Lattice open to it.
namespace lattice::spies {

// The governor (character) a spy works for, or none.
Id employer(const World& world, Id character);

// Who a governor could hire (proposed): independents with a hold, idle at planets within
// hire.range_days, not pirates, under no contract.
std::vector<Id> candidates(const World& world, Id planet);
// The spies a governor employs.
std::vector<Id> employed(const World& world, Id governor);
void hire(World& world, Id planet, Id character, Observer* observer);
void release(World& world, Id character);

// At a planet with nothing else to do: from home, out to the enemy market within
// spy.range_days it has seen least recently; from there, home.
void next(World& world, Id character, Observer* observer);
// Arrived. Abroad: it trades for cover, buying for home, and notes any army forming or
// stationed there (a sighting with its intent: spies catch armies forming at home, INF-13).
// Visiting without trading, or where an army is forming, raises suspicion. Home: it sells,
// and shares what it learned with its governor, which may raise the alarm.
void landed(World& world, Id character, Observer* observer);
// Its cover blown by an inspection: the contract is revealed and ends, the governor
// there thinks worse of it, and the planet's commanders at home attack it.
void caught(World& world, Id character, Id planet, Observer* observer);

// Monthly, at the governor's payment.
void pay(World& world, Id planet, Observer* observer);

}  // namespace lattice::spies
