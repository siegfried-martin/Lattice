#include "chars/missions.hpp"

#include <algorithm>
#include <limits>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "lattice/market.hpp"
#include "pirates/pirates.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"
#include "war/diplomacy.hpp"
#include "war/raids.hpp"
#include "war/threats.hpp"

namespace lattice::missions {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

Id post(World& w, Mission m, Observer* observer) {
    w.missions.push_back(std::move(m));
    const auto id = static_cast<Id>(w.missions.size() - 1);
    const Mission& posted = w.missions.back();
    w.planets[at(posted.at)].missions.push_back(id);
    w.events.push({.time = posted.expires, .kind = EventKind::MissionExpire, .record = id});
    ++w.ledger.missions_posted;
    if (observer) {
        constexpr const char* kinds[] = {"courier", "scout", "trade", "escort", "command", "smuggle", "security", "legal", "passenger",
                                         "order", "supply"};
        observer->log(w.now, "mission.posted", id,
                      std::string(kinds[static_cast<std::size_t>(posted.kind)]) + " at " + std::to_string(posted.at) + " to " +
                          std::to_string(posted.target));
    }
    return id;
}

void unpost(World& w, Id id) {
    auto& board = w.planets[at(w.missions[at(id)].at)].missions;
    board.erase(std::remove(board.begin(), board.end(), id), board.end());
}

double round_trip(const World& w, Id a, Id b) { return w.derived.travel(a, b) + w.derived.travel(b, a); }

ContractKind contract_kind(MissionKind k) {
    switch (k) {
        case MissionKind::Courier: return ContractKind::Courier;
        case MissionKind::Security: return ContractKind::Security;
        default: return ContractKind::Scout;
    }
}

bool pirate_run(const Mission& m) { return m.kind == MissionKind::Smuggle || m.kind == MissionKind::LegalGoods; }
// A passenger run (MSN-13): one way, no issuer, paid as free money.
bool passenger(const Mission& m) { return m.kind == MissionKind::Passenger; }
// A supply order's courier leg and its delivery (MSN-15): one way, paid by the planet
// that ordered, with no one-job contract.
bool order(const Mission& m) { return m.kind == MissionKind::Order; }
bool supply(const Mission& m) { return m.kind == MissionKind::Supply; }
bool one_way(const Mission& m) { return passenger(m) || order(m) || supply(m); }
bool contracted(const Mission& m) { return !pirate_run(m) && !passenger(m) && !order(m) && !supply(m); }

constexpr double kNoRoute = std::numeric_limits<double>::max();

// Days between planets for any hull: in open space if it can be, else by the Lattice.
double any_hull_days(const World& w, Id a, Id b) {
    const double open = w.derived.travel(a, b, false);
    return open < kNoRoute ? open : w.derived.travel(a, b);
}

// The planet that ordered: an order's courier leaves from it, a delivery goes to it.
Id orderer(const Mission& m) { return order(m) ? m.at : m.target; }

// Units ordered and no longer coming (MSN-15).
void unorder(World& w, const Mission& m, double qty) {
    double& o = w.planets[at(orderer(m))].on_order[m.good];
    o = std::max(0.0, o - qty);
}

// The days a mission should take, for its deadline (MSN-3): the reference round trip and
// any time on station. A pirate run's route isn't known when it's taken, so it gets
// mission.expiry_days.
double reference_days(const World& w, const Mission& m) {
    if (pirate_run(m)) return static_cast<double>(w.config.mission.expiry_days);
    if (passenger(m)) return std::max(1.0, w.derived.travel(m.at, m.target, false));  // any hull can fly it
    if (order(m) || supply(m)) return std::max(1.0, any_hull_days(w, m.at, m.target));
    double hold = 0;
    if (m.kind == MissionKind::Security) {
        hold = static_cast<double>(m.authority ? w.config.authority.clear_days : w.config.security.days);
    } else if (m.sortie) {
        hold = 0.5;
    }
    return std::max(1.0, round_trip(w, m.at, m.target) + hold);
}

}  // namespace

