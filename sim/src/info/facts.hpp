#pragma once

#include "lattice/world.hpp"

// Facts, sharing and automated couriers (INF-2 to INF-8, SYS-15).
namespace lattice::info {

// Records what a market's prices and stocks are now (INF-3).
void observe_market(const World& world, Facts& facts, Id planet);

const PriceFact* find(const Facts& facts, Id planet);

// Risk (INF-6): what a character believes about a sector, 0 if it has never looked.
void observe_risk(const World& world, Facts& facts, Id sector);
double believed_risk(const Facts& facts, Id sector);

// War and peace as heard (DIP-5): the state between two roots, peace if unheard of.
void learn_state(Facts& facts, Id root_a, Id root_b, DipState state, Time time);
DipState believed_state(const Facts& facts, Id root_a, Id root_b);
bool believes_war(const Facts& facts, Id root_a, Id root_b);

// Sightings (INF-3): an army seen, kept newest per army.
void add_sighting(Facts& facts, const Sighting& s);

// dst keeps the newer of each subject from either side (INF-4).
void merge_into(Facts& dst, const Facts& src);
// Both keep the newer of each subject.
void share(Facts& a, Facts& b);

// A governor's facts, with its own market freshly observed: "the planet's
// information is its governor's information" (INF-2).
Facts& governor_facts(World& world, Id planet);

// Schedules a recurring automated courier run from every liege to each direct vassal,
// staggered (SYS-15).
void schedule_couriers(World& world);
// A run is due: send it automated, or post it as a courier mission (courier.mission_share).
void courier_due(World& world, Id vassal, Observer* observer);
// Starts an automated run from the vassal's liege.
void send_courier(World& world, Id vassal);
void courier_arrive(World& world, Id run, Observer* observer);

}  // namespace lattice::info
