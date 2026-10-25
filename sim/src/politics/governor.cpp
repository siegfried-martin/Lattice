#include "politics/governor.hpp"

#include "chars/missions.hpp"
#include "politics/budget.hpp"
#include "politics/fealty.hpp"
#include "politics/relief.hpp"
#include "politics/roles.hpp"
#include "trade/merchants.hpp"
#include "war/diplomacy.hpp"
#include "war/gates.hpp"
#include "war/prizes.hpp"
#include "pirates/pirates.hpp"
#include "pirates/smuggling.hpp"
#include "war/raids.hpp"
#include "war/spies.hpp"

namespace lattice::governor {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

}  // namespace

void schedule(World& w) {
    const auto n = static_cast<std::int64_t>(w.planets.size());
    const Time week = w.config.time.budget_days * kDay;
    for (std::int64_t p = 0; p < n; ++p) {
        w.events.push({.time = stagger(week, p, n), .phase = Phase::Cadence, .kind = EventKind::GovernorWeekly, .record = p,
                       .period = week});
    }
}

void weekly(World& w, Id planet, Observer* observer) {
    if (const Id node = w.gates.empty() ? kNone : gates::held_by(w, planet); node != kNone) gates::check(w, node, observer);
    // A dead governor's seat, once its chooser has heard (FAC-17). While it waits, the
    // steward runs the budget and the planet commissions nothing.
    const bool vacant = roles::settle(w, planet, observer);
    if (w.planets[at(planet)].pirate) {
        // A pirate planet: defence and raids, no politics (PIR-1, PIR-3).
        budget::weekly(w, planet, observer);
        if (!vacant) pirates::dispatch(w, planet, observer);
        smuggling::post(w, planet, observer);
        return;
    }
    pirates::check_rebellion(w, planet, observer);
    if (w.planets[at(planet)].pirate) return;
    diplomacy::root_weekly(w, planet, observer);
    if (!vacant) {
        fealty::adjust_tribute(w, planet, observer);
        fealty::check_defection(w, planet, observer);
        relief::weekly(w, planet, observer);
    }
    roles::steward_weekly(w, planet, observer);
    budget::weekly(w, planet, observer);
    if (vacant) return;
    raids::dispatch(w, planet, observer);
    missions::root_weekly(w, planet, observer);
    missions::planet_weekly(w, planet, observer);
}

bool holds_gates(const World& w, Id planet) {
    const Group& g = w.map.groups[at(w.planets[at(planet)].group)];
    return g.holder == planet && g.node != kNone;
}

void monthly(World& w, Id planet, Observer* observer) {
    Planet& p = w.planets[at(planet)];
    const double own_net = p.income_month;  // before tribute resets it
    // Dues (SYS-20): the Authority spends authority.maintenance of them on the network, a
    // sink, and keeps the rest for mercenary contracts. Pirate planets pay none (ECO-14).
    const double dues = p.pirate ? 0 : w.config.dues.access + (holds_gates(w, planet) ? w.config.dues.gate : 0);
    p.treasury -= dues;
    const double upkeep = dues * w.config.dues.maintenance;
    w.authority.treasury += dues - upkeep;
    w.ledger.dues += dues;
    w.ledger.maintenance += upkeep;
    w.ledger.money_sunk += upkeep;

    // Security (ECO-4, MIL-5): patrols and static defence, a sink.
    p.treasury -= p.security + p.policing;
    w.ledger.security += p.security;
    w.ledger.policing += p.policing;
    w.ledger.money_sunk += p.security + p.policing;
    p.month_military += p.security;

    fealty::pay_tribute(w, planet, observer);
    if (p.liege == kNone) diplomacy::pay_ceasefire(w, planet, observer);
    budget::pay_stipends(w, planet, observer);
    spies::pay(w, planet, observer);
    if (p.liege == kNone) merchant::pay_salaries(w, planet, observer);
    if (p.liege == kNone && !p.pirate) prizes::pay(w, planet, observer);  // MIL-18

    // CHK-26: the month's flows, by its faction's state.
    if (!p.pirate) {
        const std::size_t war = diplomacy::enemies(w, fealty::root_of(w, planet)).empty() ? 0 : 1;
        Ledger& l = w.ledger;
        l.month_net[war] += own_net;
        l.month_military[war] += p.month_military;
        l.month_patronage[war] += p.month_patronage;
        if (p.liege != kNone && own_net > 0) {
            l.vassal_net[war] += own_net;
            l.vassal_tribute[war] += p.tribute_paid;
        }
    }
    p.month_military = 0;
    p.month_patronage = 0;
}

}  // namespace lattice::governor
