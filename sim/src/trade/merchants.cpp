#include "trade/merchants.hpp"

#include <algorithm>
#include <cmath>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "lattice/market.hpp"
#include "politics/fealty.hpp"
#include "pirates/suspicion.hpp"
#include "trade/escorts.hpp"
#include "war/diplomacy.hpp"
#include "war/risk.hpp"

namespace lattice::merchant {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

// One good's market as a merchant pictures it: the price curve of SYS-3 with a stock that
// moves as the merchant trades.
struct Curve {
    double base = 0, k = 1, reserve = 0, stock = 0, half_spread = 0, tax = 0, pmin = 0, pmax = 0;

    double price() const {
        if (reserve <= 0) return base * pmin;
        return base * std::clamp(std::pow(reserve / std::max(stock, reserve * 1e-9), k), pmin, pmax);
    }
    double unit_buy() const { return price() * (1 + half_spread) * (1 + tax); }
    double unit_sell() const { return price() * (1 - half_spread) * (1 - tax); }
};

Curve curve(const Config& c, const Planet& p, Good g) {
    const auto& gp = c.goods[g];
    return {gp.base_price, gp.k, market::reserve(c, p, g), p.stock[g], c.price.spread / 2, p.tax_rate, c.price.min, c.price.max};
}

// From a fact: the reserve is read back off the price curve (stock x (price/base)^(1/k));
// a market seen empty gives no reading, so assume one a few loads deep.
Curve believed(const Config& c, const Planet& p, const PriceFact& f, Good g, double load) {
    const auto& gp = c.goods[g];
    const double stock = f.stock[at(g)];
    double reserve = stock * std::pow(f.price[at(g)] / gp.base_price, 1 / gp.k);
    if (reserve < 1e-6) reserve = 4 * load;
    return {gp.base_price, gp.k, reserve, stock, c.price.spread / 2, p.tax_rate, c.price.min, c.price.max};
}

struct LegChoice {
    Good good = kGoodCount;
    double profit = 0;
};

constexpr int kSteps = 8;

// Buying at `from` and selling at `to`, in kSteps slices, while each slice still pays:
// "the amount before slippage eats the margin" (SYS-7).
double leg_profit(Curve from, Curve to, double capacity, double budget, double risk_rate) {
    const double first = from.unit_buy();
    if (first <= 0 || budget <= 0) return 0;
    const double q_max = std::min(capacity, budget / first);
    if (q_max <= 0) return 0;
    const double step = q_max / kSteps;
    double profit = 0, spent = 0;
    for (int i = 0; i < kSteps; ++i) {
        // risk_cost = cargo value x believed route risk x merchant.loss_rate (SYS-7).
        const double buy = from.unit_buy(), sell = to.unit_sell();
        if (sell <= buy * (1 + risk_rate) || step > from.stock || spent + step * buy > budget) break;
        profit += step * (sell - buy * (1 + risk_rate));
        spent += step * buy;
        from.stock -= step;
        to.stock += step;
    }
    return profit;
}

struct Ctx {
    const World& w;
    const Character& c;
    double capacity, budget, run_cost;
};

Curve curve_at(const Ctx& x, Id planet, Good g, bool actual) {
    const Planet& p = x.w.planets[at(planet)];
    if (actual) return curve(x.w.config, p, g);
    const PriceFact* f = info::find(x.c.facts, planet);
    return f ? believed(x.w.config, p, *f, g, x.capacity) : curve(x.w.config, p, g);
}

double risk_rate(const Ctx& x, Id a, Id b) {
    return risk::believed_route_risk(x.w, x.c.facts, a, b) * x.w.config.merchant.loss_rate;
}

LegChoice best_leg(const Ctx& x, Id a, bool a_actual, Id b) {
    LegChoice best;
    if (!info::find(x.c.facts, b)) return best;
    const double rr = risk_rate(x, a, b);
    for (int gi = 0; gi < kLawfulGoodCount; ++gi) {
        const auto g = static_cast<Good>(gi);
        const double p = leg_profit(curve_at(x, a, g, a_actual), curve_at(x, b, g, false), x.capacity, x.budget, rr);
        if (p > best.profit) best = {g, p};
    }
    return best;
}

// What the cargo aboard would fetch at b, by belief.
double cargo_value(const Ctx& x, Id b) {
    if (x.c.cargo_qty <= 0 || !info::find(x.c.facts, b)) return 0;
    Curve to = curve_at(x, b, x.c.cargo_good, false);
    const double step = x.c.cargo_qty / kSteps;
    double value = 0;
    for (int i = 0; i < kSteps; ++i) {
        value += step * to.unit_sell();
        to.stock += step;
    }
    return value - x.c.cargo_cost * risk_rate(x, x.c.planet, b);
}

// Values a sequence of stops; legs[i] is what to buy at stops[i].
void finish(const Ctx& x, Plan& plan, double profit) {
    double days = 0, fuel = 0;
    for (std::size_t i = 1; i < plan.stops.size(); ++i) {
        days += move::trip_days(x.w, x.c, plan.stops[i - 1].planet, plan.stops[i].planet);
        fuel += move::fuel_cost(x.w, x.c, plan.stops[i - 1].planet, plan.stops[i].planet);
    }
    days += x.w.config.merchant.dock_hours / 24 * static_cast<double>(plan.stops.size() - 1);
    plan.days = std::max(days, 1.0 / 24);
    plan.value = profit - fuel - x.run_cost * plan.days;
    plan.score = plan.value / plan.days;
}

// The candidate set: the merchant.candidates nearest planets it has facts for, by the
// reference travel times (TIM-4).
std::vector<Id> candidates(const World& w, const Character& c, Id from) {
    std::vector<std::pair<double, Id>> known;
    for (const PriceFact& f : c.facts.prices) {
        // Pirate markets are for pirates and smugglers (PIR-5, MS-6 part 2).
        if (f.planet != from && w.planets[static_cast<std::size_t>(f.planet)].pirate == c.pirate) {
            known.emplace_back(w.derived.travel(from, f.planet), f.planet);
        }
    }
    std::sort(known.begin(), known.end());
    std::vector<Id> out;
    for (std::size_t i = 0; i < known.size() && static_cast<std::int64_t>(i) < w.config.merchant.candidates; ++i) {
        out.push_back(known[i].second);
    }
    return out;
}

void consider(Plan& best, const Plan& p) {
    if (p.score > best.score) best = p;
}

}  // namespace

