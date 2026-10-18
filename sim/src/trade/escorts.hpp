#pragma once

#include "lattice/world.hpp"

// Escorts (TRD-8 to TRD-10, SYS-7): a private contract for one trip, paid at its end.
namespace lattice::escorts {

// At a stop, before setting off: hire the escort that saves most, if any (TRD-9):
//   expected_loss(e) = cargo value x believed route risk x merchant.loss_rate x (1 - protection(e))
//   protection(e)    = escort combat / (escort combat + merchant.threat_ref x route risk)
//   price(e)         = escort.rate x escort combat x trip days
// Candidates are armed characters idle at the same planet, who take it by fit and pay
// (TRD-10, CHR-10). The escort travels with the merchant.
void consider(World& world, Id merchant, Observer* observer);

// The trip is over: pay the escort (REL-9) and let it go.
void finish(World& world, Id merchant, Observer* observer);

}  // namespace lattice::escorts
