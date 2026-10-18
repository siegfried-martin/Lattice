#pragma once

#include "lattice/world.hpp"

// Population, production, labour and treasuries (SYS-1, SYS-2, SYS-5).
namespace lattice::economy {

// Starting state (GEN-10): stocks at their reserve targets, equal labour shares, and
// treasuries holding gen.start_treasury_months of operating costs.
void init_state(World& world);

// Schedules each planet's daily economy, weekly labour shift and monthly payments,
// staggered by planet so they don't all land on one tick (TIM-3).
void schedule(World& world);

// One planet's day: production (SYS-2), then the population buys life support and
// luxuries, grows or shrinks, and its happiness and productivity update (SYS-1).
void planet_day(World& world, Id planet);

// One planet's weekly labour shift toward the good that pays best (SYS-2, PRD-9).
void planet_labour(World& world, Id planet);

// One planet's monthly operating costs (SYS-5, ECO-4): a sink.
void planet_payment(World& world, Id planet);
double monthly_costs(const Config& c, const Planet& p);

}  // namespace lattice::economy
