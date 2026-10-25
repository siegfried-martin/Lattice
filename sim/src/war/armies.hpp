#pragma once

#include <utility>
#include <vector>

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
    double gain = 0;  // what taking it is worth, before the chance of winning: the purse is a share of it (MIL-2)
};
Plan best_invasion(const World& world, Id planet);

// Raises an army at a planet against a target (MIL-9). The governor's own fleet joins;
// its commanders at home choose whether to (MIL-2; all join with objective.choose off).
// For an invasion, each vassal with commanders at their posts is sent a loan request by
// courier (MIL-3), answered by the fleets that arrive by the muster deadline (INF-16). A
// relief army leaves at once with everyone at home (MIL-11), lent to the planet relieved.
// With a node, the army's objective is that gate (MIL-17): an enemy's to blockade, or its
// own faction's to take back; target is then the gate's holder.
// The objective's purse is a share of gain, rising from objective.reward_share while that
// brings more of the commanders at home in, to objective.reward_max; paid on success.
// Nobody joining is an abandoned campaign: none for objective.retry_days.
Id commission(World& world, Id planet, Id target, bool relief, Observer* observer, Id node = kNone, double gain = 0);

// A liege's loan request reaches a vassal (MIL-3): it grants if its governor's opinion of
// the liege is at least 0 or the fee covers its home's lost defence over loan.max_days
// (SYS-13's exposed term), and lends its commanders at their posts, who then choose as
// the liege's own do. Its own fleet comes the same way (MIL-20).
void loan_request(World& world, Id army, Id vassal, Observer* observer);

// What an army's objective is worth to a commander (MIL-2, SYS-9), against staying at its
// employer's planet: join = expected purse, bounties and spoils share - expected losses - fuel;
// home = its stipend x objective.stake_months x how much its being there lowers the chance
// the planet falls to the armies its planet has seen coming.
struct Weighed {
    double join = 0, home = 0;
};
Weighed weigh(const World& world, const Army& army, Id character);

// A character leaves an army it serves in and goes its own way (a loan that's run out).
void dismiss(World& world, Id army, Id character, Observer* observer);

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
// Who's at a planet to fight for it: its governor's fleet and its sworn commanders at
// their post (MIL-20). Marching, the governor comes only if it can sail (a steward stays).
std::vector<Id> ready(const World& world, Id planet, bool marching = false);
std::pair<double, double> position(const World& world, const Army& army);

}  // namespace lattice::armies