Issuer issuer_of(const World& w, const Mission& m) {
    if (m.authority) return Issuer::Authority;
    const Id home = supply(m) ? m.target : m.at;  // a delivery's issuer is the planet it goes to
    if (pirate_run(m) || w.planets[at(home)].pirate) return Issuer::PiratePlanet;
    return w.planets[at(home)].liege == kNone ? Issuer::Root : Issuer::Governor;
}

void start_clock(World& w, Id mid) {
    Mission& m = w.missions[at(mid)];
    m.deadline = w.now + static_cast<Time>(w.config.mission.deadline_mult * reference_days(w, m) * kDay);
    w.events.push({.time = m.deadline, .kind = EventKind::MissionDeadline, .record = mid});
    if (passenger(m)) return;  // no issuer to count it for
    ++w.ledger.issuer_taken[at(static_cast<Id>(issuer_of(w, m)))];
}

void fail(World& w, Id mid, Observer* observer) {
    Mission& m = w.missions[at(mid)];
    if (m.state != MissionState::Taken) return;
    m.state = MissionState::Failed;
    m.packet = {};
    const Id id = m.taker;
    Character& c = w.characters[at(id)];
    const Issuer who = issuer_of(w, m);
    if (!passenger(m)) ++w.ledger.issuer_failed[at(static_cast<Id>(who))];
    // A bad deed with the issuer; reputation is untouched (MSN-3). A passenger run has no
    // issuer to hold it against the taker.
    const double bad = w.config.deed.amount[static_cast<std::size_t>(DeedKind::MissionFailed)];
    if (passenger(m)) {
    } else if (who == Issuer::Authority) {
        c.authority_opinion += bad;
    } else if (who == Issuer::PiratePlanet) {
        pirates::set_relation(w, c, std::min(pirates::relation(w, c), w.config.pirel.fail));  // PIR-10
        if (m.kind == MissionKind::Smuggle && c.cargo_good == kStims) {
            w.ledger.destroyed[kStims] += c.cargo_qty;  // dumped: no lawful market takes it openly
            c.cargo_qty = 0, c.cargo_cost = 0, c.cargo_good = kGoodCount;
        }
    } else {
        opinion::deed(w, m.poster, id, DeedKind::MissionFailed, observer);
    }
    if (contracted(m)) contracts::end(w, contracts::held(w, id, contract_kind(m.kind)));
    if (order(m) || supply(m)) unorder(w, m, m.qty);  // the planet orders again (MSN-15)
    if (supply(m)) ++w.ledger.supplies_failed;
    if (observer) observer->log(w.now, "mission.failed", mid, "by " + std::to_string(id));
    if (c.mission != mid) return;
    if (supply(m)) {  // a merchant's trip goes on without it
        c.mission = kNone;
        return;
    }
    c.mission = kNone;
    if (c.activity == Activity::Mission || c.activity == Activity::Patrolling) c.activity = Activity::Idle;
    if (c.alive && c.planet != kNone) {
        c.idle_since = w.now;
        chars::wake(w, id, w.now);
    }
}

void deadline(World& w, Id mid, Observer* observer) {
    const Mission& m = w.missions[at(mid)];
    if (m.state == MissionState::Taken && w.now >= m.deadline) fail(w, mid, observer);
}

void post_courier(World& w, Id liege, Id vassal, Observer* observer) {
    const auto& c = w.config;
    post(w,
         {.kind = MissionKind::Courier,
          .level = MissionLevel::Planet,
          .poster = w.planets[at(liege)].governor,
          .at = liege,
          .target = vassal,
          .base_pay = c.mission.courier_pay * round_trip(w, liege, vassal),
          .posted = w.now,
          .expires = w.now + c.courier.interval_days * kDay},
         observer);
}

