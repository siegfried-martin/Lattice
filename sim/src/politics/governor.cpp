#include "politics/governor.hpp"

#include "chars/missions.hpp"
#include "politics/budget.hpp"
#include "politics/fealty.hpp"
#include "trade/merchants.hpp"

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
    fealty::adjust_tribute(w, planet, observer);
    fealty::check_defection(w, planet, observer);
    budget::weekly(w, planet, observer);
    missions::root_weekly(w, planet, observer);
}

bool holds_gates(const World& w, Id planet) {
    const Group& g = w.map.groups[at(w.planets[at(planet)].group)];
    return g.holder == planet && g.node != kNone;
}

void monthly(World& w, Id planet, Observer* observer) {
    Planet& p = w.planets[at(planet)];
    // Dues (SYS-20): the Authority spends authority.maintenance of them on the network, a
    // sink, and keeps the rest for mercenary contracts (MS-6).
    const double dues = w.config.dues.access + (holds_gates(w, planet) ? w.config.dues.gate : 0);
    p.treasury -= dues;
    const double upkeep = dues * w.config.dues.maintenance;
    w.authority.treasury += dues - upkeep;
    w.ledger.dues += dues;
    w.ledger.maintenance += upkeep;
    w.ledger.money_sunk += upkeep;

    fealty::pay_tribute(w, planet, observer);
    budget::pay_stipends(w, planet, observer);
    if (p.liege == kNone) merchant::pay_salaries(w, planet, observer);
}

}  // namespace lattice::governor
