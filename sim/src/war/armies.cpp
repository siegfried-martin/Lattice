#include "war/armies.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "politics/budget.hpp"
#include "politics/fealty.hpp"
#include "politics/governor.hpp"
#include "politics/opinion.hpp"
#include "politics/roles.hpp"
#include "war/combat.hpp"
#include "war/diplomacy.hpp"
#include "war/gates.hpp"
#include "war/loans.hpp"
#include "war/prizes.hpp"
#include "info/facts.hpp"
#include "pirates/pirates.hpp"
#include "war/raids.hpp"

namespace lattice::armies {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

double income(const World& w, Id p) {
    return std::max(0.0, w.planets[at(p)].income_avg) * static_cast<double>(w.config.time.payment_days);
}

}  // namespace

std::vector<Id> ready(const World& w, Id planet, bool marching) {
    std::vector<Id> out;
    const Id gov = w.planets[at(planet)].governor;
    if (gov == kNone) return out;
    // The governor's own fleet (MIL-20): it defends at home, and marches only with a
    // steward to leave in charge.
    const Character& g = w.characters[at(gov)];
    if (g.alive && g.planet == planet && g.army == kNone && g.battle == kNone && !g.ships.empty() &&
        (!marching || roles::can_sail(w, planet))) {
        out.push_back(gov);
    }
    for (Id k : w.characters[at(gov)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.kind != ContractKind::Service || ct.employer != gov) continue;
        const Character& c = w.characters[at(ct.employee)];
        if (c.alive && c.activity == Activity::Serving && c.planet == planet && c.army == kNone && c.battle == kNone) {
            out.push_back(ct.employee);
        }
    }
    return out;
}

