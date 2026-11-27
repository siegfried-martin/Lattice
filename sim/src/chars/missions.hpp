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

// Weekly, per lawful planet (MSN-13): keeps passenger.per_planet passenger runs posted, one
// way to a random lawful planet within passenger.max_days.
void post_passengers(World& world, Id planet, Observer* observer);

// An errand (MSN-14): one passenger run posted at a lawful planet not under siege for a
// bottom-rung character that has waited there with nothing to do: in open space, or by
// the Lattice if open space doesn't get out and its hull has a drive, the fare then
// covering the fuel, or else to the nearest lawful planet it can reach. kNone under siege.
Id post_errand(World& world, Id planet, Id flier, Observer* observer);

// Weekly, per planet (proposed): a border planet at war posts a scout mission toward the
// nearest enemy planet, flown as a sortie (INF-9); a planet whose sector's risk passes
// security.risk_post posts a patrol over its own space (MIL-5).
void planet_weekly(World& world, Id planet, Observer* observer);

// A Trade Authority clearing contract at a planet (FAC-14): a patrol paid by the
// Authority. False if one is already open there.
bool post_clearing(World& world, Id planet, Observer* observer);

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
// An open mission lapses now, off the board, its order given back; an order's goods
// don't follow. Its expiry event later finds it closed. A planet turning pirate does it.
void lapse(World& world, Id mission, Observer* observer);

// Supply orders (MSN-15). Daily, per lawful planet that can buy (ECO-8): for each
// life-support good whose stock plus what's on order is below order.trigger_fill of
// reserve, an order for the shortfall to the nearest planet it has heard has a surplus,
// at order.price_share of its price now, fixed, posted as a one-way courier mission there paid order.courier_pay a
// day. Untaken order.courier_wait_days past the trip's own days, the order arrives anyway.
// At the producer the delivery is posted, for merchants to weigh as a trip.
void post_orders(World& world, Id planet, Observer* observer);
// A merchant takes qty of a posted delivery; the rest stays posted. kNone if it's gone.
Id take_supply(World& world, Id character, Id mission, double qty, Observer* observer);
// What it could buy at the order's price; the rest goes back on the issuer's books, and
// nothing at all gives the delivery back.
void supply_bought(World& world, Id character, double qty);
// Delivered: qty units for money. The issuer's opinion and the trade skill grow.
void supply_done(World& world, Id character, double qty, double money, Observer* observer);
// Who issued a mission, for the ledger (MSN-4).
Issuer issuer_of(const World& world, const Mission& m);
// A mission was just taken: its deadline is mission.deadline_mult x its reference days
// (MSN-3), and the issuer's ledger counts it.
void start_clock(World& world, Id mission);
// The taker failed (MSN-3): the deadline passed, it lost the fleet or the cargo, or it
// walked off. A bad deed with the issuer (deed.mission_failed; the pirate relation for a
// pirate run, PIR-10), no pay, reputation unchanged, and the one-job contract ends.
void fail(World& world, Id mission, Observer* observer);
// A taken mission's deadline (MissionDeadline).
void deadline(World& world, Id mission, Observer* observer);

}  // namespace lattice::missions
