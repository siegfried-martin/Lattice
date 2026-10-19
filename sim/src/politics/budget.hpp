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

// What defence is worth at a planet against peace (MIL-5): 1, + value.war_mult at war, +
// value.border_mult with an enemy planet within dip.border_days or an enemy army seen
// coming, all as its governor believes (DIP-5).
double threat(const World& world, Id planet);

// Defence values' multiplier for money beyond the reserve (MIL-5, SYS-5): 1 +
// budget.hoard x clamp(treasury months / budget.reserve_months - 1, 0, budget.hoard_cap).
double hoard(const World& world, Id planet);

// The stipend a commander asks: hire.stipend_per_power x combat power, per month.
double stipend(const World& world, const Character& c);

// Monthly, at the governor's payment: stipends to its sworn commanders (MIL-2).
void pay_stipends(World& world, Id planet, Observer* observer);

}  // namespace lattice::budget
