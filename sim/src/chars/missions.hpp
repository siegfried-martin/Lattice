#pragma once

#include <limits>

#include "lattice/world.hpp"

// Missions and careers (SYS-8, CHR-10, CHR-11, INF-10).
namespace lattice::missions {

// A courier run from a liege to a vassal, posted as a mission at the liege's planet
// instead of automated (INF-8). Untaken, it goes automated when it expires.
void post_courier(World& world, Id liege, Id vassal, Observer* observer);

// Weekly, per root: trade scouting of the outside market it has heard least of (INF-9).
void root_weekly(World& world, Id planet, Observer* observer);

// pay = base x (1 + mission.pay_growth x days waiting).
double pay_now(const World& world, const Mission& m);
bool qualifies(const World& world, Id character, const Mission& m);

struct Offer {
    Id mission = kNone;
    double score = -std::numeric_limits<double>::infinity();  // pay per day, skill-weighted, net of costs
};
// The best mission on the board where the character is.
Offer best_offer(const World& world, Id character);

void take(World& world, Id character, Id mission, Observer* observer);
// The character landed while on a mission.
void arrived(World& world, Id character, Observer* observer);
void expire(World& world, Id mission, Observer* observer);

}  // namespace lattice::missions
