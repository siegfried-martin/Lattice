#include "war/loans.hpp"

#include <algorithm>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "politics/fealty.hpp"
#include "war/armies.hpp"

namespace lattice::loans {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

}  // namespace

Id lend(World& w, Id lender, Id borrower, Id army, const std::vector<Id>& contracts, double fee, bool relief, Observer* observer) {
    const Id to = w.planets[at(borrower)].governor;
    w.loans.push_back({.lender = lender, .borrower = borrower, .army = army, .fee = fee, .start = w.now, .relief = relief});
    const auto id = static_cast<Id>(w.loans.size() - 1);
    for (Id k : contracts) {
        Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.loan != kNone) continue;
        contracts::transfer(w, k, to);
        ct.loan = id;
        w.loans[at(id)].contracts.push_back(k);
    }
    Planet& b = w.planets[at(borrower)];
    b.treasury -= fee;
    b.month_military += fee;
    w.planets[at(lender)].treasury += fee;
    w.ledger.loan_fees += fee;
    ++w.ledger.loans;
    if (relief) ++w.ledger.relief_loans;
    w.events.push({.time = w.now + w.config.loan.max_days * kDay, .kind = EventKind::LoanEnd, .record = id});
    if (observer) {
        observer->log(w.now, "loan.start", id,
                      "from " + std::to_string(lender) + " to " + std::to_string(borrower) + ", " +
                          std::to_string(w.loans[at(id)].contracts.size()) + " commanders, fee " + std::to_string(fee));
    }
    return id;
}

void end(World& w, Id id, Observer* observer) {
    Loan& l = w.loans[at(id)];
    if (!l.alive) return;
    l.alive = false;
    const Id back = w.planets[at(l.lender)].governor;
    const bool home = back != kNone && fealty::same_faction(w, l.lender, l.borrower) &&
                      w.planets[at(l.lender)].pirate == w.planets[at(l.borrower)].pirate;
    for (Id k : std::vector<Id>(l.contracts)) {
        Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.loan != id) continue;
        ct.loan = kNone;
        const Id who = ct.employee;
        if (home) {
            contracts::transfer(w, k, back);
        } else {
            contracts::end(w, k);
        }
        const Character& c = w.characters[at(who)];
        if (c.alive && c.army == kNone && c.battle == kNone && c.planet != kNone) chars::wake(w, who, w.now);
    }
    l.contracts.clear();
    if (observer) observer->log(w.now, "loan.end", id, home ? "returned" : "ended");
}

void end_for(World& w, Id army, Observer* observer) {
    for (Id id = 0; id < static_cast<Id>(w.loans.size()); ++id) {
        if (w.loans[at(id)].alive && w.loans[at(id)].army == army) end(w, id, observer);
    }
}

void expire(World& w, Id id, Observer* observer) {
    const Loan& l = w.loans[at(id)];
    if (!l.alive) return;
    // Not in the middle of a fight: a day later.
    for (Id k : l.contracts) {
        if (w.characters[at(w.contracts[at(k)].employee)].battle != kNone) {
            w.events.push({.time = w.now + kDay, .kind = EventKind::LoanEnd, .record = id});
            return;
        }
    }
    for (Id k : std::vector<Id>(l.contracts)) {
        const Id who = w.contracts[at(k)].employee;
        const Id army = w.characters[at(who)].army;
        if (army != kNone) armies::dismiss(w, army, who, observer);
    }
    end(w, id, observer);
}

void contract_ended(World& w, Id k) {
    Contract& ct = w.contracts[at(k)];
    const Id id = ct.loan;
    ct.loan = kNone;
    Loan& l = w.loans[at(id)];
    std::erase(l.contracts, k);
    if (l.contracts.empty()) l.alive = false;
}

}  // namespace lattice::loans