namespace {

double fleet_power(const World& w, const std::vector<Id>& ids) {
    double p = 0;
    for (Id id : ids) p += ships::fleet(w, w.characters[at(id)]).combat;
    return p;
}

double stipends(const World& w, const std::vector<Id>& ids) {
    double s = 0;
    for (Id id : ids) s += budget::stipend(w, w.characters[at(id)]);
    return s;
}

double win_chance(double attack, double defence) {
    const double a = attack * attack, d = defence * defence;
    return a + d > 0 ? a / (a + d) : 0;
}

double speed(const World& w, const Army& a) {
    double s = 0;
    for (Id id : a.members) {
        const double f = ships::fleet(w, w.characters[at(id)]).speed;
        s = s == 0 ? f : std::min(s, f);
    }
    return s;
}

void attach(World& w, Id army, Id id) {
    Character& c = w.characters[at(id)];
    c.army = army;
    c.activity = Activity::Army;
}

void release(World& w, Id id, Id where) {
    Character& c = w.characters[at(id)];
    if (!c.alive) return;
    c.army = kNone;
    if (c.planet == kNone && c.route.empty()) {  // carried with the army; one on its own way keeps going
        c.planet = where;
        c.last_planet = where;
        c.route.clear();
        c.leg = 0;
        ++c.travel_seq;
    }
    c.activity = Activity::Idle;
    chars::wake(w, id, w.now);
}

void start_leg(World& w, Id id) {
    Army& a = w.armies[at(id)];
    Leg& l = a.route[at(a.leg)];
    if (l.kind == LegKind::Lattice) {
        // Each fleet pays its own fuel at entry (SYS-21).
        for (Id m : a.members) {
            Character& c = w.characters[at(m)];
            if (c.last_planet == kNone) continue;
            const double fuel = l.lattice_length * w.config.fuel.burn * w.planets[at(c.last_planet)].fuel_price *
                                static_cast<double>(c.ships.size());
            c.wealth -= fuel;
            w.ledger.fuel += fuel;
            w.ledger.money_sunk += fuel;
        }
    }
    l.depart = w.now;
    l.arrive = w.now + move::leg_time(w, l, speed(w, a));
    w.events.push({.time = l.arrive, .kind = EventKind::ArmyArrive, .record = id, .a = a.travel_seq});
}

// An army leaving a gate it held stops holding it (MIL-17).
void leave_gate(World& w, Id id) {
    Army& a = w.armies[at(id)];
    if (a.node != kNone && !w.gates.empty() && w.gates[at(a.node)].garrison == id) w.gates[at(a.node)].garrison = kNone;
}

const Facts* army_facts(const World& w, const Army& a) { return &w.characters[at(a.commissioner)].facts; }

// To a planet, or to a gate (MIL-17), from where it stands: a planet or a gate.
void go(World& w, Id id, Id dest, Id dest_node = kNone) {
    Army& a = w.armies[at(id)];
    const Id from = a.planet;
    if (from == kNone && a.node == kNone) return;  // already on its way; its arrival decides what's next
    const Id faction = fealty::root_of(w, a.home);
    const auto np = static_cast<std::int32_t>(w.planets.size());
    std::vector<Leg> route;
    if (from != kNone && dest_node == kNone) {
        route = move::plan(w, faction, from, dest, army_facts(w, a));
    } else if (from != kNone || a.node != dest_node) {
        route = move::plan_between(w, faction, from != kNone ? from : np + a.node, dest_node != kNone ? np + dest_node : dest,
                                   army_facts(w, a));
    }
    if (route.empty()) {
        disband(w, id, nullptr);
        return;
    }
    leave_gate(w, id);
    a.route = std::move(route);
    a.leg = 0;
    ++a.travel_seq;
    a.planet = kNone;
    a.node = kNone;
    for (Id m : a.members) {
        Character& c = w.characters[at(m)];
        if (c.governs != kNone && c.governs == c.planet) roles::handover(w, c.governs);  // leaving home (SYS-22)
        c.planet = kNone;
        c.route.clear();
        ++c.travel_seq;
    }
    start_leg(w, id);
}

void retreat(World& w, Id id, Observer* observer) {
    Army& a = w.armies[at(id)];
    if (a.members.empty()) {
        disband(w, id, observer);
        return;
    }
    a.action = ArmyAction::Retreat;
    a.target = kNone;
    a.target_node = kNone;
    if (a.planet == kNone && a.node == kNone) return;  // on the move: it heads home when it arrives
    if (a.planet == a.home) {
        disband(w, id, observer);
        return;
    }
    go(w, id, a.home);
}

void end_siege(World& w, Id siege) {
    Siege& s = w.sieges[at(siege)];
    if (!s.alive) return;
    s.alive = false;
    if (s.node != kNone) {
        w.gates[at(s.node)].siege = kNone;
    } else {
        w.planets[at(s.planet)].siege = kNone;
    }
}

// A battle at a gate, there rather than at a planet (MIL-17).
Id fight_at_gate(World& w, Id node, std::vector<Id> attackers, std::vector<Id> defenders, Id army0, Id army1, Observer* observer) {
    const Id b = combat::start(w, std::move(attackers), std::move(defenders), kNone, false, army0, army1, observer);
    w.battles[at(b)].x = w.map.nodes[at(node)].x;
    w.battles[at(b)].y = w.map.nodes[at(node)].y;
    return b;
}

// The holder's faction's armies garrisoning a gate.
std::vector<Id> gate_guards(const World& w, Id node, Id& first) {
    std::vector<Id> out;
    first = kNone;
    const Id h = gates::holder(w, node);
    for (Id g = 0; g < static_cast<Id>(w.armies.size()); ++g) {
        const Army& other = w.armies[at(g)];
        if (!other.alive || other.action != ArmyAction::Garrison || other.node != node || !fealty::same_faction(w, other.home, h)) continue;
        if (first == kNone) first = g;
        for (Id m : other.members) out.push_back(m);
    }
    return out;
}

void besiege_gate(World& w, Id id, Observer* observer) {
    Army& a = w.armies[at(id)];
    const Id n = a.node, h = gates::holder(w, n);
    a.action = ArmyAction::Besiege;
    w.sieges.push_back({.planet = h, .node = n, .army = id, .start = w.now, .called = true});
    const auto siege = static_cast<Id>(w.sieges.size() - 1);
    a.siege = siege;
    w.gates[at(n)].siege = siege;
    ++w.ledger.gate_attacks;
    if (observer) observer->log(w.now, "gate.attacked", n, "army " + std::to_string(id) + " at holder " + std::to_string(h));
    // The holder, beside its gate, knows at once and calls for help (MIL-11).
    w.events.push({.time = w.now + kHour, .kind = EventKind::CallForHelp, .record = siege, .a = h});
    Id garrison = kNone;
    std::vector<Id> defenders = gate_guards(w, n, garrison);
    if (!defenders.empty()) {
        fight_at_gate(w, n, a.members, defenders, id, garrison, observer);
        return;
    }
    w.events.push({.time = w.now + kDay, .kind = EventKind::SiegeRound, .record = siege});
}

// An army reached the gate it was sent to (MIL-17).
void at_gate(World& w, Id id, Observer* observer) {
    Army& a = w.armies[at(id)];
    const Id n = a.node, h = gates::holder(w, n);
    const Gate& g = w.gates[at(n)];
    const Id faction = fealty::root_of(w, a.home);
    for (Id m : a.members) gates::observe(w, w.characters[at(m)].facts, n);
    if (a.action == ArmyAction::Retreat || n != a.target_node || h == kNone) {
        retreat(w, id, observer);
        return;
    }
    if (a.relief) {
        if (g.siege != kNone && w.sieges[at(g.siege)].alive) {
            const Id besiegers = w.sieges[at(g.siege)].army;
            fight_at_gate(w, n, w.armies[at(besiegers)].members, a.members, besiegers, id, observer);
            return;
        }
        if (g.captor == kNone) {
            // Here before the attack, or after it failed: hold the gate a while.
            a.action = ArmyAction::Garrison;
            w.events.push({.time = w.now + w.config.army.muster_days * kDay, .kind = EventKind::ArmyMuster, .record = id,
                           .a = a.travel_seq + 1});
            return;
        }
        // Too late: it's blockaded. Take it back.
    }
    if (g.captor != kNone) {
        const Id captor = gates::captor_faction(w, n);
        if (captor == faction || !fealty::same_faction(w, h, a.home)) {
            retreat(w, id, observer);  // already ours, or not ours to take back
            return;
        }
        const Id held = g.garrison;
        if (held != kNone && w.armies[at(held)].alive && !w.armies[at(held)].members.empty()) {
            fight_at_gate(w, n, a.members, w.armies[at(held)].members, id, held, observer);
            return;
        }
        gates::retake(w, n, observer);  // nobody holds it (MIL-17)
        retreat(w, id, observer);
        return;
    }
    if (w.planets[at(h)].pirate || !diplomacy::hostile_planets(w, a.home, h)) {
        retreat(w, id, observer);
        return;
    }
    besiege_gate(w, id, observer);
}

// FAC-8: keep or grant, down the tree from the commissioner (proposed scores):
// keep = 1 - value.protect x the keeper's travel days to the planet;
// grant to vassal V = fealty.w_favor x favor / 100 + its tribute rate (the share that still
// comes back up) - value.protect x V's travel days. The highest wins; a grant recurses at V.
Id keep_or_grant(World& w, Id keeper, Id target, Observer* observer) {
    const auto& c = w.config;
    for (;;) {
        const Id gov = w.planets[at(keeper)].governor;
        const double keep = 1 - c.value.protect * w.derived.travel(keeper, target);
        Id best = kNone;
        double best_score = keep;
        const std::vector<Id> vassals = fealty::vassals(w, keeper);
        for (Id v : vassals) {
            if (v == target) continue;
            const double favor = opinion::get(w, gov, w.planets[at(v)].governor);
            const double score = c.fealty.w_favor * favor / 100 + w.planets[at(v)].tribute_rate - c.value.protect * w.derived.travel(v, target);
            if (score > best_score) best_score = score, best = v;
        }
        if (best == kNone) return keeper;
        opinion::deed(w, w.planets[at(best)].governor, gov, DeedKind::PlanetGranted, observer);
        for (Id v : vassals) {
            // Nearer the planet than the keeper and passed over (REL-3).
            if (v != best && v != target && w.derived.travel(v, target) < w.derived.travel(keeper, target)) {
                opinion::deed(w, w.planets[at(v)].governor, gov, DeedKind::PassedOver, observer);
            }
        }
        keeper = best;
    }
}

// FAC-9, SYS-22: the keeper chooses among its commanders and the army that took the
// planet by the governor post's fitness, plus fealty.capture_bonus for the army's leader;
// else from the fallback pool; else none (the caller generates one).
Id choose_governor(const World& w, Id keeper, const Army& army, Id planet) {
    const Id gov = w.planets[at(keeper)].governor;
    std::vector<Id> pool = army.members;
    for (Id k : w.characters[at(gov)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.kind != ContractKind::Service || ct.employer != gov) continue;
        const Character& c = w.characters[at(ct.employee)];
        if (c.planet != kNone && c.army == kNone && c.battle == kNone) pool.push_back(ct.employee);
    }
    Id leader = kNone;
    double strongest = -1;
    for (Id m : army.members) {
        const double p = ships::fleet(w, w.characters[at(m)]).combat;
        if (p > strongest) strongest = p, leader = m;
    }
    const Id best = roles::best_governor(w, pool, gov, planet, leader);
    return best != kNone ? best : roles::best_governor(w, roles::fallback_pool(w, planet), gov, planet);
}

double capture_value(const World& w, Id t) {
    const auto& v = w.config.value;
    return income(w, t) * (v.invade_months + (governor::holds_gates(w, t) ? v.gate_months : 0) - v.aftermath_months);
}

bool worth_holding(const World& w, Id id, Id t) {
    const Config& c = w.config;
    const Army& a = w.armies[at(id)];
    const double attack = power(w, a);
    // The enemy power near the prize: commanders at home on enemy planets within
    // dip.border_days (MIL-11's relief), as invasions see defence.
    const Id root = fealty::root_of(w, a.home);
    double enemy = 0;
    for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
        if (q == t || w.derived.travel(q, t) > c.dip.border_days) continue;
        if (!diplomacy::hostile_planets(w, q, a.home)) continue;
        if (fealty::root_of(w, q) == root) continue;
        enemy += fleet_power(w, ready(w, q));
    }
    const double lose = win_chance(enemy, attack);
    const double months = static_cast<double>(c.capture.hold_days) / static_cast<double>(c.time.payment_days);
    const double exposed = c.value.defence * income(w, a.home) * attack / c.value.power_ref * budget::threat(w, a.home);
    const double cost = (stipends(w, a.members) + exposed) * months;
    return lose * capture_value(w, t) > cost;
}

