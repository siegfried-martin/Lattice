#pragma once

#include <utility>

#include "lattice/world.hpp"

// Armies, invasions, sieges, relief and capture (SYS-13, MIL-7 to MIL-12, FAC-8 to FAC-10).
namespace lattice::armies {

// The best invasion the governor of a planet could commission now, for the budget
// (SYS-5): value = P(win) x (the target's income x value.invade_months, more for a gate
// holder, less value.aftermath_months of rebuilding); cost = the stipends of the
// commanders it would use over the campaign. P(win) = A^2 / (A^2 + D^2) from the power
// it can raise and the target's static defence and sworn commanders. Gates are scored
// too (MIL-17): an enemy's open gate for value.blockade_months of the income of the
// planets it serves, or its own faction's blockaded one for value.retake_months.
struct Plan {
    Id target = kNone;
    double value = 0, cost = 0;
    Id node = kNone;  // a gate to blockade or take back, with its holder as target (MIL-17)
};
Plan best_invasion(const World& world, Id planet);

// Raises an army at a planet against a target (MIL-9). Its governor's commanders at home
// join at once; for an invasion its vassals are called (a deed either way) and their
// commanders travel to the muster. A relief army leaves at once (MIL-11).
// With a node, the army's objective is that gate (MIL-17): an enemy's to blockade, or its
// own faction's to take back; target is then the gate's holder.
Id commission(World& world, Id planet, Id target, bool relief, Observer* observer, Id node = kNone);

// An army's muster deadline (token 0), or a garrison's time up (token: its travel_seq + 1).
void muster_deadline(World& world, Id army, std::uint32_t token, Observer* observer);
// A called commander reached the muster point.
void member_arrived(World& world, Id character, Observer* observer);
void on_arrive(World& world, Id army, std::uint32_t seq, Observer* observer);
void battle_over(World& world, Id battle, int winner, Observer* observer);
void siege_round(World& world, Id siege, Observer* observer);
// A call for help reaches a liege: send relief, or pass the call up (MIL-11).
void call_for_help(World& world, Id siege, Id liege, Observer* observer);

void leave(World& world, Id army, Id character);
void disband(World& world, Id army, Observer* observer);

double power(const World& world, const Army& army);
std::pair<double, double> position(const World& world, const Army& army);

}  // namespace lattice::armies
