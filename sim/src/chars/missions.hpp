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
