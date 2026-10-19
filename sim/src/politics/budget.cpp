#include "politics/budget.hpp"

#include <algorithm>
#include <cmath>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/ships.hpp"
#include "economy/economy.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"

namespace lattice::budget {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

struct Unit {
    double cost = 0, value = 0;
};

// An option is a list of units; the k-th is worth d^k times its own value.
struct Option {
    std::vector<Unit> units;
    std::size_t funded = 0;
};

// Who a governor considers hiring (SYS-9): armed characters at planets within
// hire.range_days, with no service or salary contract and not on a mission.
std::vector<Id> candidates(const World& w, Id planet) {
    std::vector<std::pair<double, Id>> scored;
    const Planet& home = w.planets[at(planet)];
    const auto& h = w.config.hire;
    for (Id id = 0; id < static_cast<Id>(w.characters.size()); ++id) {
        const Character& c = w.characters[at(id)];
        if (!c.alive || c.governs != kNone || c.planet == kNone || c.mission != kNone || c.ships.empty()) continue;
        if (w.derived.travel(c.planet, planet) > h.range_days) continue;
        if (contracts::held(w, id, ContractKind::Service) != kNone || contracts::held(w, id, ContractKind::MerchantSalary) != kNone) continue;
        const double power = ships::fleet(w, c).combat;
        if (power < w.config.scout.min_combat) continue;
        const double score = h.w_power * power + h.w_opinion * opinion::get(w, home.governor, id) +
                             h.w_culture * (c.culture == home.region ? 1 : 0);
        scored.emplace_back(-score, id);
    }
    std::sort(scored.begin(), scored.end());
    std::vector<Id> out;
    for (auto [score, id] : scored) out.push_back(id);
    return out;
}

std::vector<Id> sworn(const World& w, Id governor) {
    std::vector<Id> out;
    for (Id k : w.characters[at(governor)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (ct.alive && ct.kind == ContractKind::Service && ct.employer == governor) out.push_back(ct.employee);
    }
    return out;
}

}  // namespace

double stipend(const World& w, const Character& c) { return w.config.hire.stipend_per_power * ships::fleet(w, c).combat; }

void weekly(World& w, Id planet, Observer* observer) {
    const Planet& p = w.planets[at(planet)];
    if (p.governor == kNone) return;
    const Id gov = p.governor;
    const Character& g = w.characters[at(gov)];
    const Config& c = w.config;
    const double income = std::max(0.0, p.income_avg) * static_cast<double>(c.time.payment_days);
    const double d = std::pow(c.budget.diminishing, 1 - g.risk / 2);
    double money = p.treasury + c.budget.credit_risk * income * g.risk;
    const double pressure = p.treasury < 0 ? 1 + c.budget.debt_pressure * static_cast<double>(p.deficit_days) / c.time.payment_days : 1;

    // Defence: the commanders it has first (no churn), then the best candidates.
    const std::vector<Id> current = sworn(w, gov);
    std::vector<Id> people = current;
    for (Id id : candidates(w, planet)) people.push_back(id);
    Option defence;
    for (Id id : people) {
        const double power = ships::fleet(w, w.characters[at(id)]).combat;
        defence.units.push_back({stipend(w, w.characters[at(id)]) * pressure, c.value.defence * income * power / c.value.power_ref});
    }

    // Patronage to vassals in the red: each unit a month of their operating costs, worth
    // the tribute it keeps flowing, more for vassals in favour (FAC-7).
    const std::vector<Id> vassals = fealty::vassals(w, planet);
    std::vector<Option> patronage(vassals.size());
    for (std::size_t i = 0; i < vassals.size(); ++i) {
        const Planet& v = w.planets[at(vassals[i])];
        if (v.treasury >= 0) continue;
        const double unit = economy::monthly_costs(c, v);
        const double favor = opinion::get(w, gov, v.governor);
        const double value = v.tribute_paid * c.value.patronage_months * std::max(0.0, 1 + c.fealty.w_favor * favor / 100);
        const auto units = static_cast<std::size_t>(std::ceil(-v.treasury / unit));
        for (std::size_t k = 0; k < units; ++k) patronage[i].units.push_back({unit * pressure, value});
    }

    // Fund the best value per cost first.
    std::vector<Option*> options{&defence};
    for (Option& o : patronage) options.push_back(&o);
    for (;;) {
        Option* best = nullptr;
        double best_ratio = 0;
        for (Option* o : options) {
            if (o->funded >= o->units.size()) continue;
            const Unit& u = o->units[o->funded];
            const double value = u.value * std::pow(d, static_cast<double>(o->funded));
            const double ratio = u.cost > 0 ? value / u.cost : 0;
            if (ratio >= 1 && u.cost <= money && ratio > best_ratio) best_ratio = ratio, best = o;
        }
        if (!best) break;
        money -= best->units[best->funded].cost;
        ++best->funded;
    }

    // Commanders: keep the funded, let the rest go, offer the funded candidates a stipend
    // they accept if it beats what they make now (SYS-9).
    for (std::size_t i = 0; i < people.size(); ++i) {
        const Id id = people[i];
        Character& ch = w.characters[at(id)];
        const bool funded = i < defence.funded;
        if (i < current.size()) {
            const Id k = contracts::held(w, id, ContractKind::Service);
            if (funded) {
                w.contracts[at(k)].pay = stipend(w, ch);
            } else {
                contracts::end(w, k);
                ++w.ledger.fires;
                if (observer) observer->log(w.now, "commander.released", id, "by " + std::to_string(planet));
            }
            continue;
        }
        if (!funded) continue;
        const double asked = stipend(w, ch);
        if (asked <= ch.earned_cargo + ch.earned_combat) continue;  // it makes more on its own
        contracts::sign(w, ContractKind::Service, gov, id, asked, c.time.payment_days * kDay);
        ++w.ledger.hires;
        chars::wake(w, id, w.now);
        if (observer) observer->log(w.now, "commander.hired", id, "by " + std::to_string(planet));
    }

    // Patronage: paid now, a good deed once per grant.
    for (std::size_t i = 0; i < vassals.size(); ++i) {
        if (patronage[i].funded == 0) continue;
        Planet& v = w.planets[at(vassals[i])];
        const double amount = economy::monthly_costs(c, v) * static_cast<double>(patronage[i].funded);
        w.planets[at(planet)].treasury -= amount;
        v.treasury += amount;
        w.ledger.patronage += amount;
        opinion::deed(w, v.governor, gov, DeedKind::Patronage, observer);
    }
}

void pay_stipends(World& w, Id planet, Observer*) {
    const Id gov = w.planets[at(planet)].governor;
    if (gov == kNone) return;
    for (Id id : sworn(w, gov)) {
        const Contract& ct = w.contracts[at(contracts::held(w, id, ContractKind::Service))];
        w.planets[at(planet)].treasury -= ct.pay;
        chars::earn(w.characters[at(id)], ct.pay, true);
        w.ledger.stipends += ct.pay;
    }
}

}  // namespace lattice::budget
