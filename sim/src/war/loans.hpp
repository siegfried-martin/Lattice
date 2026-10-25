#pragma once

#include <vector>

#include "lattice/world.hpp"

// Loans of service contracts between governors (MIL-3, MIL-11, REL-8): the employer
// moves to the borrower, who pays the stipends and whose objectives the commanders choose
// from, until the loan ends on its own terms (INF-16).
namespace lattice::loans {

// Lends the contracts from the lender planet's governor to the borrower's, for an army or
// loan.max_days. The borrower pays the fee to the lender now. Returns the loan.
Id lend(World& world, Id lender, Id borrower, Id army, const std::vector<Id>& contracts, double fee, bool relief,
        Observer* observer);
// The loan ends: each contract goes back to the lender planet's governor, or ends if that
// planet is no longer of the borrower's faction. The commanders go home on their own.
void end(World& world, Id loan, Observer* observer);
// Its army disbanded: the purpose is done.
void end_for(World& world, Id army, Observer* observer);
// loan.max_days are up (LoanEnd). Commanders lent to an army that's still out leave it.
void expire(World& world, Id loan, Observer* observer);
// A lent contract ended early (a lost fleet, a commander that quit).
void contract_ended(World& world, Id contract);

}  // namespace lattice::loans