Plan best_trip(const World& w, const Character& c, Id root) {
    Plan best;
    const Id here = c.planet;
    if (here == kNone) return best;
    const ships::Fleet f = ships::fleet(w, c);
    const Ctx x{w, c, f.cargo, std::max(0.0, c.wealth - chars::reserve(w, c)), f.run_cost};
    const std::vector<Id> near = candidates(w, c, here);

    // Cargo already aboard is sold first: a single leg to wherever it fetches most.
    if (c.cargo_qty > 0) {
        for (Id b : near) {
            if (root != kNone && !fealty::same_faction(w, b, root) && !fealty::same_faction(w, here, root)) continue;
            if (root != kNone && info::believes_war(c.facts, root, fealty::root_of(w, b))) continue;
            Plan p{.stops = {{here, kGoodCount}, {b, kGoodCount}}};
            finish(x, p, cargo_value(x, b));
            consider(best, p);
        }
        return best;
    }

    if (root == kNone) {
        // Independent: single trips from where it is (TRD-4).
        for (Id b : near) {
            const LegChoice l = best_leg(x, here, true, b);
            Plan p{.stops = {{here, l.good}, {b, kGoodCount}}};
            finish(x, p, l.profit);
            consider(best, p);
        }
        return best;
    }

    // Faction merchant (TRD-3): trips start and end at a faction planet, and never touch
    // a faction at war with it (TRD-6).
    std::vector<Id> inside, outside;
    for (Id b : near) {
        if (info::believes_war(c.facts, root, fealty::root_of(w, b))) continue;  // as it has heard (DIP-5)
        (fealty::same_faction(w, b, root) ? inside : outside).push_back(b);
    }
    if (!fealty::same_faction(w, here, root)) {
        // Away from home: the leg back in, carrying what pays.
        for (Id b : inside) {
            const LegChoice l = best_leg(x, here, true, b);
            Plan p{.stops = {{here, l.good}, {b, kGoodCount}}};
            finish(x, p, l.profit);
            consider(best, p);
        }
        if (best.stops.empty()) {
            Plan p{.stops = {{here, kGoodCount}, {root, kGoodCount}}};
            finish(x, p, 0);
            best = p;
        }
        return best;
    }
    for (Id b : inside) {  // 1. inside
        const LegChoice l = best_leg(x, here, true, b);
        Plan p{.stops = {{here, l.good}, {b, kGoodCount}}};
        finish(x, p, l.profit);
        consider(best, p);
    }
    for (Id o : outside) {  // 2. out and back
        const LegChoice out = best_leg(x, here, true, o);
        const LegChoice back = best_leg(x, o, false, here);
        Plan p{.stops = {{here, out.good}, {o, back.good}, {here, kGoodCount}}};
        finish(x, p, out.profit + back.profit);
        consider(best, p);
    }
    for (Id o1 : outside) {  // 3. triangle
        const LegChoice first = best_leg(x, here, true, o1);
        for (Id o2 : outside) {
            if (o2 == o1) continue;
            const LegChoice second = best_leg(x, o1, false, o2);
            const LegChoice third = best_leg(x, o2, false, here);
            Plan p{.stops = {{here, first.good}, {o1, second.good}, {o2, third.good}, {here, kGoodCount}}};
            finish(x, p, first.profit + second.profit + third.profit);
            consider(best, p);
        }
    }
    return best;
}

