#include "pirates/smuggling.hpp"

#include <algorithm>
#include <limits>

#include "chars/characters.hpp"
#include "chars/missions.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "lattice/happiness.hpp"
#include "lattice/market.hpp"
#include "pirates/pirates.hpp"
#include "pirates/suspicion.hpp"

namespace lattice::smuggling {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

bool open_for(const World& w, Id planet, MissionKind kind, Good g) {
    for (Id m : w.planets[at(planet)].missions) {
        const Mission& mi = w.missions[at(m)];
        if (mi.kind == kind && mi.good == g && mi.state == MissionState::Open) return true;
    }
    return false;
}

Id post_run(World& w, Id planet, MissionKind kind, Good g, double qty, Observer* observer) {
    const Planet& p = w.planets[at(planet)];
    Mission m{.kind = kind, .poster = p.governor, .at = planet, .target = planet,
              .base_pay = kind == MissionKind::LegalGoods ? w.config.pirate.legal_bonus * qty : 0,
              .min_cargo = 1, .posted = w.now, .expires = w.now + w.config.mission.expiry_days * kDay,
              .good = g, .qty = qty};
    w.missions.push_back(std::move(m));
    const auto id = static_cast<Id>(w.missions.size() - 1);
    w.planets[at(planet)].missions.push_back(id);
    w.events.push({.time = w.missions[at(id)].expires, .kind = EventKind::MissionExpire, .record = id});
    ++w.ledger.missions_posted;
    if (observer) {
        observer->log(w.now, kind == MissionKind::Smuggle ? "pirate.smuggling_run" : "pirate.legal_run", id,
                      std::string(good_name(g)) + " " + std::to_string(static_cast<long long>(qty)));
    }
    return id;
}

void finish(World& w, Id id, bool success, Observer* observer) {
    Character& c = w.characters[at(id)];
    if (!success) {
        if (observer) observer->log(w.now, "pirate.run_failed", c.mission, "by " + std::to_string(id));
        missions::fail(w, c.mission, observer);  // PIR-10, MSN-3
        chars::wake(w, id, w.now + kHour);
        return;
    }
    Mission& m = w.missions[at(c.mission)];
    m.state = MissionState::Done;
    pirates::set_relation(w, c, pirates::relation(w, c) + w.config.pirel.success);
    auto& skill = c.skills[skill_of(m.kind)];
    skill = std::min(w.config.skill.max, skill + w.config.skill.gain);
    ++c.missions_done;
    ++w.ledger.missions_done;
    ++w.ledger.issuer_done[static_cast<std::size_t>(Issuer::PiratePlanet)];
    ++(m.kind == MissionKind::Smuggle ? w.ledger.smuggled_runs : w.ledger.legal_runs);
    if (observer) observer->log(w.now, "pirate.run_done", c.mission, "by " + std::to_string(id));
    c.mission = kNone;
    c.activity = Activity::Idle;
    c.idle_since = w.now;
    chars::wake(w, id, w.now + kHour);
}

// The lawful market a character believes buys this good cheapest, among those it knows.
Id cheapest_source(const World& w, const Character& c, Good g) {
    Id best = kNone;
    double best_price = std::numeric_limits<double>::max();
    for (const PriceFact& f : c.facts.prices) {
        if (w.planets[at(f.planet)].pirate || f.stock[at(g)] <= 0) continue;
        const double price = f.price[at(g)] * (1 + w.config.merchant.loss_rate * w.derived.travel(c.planet, f.planet));
        if (price < best_price) best_price = price, best = f.planet;
    }
    return best;
}

// Where to sell contraband: the most populous lawful market it knows, near (PRD-6: the
// unhappy want it most, but happiness isn't something it can see).
Id best_market(const World& w, const Character& c, Id from) {
    Id best = kNone;
    double best_score = 0;
    for (const PriceFact& f : c.facts.prices) {
        const Planet& p = w.planets[at(f.planet)];
        if (p.pirate || f.planet == from) continue;
        const double score = p.population / (1 + w.derived.travel(from, f.planet));
        if (score > best_score) best_score = score, best = f.planet;
    }
    return best;
}

void buy_here(World& w, Id id, Good g, double qty) {
    Character& c = w.characters[at(id)];
    const double room = ships::fleet(w, c).cargo - (c.cargo_good == g ? c.cargo_qty : 0);
    if (room <= 0 || (c.cargo_qty > 0 && c.cargo_good != g)) return;
    const market::Fill f = market::buy(w.planets[at(c.planet)], w.config, g, std::min(qty, room),
                                       std::max(0.0, c.wealth - chars::reserve(w, c)), std::numeric_limits<double>::max());
    if (f.qty <= 0) return;
    c.wealth -= f.money;
    c.cargo_good = g;
    c.cargo_qty += f.qty;
    c.cargo_cost += f.money;
}

}  // namespace

