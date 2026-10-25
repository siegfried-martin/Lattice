#pragma once

#include <vector>

#include "lattice/world.hpp"

// What war pays a commander (MIL-18, MIL-19): bounties for kills, paid by the root, and
// spoils of capture, split by combat power x days in the siege.
namespace lattice::prizes {

// Each member's share of its side's combat power this round, the static defence counted
// in the total (its share credits nobody). Taken before the round's losses.
std::vector<Owed> shares(const World& world, const std::vector<Id>& side, double static_power);

// A ship worth `price` was destroyed by a side with these shares; the victim fought for
// victim_faction (a root, kPirates or kNone). Each member whose faction is at war with it
// (a pirate is at war with everyone) gets bounty.share x price x its share on its root's
// tally. Pirates and characters with no faction earn none.
void credit(World& world, const std::vector<Owed>& by, Id victim_faction, double price);

// What a root owes in bounties.
double owed(const World& world, Id root);
// Monthly at a root's payment: each tally is paid as far as the treasury goes; what's left
// stays owed and is deed.bounty_unpaid with that commander.
void pay(World& world, Id root, Observer* observer);
// The tallies follow the faction when its root moves (FAC-17, SYS-13); a faction that's
// gone (to = none) leaves them unpaid.
void move(World& world, Id from, Id to);

// A captured planet's spoils: min(treasury, spoils.per_prosperity x population x productivity).
double spoils_of(const World& world, Id planet);
// A day of a siege: each attacker's combat power, as a day served.
void siege_day(World& world, Id siege);
// At capture: the spoils leave the planet's treasury for the army's commanders, by their
// power x days in the siege (all alike on the first day).
void take_spoils(World& world, Id siege, Observer* observer);
// An objective done (a planet or gate taken, a gate taken back): the army's home planet
// pays its purse to the members, split as the spoils are (by power x days in the siege, or
// by power with no siege), whatever its treasury holds (MIL-2).
void pay_purse(World& world, Id army, Id siege, Observer* observer);

}  // namespace lattice::prizes
