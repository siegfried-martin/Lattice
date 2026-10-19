#pragma once

#include "lattice/world.hpp"

// A governor's regular decisions and payments (TIM-3).
namespace lattice::governor {

// Schedules every planet's weekly decisions, staggered.
void schedule(World& world);

// Weekly: the liege's tribute tools, the defection check, the budget, and a root's trade
// scouting.
void weekly(World& world, Id planet, Observer* observer);

// Monthly, after operating costs: Authority dues (SYS-20), tribute (FAC-6), stipends
// (MIL-2) and, for a root, merchant salaries (SYS-9).
void monthly(World& world, Id planet, Observer* observer);

// A gate holder on the network pays the higher tier of dues (ECO-10).
bool holds_gates(const World& world, Id planet);

}  // namespace lattice::governor