void post(World& w, Id planet, Observer* observer) {
    const Planet& p = w.planets[at(planet)];
    if (!p.pirate) return;
    const auto& c = w.config;
    for (Good g : kLifeSupport) {
        const double r = market::reserve(c, p, g);
        if (r <= 0 || p.stock[g] / r >= c.pirate.need_fill || open_for(w, planet, MissionKind::LegalGoods, g)) continue;
        post_run(w, planet, MissionKind::LegalGoods, g, r - p.stock[g], observer);
    }
    const double r = market::reserve(c, p, kStims);
    if (r > 0 && p.stock[kStims] / r > c.pirate.surplus_fill && !open_for(w, planet, MissionKind::Smuggle, kStims)) {
        post_run(w, planet, MissionKind::Smuggle, kStims, p.stock[kStims] - r, observer);
    }
}

bool qualifies(const World& w, Id id, const Mission& m) {
    const Character& c = w.characters[at(id)];
    if (c.pirate || c.governs != kNone || c.ships.empty() || c.cargo_qty > 0 || c.mission != kNone) return false;
    if (chars::faction_of(w, c) != kNone) return false;  // independents (PIR-13)
    if (ships::fleet(w, c).cargo < m.min_cargo) return false;
    return c.risk > w.config.pirate.smuggler_risk || pirates::relation(w, c) >= w.config.pirel.accept;
}

bool offer(World& w, Id pirate, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    if (c.offer != kNone || c.risk <= w.config.pirate.smuggler_risk || c.pirate || c.governs != kNone) return false;
    if (chars::faction_of(w, c) != kNone || ships::fleet(w, c).cargo < 1) return false;
    const Id home = w.characters[at(pirate)].governs != kNone ? w.characters[at(pirate)].governs
                                                              : w.characters[at(pirate)].last_planet;
    if (home == kNone || !w.planets[at(home)].pirate) return false;
    Id best = kNone;
    double best_pay = -1;
    for (Id m : w.planets[at(home)].missions) {
        const Mission& mi = w.missions[at(m)];
        if (mi.state != MissionState::Open || (mi.kind != MissionKind::Smuggle && mi.kind != MissionKind::LegalGoods)) continue;
        if (mi.base_pay > best_pay) best_pay = mi.base_pay, best = m;
    }
    if (best == kNone) return false;
    c.offer = best;
    pirates::set_relation(w, c, std::max(pirates::relation(w, c), w.config.pirel.accept));  // PIR-10
    ++w.ledger.offers;
    if (observer) observer->log(w.now, "pirate.offer", id, "run " + std::to_string(best));
    return true;
}

void take(World& w, Id id, Id mid, Observer* observer) {
    Mission& m = w.missions[at(mid)];
    Character& c = w.characters[at(id)];
    c.offer = kNone;
    if (m.state != MissionState::Open || !qualifies(w, id, m)) return;
    m.state = MissionState::Taken;
    m.taker = id;
    m.pay = m.base_pay;
    auto& board = w.planets[at(m.at)].missions;
    board.erase(std::remove(board.begin(), board.end(), mid), board.end());
    c.mission = mid;
    c.activity = Activity::Mission;
    pirates::set_relation(w, c, std::max(pirates::relation(w, c), w.config.pirel.accept));
    missions::start_clock(w, mid);
    if (observer) observer->log(w.now, "pirate.run_taken", mid, "by " + std::to_string(id));
    // First stop: the haven for contraband, a lawful market for the legal good.
    Id first = m.at;
    if (m.kind == MissionKind::LegalGoods) {
        first = cheapest_source(w, c, m.good);
        if (first == kNone) first = c.planet;
    }
    m.target = first;
    if (c.planet == first) {
        landed(w, id, observer);
        return;
    }
    if (!move::travel_to(w, id, first)) finish(w, id, false, observer);
}