void capture(World& w, Id siege, Observer* observer) {
    const Siege s = w.sieges[at(siege)];
    end_siege(w, siege);
    prizes::take_spoils(w, siege, observer);  // from its treasury, before anything else (MIL-19)
    Army& army = w.armies[at(s.army)];
    const Id t = s.planet;
    Planet& planet = w.planets[at(t)];
    const auto& cap = w.config.capture;
    planet.captured_at = w.now;

    // FAC-10: capture costs prosperity: a shock, then recovery toward the target (SYS-1).
    planet.happiness -= cap.happiness_drop;
    for (int g = 0; g < kGoodCount; ++g) {
        w.ledger.destroyed[at(g)] += planet.stock[at(g)] * cap.stock_loss;
        planet.stock[at(g)] *= 1 - cap.stock_loss;
    }
    planet.population *= 1 - cap.pop_loss;

    const Id old_root = fealty::root_of(w, t);
    const Id winner_root = fealty::root_of(w, army.home);
    diplomacy::record_loss(w, old_root, winner_root, 1, 0);
    const bool was_pirate = planet.pirate;
    pirates::restore(w, t);  // back to its old production (PIR-3, MIL-12)

    // The deposed governor stays, as a character with no planet; those it paid lose their
    // contracts with it.
    const Id old_gov = planet.governor;
    Character& deposed = w.characters[at(old_gov)];
    for (Id k : std::vector<Id>(deposed.contracts)) {
        if (w.contracts[at(k)].employer == old_gov) contracts::end(w, k);
    }
    deposed.governs = kNone;
    deposed.activity = Activity::Idle;
    chars::wake(w, old_gov, w.now + kDay);

    // Its vassals stay with the old faction: under its old liege, or, if it was the root,
    // under the largest of them, which becomes the new root and takes over its wars
    // (MOD-5, proposed).
    const std::vector<Id> below = fealty::vassals(w, t);
    if (planet.liege != kNone) {
        for (Id v : below) fealty::swear(w, v, planet.liege);
    } else if (was_pirate) {
        // A pirate planet has no vassals.
    } else if (!below.empty()) {
        Id heir = below[0];
        for (Id v : below) {
            if (w.planets[at(v)].population > w.planets[at(heir)].population) heir = v;
        }
        fealty::swear(w, heir, kNone);
        for (Id v : below) {
            if (v != heir) fealty::swear(w, v, heir);
        }
        diplomacy::move_root(w, t, heir);
    } else {
        diplomacy::forget(w, t);  // the faction is gone
        prizes::move(w, t, kNone);
    }

    const Id keeper = keep_or_grant(w, army.home, t, observer);
    Id chosen = choose_governor(w, keeper, army, t);
    fealty::swear(w, t, keeper);
    if (chosen == kNone) {
        chosen = roles::generate(w, t);  // every planet always has a governor (FAC-1)
    } else if (std::find(army.members.begin(), army.members.end(), chosen) != army.members.end()) {
        // From the army at the planet: it lands there.
        Character& g = w.characters[at(chosen)];
        leave(w, s.army, chosen);
        g.army = kNone;
        g.battle = kNone;
        g.planet = t;
        g.last_planet = t;
        g.route.clear();
        ++g.travel_seq;
    }
    roles::seat(w, t, chosen, observer);
    planet.governor_stipend = 0;
    planet.vacant_since = kNever;
    planet.succession_at = kNever;
    planet.security = 0;
    ++w.ledger.captures;
    if (observer) {
        observer->log(w.now, "planet.captured", t,
                      "by army " + std::to_string(s.army) + " kept by " + std::to_string(keeper) + " governor " + std::to_string(chosen));
    }
    // Holding the prize for capture.hold_days while its defences rebuild is a choice
    // (MIL-12, SYS-13): worth it when the believed risk of losing it outweighs the
    // stipends and the home left exposed.
    Army& held = w.armies[at(s.army)];
    if (held.members.empty() || w.config.capture.hold_days <= 0 || !worth_holding(w, s.army, t)) {
        disband(w, s.army, observer);
        return;
    }
    ++w.ledger.holds;
    held.siege = kNone;
    held.target = kNone;
    held.action = ArmyAction::Garrison;
    w.events.push({.time = w.now + w.config.capture.hold_days * kDay, .kind = EventKind::ArmyMuster, .record = s.army,
                   .a = held.travel_seq + 1});
}

