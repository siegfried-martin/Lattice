#include "trade/escorts.hpp"

#include <algorithm>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "politics/opinion.hpp"
#include "war/risk.hpp"

namespace lattice::escorts {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

}  // namespace

void consider(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    if (c.escort != kNone || c.planet == kNone || c.cargo_qty <= 0 || c.trip.empty()) return;
    // Believed risk and days over the rest of the trip.
    double risk = 0, days = 0;
    Id from = c.planet;
    for (const TripStop& s : c.trip) {
        risk += risk::believed_route_risk(w, c.facts, from, s.planet);
        days += move::trip_days(w, c, from, s.planet);
        from = s.planet;
    }
    if (risk <= 0) return;
    const auto& m = w.config.merchant;
    const double value = c.cargo_cost;
    const double loss_none = value * risk * m.loss_rate;
    const double budget = c.wealth - chars::reserve(w, c);
    Id best = kNone;
    double best_gain = 0, best_price = 0;
    for (Id e = 0; e < static_cast<Id>(w.characters.size()); ++e) {
        const Character& x = w.characters[at(e)];
        if (e == id || !x.alive || x.planet != c.planet || x.activity != Activity::Idle || x.governs != kNone) continue;
        if (x.army != kNone || x.escorting != kNone || x.mission != kNone || x.cargo_qty > 0) continue;
        if (contracts::held(w, e, ContractKind::Service) != kNone) continue;
        const double power = ships::fleet(w, x).combat;
        if (power <= 0) continue;
        const double protection = power / (power + m.threat_ref * risk);
        const double price = w.config.escort.rate * power * days;
        const double gain = loss_none * protection - price;
        if (gain > best_gain && price <= budget) best_gain = gain, best = e, best_price = price;
    }
    if (best == kNone) return;
    contracts::sign(w, ContractKind::Escort, id, best, best_price, 0);
    Character& e = w.characters[at(best)];
    e.escorting = id;
    e.activity = Activity::Escorting;
    ++e.wake_seq;  // it decides nothing while it escorts
    c.escort = best;
    ++w.ledger.escorts;
    if (observer) observer->log(w.now, "escort.hired", best, "by " + std::to_string(id) + " for " + std::to_string(best_price));
}

void finish(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    if (c.escort == kNone) return;
    const Id e = c.escort;
    Character& escort = w.characters[at(e)];
    const Id k = contracts::held(w, e, ContractKind::Escort);
    if (k != kNone) {
        const double pay = w.contracts[at(k)].pay;
        c.wealth -= pay;
        chars::earn(escort, pay, true);
        w.ledger.escort_pay += pay;
        auto& skill = escort.skills[skill_of(MissionKind::Escort)];
        skill = std::min(w.config.skill.max, skill + w.config.skill.gain);
        ++escort.missions_done;
        escort.reputation += 1;
        opinion::deed(w, id, e, DeedKind::JobDone, observer);
        contracts::end(w, k);
    }
    c.escort = kNone;
    escort.escorting = kNone;
    escort.activity = Activity::Idle;
    if (escort.planet == kNone && escort.route.empty()) escort.planet = escort.last_planet = c.planet;
    chars::wake(w, e, w.now);
}

}  // namespace lattice::escorts
