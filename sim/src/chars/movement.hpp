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

// The legs a traveller of a faction (or none) takes from a to b: the reference route,
// unless it would enter or leave the Lattice at a gate held by a faction at war with
// the traveller's, which both keys shut to it (WLD-12, DIP-6); then a route that
// gets on and off elsewhere and crosses the rest in open space (WLD-13).
std::vector<Leg> plan(const World& world, Id faction, Id a, Id b);

// How long a leg takes a fleet with this open-space speed.
Time leg_time(const World& world, const Leg& leg, double speed);

// A sortie: out toward a planet without landing, to a point `standoff` sector widths short
// of it on the way in, held there for `hold`, then home (raids, war scouting, patrols).
// Empty if there's no way.
std::vector<Leg> sortie(const World& world, Id faction, Id home, Id target, double standoff, Time hold);

// Sets a character, which must be at a planet, off on a prepared route.
bool start_route(World& world, Id character, std::vector<Leg> route);

// A battle stopped a character mid-leg; it picks up where it left off (TIM-6).
void pause(World& world, Id character);
void resume(World& world, Id character);

// Estimates along the reference route for this character's fleet.
double trip_days(const World& world, const Character& c, Id from, Id to);
double fuel_cost(const World& world, const Character& c, Id from, Id to);

// A leg ended: start the next, or land.
void on_arrive(World& world, Id character, std::uint32_t seq, Observer* observer);

// Where a character is now, in sector widths.
std::pair<double, double> position(const World& world, const Character& c);

}  // namespace lattice::move
