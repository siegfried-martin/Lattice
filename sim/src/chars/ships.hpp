#pragma once

#include "lattice/world.hpp"

// Preset ship variants (MOD-1, CHR-12, CHR-13) and what a fleet adds up to.
namespace lattice::ships {

// Builds every culture's catalogue: each class at tiers 1 to 3 (GEN).
void make_catalogue(World& world);

Id variant(const Map& map, Id culture, ShipClass cls, int tier);

// A ship's upkeep per day (CHR-9, SYS-21): maintenance on its price, and its class's
// average crew for its tier at the crew wage. Fuel is charged per Lattice transit.
double upkeep(const Config& config, const ShipVariant& v);

struct Fleet {
    double combat = 0;  // CHR-12
    double cargo = 0;
    double speed = 0;   // the slowest ship's, in open space
    double run_cost = 0;  // upkeep per day (SYS-21)
    double value = 0;   // what the ships cost new
    int count = 0;
    bool lattice_drive = false;  // every ship has one
};
Fleet fleet(const World& world, const Character& c);

}  // namespace lattice::ships
