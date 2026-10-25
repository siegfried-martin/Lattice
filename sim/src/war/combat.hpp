#pragma once

#include "lattice/world.hpp"

// Off-screen combat (SYS-14, MIL-13): fleet power plus randomness, in hourly rounds.
namespace lattice::combat {

// Starts a battle. Side 0 attacks; side 1 may have the planet's static defence. The
// fighters stop where they are until it ends. Returns the battle.
Id start(World& world, std::vector<Id> attackers, std::vector<Id> defenders, Id planet, bool planet_side, Id attacking_army,
         Id defending_army, Observer* observer);
// A battle in open space at a position: travellers stop until it's over (TIM-6).
Id start_in_space(World& world, std::vector<Id> attackers, std::vector<Id> defenders, double x, double y, Observer* observer);

// One hourly round: each side loses combat.lethality x the other's power x noise, as whole
// ships, weakest first and each commander's flagship last; a side retreats when its power
// falls below combat.retreat_base - risk x combat.retreat_risk of the enemy's.
void round(World& world, Id battle, Observer* observer);

// Combat power on a side now, static defence included.
double power(const World& world, const Battle& battle, int side);

// Damage to a group of characters: ships removed weakest first, flagships last. Returns
// the combat power destroyed. Characters left with no ships lose their contracts and
// roll for their pods (CHR-15, CHR-16). With `by`, the shares of the side that did it,
// each ship destroyed is a kill for bounties (MIL-18).
double destroy(World& world, std::vector<Id>& characters, double damage, double& carried, Observer* observer,
               const std::vector<Owed>* by = nullptr);

}  // namespace lattice::combat