namespace {

// Buys good g at the character's planet for the leg to `next`, as much as still pays by
// belief.
void buy_for_leg(World& w, Id id, Good g, Id next, Observer*) {
    Character& c = w.characters[at(id)];
    if (g == kGoodCount || next == kNone) return;
    if (c.cargo_qty > 0 && c.cargo_good != g) return;  // one good aboard at a time
    const ships::Fleet f = ships::fleet(w, c);
    const Ctx x{w, c, f.cargo, std::max(0.0, c.wealth - chars::reserve(w, c)), f.run_cost};
    Curve from = curve_at(x, c.planet, g, true);
    const Curve to = curve_at(x, next, g, false);
    if (from.unit_buy() >= to.unit_sell()) return;
    // The largest amount whose last slice still pays, as leg_profit counts it.
    const double first = from.unit_buy();
    const double q_max = std::min(x.capacity, x.budget / first);
    const double step = q_max / kSteps;
    double qty = 0;
    Curve sell = to;
    for (int i = 0; i < kSteps; ++i) {
        if (sell.unit_sell() <= from.unit_buy() || step > from.stock) break;
        qty += step;
        from.stock -= step;
        sell.stock += step;
    }
    if (qty <= 0) return;
    const market::Fill fill = market::buy(w.planets[at(c.planet)], w.config, g, qty, x.budget, to.unit_sell());
    if (fill.qty <= 0) return;
    c.wealth -= fill.money;
    c.cargo_good = g;
    c.cargo_qty += fill.qty;
    c.cargo_cost += fill.money;
    w.ledger.traded[g] += fill.qty;
    suspicion::add(w, c, fealty::root_of(w, c.planet), -w.config.sus.trade);  // genuine trade (INF-14)
}

void sell_cargo(World& w, Id id) {
    Character& c = w.characters[at(id)];
    if (c.cargo_qty <= 0) return;
    const market::Fill fill = market::sell(w.planets[at(c.planet)], w.config, c.cargo_good, c.cargo_qty, 0);
    if (fill.qty <= 0) return;
    const double cost = c.cargo_cost * fill.qty / c.cargo_qty;
    c.trip_profit += fill.money - cost;
    chars::earn(c, fill.money, false);
    // Only the margin counts toward what pays (SYS-8): undo the cost part of earn's average.
    c.earned_cargo -= cost;
    c.cargo_qty -= fill.qty;
    c.cargo_cost -= cost;
    if (c.cargo_qty <= 1e-9) c.cargo_qty = 0, c.cargo_cost = 0, c.cargo_good = kGoodCount;
}

void end_trip(World& w, Id id) {
    escorts::finish(w, id, nullptr);
    Character& c = w.characters[at(id)];
    w.ledger.trade_profit += c.trip_profit;
    ++w.ledger.trips;
    c.trip.clear();
    c.trip_profit = 0;
    c.activity = Activity::Idle;
    c.idle_since = w.now;
}

}  // namespace

void start(World& w, Id id, const Plan& plan, Observer* observer) {
    Character& c = w.characters[at(id)];
    c.trip = plan.stops;
    c.trip_profit = 0;
    c.trip_start = w.now;
    c.activity = Activity::Trading;
    at_stop(w, id, observer);
}

void at_stop(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    // A trip that ends here waits out the stop's docking time before the next decision,
    // so no plan can repeat within an instant.
    const Time docked = w.now + static_cast<Time>(w.config.merchant.dock_hours * kHour) + 1;
    if (c.trip.empty() || c.trip.front().planet != c.planet) {
        // Not where the plan expected (it was replanned elsewhere): end the trip here.
        sell_cargo(w, id);
        end_trip(w, id);
        chars::wake(w, id, docked);
        return;
    }
    sell_cargo(w, id);
    const Good g = c.trip.front().buy;
    c.trip.erase(c.trip.begin());
    // A faction merchant bound for an enemy turns for home instead (TRD-6).
    const Id employer = employer_root(w, id);
    if (!c.trip.empty() && employer != kNone && info::believes_war(c.facts, employer, fealty::root_of(w, c.trip.front().planet))) {
        c.trip.clear();
    }
    if (c.trip.empty()) {
        end_trip(w, id);
        chars::wake(w, id, docked);
        return;
    }
    buy_for_leg(w, id, g, c.trip.front().planet, observer);
    escorts::consider(w, id, observer);
    if (!move::travel_to(w, id, c.trip.front().planet)) {
        end_trip(w, id);
        chars::wake(w, id, w.now + kDay);
    }
}