// A faction scouts for trade: prices in neutral territory no single planet would pay to
// learn (INF-9). The root posts a run to the nearest outside market it knows least
// about. *(proposed)* Posted as a planet-level mission at the root's own planet, so open
// to anyone, since peacetime has no members to take faction-level work.
void root_weekly(World& w, Id root, Observer* observer) {
    const Planet& planet = w.planets[at(root)];
    if (planet.liege != kNone || planet.governor == kNone) return;
    for (Id m : planet.missions) {
        if (w.missions[at(m)].kind == MissionKind::Scout) return;  // one at a time
    }
    const Facts& known = info::planet_facts(w, root);
    const double stale = w.config.scout.stale_days * kDay;
    Id target = kNone;
    double best = 0;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (fealty::same_faction(w, p, root)) continue;
        const PriceFact* f = info::find(known, p);
        if (f && static_cast<double>(w.now - f->time) < stale) continue;
        const double d = w.derived.travel(root, p);
        if (target == kNone || d < best) target = p, best = d;
    }
    if (target == kNone) return;
    const auto& c = w.config;
    post(w,
         {.kind = MissionKind::Scout,
          .level = MissionLevel::Planet,
          .poster = planet.governor,
          .at = root,
          .target = target,
          .base_pay = c.mission.scout_pay * round_trip(w, root, target),
          .min_combat = c.scout.min_combat,
          .posted = w.now,
          .expires = w.now + c.mission.expiry_days * kDay},
         observer);
}

namespace {

// Where a passenger run from a planet may go: short hops in open space to a lawful
// planet, so a hull without a Lattice Drive can fly them (TRD-18), or, with lattice,
// within the same days by the Lattice.
std::vector<Id> passenger_targets(const World& w, Id planet, bool lattice) {
    std::vector<Id> near;
    for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
        if (q != planet && !w.planets[at(q)].pirate && w.derived.travel(planet, q, lattice) <= w.config.passenger.max_days) near.push_back(q);
    }
    return near;
}

// extra: what the fare adds on top of passenger.fare a day (an errand's fuel, MSN-14).
Id post_passenger(World& w, Id planet, const std::vector<Id>& near, bool lattice, Observer* observer,
                  const Character* flier = nullptr) {
    const auto& c = w.config.passenger;
    const Id to = near[at(static_cast<Id>(w.rng[Stream::Missions].uniform_int(0, near.size() - 1)))];
    const double fuel = flier != nullptr ? move::fuel_cost(w, *flier, planet, to) : 0;
    return post(w,
                {.kind = MissionKind::Passenger, .level = MissionLevel::Planet, .at = planet, .target = to,
                 .base_pay = c.fare * std::max(0.5, w.derived.travel(planet, to, lattice)) + fuel, .posted = w.now,
                 .expires = w.now + c.expiry_days * kDay},
                observer);
}

}  // namespace

void post_passengers(World& w, Id planet, Observer* observer) {
    if (w.planets[at(planet)].pirate) return;
    std::int64_t open = 0;
    for (Id m : w.planets[at(planet)].missions) open += passenger(w.missions[at(m)]);
    if (open >= w.config.passenger.per_planet) return;
    const std::vector<Id> near = passenger_targets(w, planet, false);
    if (near.empty()) return;
    for (; open < w.config.passenger.per_planet; ++open) post_passenger(w, planet, near, false, observer);
}

Id post_errand(World& w, Id planet, Id flier, Observer* observer) {
    if (w.planets[at(planet)].siege != kNone) return kNone;
    // In open space if it can be; else, for a hull with a Lattice Drive, by the Lattice,
    // the fare covering the fuel; else the nearest lawful planet it can reach at all, so
    // no one is stranded (a pirate planet posts them too, for the lawful).
    const Character& c = w.characters[at(flier)];
    std::vector<Id> near = passenger_targets(w, planet, false);
    bool lattice = false;
    if (near.empty() && ships::fleet(w, c).lattice_drive) {
        lattice = true;
        near = passenger_targets(w, planet, true);
    }
    if (near.empty()) {
        Id best = kNone;
        for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
            if (q == planet || w.planets[at(q)].pirate) continue;
            if (best == kNone || w.derived.travel(planet, q, lattice) < w.derived.travel(planet, best, lattice)) best = q;
        }
        if (best == kNone || w.derived.travel(planet, best, lattice) >= std::numeric_limits<double>::max()) return kNone;  // no route
        near = {best};
    }
    ++w.ledger.errands;
    return post_passenger(w, planet, near, lattice, observer, lattice ? &c : nullptr);
}