void besiege(World& w, Id id, Observer* observer) {
    Army& a = w.armies[at(id)];
    const Id t = a.target;
    a.action = ArmyAction::Besiege;
    w.sieges.push_back({.planet = t, .army = id, .start = w.now});
    const auto siege = static_cast<Id>(w.sieges.size() - 1);
    a.siege = siege;
    w.planets[at(t)].siege = siege;
    ++w.ledger.sieges;
    if (observer) observer->log(w.now, "siege.start", siege, "army " + std::to_string(id) + " at " + std::to_string(t));
    // The besieged planet calls for help up the tree (MIL-11).
    const Id liege = w.planets[at(t)].liege;
    if (liege != kNone) {
        const Time trip = std::max<Time>(1, static_cast<Time>(w.derived.travel(t, liege) * kDay));
        w.events.push({.time = w.now + trip, .kind = EventKind::CallForHelp, .record = siege, .a = liege});
        w.sieges[at(siege)].called = true;
    }
    // The defenders: its commanders at home, and any garrison sent ahead.
    std::vector<Id> defenders = ready(w, t);
    Id garrison = kNone;
    for (Id g = 0; g < static_cast<Id>(w.armies.size()); ++g) {
        const Army& other = w.armies[at(g)];
        if (!other.alive || other.action != ArmyAction::Garrison || other.planet != t) continue;
        if (garrison == kNone) garrison = g;
        for (Id m : other.members) defenders.push_back(m);
    }
    if (!defenders.empty()) {
        combat::start(w, w.armies[at(id)].members, defenders, t, true, id, garrison, observer);
        return;
    }
    w.events.push({.time = w.now + kDay, .kind = EventKind::SiegeRound, .record = siege});
}

}  // namespace

double power(const World& w, const Army& a) { return fleet_power(w, a.members); }

std::pair<double, double> position(const World& w, const Army& a) {
    if (a.planet != kNone) return {w.planets[at(a.planet)].x, w.planets[at(a.planet)].y};
    if (a.node != kNone) return {w.map.nodes[at(a.node)].x, w.map.nodes[at(a.node)].y};
    if (a.route.empty()) return {0, 0};
    const Leg& l = a.route[at(a.leg)];
    const double span = static_cast<double>(std::max<Time>(1, l.arrive - l.depart));
    const double t = std::clamp(static_cast<double>(w.now - l.depart) / span, 0.0, 1.0);
    return {l.x0 + (l.x1 - l.x0) * t, l.y0 + (l.y1 - l.y0) * t};
}

Plan best_invasion(const World& w, Id planet) {
    Plan best;
    const std::vector<Id> enemies = raids::believed_enemies(w, planet);  // as it has heard (DIP-5)
    const Id gov = w.planets[at(planet)].governor;
    for (const Army& a : w.armies) {
        if (a.alive && a.commissioner == gov && !a.relief) return best;  // one campaign at a time
    }
    // Nor one while the governor is away with its fleet, or the seat is empty (SYS-22).
    if (!roles::home(w, planet) || w.characters[at(gov)].army != kNone || roles::vacant(w, planet)) return best;
    std::vector<Id> own = ready(w, planet, true), called;
    for (Id v : fealty::vassals(w, planet)) {
        for (Id id : ready(w, v, true)) called.push_back(id);
    }
    const double attack = fleet_power(w, own) + 0.5 * fleet_power(w, called);
    if (attack <= 0) return best;
    const double pay = stipends(w, own) + stipends(w, called);
    const auto& v = w.config.value;
    double best_ratio = 0;
    for (Id t = 0; t < static_cast<Id>(w.planets.size()); ++t) {
        // An enemy's planet, or a pirate planet it knows of (MIL-9).
        const Id troot = fealty::root_of(w, t);
        const bool pirate = w.planets[at(t)].pirate && info::knows_pirate(info::planet_facts(w, planet), t);
        if (!pirate && std::find(enemies.begin(), enemies.end(), troot) == enemies.end()) continue;
        if (w.derived.travel(planet, t) > w.config.dip.border_days) continue;
        if (w.planets[at(t)].siege != kNone) continue;
        const double defence = w.planets[at(t)].defence + fleet_power(w, ready(w, t));
        const double p = win_chance(attack, defence);
        const double gain = capture_value(w, t);
        const double days = w.derived.travel(planet, t) + 3 + 10;  // muster, travel, and the fight
        const double cost = std::max(1.0, pay * days / static_cast<double>(w.config.time.payment_days));
        const double value = p * gain;
        if (value / cost > best_ratio) best_ratio = value / cost, best = {t, value, cost};
    }
    // Gates (MIL-17): an enemy's to blockade, or one's own to take back.
    if (!w.config.blockade.enabled || w.gates.empty()) return best;
    const Facts& facts = info::planet_facts(w, planet);
    for (Id n = 0; n < static_cast<Id>(w.map.nodes.size()); ++n) {
        const Id h = gates::holder(w, n);
        if (h == kNone || w.planets[at(h)].pirate || w.gates[at(n)].siege != kNone) continue;
        if (w.derived.travel(planet, h) > w.config.dip.border_days) continue;
        const Gate& g = w.gates[at(n)];
        const GateFact* known = gates::find(facts, n);
        const bool retake = known && known->captor != kNone && g.captor != kNone && fealty::same_faction(w, h, planet) &&
                            gates::captor_faction(w, n) != fealty::root_of(w, planet);
        const bool assault = g.captor == kNone &&
                             std::find(enemies.begin(), enemies.end(), fealty::root_of(w, h)) != enemies.end();
        if (!retake && !assault) continue;
        double defence = 0;
        if (assault) {
            Id first = kNone;
            defence = g.defence + fleet_power(w, gate_guards(w, n, first));
        } else if (g.garrison != kNone && w.armies[at(g.garrison)].alive) {
            defence = power(w, w.armies[at(g.garrison)]);
        }
        const double p = defence > 0 ? win_chance(attack, defence) : 1;
        const double gain = gates::group_income(w, n) * (retake ? v.retake_months : v.blockade_months);
        const double days = w.derived.travel(planet, h) + 3 + 10;
        const double cost = std::max(1.0, pay * days / static_cast<double>(w.config.time.payment_days));
        const double value = p * gain;
        if (value / cost > best_ratio) best_ratio = value / cost, best = {h, value, cost, n};
    }
    return best;
}

