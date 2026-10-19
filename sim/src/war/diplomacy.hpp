#pragma once

#include "lattice/world.hpp"

// War, peace and ceasefire between factions (DIP-1 to DIP-6, SYS-12). The state is held
// between roots; a record that names a planet which is no longer a root is ignored.
namespace lattice::diplomacy {

DipState state(const World& world, Id root_a, Id root_b);
bool at_war(const World& world, Id root_a, Id root_b);
// Whether the factions of two planets are at war.
bool planets_at_war(const World& world, Id a, Id b);
// At war, or one is a pirate planet and the other isn't (DIP-7: the pirates are hostile to
// everyone outside them).
bool hostile_planets(const World& world, Id a, Id b);
// Roots this root is at war with.
std::vector<Id> enemies(const World& world, Id root);

// Some planet of one within dip.neighbour_days of a planet of the other (DIP-1).
bool neighbours(const World& world, Id root_a, Id root_b);

// A faction's monthly income, and its combat power: sworn commanders plus static defence.
double faction_income(const World& world, Id root);
double faction_power(const World& world, Id root);

// Weekly per root: the war vote against each neighbour at peace, suing for peace in each
// war, and ceasefires ending (SYS-12).
void root_weekly(World& world, Id root, Observer* observer);

void declare_war(World& world, Id root_a, Id root_b, Observer* observer);
void make_ceasefire(World& world, Id payer, Id payee, double per_month, Observer* observer);

// Monthly, at a root's payment: ceasefire tribute it owes (DIP-2).
void pay_ceasefire(World& world, Id root, Observer* observer);

// A root's planet stopped being the root (captured): its wars and ceasefires pass to the
// faction's new root (MOD-5).
void move_root(World& world, Id old_root, Id new_root);
// Drops every relation naming the planet: a new root starts at peace with everyone
// (MOD-5), and a root that's gone takes its relations with it.
void forget(World& world, Id planet);

// The war ledger (DIP-3): a side of a war lost a planet or combat power.
void record_loss(World& world, Id loser_root, Id winner_root, double planets, double power);

}  // namespace lattice::diplomacy
