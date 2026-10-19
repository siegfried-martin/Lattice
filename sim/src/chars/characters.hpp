#pragma once

#include "lattice/world.hpp"

// Characters (CHR, SYS-18): who they are, their daily costs, and the decision they take
// when idle or on arrival (TIM-3).
namespace lattice::chars {

// A new bottom-rung character at a planet (CHR-6, SYS-18): the planet's culture, risk
// tolerance from char.risk_dist (uniform 0-1), char.start_wealth and a first-tier hull,
// armed with chance char.start_armed_p.
Id spawn(World& world, Id planet);
// A governor for a planet (FAC-1, GEN-9). Governors stay at their planet.
Id make_governor(World& world, Id planet);
// The starting characters (GEN-9 as far as MS-3 goes): a governor for every planet, and
// bottom-rung characters for the rest of char.target, at planets weighted by population.
void populate(World& world);

// The faction a character belongs to: the root of the planet it governs, or of the
// employer of a standing contract it holds; none for an independent (INF-4).
Id faction_of(const World& world, const Character& c);

// Daily: running costs (SYS-21), going broke, spawning toward char.target (SYS-18).
void daily(World& world, Observer* observer);

// Schedules the character's next decision.
void wake(World& world, Id character, Time at);
// Decides what to do next, at a planet (TIM-3). seq is the wake it was scheduled by; a
// superseded wake does nothing.
void decide(World& world, Id character, std::uint32_t seq, Observer* observer);
// Arrived at a planet: observe, share, then hand over to whatever it was doing.
void landed(World& world, Id character, Observer* observer);
// Two characters in open space within detection range (TIM-6).
void detected(World& world, Id a, Id b);

// Money in and out of a character, with the running averages that steer spending
// (SYS-8).
void earn(Character& c, double amount, bool combat_work);

// The cash a character keeps back: spend.reserve_months of running costs x (1 - risk).
double reserve(const World& world, const Character& c);

}  // namespace lattice::chars