Weighed weigh(const World& w, const Army& a, Id id) {
    const Config& c = w.config;
    const Character& ch = w.characters[at(id)];
    const ships::Fleet f = ships::fleet(w, ch);
    if (f.combat <= 0) return {};
    double worth = 0;
    for (Id v : ch.ships) worth += w.map.variants[at(v)].price;
    // What it's up against, and what it could kill and take there.
    double defence = 0, kills = 0, spoils = 0;
    std::vector<Id> enemy;
    if (a.target_node != kNone) {
        const Gate& g = w.gates[at(a.target_node)];
        if (g.captor == kNone) {
            Id first = kNone;
            enemy = gate_guards(w, a.target_node, first);
            defence = g.defence;
        } else if (g.garrison != kNone && w.armies[at(g.garrison)].alive) {
            enemy = w.armies[at(g.garrison)].members;
        }
    } else if (a.target != kNone) {
        enemy = ready(w, a.target);
        defence = w.planets[at(a.target)].defence;
        spoils = prizes::spoils_of(w, a.target);
    }
    for (Id e : enemy) {
        for (Id v : w.characters[at(e)].ships) kills += w.map.variants[at(v)].price;
    }
    defence += fleet_power(w, enemy);
    const bool in = std::find(a.members.begin(), a.members.end(), id) != a.members.end() ||
                    std::find(a.called.begin(), a.called.end(), id) != a.called.end();
    const double attack = std::max(a.plan_power, power(w, a) + fleet_power(w, a.called) + (in ? 0 : f.combat));
    const double p = win_chance(attack, defence);
    const double share = f.combat / attack;
    Weighed out;
    const Id to = a.target != kNone ? a.target : a.home;
    const double fuel = (ch.planet != kNone && ch.planet != a.home ? move::fuel_cost(w, ch, ch.planet, a.home) : 0) +
                        2 * move::fuel_cost(w, ch, a.home, to);
    out.join = p * share * (c.bounty.share * kills + spoils) - (1 - p) * worth * c.combat.retreat_loss - fuel;
    // Staying: its employer's planet, against the armies seen coming for it.
    const Id k = contracts::held(w, id, ContractKind::Service);
    const Id post = k != kNone ? w.characters[at(w.contracts[at(k)].employer)].governs : ch.governs;
    if (post == kNone) return out;
    double coming = 0;
    for (const Sighting& s : info::planet_facts(w, post).sightings) {
        if (s.target == post && s.time >= w.now - c.scout.threat_days * kDay) coming += s.power;
    }
    if (coming <= 0) return out;
    const std::vector<Id> there = ready(w, post);
    const bool counted = std::find(there.begin(), there.end(), id) != there.end();
    const double without = w.planets[at(post)].defence + fleet_power(w, there) - (counted ? f.combat : 0);
    out.home = budget::stipend(w, ch) * c.objective.stake_months * (win_chance(coming, without) - win_chance(coming, without + f.combat));
    return out;
}

namespace {

// A commander chooses between an army's objective and staying at its post (MIL-2).
// Declining both is a small bad deed with its employer.
bool offer(World& w, Id army, Id id, Observer* observer) {
    const Weighed v = weigh(w, w.armies[at(army)], id);
    if (v.join > std::max(0.0, v.home)) {
        ++w.ledger.objectives_taken;
        return true;
    }
    if (v.home > 0) {
        ++w.ledger.objectives_home;
        return false;
    }
    ++w.ledger.objectives_declined;
    const Id k = contracts::held(w, id, ContractKind::Service);
    if (k != kNone) opinion::deed(w, w.contracts[at(k)].employer, id, DeedKind::ObjectiveDeclined, observer);
    return false;
}

// The liege records each vassal it asked as answering or refusing by whether its fleets
// came (INF-16): a deed either way (REL-10).
void settle_calls(World& w, Id id, Observer* observer) {
    Army& a = w.armies[at(id)];
    for (Id v : a.asked) {
        const bool came = std::find(a.answered.begin(), a.answered.end(), v) != a.answered.end();
        opinion::deed(w, a.commissioner, w.planets[at(v)].governor, came ? DeedKind::CallAnswered : DeedKind::CallRefused, observer);
    }
    a.asked.clear();
}

// Every request answered and everyone called arrived: march now rather than at the deadline.
void march_if_ready(World& w, Id id, Observer* observer) {
    Army& a = w.armies[at(id)];
    if (!a.alive || a.action != ArmyAction::Muster || a.awaiting > 0 || !a.called.empty()) return;
    settle_calls(w, id, observer);
    if (a.members.empty() || a.target == kNone) {
        disband(w, id, observer);
        return;
    }
    a.action = ArmyAction::Move;
    go(w, id, a.target, a.target_node);
}

}  // namespace

