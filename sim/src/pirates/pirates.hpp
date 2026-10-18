#pragma once

#include "lattice/world.hpp"

// The pirate faction and pirate planets (PIR-1 to PIR-9, ECO-13, ECO-14, SYS-16).
namespace lattice::pirates {

// Weekly, per lawful planet (ECO-13, SYS-16): a planet fails with a chance that grows as
// its happiness falls below neutral and its debt deepens. Then it rebels: with no
// vassals it turns pirate; with vassals it secedes with them and its old faction
// declares war on it.
void check_rebellion(World& world, Id planet, Observer* observer);
// That weekly chance.
double rebellion_chance(const World& world, Id planet);
// The second kind of rebellion: the planet becomes a root with its branch, at war with
// its old root.
void secede(World& world, Id planet, Observer* observer);
// Turns a planet pirate (PIR-3): it leaves its faction (its vassals become roots), its
// whole labour goes to contraband, its debt is forgotten, and its governor is a pirate.
void flip(World& world, Id planet, Observer* observer);
// A captured pirate planet goes back to its old production (PIR-3, MIL-12).
void restore(World& world, Id planet);

// GEN-8, GEN-9: gen.pirate_planets fringe planets start as pirate planets, each with
// pirate.commanders pirate commanders.
void seed(World& world);

// Weekly, per pirate planet (proposed): each of its commanders at home goes raiding with
// chance raid.share, more when the planet is short of life support (PIR-3), toward a
// lawful planet within pirate.range_days of open space, weighted by population (a stand-in
// for believed traffic, SYS-16).
void dispatch(World& world, Id planet, Observer* observer);

// The pirate relation (PIR-9): fades weekly by pirel.fade toward pirel.rest.
double relation(const World& world, const Character& c);
void set_relation(World& world, Character& c, double value);
// Pirates attack non-pirates whose relation is below neutral.
bool attacks(const World& world, const Character& pirate, const Character& other);

// Broke (PIR-4): with chance pirate.turn_p(risk) (risk tolerance, or 1 under
// pirate.turn_all, EXP-9) a commander who knows a pirate planet heads there, its running
// costs suspended. Returns whether it went.
bool try_turn(World& world, Id character, Observer* observer);
// Arrived: it renounces its allegiances and is hired by the planet at pirate.salary of a
// lawful stipend.
void renounce(World& world, Id character, Observer* observer);

// Whether a planet is a pirate planet (MOD-8): seen on landing for every planet in the
// system.
void observe(const World& world, Facts& facts, Id planet);
void observe_system(const World& world, Facts& facts, Id planet);

}  // namespace lattice::pirates
