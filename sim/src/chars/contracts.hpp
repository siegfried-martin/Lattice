#pragma once

#include "lattice/world.hpp"

// Contracts (MOD-4, REL-8): either side can end any contract at any time.
namespace lattice::contracts {

Id sign(World& world, ContractKind kind, Id employer, Id employee, double pay, Time period, Id mission = kNone);
// Ends a contract; kNone (one already ended) does nothing.
void end(World& world, Id contract);
// The alive contract of this kind the character holds as employee, or none.
Id held(const World& world, Id character, ContractKind kind);

}  // namespace lattice::contracts
