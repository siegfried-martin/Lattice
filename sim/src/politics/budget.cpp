#include "politics/budget.hpp"

#include <algorithm>
#include <cmath>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/ships.hpp"
#include "economy/economy.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"
#include "war/armies.hpp"
#include "war/diplomacy.hpp"
#include "war/raids.hpp"
#include "war/spies.hpp"

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
        if (c.pirate != home.pirate) continue;  // pirates serve pirate planets, and only pirates do
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

double threat(const World& w, Id planet) {
    const Config& c = w.config;
    const Character& g = w.characters[at(w.planets[at(planet)].governor)];
    const std::vector<Id> enemies = raids::believed_enemies(w, planet);
    bool border = false;
    for (const Sighting& s : g.facts.sightings) {
        if (s.target == planet && s.time >= w.now - c.scout.threat_days * kDay) border = true;
    }
    for (Id q = 0; q < static_cast<Id>(w.planets.size()) && !enemies.empty() && !border; ++q) {
        if (std::find(enemies.begin(), enemies.end(), fealty::root_of(w, q)) != enemies.end() &&
            w.derived.travel(planet, q) <= c.dip.border_days) {
            border = true;
        }
    }
    return 1 + (enemies.empty() ? 0 : c.value.war_mult) + (border ? c.value.border_mult : 0);
}

double hoard(const World& w, Id planet) {
    const Config& c = w.config;
    const Planet& p = w.planets[at(planet)];
    const double months = p.treasury / std::max(1.0, economy::monthly_costs(c, p));
    return 1 + c.budget.hoard * std::clamp(months / c.budget.reserve_months - 1, 0.0, c.budget.hoard_cap);
}

double stipend(const World& w, const Character& c) { return w.config.hire.stipend_per_power * ships::fleet(w, c).combat; }

namespace {

// What a planet pays a commander: a pirate planet pays pirate.salary of it (PIR-5).
double pay_at(const World& w, Id planet, const Character& c) {
    return stipend(w, c) * (w.planets[at(planet)].pirate ? w.config.pirate.salary : 1);
}

}  // namespace

void weekly(World& w, Id planet, Observer* observer) {
    const Planet& p = w.planets[at(planet)];
    if (p.governor == kNone) return;
    const Id gov = p.governor;
    const Character& g = w.characters[at(gov)];
    const Config& c = w.config;
    // Net income: what's left after paying for goods (OPN-39, round 4).
    const double income = std::max(0.0, p.net_avg) * static_cast<double>(c.time.payment_days);
    const double d = std::pow(c.budget.diminishing, 1 - g.risk / 2);
    double money = p.treasury + c.budget.credit_risk * income * g.risk;
    const double pressure = p.treasury < 0 ? 1 + c.budget.debt_pressure * static_cast<double>(p.deficit_days) / c.time.payment_days : 1;

    const std::vector<Id> enemies = raids::believed_enemies(w, planet);
    const double threat = budget::threat(w, planet);
    // Money beyond a reserve makes defence worth more (MIL-5): deep treasuries drain into
    // fleets and guns.
    const double guard = threat * hoard(w, planet);

    // Defence: the commanders it has first (no churn), then the best candidates. A pirate
    // planet keeps whoever joins it (PIR-4: renounced commanders are hired) and hires no
    // one else, so it budgets only static defence (proposed).
    const std::vector<Id> current = p.pirate ? std::vector<Id>{} : sworn(w, gov);
    std::vector<Id> people = current;
    if (!p.pirate) {
        for (Id id : candidates(w, planet)) people.push_back(id);
    }
    Option defence;
    for (Id id : people) {
        const double power = ships::fleet(w, w.characters[at(id)]).combat;
        defence.units.push_back(
            {pay_at(w, planet, w.characters[at(id)]) * pressure, guard * c.value.defence * income * power / c.value.power_ref});
    }

    // Static defence (MIL-5), bought in units of value.security_share of monthly income,
    // so a rich planet's spending is on the scale of its wealth. Each unit is worth
    // value.security_ratio times its cost, x threat x hoard: richer planets buy bigger
    // units, not more of them (round 3: with value growing as income squared, the richest
    // spent a third of their income on guns and fell into debt).
    Option security;
    const double security_unit = c.value.security_share * income;
    for (int k = 0; k < 64 && security_unit > 0; ++k) {
        security.units.push_back({security_unit * pressure, guard * c.value.security_ratio * security_unit});
    }

    // Policing (PIR-15) *(proposed)*: units of value.security_unit a month, worth what the
    // productivity lost to contraband costs the planet (pop.contraband_penalty x C of
    // its income) x sus.police_effect.
    Option policing;
    if (!p.pirate && p.contraband_avg > 0) {
        const double harm = income * c.pop.contraband_penalty * p.contraband_avg * c.sus.police_effect;
        for (int k = 0; k < 16; ++k) policing.units.push_back({c.value.security_unit * pressure, harm});
    }

    // Spies (INF-13) *(proposed)*: at war, each is worth spy.value of monthly income x the
    // threat, for spy.salary a month; those it has come first.
    Option spying;
    std::vector<Id> spy_people;
    if (c.spy.enabled && !p.pirate && !enemies.empty()) {
        spy_people = spies::employed(w, gov);
        for (Id id : spies::candidates(w, planet)) spy_people.push_back(id);
        for (std::size_t i = 0; i < spy_people.size(); ++i) spying.units.push_back({c.spy.salary * pressure, c.spy.value * income * threat});
    }

    // An invasion, at war or facing a pirate planet (SYS-13): one unit for the best target.
    // Pirates raid; they don't invade (PIR-8).
    Option invasion;
    const bool warming_up = w.now < c.gen.warmup_days * kDay;  // GEN-10: the world settles first
    const armies::Plan plan = p.pirate || warming_up ? armies::Plan{} : armies::best_invasion(w, planet);
    if (plan.target != kNone) invasion.units.push_back({plan.cost * pressure, plan.value});

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
    std::vector<Option*> options{&defence, &security, &policing, &spying, &invasion};
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
                w.contracts[at(k)].pay = pay_at(w, planet, ch);
            } else {
                contracts::end(w, k);
                ++w.ledger.fires;
                if (observer) observer->log(w.now, "commander.released", id, "by " + std::to_string(planet));
            }
            continue;
        }
        if (!funded) continue;
        const double asked = pay_at(w, planet, ch);
        if (asked <= ch.earned_cargo + ch.earned_combat) continue;  // it makes more on its own
        contracts::sign(w, ContractKind::Service, gov, id, asked, c.time.payment_days * kDay);
        ++w.ledger.hires;
        chars::wake(w, id, w.now);
        if (observer) observer->log(w.now, "commander.hired", id, "by " + std::to_string(planet));
    }

    w.planets[at(planet)].security = static_cast<double>(security.funded) * security_unit;
    w.planets[at(planet)].policing = static_cast<double>(policing.funded) * c.value.security_unit;
    // Spies: keep the funded, let the rest go, hire funded candidates.
    const std::vector<Id> spies_now = spies::employed(w, gov);
    for (std::size_t i = 0; i < spy_people.size(); ++i) {
        const bool kept = i < spying.funded;
        const bool has = std::find(spies_now.begin(), spies_now.end(), spy_people[i]) != spies_now.end();
        if (has && !kept) spies::release(w, spy_people[i]);
        if (!has && kept) spies::hire(w, planet, spy_people[i], observer);
    }
    if (spy_people.empty()) {
        for (Id id : spies_now) spies::release(w, id);  // at peace: no more need of them
    }
    if (invasion.funded > 0) armies::commission(w, planet, plan.target, false, observer, plan.node);

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