Id commission(World& w, Id planet, Id target, bool relief, Observer* observer, Id node) {
    const Id gov = w.planets[at(planet)].governor;
    const std::vector<Id> home = ready(w, planet, true);
    double plan = fleet_power(w, home);
    std::vector<Id> lenders;
    if (!relief) {
        for (Id v : fealty::vassals(w, planet)) {
            const std::vector<Id> there = ready(w, v, true);
            if (there.empty()) continue;  // nothing to borrow
            lenders.push_back(v);
            plan += 0.5 * fleet_power(w, there);  // as best_invasion counts them
        }
    }
    w.armies.push_back({.commissioner = gov, .home = planet, .target = target, .target_node = node, .relief = relief,
                        .formed = w.now, .planet = planet, .plan_power = plan});
    const auto id = static_cast<Id>(w.armies.size() - 1);
    // The commissioner's own fleet joins (MIL-20); its commanders at home choose (MIL-2).
    // A relief goes with everyone, as ordered (MIL-11).
    const bool choose = !relief && w.config.objective.choose;
    for (Id m : home) {
        if (choose && m != gov && !offer(w, id, m, observer)) continue;
        attach(w, id, m);
        w.armies[at(id)].members.push_back(m);
    }
    // Loan requests to the vassals (MIL-3), by courier.
    for (Id v : lenders) {
        const Time delay = std::max<Time>(1, static_cast<Time>(w.derived.travel(planet, v) * kDay));
        w.events.push({.time = w.now + delay, .kind = EventKind::LoanRequest, .record = id, .a = v});
        w.armies[at(id)].asked.push_back(v);
        ++w.armies[at(id)].awaiting;
    }
    Army& a = w.armies[at(id)];
    if (a.members.empty() && a.awaiting == 0) {
        a.alive = false;
        return kNone;
    }
    // A relief is a loan to the planet relieved: it pays the stipends while they serve,
    // and its governor spends favour (MIL-11).
    if (relief && target != planet && fealty::same_faction(w, planet, target)) {
        std::vector<Id> lent;
        double pay = 0;
        for (Id m : a.members) {
            const Id k = contracts::held(w, m, ContractKind::Service);
            if (k == kNone || w.contracts[at(k)].employer != gov) continue;
            lent.push_back(k);
            pay += w.contracts[at(k)].pay;
        }
        if (!lent.empty()) loans::lend(w, planet, target, id, lent, w.config.loan.relief_fee * pay, true, observer);
        opinion::deed(w, gov, w.planets[at(target)].governor, DeedKind::ReliefUsed, observer);
    }
    Army& raised = w.armies[at(id)];
    if (observer) {
        observer->log(w.now, relief ? "army.relief" : "army.raised", id,
                      "from " + std::to_string(planet) + " against " + std::to_string(target) +
                          (node != kNone ? " gate " + std::to_string(node) : std::string()) + ", " +
                          std::to_string(raised.members.size()) + " now, " + std::to_string(raised.asked.size()) + " asked");
    }
    if (relief || raised.awaiting == 0) {
        raised.action = ArmyAction::Move;
        go(w, id, target, node);
    } else {
        w.events.push({.time = w.now + w.config.army.muster_days * kDay, .kind = EventKind::ArmyMuster, .record = id});
    }
    return id;
}

void loan_request(World& w, Id id, Id v, Observer* observer) {
    Army& a = w.armies[at(id)];
    if (a.awaiting > 0) --a.awaiting;
    if (!a.alive || a.action != ArmyAction::Muster) return;  // too late: it has gone (INF-16)
    const Config& c = w.config;
    const Planet& vp = w.planets[at(v)];
    const Id vgov = vp.governor;
    bool grant = vp.liege == a.home && !roles::vacant(w, v);
    std::vector<Id> lent, fleets;
    double pay = 0, lost = 0;
    if (grant) {
        for (Id m : ready(w, v, true)) {
            fleets.push_back(m);
            if (m == vgov) continue;  // a governor's own fleet has no contract to lend (MIL-3)
            const Id k = contracts::held(w, m, ContractKind::Service);
            lent.push_back(k);
            pay += w.contracts[at(k)].pay;
            lost += ships::fleet(w, w.characters[at(m)]).combat;
        }
        const double fee = c.loan.fee_months * pay;
        // Its home's lost defence over the loan, as SYS-13's hold decision counts it.
        const double exposed = c.value.defence * income(w, v) * lost / c.value.power_ref * budget::threat(w, v) *
                               static_cast<double>(c.loan.max_days) / static_cast<double>(c.time.payment_days);
        grant = !fleets.empty() && (opinion::get(w, vgov, a.commissioner) >= 0 || fee >= exposed);
        if (grant && !lent.empty()) loans::lend(w, v, a.home, id, lent, fee, false, observer);
    }
    if (!grant) {
        ++w.ledger.loans_refused;
        if (observer) observer->log(w.now, "loan.refused", v, "army " + std::to_string(id));
        march_if_ready(w, id, observer);
        return;
    }
    for (Id m : fleets) {
        if (c.objective.choose && !offer(w, id, m, observer)) {
            chars::wake(w, m, w.now);  // to its new employer's post
            continue;
        }
        attach(w, id, m);
        w.armies[at(id)].called.push_back(m);
        chars::wake(w, m, w.now);
    }
    march_if_ready(w, id, observer);
}

void dismiss(World& w, Id army, Id id, Observer* observer) {
    Army& a = w.armies[at(army)];
    const Id where = a.planet != kNone ? a.planet : a.home;
    leave(w, army, id);
    release(w, id, where);
    if (a.alive && a.members.empty() && a.called.empty()) disband(w, army, observer);
}

void member_arrived(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    Army& a = w.armies[at(c.army)];
    if (!a.alive || a.action != ArmyAction::Muster) {
        release(w, id, c.planet);
        return;
    }
    auto it = std::find(a.called.begin(), a.called.end(), id);
    if (it == a.called.end()) return;
    a.called.erase(it);
    a.members.push_back(id);
    // The fleet is the reply (INF-16): note whose it is.
    Id from = c.governs != kNone && c.governs != a.home ? c.governs : kNone;
    const Id k = contracts::held(w, id, ContractKind::Service);
    if (k != kNone && w.contracts[at(k)].loan != kNone) from = w.loans[at(w.contracts[at(k)].loan)].lender;
    if (from != kNone && std::find(a.answered.begin(), a.answered.end(), from) == a.answered.end()) a.answered.push_back(from);
    march_if_ready(w, c.army, observer);
}

