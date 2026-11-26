#pragma once

#include "lattice/world.hpp"

// Preset ship variants (MOD-1, CHR-12, CHR-13) and what a fleet adds up to.
namespace lattice::ships {

// Builds every culture's catalogue: each class at tiers 1 to 3 (GEN), with a Lattice
// Drive and without (TRD-18).
void make_catalogue(World& world);

Id variant(const Map& map, Id culture, ShipClass cls, int tier, bool drive = true);

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
    bool lattice_drive = false;  // every ship has one (TRD-18)
};
Fleet fleet(const World& world, const Character& c);

// Insurance in the price (CHR-21). What a buyer pays for a new ship: its price and the
// premium, ship.insurance_premium of it.
double buy_cost(const Config& config, const ShipVariant& v);
// The cover a ship adds: ship.insurance_payout of its price. A character's cover is one
// sum, not per hull, so payouts never exceed the cover bought.
double cover_of(const Config& config, const ShipVariant& v);
// A ship joined the fleet insured: bought, or seeded at world generation.
void insure(const World& world, Character& c, Id variant);
// A ship left the fleet unlost (sold, replaced): its cover goes with it, no refund.
void lapse(const World& world, Character& c, Id variant);
// A ship was destroyed: pays out its cover, up to what the character holds, as free money
// (MSN-5). Returns the payout.
double claim(World& world, Character& c, Id variant);

}  // namespace lattice::ships
