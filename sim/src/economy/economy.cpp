#include "economy/economy.hpp"

#include <algorithm>
#include <cmath>

#include "lattice/happiness.hpp"
#include "lattice/market.hpp"
#include "war/gates.hpp"

namespace lattice::economy {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

}  // namespace

double monthly_costs(const Config& c, const Planet& p) {
    const double cost = c.econ.op_base + c.econ.op_per_capita * p.population;
    return p.pirate ? cost * c.pirate.cost_mult : cost;  // a low cost base (ECO-14)
}

void init_state(World& w) {
    const Config& c = w.config;
    for (Planet& p : w.planets) {
        for (int g = 0; g < kGoodCount; ++g) p.stock[at(g)] = market::reserve(c, p, static_cast<Good>(g));
        p.labour = {1.0 / 3, 1.0 / 3, 1.0 / 3};
        p.tax_rate = c.econ.tax;
        p.treasury = c.gen.start_treasury_months * monthly_costs(c, p);
        // The income the planet would have if its population spent its whole budget,
        // so the credit limit means something from the first day.
        p.income_avg = p.population * c.econ.wage * p.productivity;
    }
}

void schedule(World& w) {
    const auto n = static_cast<std::int64_t>(w.planets.size());
    const Time labour = w.config.time.labour_days * kDay, payment = w.config.time.payment_days * kDay;
    for (std::int64_t i = 0; i < n; ++i) {
        w.events.push({.time = stagger(kDay, i, n), .phase = Phase::Cadence, .kind = EventKind::PlanetDay, .record = i, .period = kDay});
        w.events.push({.time = labour + stagger(labour, i, n), .phase = Phase::Cadence, .kind = EventKind::PlanetLabour, .record = i, .period = labour});
        w.events.push({.time = stagger(payment, i, n), .phase = Phase::Cadence, .kind = EventKind::PlanetPayment, .record = i, .period = payment});
    }
}