void muster_deadline(World& w, Id id, std::uint32_t token, Observer* observer) {
    Army& a = w.armies[at(id)];
    if (a.alive && a.action == ArmyAction::Garrison) {
        if (token != a.travel_seq + 1) return;  // a muster deadline from before it marched
        for (Id m : a.members) {
            if (w.characters[at(m)].battle != kNone) {  // not in the middle of a fight
                w.events.push({.time = w.now + kDay, .kind = EventKind::ArmyMuster, .record = id, .a = token});
                return;
            }
        }
        retreat(w, id, observer);  // the garrison's time is up
        return;
    }
    if (!a.alive || a.action != ArmyAction::Muster || token != 0) return;
    // Those not there yet are left behind, and a vassal none of whose fleets came has
    // refused (INF-16).
    for (Id m : std::vector<Id>(a.called)) release(w, m, w.characters[at(m)].planet != kNone ? w.characters[at(m)].planet : a.home);
    a.called.clear();
    a.awaiting = 0;
    settle_calls(w, id, observer);
    if (a.members.empty() || a.target == kNone) {
        disband(w, id, observer);
        return;
    }
    a.action = ArmyAction::Move;
    go(w, id, a.target, a.target_node);
}

void on_arrive(World& w, Id id, std::uint32_t seq, Observer* observer) {
    Army& a = w.armies[at(id)];
    if (!a.alive || seq != a.travel_seq || a.route.empty()) return;
    if (a.leg + 1 < static_cast<std::int32_t>(a.route.size())) {
        // A gate it meant to get on or off at is blockaded (MIL-17): it finds out, and
        // goes another way.
        const Leg& done = a.route[at(a.leg)];
        const Leg& next = a.route[at(a.leg + 1)];
        const bool off = done.kind == LegKind::Lattice && next.kind != LegKind::Lattice;
        const bool on = done.kind != LegKind::Lattice && next.kind == LegKind::Lattice;
        const Id faction = fealty::root_of(w, a.home);
        if (done.to_node != kNone && (on || off) && gates::shut_to(w, done.to_node, faction)) {
            const Id node = done.to_node;
            gates::observe(w, w.characters[at(a.commissioner)].facts, node);
            for (Id m : a.members) gates::observe(w, w.characters[at(m)].facts, node);
            const auto np = static_cast<std::int32_t>(w.planets.size());
            const Leg& last = a.route.back();
            const std::int32_t dest = last.to_planet != kNone ? last.to_planet : np + last.to_node;
            std::vector<Leg> again = move::plan_between(w, faction, np + node, dest, army_facts(w, a), off);
            if (!again.empty()) {
                a.route = std::move(again);
                a.leg = 0;
                ++a.travel_seq;
                start_leg(w, id);
                return;
            }
        }
        ++a.leg;
        start_leg(w, id);
        return;
    }
    const Leg last = a.route.back();
    a.planet = last.to_planet;
    a.node = last.to_planet == kNone ? last.to_node : kNone;
    a.route.clear();
    a.leg = 0;
    if (a.node != kNone) {
        at_gate(w, id, observer);
        return;
    }
    for (Id m : a.members) w.characters[at(m)].last_planet = a.planet;
    if (a.action == ArmyAction::Retreat || a.target == kNone || a.planet != a.target || a.target_node != kNone) {
        if (a.planet == a.home || a.target == kNone) {
            disband(w, id, observer);
        } else {
            go(w, id, a.target, a.target_node);
        }
        return;
    }
    const Id t = a.target;
    if (a.relief) {
        const Id siege = w.planets[at(t)].siege;
        if (siege == kNone || !w.sieges[at(siege)].alive) {
            // Here before the siege: garrison the planet until the threat passes or
            // army.muster_days go by (MIL-11, INF-12).
            a.action = ArmyAction::Garrison;
            w.events.push({.time = w.now + w.config.army.muster_days * kDay, .kind = EventKind::ArmyMuster, .record = id,
                           .a = a.travel_seq + 1});
            if (observer) observer->log(w.now, "army.garrison", id, "at " + std::to_string(t));
            return;
        }
        // Relief joins the defenders against the besiegers (MIL-11).
        const Id besiegers = w.sieges[at(siege)].army;
        std::vector<Id> defenders = a.members;
        for (Id d : ready(w, t)) defenders.push_back(d);
        combat::start(w, w.armies[at(besiegers)].members, defenders, t, true, besiegers, id, observer);
        return;
    }
    if (!diplomacy::hostile_planets(w, a.home, t)) {
        retreat(w, id, observer);
        return;
    }
    besiege(w, id, observer);
}

void battle_over(World& w, Id battle, int winner, Observer* observer) {
    const Battle& b = w.battles[at(battle)];
    const Id attacker = b.armies[0], reliever = b.armies[1];
    if (attacker == kNone) return;
    Army& a = w.armies[at(attacker)];
    if (!a.alive) return;
    const Id siege = a.siege;
    if ((siege == kNone || !w.sieges[at(siege)].alive) && a.target_node != kNone) {
        // A fight to take back a blockaded gate from its garrison (MIL-17).
        const Id n = a.target_node;
        if (winner == 0 && w.gates[at(n)].captor != kNone && fealty::same_faction(w, gates::holder(w, n), a.home)) {
            gates::retake(w, n, observer);
        }
        if (reliever != kNone && w.armies[at(reliever)].alive) retreat(w, reliever, observer);
        retreat(w, attacker, observer);
        return;
    }
    if (winner == 0) {
        // The defenders are gone: the siege goes on against the static defence.
        if (reliever != kNone) retreat(w, reliever, observer);
        if (siege != kNone && w.sieges[at(siege)].alive) {
            w.events.push({.time = w.now + kDay, .kind = EventKind::SiegeRound, .record = siege});
        }
        return;
    }
    // The attackers lost: the siege is lifted.
    const Id t = b.planet;
    if (siege != kNone) end_siege(w, siege);
    if (reliever != kNone) {
        w.sieges[at(siege)].relieved = true;
        ++w.ledger.reliefs;
        retreat(w, reliever, observer);
    }
    if (t != kNone && w.planets[at(t)].liege != kNone) {
        // A border held (REL-3): the liege thinks better of the planet's governor.
        opinion::deed(w, w.planets[at(w.planets[at(t)].liege)].governor, w.planets[at(t)].governor, DeedKind::BorderHeld, observer);
    }
    retreat(w, attacker, observer);
}