namespace {

// The nearest planet a short planet has heard has a surplus of g (a price below base),
// lawful, not under siege and not at war with it, that some hull can reach (MSN-15).
Id order_source(const World& w, Id planet, Good g) {
    const Id root = fealty::root_of(w, planet);
    Id best = kNone;
    double best_days = 0;
    for (const PriceFact& f : info::planet_facts(w, planet).prices) {
        const Id q = f.planet;
        const Planet& s = w.planets[at(q)];
        if (q == planet || s.pirate || s.siege != kNone || diplomacy::at_war(w, root, fealty::root_of(w, q))) continue;
        if (f.price[g] >= w.config.goods[g].base_price) continue;
        const double d = any_hull_days(w, planet, q);
        if (d >= kNoRoute) continue;
        if (best == kNone || d < best_days) best = q, best_days = d;
    }
    return best;
}

// The order is at the producer, carried or not: its board posts the delivery.
void post_supply(World& w, Id order_id, Observer* observer) {
    const Mission o = w.missions[at(order_id)];
    post(w,
         {.kind = MissionKind::Supply, .level = MissionLevel::Planet, .poster = o.poster, .at = o.target, .target = o.at,
          .base_pay = o.qty * o.unit, .posted = w.now, .expires = w.now + w.config.order.supply_days * kDay, .good = o.good,
          .qty = o.qty, .unit = o.unit},
         observer);
}

// No one took the courier leg in time, or the one who did couldn't go: the order arrives
// anyway, with no one paid to carry it (MSN-15).
void order_arrives(World& w, Id order_id, Observer* observer) {
    w.missions[at(order_id)].state = MissionState::Expired;
    ++w.ledger.orders_auto;
    post_supply(w, order_id, observer);
}

}  // namespace

void post_orders(World& w, Id planet, Observer* observer) {
    Planet& p = w.planets[at(planet)];
    const auto& c = w.config;
    if (p.pirate || p.governor == kNone || p.siege != kNone || !market::can_buy(c, p)) return;
    for (Good g : kLifeSupport) {
        const double r = market::reserve(c, p, g);
        if (r <= 0 || p.stock[g] + p.on_order[g] >= c.order.trigger_fill * r) continue;
        const Id from = order_source(w, planet, g);
        if (from == kNone) continue;
        const double qty = r - p.stock[g] - p.on_order[g];
        const double days = any_hull_days(w, planet, from);
        p.on_order[g] += qty;
        ++w.ledger.orders;
        post(w,
             {.kind = MissionKind::Order, .level = MissionLevel::Planet, .poster = p.governor, .at = planet, .target = from,
              .base_pay = c.order.courier_pay * std::max(0.5, days), .posted = w.now,
              .expires = w.now + static_cast<Time>((c.order.courier_wait_days + days) * kDay), .good = g, .qty = qty,
              .unit = c.order.price_share * market::price(c, p, g)},
             observer);
    }
}

