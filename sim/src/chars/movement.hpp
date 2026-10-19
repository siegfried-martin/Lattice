#pragma once

#include <utility>

#include "lattice/world.hpp"

// Moving characters (TIM-4 to TIM-6): routes of legs over open space, the Lattice and
// hop lanes, Lattice fuel at entry (SYS-21), and the open-space step that finds
// encounters.
namespace lattice::move {

// Starts the character, which must be at a planet, on the reference route to dest
// (TIM-4). Returns false if it's already there or there's no route.
bool travel_to(World& world, Id character, Id dest);

// The legs of the reference route from planet a to planet b.
std::vector<Leg> route_legs(const World& world, Id a, Id b);

// Estimates along the reference route for this character's fleet.
double trip_days(const World& world, const Character& c, Id from, Id to);
double fuel_cost(const World& world, const Character& c, Id from, Id to);

// A leg ended: start the next, or land.
void on_arrive(World& world, Id character, std::uint32_t seq, Observer* observer);

// The coarse open-space step (TIM-6): positions of everyone in open space, a spatial
// hash, and a detection for every pair within space.detect_range.
void kinematic_step(World& world, Observer* observer);

// Where a character is now, in sector widths.
std::pair<double, double> position(const World& world, const Character& c);

}  // namespace lattice::move
