#pragma once

#include "lattice/world.hpp"

// The governor's budget (SYS-5, ECO-8) and commander hiring (SYS-9, MIL-2, MIL-6).
namespace lattice::budget {

// One weekly decision for the governor of a planet (proposed):
//  - available = treasury + budget.credit_risk x monthly income x risk tolerance;
//  - options are units with a monthly cost and a believed monthly value: commanders
//    (kept or hired) and patronage to vassals in the red;
//  - units are funded best value per cost first, each further unit of one option worth
//    d times the last, d = budget.diminishing ^ (1 - risk / 2), while value covers cost
//    and money lasts;
//  - in the red, every cost is x (1 + budget.debt_pressure x months in the red).
// Commanders not funded are let go; funded candidates get an offer.
void weekly(World& world, Id planet, Observer* observer);

// The stipend a commander asks: hire.stipend_per_power x combat power, per month.
double stipend(const World& world, const Character& c);

// Monthly, at the governor's payment: stipends to its sworn commanders (MIL-2).
void pay_stipends(World& world, Id planet, Observer* observer);

}  // namespace lattice::budget