Id take_supply(World& w, Id id, Id mid, double qty, Observer* observer) {
    if (w.missions[at(mid)].state != MissionState::Open || qty <= 0) return kNone;
    if (qty < w.missions[at(mid)].qty - 1e-9) {
        // It carries what its hold takes; the rest stays posted for the next merchant.
        Mission rest = w.missions[at(mid)];
        rest.qty -= qty;
        rest.base_pay = rest.qty * rest.unit;
        post(w, rest, observer);
        w.missions[at(mid)].qty = qty;
        w.missions[at(mid)].base_pay = qty * w.missions[at(mid)].unit;
    }
    Mission& m = w.missions[at(mid)];
    m.state = MissionState::Taken;
    m.taker = id;
    m.pay = m.base_pay;
    unpost(w, mid);
    w.characters[at(id)].mission = mid;
    if (observer) observer->log(w.now, "mission.taken", mid, "by " + std::to_string(id));
    start_clock(w, mid);
    return mid;
}

void supply_bought(World& w, Id id, double qty) {
    Character& c = w.characters[at(id)];
    Mission& m = w.missions[at(c.mission)];
    if (qty >= m.qty - 1e-9) return;
    unorder(w, m, m.qty - qty);
    m.qty = qty;
    m.pay = qty * m.unit;
    if (qty > 0) return;
    m.state = MissionState::Expired;  // nothing to be had at its price: given back
    c.mission = kNone;
}

void supply_done(World& w, Id id, double qty, double money, Observer* observer) {
    Character& c = w.characters[at(id)];
    const Id mid = c.mission;
    Mission& m = w.missions[at(mid)];
    unorder(w, m, m.qty);
    m.state = MissionState::Done;
    ++w.ledger.supplies;
    w.ledger.supply_qty += qty;
    w.ledger.supply_paid += money;
    ++w.ledger.issuer_done[at(static_cast<Id>(issuer_of(w, m)))];
    auto& skill = c.skills[skill_of(m.kind)];
    skill = std::min(w.config.skill.max, skill + w.config.skill.gain);
    ++c.missions_done;
    c.reputation += 1;  // REL-7
    opinion::deed(w, m.poster, id, DeedKind::JobDone, observer);
    const Id root = fealty::root_of(w, m.target);
    if (root >= 0 && !w.planets[at(root)].pirate && w.planets[at(root)].governor != m.poster) {
        opinion::deed(w, w.planets[at(root)].governor, id, DeedKind::FactionJob, observer);
    }
    ++w.ledger.missions_done;
    if (observer) observer->log(w.now, "mission.done", mid, "by " + std::to_string(id));
    c.mission = kNone;
}

void planet_weekly(World& w, Id planet, Observer* observer) {
    const Planet& p = w.planets[at(planet)];
    if (p.governor == kNone) return;
    bool scouting = false, patrolling = false;
    for (Id m : p.missions) {
        scouting = scouting || (w.missions[at(m)].kind == MissionKind::Scout && w.missions[at(m)].sortie);
        patrolling = patrolling || w.missions[at(m)].kind == MissionKind::Security;
    }
    const auto& c = w.config;
    const std::vector<Id> enemies = raids::believed_enemies(w, planet);
    if (!scouting && !enemies.empty()) {
        Id target = kNone;
        for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
            if (std::find(enemies.begin(), enemies.end(), fealty::root_of(w, q)) == enemies.end()) continue;
            if (w.derived.travel(planet, q) > c.dip.border_days) continue;
            if (target == kNone || w.derived.travel(planet, q) < w.derived.travel(planet, target)) target = q;
        }
        if (target != kNone) {
            post(w,
                 {.kind = MissionKind::Scout, .level = MissionLevel::Planet, .poster = p.governor, .at = planet, .target = target,
                  .base_pay = c.mission.scout_pay * round_trip(w, planet, target), .min_combat = c.scout.min_combat,
                  .posted = w.now, .expires = w.now + 7 * kDay, .sortie = true},
                 observer);
        }
    }
    post_passengers(w, planet, observer);
    Facts& known = info::planet_facts(w, planet);
    info::observe_risk(w, known, p.sector);
    if (!patrolling && info::believed_risk(known, p.sector) >= c.security.risk_post) {
        post(w,
             {.kind = MissionKind::Security, .level = MissionLevel::Planet, .poster = p.governor, .at = planet, .target = planet,
              .base_pay = c.security.pay * static_cast<double>(c.security.days), .min_combat = c.scout.min_combat,
              .posted = w.now, .expires = w.now + 7 * kDay, .sortie = true},
             observer);
    }
}

