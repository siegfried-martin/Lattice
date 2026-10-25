#pragma once

#include "lattice/world.hpp"

// Asking for tribute relief (FAC-15, SYS-11): a vassal that passes a fixed need test asks
// its liege by courier for relief.rate; the liege weighs it as a budget option (SYS-5).
namespace lattice::relief {

// The need tests, as anyone can check them: life-support shortfall of at least
// relief.shortfall, monthly net income per thousand below relief.income_per_head, or
// captured within relief.capture_months.
bool in_need(const World& world, Id planet);

// Weekly, for a vassal: ask when it qualifies and has no request out; take a request
// past its deadline as refused (INF-16); end relief once it has passed no test for
// relief.clear_weeks, its rate going back to what it was.
void weekly(World& world, Id planet, Observer* observer);
// The request reaches the liege: grant if the tribute it keeps flowing, valued as
// patronage, covers the tribute given up over relief.clear_weeks, and money allows.
void request(World& world, Id vassal, Id liege, Observer* observer);
// The answer reaches the vassal: relief starts, a good deed, or a refusal, a bad one.
void answer(World& world, Id vassal, bool granted, Id liege, Observer* observer);
// Relief is between a vassal and this liege; swearing elsewhere drops it.
void clear(Planet& planet);

}  // namespace lattice::relief
