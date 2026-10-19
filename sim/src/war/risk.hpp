#pragma once

#include "lattice/world.hpp"

// The risk number (SYS-19, TRD-16): one value per sector, the truth that characters'
// beliefs approximate (INF-6).
namespace lattice::risk {

// The sector a position falls in (nearest centre), clamped to the map.
Id sector_at(const Map& map, double x, double y);

// Daily: risk += (raids + risk.presence x hostile ship-days - risk.clear x patrol days -
// risk) / risk.window, floored at 0.
void daily(World& world);

void add_raid(World& world, double x, double y);
void add_hostile(World& world, double x, double y, double days);
void add_patrol(World& world, double x, double y, double days);

// The open-space sectors on the reference route between two planets (the Lattice is
// safe, WLD-2), and what a character believes their risk adds up to.
std::vector<Id> route_sectors(const World& world, Id a, Id b);
double believed_route_risk(const World& world, const Facts& facts, Id a, Id b);

}  // namespace lattice::risk