bool post_clearing(World& w, Id planet, Observer* observer) {
    for (Id m : w.planets[at(planet)].missions) {
        if (w.missions[at(m)].authority) return false;
    }
    const auto& a = w.config.authority;
    post(w,
         {.kind = MissionKind::Security, .level = MissionLevel::Planet, .poster = w.planets[at(planet)].governor, .at = planet,
          .target = planet, .base_pay = a.pay * static_cast<double>(a.clear_days), .min_combat = w.config.scout.min_combat,
          .posted = w.now, .expires = w.now + 7 * kDay, .sortie = true, .authority = true},
         observer);
    ++w.ledger.authority_contracts;
    return true;
}

double pay_now(const World& w, const Mission& m) {
    const double waited = static_cast<double>(w.now - m.posted) / kDay;
    return m.base_pay * (1 + w.config.mission.pay_growth * waited);
}

bool qualifies(const World& w, Id id, const Mission& m) {
    const Character& c = w.characters[at(id)];
    if (c.governs != kNone || c.ships.empty() || c.cargo_qty > 0 || c.pirate) return false;
    const ships::Fleet f = ships::fleet(w, c);
    // Only passenger runs are for a hull without a Lattice Drive (TRD-18, MSN-13).
    // and a supply order's courier leg, if open space gets it there (MSN-15).
    if (supply(m) || f.combat < m.min_combat || f.cargo < m.min_cargo) return false;
    if (!f.lattice_drive && !passenger(m) && !(order(m) && w.derived.travel(m.at, m.target, false) < kNoRoute)) return false;
    // Passenger runs are for the bottom rung: one tier-1 hull (MSN-13).
    if (passenger(m) && (c.ships.size() != 1 || w.map.variants[at(c.ships[0])].tier != 1)) return false;
    // A courier run carries the faction's news (INF-16): it goes to those it trusts (REL-5).
    if (m.kind == MissionKind::Courier &&
        opinion::standing(w, fealty::root_of(w, m.at), id) < w.config.mission.courier_standing) {
        return false;
    }
    if (m.level == MissionLevel::Faction) {
        const Id faction = chars::faction_of(w, c);
        if (faction == kNone || faction != fealty::root_of(w, m.at)) return false;  // INF-10
    }
    return true;
}

Offer best_offer(const World& w, Id id) {
    Offer best;
    const Character& c = w.characters[at(id)];
    if (c.planet == kNone) return best;
    const ships::Fleet f = ships::fleet(w, c);
    for (Id mid : w.planets[at(c.planet)].missions) {
        const Mission& m = w.missions[at(mid)];
        if (m.state != MissionState::Open || !qualifies(w, id, m)) continue;
        const double hold = m.kind != MissionKind::Security ? 0
                            : m.authority                    ? static_cast<double>(w.config.authority.clear_days)
                                                             : static_cast<double>(w.config.security.days);
        if (supply(m)) continue;  // a merchant weighs a delivery as a trip (MSN-15)
        const bool back = !one_way(m);
        const double days = std::max(move::trip_days(w, c, m.at, m.target) + (back ? move::trip_days(w, c, m.target, m.at) : 0) + hold, 1.0 / 24);
        const double fuel = move::fuel_cost(w, c, m.at, m.target) + (back ? move::fuel_cost(w, c, m.target, m.at) : 0);
        const double skill = c.skills[skill_of(m.kind)];
        const double score = (pay_now(w, m) / days) * (1 + w.config.skill.bonus * skill) - f.run_cost - fuel / days;
        if (score > best.score) best = {mid, score};
    }
    return best;
}