void planet_day(World& w, Id id) {
    const Config& c = w.config;
    Planet& p = w.planets[at(id)];
    const double n = p.population;

    // SYS-2: output = N x labour_per_capita x share x efficiency x yield x P, owned by the
    // planet.
    for (int s = 0; s < 3; ++s) {
        const Good g = p.makes[at(s)];
        const double out = n * c.prod.labour_per_capita * p.labour[at(s)] * market::efficiency(c, p, s) * c.goods[g].yield * p.productivity;
        p.stock[g] += out;
        w.ledger.produced[g] += out;
    }

    // SYS-1: purchasing power is new money (ECO-1); it becomes money only when goods are
    // bought, and is paid to the treasury (ECO-2). Life support first, at the ask; a
    // short budget scales every life-support purchase down alike.
    double budget = n * c.econ.wage * p.productivity;
    std::array<double, kGoodCount> buy{};
    double cost = 0;
    for (Good g : kLifeSupport) {
        buy[g] = std::min(p.stock[g], n * c.goods[g].consumption);
        cost += buy[g] * market::ask(c, p, g);
    }
    if (cost > budget && cost > 0) {
        const double scale = budget / cost;
        for (Good g : kLifeSupport) buy[g] *= scale;
        cost = budget;
    }
    double shortfall = 0;
    for (Good g : kLifeSupport) {
        const double need = n * c.goods[g].consumption;
        if (need > 0) shortfall = std::max(shortfall, 1 - buy[g] / need);
    }
    budget -= cost;
    double spent = cost;

    // Contraband, the cheapest happiness (PRD-6), comes next. A lawful population buys it
    // from smugglers (SYS-16), counted as they sell; a pirate planet's from its own market,
    // at the ask, as much as its unhappiness wants up to pirate.demand_cap.
    if (p.pirate && p.stock[kStims] > 0 && n > 0) {
        const double want = std::min(n * c.goods[kStims].consumption * happiness::craving(c, p.happiness), c.pirate.demand_cap * n);
        const double stims_ask = market::ask(c, p, kStims);
        const double q = std::min({want, p.stock[kStims], std::max(0.0, budget) / stims_ask});
        if (q > 0) {
            p.stock[kStims] -= q;
            w.ledger.consumed[kStims] += q;
            budget -= q * stims_ask;
            spent += q * stims_ask;
            p.contraband_today += q;
        }
    }

    // Luxuries (PRD-4): each one's use scales with how full its stock is against its
    // reserve, and falls as the population gets happier; then the budget limits it.
    const double lux_need = c.happy.need * n;
    const double appetite = std::pow(c.event.festival_mult, p.festivals) / (1 + c.happy.sated * std::max(0.0, p.happiness));
    double lux_cost = 0;
    for (Good g : kLuxuries) {
        const double r = market::reserve(c, p, g);
        const double fill = r > 0 ? std::clamp(p.stock[g] / r, 0.0, c.lux.max_fill) : 0;
        buy[g] = std::min(p.stock[g], lux_need * fill * appetite);
        lux_cost += buy[g] * market::ask(c, p, g);
    }
    if (lux_cost > budget && lux_cost > 0) {
        const double scale = std::max(0.0, budget) / lux_cost;
        for (Good g : kLuxuries) buy[g] *= scale;
        lux_cost = std::max(0.0, budget);
    }
    spent += lux_cost;

    for (Good g : kLifeSupport) p.stock[g] -= buy[g], w.ledger.consumed[g] += buy[g];
    for (Good g : kLuxuries) p.stock[g] -= buy[g], w.ledger.consumed[g] += buy[g];
    market::receive(p, spent);
    w.ledger.money_created += spent;

    // Happiness settles toward a target (PRD-4): below neutral with no luxuries, raised
    // by each luxury on a curve of its own (so variety pays) and by contraband, lowered
    // by hunger (PRD-2). No clamp.
    if (n > 0) {
        double target = c.happy.base - c.happy.hunger * std::min(shortfall, 1.0);
        for (Good g : kLuxuries) target += c.happy.lux * std::log1p(buy[g] / lux_need);
        const double contraband = p.contraband_today;
        p.contraband_today = 0;
        target += c.happy.contraband * std::log1p(contraband / lux_need);
        p.happiness += (target - p.happiness) / c.happy.settle_days;
        p.contraband_avg += (contraband / n - p.contraband_avg) / static_cast<double>(c.pop.contraband_window);
    }

    // Growth follows the excess of the scarcest life-support good after buying, up to a
    // full reserve's worth; any shortfall shrinks the population instead (PRD-2).
    if (shortfall > 1e-9) {
        p.population -= n * c.pop.shrink_rate * std::min(shortfall, 1.0);
    } else {
        double excess = 1;
        for (Good g : kLifeSupport) {
            const double r = market::reserve(c, p, g);
            excess = std::min(excess, r > 0 ? (p.stock[g] - r) / r : 0);
        }
        p.population += n * p.growth_factor * happiness::growth(c, p.happiness) * c.pop.growth_rate * std::max(0.0, excess);
    }

    // PRD-5: 1 at neutral, toward a ceiling and the poverty floor; contraband on top (PRD-6).
    p.productivity = happiness::productivity(c, p.happiness) * std::max(0.0, 1 - c.pop.contraband_penalty * p.contraband_avg);

    // Static defence (MIL-5) rebuilds toward what security spending buys, unless besieged;
    // a cut in spending lowers it at once.
    const double max_defence = c.defence.per_spend * p.security;
    if (p.siege == kNone) p.defence = std::min(max_defence, p.defence + c.defence.regen * max_defence);
    if (const Id node = w.gates.empty() ? kNone : gates::held_by(w, id); node != kNone) gates::regen(w, node);

    // The income average, for the credit limit (SYS-3).
    p.income_avg += (p.income_today - p.income_avg) / static_cast<double>(c.time.payment_days);
    p.income_today = 0;
    p.deficit_days = p.treasury < 0 ? p.deficit_days + 1 : 0;
}

void planet_labour(World& w, Id id) {
    const Config& c = w.config;
    Planet& p = w.planets[at(id)];
    if (p.pirate) return;  // its whole labour is on contraband (PIR-3)
    // Score each good by what a unit of labour makes of it at the planet's own bid (bid x
    // efficiency x yield), and move prod.labour_shift of all labour from the worst to the
    // best.
    std::array<double, 3> score{};
    for (int s = 0; s < 3; ++s) {
        const Good g = p.makes[at(s)];
        score[at(s)] = market::bid(c, p, g) * market::efficiency(c, p, s) * c.goods[g].yield;
    }
    int hi = 0, lo = 0;
    for (int s = 1; s < 3; ++s) {
        if (score[at(s)] > score[at(hi)]) hi = s;
        if (score[at(s)] < score[at(lo)]) lo = s;
    }
    if (score[at(hi)] <= score[at(lo)]) return;
    const double move = std::min(c.prod.labour_shift, p.labour[at(lo)]);
    p.labour[at(lo)] -= move;
    p.labour[at(hi)] += move;
}

void planet_payment(World& w, Id id) {
    Planet& p = w.planets[at(id)];
    const double cost = monthly_costs(w.config, p);
    p.treasury -= cost;
    w.ledger.money_sunk += cost;
}

}  // namespace lattice::economy