void buy_for(World& w, Id id, Id next) {
    const Character& c = w.characters[at(id)];
    if (c.planet == kNone || c.cargo_qty > 0) return;
    const ships::Fleet f = ships::fleet(w, c);
    const Ctx x{w, c, f.cargo, std::max(0.0, c.wealth - chars::reserve(w, c)), f.run_cost};
    const LegChoice leg = best_leg(x, c.planet, true, next);
    if (leg.good != kGoodCount) buy_for_leg(w, id, leg.good, next, nullptr);
}

void sell_here(World& w, Id id) { sell_cargo(w, id); }

Id explore_target(const World& w, const Character& c) {
    if (c.planet == kNone) return kNone;
    const double stale = w.config.merchant.stale_days * kDay;
    Id best = kNone;
    double best_days = 0;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (p == c.planet || w.planets[at(p)].pirate != c.pirate) continue;
        const PriceFact* f = info::find(c.facts, p);
        if (f && static_cast<double>(w.now - f->time) < stale) continue;
        const double d = w.derived.travel(c.planet, p);
        if (best == kNone || d < best_days) best = p, best_days = d;
    }
    return best;
}

Id employer_root(const World& w, Id id) {
    const Id k = contracts::held(w, id, ContractKind::MerchantSalary);
    if (k == kNone) return kNone;
    const Id governs = w.characters[at(w.contracts[at(k)].employer)].governs;
    return governs == kNone ? kNone : fealty::root_of(w, governs);
}

void consider_salary(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    if (c.planet == kNone || c.governs != kNone || c.ships.empty()) return;
    // Weighed at most once a payment period: the comparison plans every trip twice.
    if (w.now - c.salary_checked < w.config.time.payment_days * kDay) return;
    const auto& m = w.config.merchant;
    const double offer_per_day = (m.salary + m.info_value) / static_cast<double>(w.config.time.payment_days);
    const Id employer = employer_root(w, id);
    if (employer != kNone) {
        c.salary_checked = w.now;
        const double s_i = best_trip(w, c, kNone).score, s_f = best_trip(w, c, employer).score;
        if (std::isfinite(s_i) && (!std::isfinite(s_f) || s_i - s_f > offer_per_day)) {
            contracts::end(w, contracts::held(w, id, ContractKind::MerchantSalary));
            if (observer) observer->log(w.now, "merchant.quit", id, "root " + std::to_string(employer));
        }
        return;
    }
    if (m.salary <= 0 || c.pirate) return;
    const Id root = fealty::root_of(w, c.planet);
    if (w.planets[at(root)].pirate) return;  // the pirates have no merchants (PIR-1)
    const Id root_governor = w.planets[at(root)].governor;
    int hired = 0;
    for (Id k : w.characters[at(root_governor)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        hired += ct.alive && ct.kind == ContractKind::MerchantSalary && ct.employer == root_governor;
    }
    const double wanted = m.per_planet * static_cast<double>(fealty::faction_planets(w, root).size());
    if (hired >= wanted) return;
    c.salary_checked = w.now;
    const double s_i = best_trip(w, c, kNone).score, s_f = best_trip(w, c, root).score;
    if (!std::isfinite(s_f)) return;
    if (offer_per_day + s_f >= (std::isfinite(s_i) ? s_i : 0)) {
        contracts::sign(w, ContractKind::MerchantSalary, root_governor, id, m.salary, w.config.time.payment_days * kDay);
        if (observer) observer->log(w.now, "merchant.hired", id, "root " + std::to_string(root));
    }
}

void pay_salaries(World& w, Id root, Observer*) {
    const Id governor = w.planets[at(root)].governor;
    if (governor == kNone) return;
    Planet& p = w.planets[at(root)];
    const std::vector<Id> held = w.characters[at(governor)].contracts;
    for (Id k : held) {
        const Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.kind != ContractKind::MerchantSalary || ct.employer != governor) continue;
        p.treasury -= ct.pay;
        chars::earn(w.characters[at(ct.employee)], ct.pay, false);
        w.ledger.salaries += ct.pay;
    }
}

}  // namespace lattice::merchant