void take(World& w, Id id, Id mid, Observer* observer) {
    Mission& m = w.missions[at(mid)];
    m.state = MissionState::Taken;
    m.taker = id;
    m.pay = pay_now(w, m);
    unpost(w, mid);
    if (contracted(m)) contracts::sign(w, contract_kind(m.kind), m.poster, id, m.pay, 0, mid);
    if (m.kind == MissionKind::Courier) m.packet = info::planet_facts(w, m.at);
    Character& c = w.characters[at(id)];
    c.mission = mid;
    c.activity = m.kind == MissionKind::Security ? Activity::Patrolling : Activity::Mission;
    if (observer) observer->log(w.now, "mission.taken", mid, "by " + std::to_string(id));
    bool going = false;
    if (m.sortie) {
        // Out and back without landing: war scouting holds half a day off the target, a
        // patrol security.days over its own space.
        const Time hold = m.kind != MissionKind::Security ? kDay / 2
                          : m.authority                    ? w.config.authority.clear_days * kDay
                                                           : w.config.security.days * kDay;
        going = move::start_route(w, id, move::sortie(w, chars::faction_of(w, c), m.at, m.target, w.config.raid.standoff, hold));
        m.outbound = false;
    } else {
        going = move::travel_to(w, id, m.target);
    }
    if (!going) {
        // Unreachable: give it back.
        m.state = MissionState::Expired;
        if (contracted(m)) contracts::end(w, contracts::held(w, id, contract_kind(m.kind)));
        c.mission = kNone;
        c.activity = Activity::Idle;
        chars::wake(w, id, w.now + kDay);
        if (order(w.missions[at(mid)])) order_arrives(w, mid, observer);
        return;
    }
    start_clock(w, mid);
}

