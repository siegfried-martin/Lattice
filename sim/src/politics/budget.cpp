#include "politics/budget.hpp"

#include <algorithm>
#include <cmath>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/ships.hpp"
#include "economy/economy.hpp"
#include "politics/fealty.hpp"
#include "info/facts.hpp"
#include "politics/opinion.hpp"
#include "politics/roles.hpp"
#include "war/armies.hpp"
#include "war/diplomacy.hpp"
#include "war/prizes.hpp"
#include "war/raids.hpp"
#include "war/spies.hpp"

namespace lattice::budget {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

struct Unit {
    double cost = 0, value = 0;
};

// An option is a list of units; the k-th is worth d^k times its own value, d set by the
// risk of whoever decides the option's line (SYS-5).
struct Option {
    std::vector<Unit> units;
    std::size_t funded = 0;
    double d = 0;
};

// Who a governor considers hiring (SYS-9): armed characters at planets within
// hire.range_days, with no service, salary or steward contract and not on a mission,
// best commander fitness first (CHR-20).
std::vector<Id> candidates(const World& w, Id planet) {
    std::vector<std::pair<double, Id>> scored;
    const Planet& home = w.planets[at(planet)];
    const auto& h = w.config.hire;
    for (Id id = 0; id < static_cast<Id>(w.characters.size()); ++id) {
        const Character& c = w.characters[at(id)];
        if (!c.alive || c.governs != kNone || c.stewards != kNone || c.planet == kNone || c.mission != kNone || c.ships.empty()) continue;
        if (w.derived.travel(c.planet, planet) > h.range_days) continue;
        if (contracts::held(w, id, ContractKind::Service) != kNone || contracts::held(w, id, ContractKind::MerchantSalary) != kNone) continue;
        if (c.pirate != home.pirate) continue;  // pirates serve pirate planets, and only pirates do
        const double power = ships::fleet(w, c).combat;
        if (power < w.config.scout.min_combat) continue;
        scored.emplace_back(-roles::fitness(w, roles::Post::Commander, home.governor, id, planet), id);
    }
    std::sort(scored.begin(), scored.end());
    std::vector<Id> out;
    for (auto [score, id] : scored) out.push_back(id);
    return out;
}

// Its commanders; with borrowed false, only its own, not those lent to it (MIL-3), which
// it pays but doesn't hire or let go.
std::vector<Id> sworn(const World& w, Id governor, bool borrowed = true) {
    std::vector<Id> out;
    for (Id k : w.characters[at(governor)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (ct.alive && ct.kind == ContractKind::Service && ct.employer == governor && (borrowed || ct.loan == kNone)) {
            out.push_back(ct.employee);
        }
    }
    return out;
}

}  // namespace

double threat(const World& w, Id planet) {
    const Config& c = w.config;
    const std::vector<Id> enemies = raids::believed_enemies(w, planet);
    bool border = false;
    for (const Sighting& s : info::planet_facts(w, planet).sightings) {
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

double rep_factor(const World& w, const Character& c) {
    const auto& h = w.config.hire;
    const double rep = std::max(0.0, c.reputation);
    return 1 + h.rep_mult * rep / (rep + h.rep_half);
}

double stipend(const World& w, const Character& c) {
    return w.config.hire.stipend_per_power * ships::fleet(w, c).combat * rep_factor(w, c);
}

namespace {

// What a planet pays a commander: a pirate planet pays pirate.salary of it (PIR-5).
double pay_at(const World& w, Id planet, const Character& c) {
    return stipend(w, c) * (w.planets[at(planet)].pirate ? w.config.pirate.salary : 1);
}

}  // namespace

void weekly(World& w, Id planet, Observer* observer) {
    const Planet& p = w.planets[at(planet)];
    if (p.governor == kNone) return;
    const bool vacant = roles::vacant(w, planet);
    if (vacant && p.steward == kNone) return;  // nobody to run it until a successor comes (FAC-17)
    const Id gov = p.governor;
    const Character& g = w.characters[at(gov)];
    const Config& c = w.config;
    // Net income: what's left after paying for goods (OPN-39, round 4).
    const double income = std::max(0.0, p.net_avg) * static_cast<double>(c.time.payment_days);
    // Two deciders, one budget (SYS-5, FAC-16): each line falls off by its decider's risk.
    const double gov_risk = roles::governor_risk(w, planet);
    const double d_gov = std::pow(c.budget.diminishing, 1 - gov_risk / 2);
    const double d_stew = std::pow(c.budget.diminishing, 1 - roles::steward_risk(w, planet) / 2);
    double money = p.treasury + c.budget.credit_risk * income * gov_risk;
    // Obligations come before options (SYS-5): the steward's salary, the stipends of
    // commanders it has borrowed, and a root's bounties owed.
    if (p.steward != kNone) {
        const Id k = contracts::held(w, p.steward, ContractKind::Steward);
        if (k != kNone) money -= w.contracts[at(k)].pay;
    }
    for (Id k : g.contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (ct.alive && ct.kind == ContractKind::Service && ct.employer == gov && ct.loan != kNone) money -= ct.pay;
    }
    if (p.liege == kNone) money -= prizes::owed(w, planet);
    const double pressure = p.treasury < 0 ? 1 + c.budget.debt_pressure * static_cast<double>(p.deficit_days) / c.time.payment_days : 1;

    const std::vector<Id> enemies = raids::believed_enemies(w, planet);
    const double threat = budget::threat(w, planet);
    // Money beyond a reserve makes defence worth more (MIL-5): deep treasuries drain into
    // fleets and guns.
    const double guard = threat * hoard(w, planet);

    // Defence: the commanders it has first (no churn), then the best candidates. A pirate
    // planet keeps whoever joins it (PIR-4: renounced commanders are hired) and hires no
    // one else, so it budgets only static defence (proposed).
    // The governor's own fleet is a commander unit, first of all (MIL-20).
    const bool own = !vacant && g.alive && !g.ships.empty();
    const std::vector<Id> current = p.pirate ? std::vector<Id>{} : sworn(w, gov, false);
    std::vector<Id> people;
    if (own) people.push_back(gov);
    for (Id id : current) people.push_back(id);
    const std::size_t n_kept = people.size();
    if (!p.pirate) {
        for (Id id : candidates(w, planet)) people.push_back(id);
    }
    Option defence;
    defence.d = d_gov;
    for (Id id : people) {
        // Reputation is worth what it costs (MIL-2): a unit's value takes the stipend's factor.
        const Character& ch = w.characters[at(id)];
        const double power = ships::fleet(w, ch).combat;
        defence.units.push_back({pay_at(w, planet, ch) * pressure,
                                 guard * c.value.defence * income * power / c.value.power_ref * rep_factor(w, ch)});
    }

    // Static defence (MIL-5), bought in units of value.security_share of monthly income,
    // so a rich planet's spending is on the scale of its wealth. Each unit is worth
    // value.security_ratio times its cost, x threat x hoard: richer planets buy bigger
    // units, not more of them (round 3: with value growing as income squared, the richest
    // spent a third of their income on guns and fell into debt).
    Option security;
    security.d = d_gov;
    const double security_unit = c.value.security_share * income;
    for (int k = 0; k < 64 && security_unit > 0; ++k) {
        security.units.push_back({security_unit * pressure, guard * c.value.security_ratio * security_unit});
    }

    // Policing (PIR-15) *(proposed)*: units of value.security_unit a month, worth what the
    // productivity lost to contraband costs the planet (pop.contraband_penalty x C of
    // its income) x sus.police_effect.
    Option policing;
    policing.d = d_gov;
    if (!p.pirate && p.contraband_avg > 0) {
        const double harm = income * c.pop.contraband_penalty * p.contraband_avg * c.sus.police_effect;
        for (int k = 0; k < 16; ++k) policing.units.push_back({c.value.security_unit * pressure, harm});
    }

    // Spies (INF-13) *(proposed)*: at war, each is worth spy.value of monthly income x the
    // threat, for spy.salary a month; those it has come first.
    Option spying;
    spying.d = d_gov;
    std::vector<Id> spy_people;
    if (c.spy.enabled && !p.pirate && !enemies.empty()) {
        spy_people = spies::employed(w, gov);
        for (Id id : spies::candidates(w, planet)) spy_people.push_back(id);
        for (std::size_t i = 0; i < spy_people.size(); ++i) spying.units.push_back({c.spy.salary * pressure, c.spy.value * income * threat});
    }

    // An invasion, at war or facing a pirate planet (SYS-13): one unit for the best target.
    // Pirates raid; they don't invade (PIR-8).
    Option invasion;
    invasion.d = d_gov;
    const bool warming_up = w.now < c.gen.warmup_days * kDay;  // GEN-10: the world settles first
    // A vacant seat commissions nothing (FAC-17).
    const armies::Plan plan = p.pirate || warming_up || vacant ? armies::Plan{} : armies::best_invasion(w, planet);
    // Its cost counts the most the purse can come to, by the chance of paying it (MIL-2).
    const double purse = c.objective.choose ? c.objective.reward_max * plan.value : 0;
    if (plan.target != kNone) invasion.units.push_back({(plan.cost + purse) * pressure, plan.value});

    // Patronage to vassals in the red that were captured within relief.capture_months: each
    // unit a month of their operating costs, worth the tribute it keeps flowing, more for
    // vassals in favour (FAC-7). Patronage is for rebuilding; a poor vassal asks for tribute
    // relief instead (FAC-15).
    const std::vector<Id> vassals = fealty::vassals(w, planet);
    std::vector<Option> patronage(vassals.size());
    for (Option& o : patronage) o.d = d_stew;  // the steward's line
    for (std::size_t i = 0; i < vassals.size(); ++i) {
        const Planet& v = w.planets[at(vassals[i])];
        if (v.treasury >= 0 || v.captured_at == kNever) continue;
        if (static_cast<double>(w.now - v.captured_at) > c.relief.capture_months * static_cast<double>(c.time.payment_days * kDay)) continue;
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
            const double value = u.value * std::pow(o->d, static_cast<double>(o->funded));
            const double ratio = u.cost > 0 ? value / u.cost : 0;
            if (ratio >= 1 && u.cost <= money && ratio > best_ratio) best_ratio = ratio, best = o;
        }
        if (!best) break;
        money -= best->units[best->funded].cost;
        ++best->funded;
    }

    // Commanders: keep the funded, let the rest go, offer the funded candidates a stipend
    // they accept if it beats what they make now (SYS-9).
    w.planets[at(planet)].governor_stipend = 0;
    for (std::size_t i = 0; i < people.size(); ++i) {
        const Id id = people[i];
        Character& ch = w.characters[at(id)];
        const bool funded = i < defence.funded;
        if (own && i == 0) {
            if (funded) w.planets[at(planet)].governor_stipend = pay_at(w, planet, ch);  // a self-stipend (MIL-20)
            continue;
        }
        if (i < n_kept) {
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
        roles::record_path(ch);
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
    if (invasion.funded > 0) armies::commission(w, planet, plan.target, false, observer, plan.node, plan.gain);

    // Patronage: paid now, a good deed once per grant.
    for (std::size_t i = 0; i < vassals.size(); ++i) {
        if (patronage[i].funded == 0) continue;
        Planet& v = w.planets[at(vassals[i])];
        const double amount = economy::monthly_costs(c, v) * static_cast<double>(patronage[i].funded);
        w.planets[at(planet)].treasury -= amount;
        w.planets[at(planet)].month_patronage += amount;
        v.treasury += amount;
        w.ledger.patronage += amount;
        opinion::deed(w, v.governor, gov, DeedKind::Patronage, observer);
    }
}

void pay_stipends(World& w, Id planet, Observer*) {
    Planet& p = w.planets[at(planet)];
    const Id gov = p.governor;
    if (gov == kNone) return;
    for (Id id : sworn(w, gov)) {
        const Contract& ct = w.contracts[at(contracts::held(w, id, ContractKind::Service))];
        p.treasury -= ct.pay;
        p.month_military += ct.pay;
        chars::earn(w.characters[at(id)], ct.pay, true);
        w.ledger.stipends += ct.pay;
    }
    // The governor's own fleet, when the budget funded it (MIL-20).
    if (p.governor_stipend > 0 && w.characters[at(gov)].alive) {
        p.treasury -= p.governor_stipend;
        p.month_military += p.governor_stipend;
        chars::earn(w.characters[at(gov)], p.governor_stipend, true);
        w.ledger.stipends += p.governor_stipend;
    }
    // The steward's salary (FAC-16).
    if (p.steward != kNone) {
        const Id k = contracts::held(w, p.steward, ContractKind::Steward);
        if (k != kNone) {
            const double pay = w.contracts[at(k)].pay;
            p.treasury -= pay;
            chars::earn(w.characters[at(p.steward)], pay, false);
            w.ledger.steward_pay += pay;
        }
    }
}

}  // namespace lattice::budget