void siege_round(World& w, Id id, Observer* observer) {
    Siege& s = w.sieges[at(id)];
    if (!s.alive) return;
    Army& a = w.armies[at(s.army)];
    // Peace, or the planet came over to the attacker's side (a defection): no capture.
    if (a.alive && a.target != kNone && !diplomacy::hostile_planets(w, a.home, s.planet)) a.target = kNone;
    if (!a.alive || a.members.empty() || a.target == kNone) {
        end_siege(w, id);
        if (a.alive) retreat(w, s.army, observer);
        return;
    }
    prizes::siege_day(w, id);  // a day served, for the spoils (MIL-19)
    // A battle at the planet (a relief) pauses the siege.
    for (Id m : a.members) {
        if (w.characters[at(m)].battle != kNone) {
            w.events.push({.time = w.now + kDay, .kind = EventKind::SiegeRound, .record = id});
            return;
        }
    }
    // A gate's static defence, or the planet's (MIL-17, MIL-10).
    double& defence = s.node != kNone ? w.gates[at(s.node)].defence : w.planets[at(s.planet)].defence;
    const auto& c = w.config.combat;
    Rng& rng = w.rng[Stream::Combat];
    const double attack = power(w, a);
    if (defence > 0) {
        const double hit = c.lethality * attack * rng.uniform(1 - c.noise, 1 + c.noise);
        const double back = c.lethality * defence * rng.uniform(1 - c.noise, 1 + c.noise);
        defence = std::max(0.0, defence - hit);
        double carried = 0;
        const double lost = combat::destroy(w, a.members, back, carried, observer);
        diplomacy::record_loss(w, fealty::root_of(w, a.home), fealty::root_of(w, s.planet), 0, lost);
    }
    if (a.members.empty()) {
        end_siege(w, id);
        disband(w, s.army, observer);
        return;
    }
    if (s.node != kNone && defence <= 0) {
        // The gate falls: a blockade, held by the army for blockade.hold_days (MIL-17).
        const Id n = s.node, army = s.army;
        end_siege(w, id);
        Army& held = w.armies[at(army)];
        held.siege = kNone;
        held.action = ArmyAction::Garrison;
        gates::capture(w, n, army, observer);
        w.events.push({.time = w.now + w.config.blockade.hold_days * kDay, .kind = EventKind::ArmyMuster, .record = army,
                       .a = held.travel_seq + 1});
        return;
    }
    if (s.node == kNone && defence <= 0 && ready(w, s.planet).empty()) {
        capture(w, id, observer);
        return;
    }
    double risk = 0, strongest = -1;
    for (Id m : a.members) {
        const double f = ships::fleet(w, w.characters[at(m)]).combat;
        if (f > strongest) strongest = f, risk = w.characters[at(m)].risk;
    }
    if (power(w, a) < (c.retreat_base - c.retreat_risk * risk) * defence) {
        end_siege(w, id);
        retreat(w, s.army, observer);
        return;
    }
    w.events.push({.time = w.now + kDay, .kind = EventKind::SiegeRound, .record = id});
}

void call_for_help(World& w, Id id, Id liege, Observer* observer) {
    const Siege& s = w.sieges[at(id)];
    if (!s.alive) return;
    const Id gov = w.planets[at(liege)].governor;
    const Id asking = w.planets[at(s.planet)].governor;
    const std::vector<Id> free = ready(w, liege, true);
    const double relief = fleet_power(w, free);
    const double besiegers = power(w, w.armies[at(s.army)]);
    // Relief goes when it stands a fair chance (proposed): P(win) of at least a third
    // against the besiegers. A gate's holder answers its own gate's call first (MIL-17).
    const bool self = liege == s.planet;
    if (relief > 0 && win_chance(relief, besiegers) >= 1.0 / 3 && fealty::same_faction(w, liege, s.planet)) {
        if (commission(w, liege, s.planet, true, observer, s.node) != kNone) {
            if (!self) opinion::deed(w, asking, gov, DeedKind::HelpSent, observer);
            return;
        }
    }
    if (!self) opinion::deed(w, asking, gov, DeedKind::HelpMissed, observer);
    // Up the tree (INF-8).
    const Id up = w.planets[at(liege)].liege;
    if (up != kNone) {
        const Time trip = std::max<Time>(1, static_cast<Time>(w.derived.travel(liege, up) * kDay));
        w.events.push({.time = w.now + trip, .kind = EventKind::CallForHelp, .record = id, .a = up});
    }
}

void leave(World& w, Id army, Id id) {
    Army& a = w.armies[at(army)];
    std::erase(a.members, id);
    std::erase(a.called, id);
}

void disband(World& w, Id id, Observer* observer) {
    Army& a = w.armies[at(id)];
    if (!a.alive) return;
    leave_gate(w, id);
    a.alive = false;
    a.action = ArmyAction::Disband;
    const Id where = a.planet != kNone ? a.planet : a.home;
    for (Id m : std::vector<Id>(a.members)) release(w, m, where);
    for (Id m : std::vector<Id>(a.called)) release(w, m, w.characters[at(m)].planet != kNone ? w.characters[at(m)].planet : a.home);
    a.members.clear();
    a.called.clear();
    loans::end_for(w, id, observer);  // its purpose is done (MIL-3)
    if (observer) observer->log(w.now, "army.disbanded", id, "at " + std::to_string(where));
}

}  // namespace lattice::armies