double sell_to_population(World& w, Id id) {
    Character& c = w.characters[at(id)];
    Planet& p = w.planets[at(c.planet)];
    if (c.cargo_good != kStims || c.cargo_qty <= 0 || p.pirate) return 0;
    const auto& cfg = w.config;
    const double want = std::min(p.population * cfg.goods[kStims].consumption * happiness::craving(cfg, p.happiness), cfg.pirate.demand_cap * p.population);
    const double q = std::min(c.cargo_qty, std::max(0.0, want - p.contraband_today));
    if (q <= 0) return 0;
    const double revenue = q * cfg.goods[kStims].base_price * cfg.pirate.contraband_price;
    // Money from the population's purchasing power (ECO-1), untaxed and past the treasury (PRD-6).
    w.ledger.money_created += revenue;
    w.ledger.consumed[kStims] += q;
    w.ledger.smuggled += q;
    w.ledger.smuggler_profit += revenue - c.cargo_cost * q / c.cargo_qty;
    p.contraband_today += q;
    c.cargo_cost -= c.cargo_cost * q / c.cargo_qty;
    c.cargo_qty -= q;
    chars::earn(c, revenue, false);
    if (c.cargo_qty <= 1e-9) c.cargo_qty = 0, c.cargo_cost = 0, c.cargo_good = kGoodCount;
    return q;
}

void landed(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    Mission& m = w.missions[at(c.mission)];
    if (m.kind == MissionKind::Smuggle) {
        if (c.planet == m.at) {
            // Load the contraband (PIR-6: smugglers buy from the pirate planet).
            buy_here(w, id, kStims, m.qty);
            const Id market = best_market(w, c, c.planet);
            if (c.cargo_qty <= 0 || market == kNone || !move::travel_to(w, id, market)) {
                finish(w, id, false, observer);
                return;
            }
            m.target = market;
            return;
        }
        if (c.cargo_good != kStims || c.cargo_qty <= 0) {
            finish(w, id, false, observer);  // caught on landing
            return;
        }
        sell_to_population(w, id);
        if (c.cargo_qty <= 0) {
            finish(w, id, true, observer);
            return;
        }
        // Not all of it sold here: on to another market, or dump the rest.
        const Id next = best_market(w, c, c.planet);
        if (next != kNone && next != m.target && move::travel_to(w, id, next)) {
            m.target = next;
            return;
        }
        w.ledger.destroyed[kStims] += c.cargo_qty;
        c.cargo_qty = 0, c.cargo_cost = 0, c.cargo_good = kGoodCount;
        finish(w, id, true, observer);
        return;
    }
    // Legal goods.
    if (c.planet != m.at) {
        buy_here(w, id, m.good, m.qty);
        if (c.cargo_qty <= 0 || !move::travel_to(w, id, m.at)) {
            finish(w, id, false, observer);
            return;
        }
        return;
    }
    // At the haven: sell into its market, and the bonus (PIR-3: its life support comes from
    // these runs).
    Planet& haven = w.planets[at(m.at)];
    const market::Fill f = market::sell(haven, w.config, c.cargo_good, c.cargo_qty, 0);
    c.wealth += f.money;
    const double bonus = w.config.pirate.legal_bonus * f.qty;
    haven.treasury -= bonus;
    w.ledger.issuer_spend[static_cast<std::size_t>(Issuer::PiratePlanet)] += bonus;
    chars::earn(c, bonus, false);
    c.cargo_qty -= f.qty;
    c.cargo_cost = 0;
    if (c.cargo_qty <= 1e-9) c.cargo_qty = 0, c.cargo_good = kGoodCount;
    finish(w, id, f.qty > 0, observer);
}

}  // namespace lattice::smuggling
