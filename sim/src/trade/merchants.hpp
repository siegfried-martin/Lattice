#pragma once

#include <limits>

#include "lattice/world.hpp"

// Merchants (SYS-7, TRD-3 to TRD-5) and faction merchant salaries (SYS-9).
namespace lattice::merchant {

// A planned trip: stops[0] is where the merchant is now; at each stop it sells what's
// aboard and buys stops[i].buy for the next leg.
struct Plan {
    std::vector<TripStop> stops;
    double value = 0;  // believed profit after fuel and running costs
    double days = 0;
    double score = -std::numeric_limits<double>::infinity();  // value per day
};

// The best trip from the character's planet. As a faction merchant of `root` it uses the
// three shapes (TRD-3); otherwise single trips (TRD-4). Uses the character's own facts,
// and the actual market where it stands.
Plan best_trip(const World& world, const Character& c, Id root);

// Starts a plan: buys for the first leg and sets off.
void start(World& world, Id character, const Plan& plan, Observer* observer);
// At a stop: sells what's aboard, buys for the next leg, moves on or ends the trip.
void at_stop(World& world, Id character, Observer* observer);

// The nearest planet whose prices the character hasn't seen for merchant.stale_days,
// or none (TRD-4: look somewhere else).
Id explore_target(const World& world, const Character& c);

// At a planet of a faction whose root offers merchant salaries: an independent takes one
// when salary + merchant.info_value beats what the shapes cost it; a faction merchant
// quits when that flips (SYS-9).
void consider_salary(World& world, Id character, Observer* observer);
// Monthly, from the root's treasury.
void pay_salaries(World& world, Id root, Observer* observer);

// The salaried merchant's root, or none.
Id employer_root(const World& world, Id character);

// One leg's trade, for those whose trips aren't their own (a spy's cover, INF-14): buy what
// best pays at `next` by belief, and sell what's aboard here.
void buy_for(World& world, Id character, Id next);
void sell_here(World& world, Id character);

}  // namespace lattice::merchant