void arrived(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    Mission& m = w.missions[at(c.mission)];
    if (order(m)) {
        if (c.planet != m.target) {
            if (!move::travel_to(w, id, m.target)) chars::wake(w, id, w.now + kDay);
            return;
        }
        // The order is delivered: the issuer pays the courier, and the producer's board
        // posts the delivery (MSN-15).
        const Id mid = c.mission;
        m.state = MissionState::Done;
        w.planets[at(m.at)].treasury -= m.pay;
        const auto who = static_cast<std::size_t>(issuer_of(w, m));
        w.ledger.issuer_spend[who] += m.pay;
        ++w.ledger.issuer_done[who];
        w.ledger.order_fees += m.pay;
        ++w.ledger.orders_carried;
        chars::earn(c, m.pay, false);
        auto& skill = c.skills[skill_of(m.kind)];
        skill = std::min(w.config.skill.max, skill + w.config.skill.gain);
        ++c.missions_done;
        c.reputation += 1;  // REL-7
        opinion::deed(w, m.poster, id, DeedKind::JobDone, observer);
        const Id root = fealty::root_of(w, m.at);
        if (root >= 0 && !w.planets[at(root)].pirate && w.planets[at(root)].governor != m.poster) {
            opinion::deed(w, w.planets[at(root)].governor, id, DeedKind::FactionJob, observer);
        }
        ++w.ledger.missions_done;
        if (observer) observer->log(w.now, "mission.done", mid, "by " + std::to_string(id));
        c.mission = kNone;
        c.activity = Activity::Idle;
        c.idle_since = w.now;
        chars::wake(w, id, w.now);
        post_supply(w, mid, observer);
        return;
    }
    if (passenger(m)) {
        if (c.planet != m.target) {
            if (!move::travel_to(w, id, m.target)) chars::wake(w, id, w.now + kDay);
            return;
        }
        // Delivered: the fare is free money (MSN-5, MSN-13). No issuer, so no reputation;
        // the courier skill grows, and the faction it was posted in counts it as a small
        // job (REL-5): the way into its work.
        m.state = MissionState::Done;
        const Id root = fealty::root_of(w, m.at);
        if (root >= 0 && !w.planets[at(root)].pirate) {
            opinion::deed(w, w.planets[at(root)].governor, id, DeedKind::FactionJob, observer);
        }
        w.ledger.passenger_fares += m.pay;
        w.ledger.free_money += m.pay;
        ++w.ledger.passenger_runs;
        chars::earn(c, m.pay, false);
        auto& skill = c.skills[skill_of(m.kind)];
        skill = std::min(w.config.skill.max, skill + w.config.skill.gain);
        ++c.missions_done;
        ++w.ledger.missions_done;
        if (observer) observer->log(w.now, "mission.done", c.mission, "by " + std::to_string(id));
        c.mission = kNone;
        c.activity = Activity::Idle;
        c.idle_since = w.now;
        chars::wake(w, id, w.now);
        return;
    }
    if (m.outbound && c.planet == m.target) {
        if (m.kind == MissionKind::Courier) {
            Facts& there = info::planet_facts(w, m.target);
            info::merge_into(there, m.packet);
            m.packet = there;
        } else {
            // A scout looks over the whole system it was sent to.
            for (Id p : w.map.systems[at(w.planets[at(m.target)].system)].planets) info::observe_market(w, m.packet, p);
            info::merge_into(c.facts, m.packet);
        }
        m.outbound = false;
        if (move::travel_to(w, id, m.at)) return;
    }
    if (c.planet != m.at) {
        if (!move::travel_to(w, id, m.at)) chars::wake(w, id, w.now + kDay);
        return;
    }
    // Home: deliver, and get paid (REL-8: the contract ends with the job).
    info::merge_into(info::planet_facts(w, m.at), m.packet);
    threats::check(w, m.at, observer);
    m.packet = {};
    m.state = MissionState::Done;
    if (m.authority) {
        w.authority.treasury -= m.pay;  // FAC-14: the Authority's mercenaries
        c.authority_opinion += w.config.authority.opinion_per_job;
    } else {
        w.planets[at(m.at)].treasury -= m.pay;
    }
    const auto who = static_cast<std::size_t>(issuer_of(w, m));
    w.ledger.issuer_spend[who] += m.pay;
    ++w.ledger.issuer_done[who];
    chars::earn(c, m.pay, m.kind != MissionKind::Courier);
    if (m.kind == MissionKind::Security) ++w.ledger.patrols;
    auto& skill = c.skills[skill_of(m.kind)];
    skill = std::min(w.config.skill.max, skill + w.config.skill.gain);
    ++c.missions_done;
    c.reputation += 1;  // REL-7: grows with missions completed (OPN-11)
    contracts::end(w, contracts::held(w, id, contract_kind(m.kind)));
    opinion::deed(w, m.poster, id, DeedKind::JobDone, observer);  // REL-3: a job done well under contract
    // Work for one of a faction's planets is work for the faction: its standing grows too
    // (REL-5), a little.
    const Id root = fealty::root_of(w, m.at);
    if (root >= 0 && !w.planets[at(root)].pirate && w.planets[at(root)].governor != m.poster) {
        opinion::deed(w, w.planets[at(root)].governor, id, DeedKind::FactionJob, observer);
    }
    ++w.ledger.missions_done;
    if (observer) observer->log(w.now, "mission.done", c.mission, "by " + std::to_string(id));
    c.mission = kNone;
    c.activity = Activity::Idle;
    c.idle_since = w.now;
    chars::wake(w, id, w.now);
}

void expire(World& w, Id mid, Observer* observer) {
    Mission& m = w.missions[at(mid)];
    if (m.state != MissionState::Open) return;
    m.state = MissionState::Expired;
    unpost(w, mid);
    ++w.ledger.missions_expired;
    if (observer) observer->log(w.now, "mission.expired", mid, "");
    if (m.kind == MissionKind::Courier) info::send_courier(w, m.target);
    if (supply(m)) unorder(w, m, m.qty);  // no one would carry it: the planet may order again
    if (order(m)) order_arrives(w, mid, observer);  // last: it posts, which moves the missions
}

}  // namespace lattice::missions
