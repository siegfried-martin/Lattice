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
#include "war/combat.hpp"
#include "war/diplomacy.hpp"

namespace lattice::armies {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

double income(const World& w, Id p) {
    return std::max(0.0, w.planets[at(p)].income_avg) * static_cast<double>(w.config.time.payment_days);
}

// A governor's sworn commanders at its planet, free to march.
std::vector<Id> ready(const World& w, Id planet) {
    std::vector<Id> out;
    const Id gov = w.planets[at(planet)].governor;
    if (gov == kNone) return out;
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

void go(World& w, Id id, Id dest) {
    Army& a = w.armies[at(id)];
    const Id from = a.planet;
    std::vector<Leg> route = move::plan(w, fealty::root_of(w, a.home), from, dest);
    if (route.empty()) {
        disband(w, id, nullptr);
        return;
    }
    a.route = std::move(route);
    a.leg = 0;
    ++a.travel_seq;
    a.planet = kNone;
    for (Id m : a.members) {
        Character& c = w.characters[at(m)];
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
    w.planets[at(s.planet)].siege = kNone;
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

// FAC-9: the keeper chooses among its commanders and the army that took the planet:
// reputation + its opinion + fealty.capture_bonus for the army's leader (proposed).
Id choose_governor(const World& w, Id keeper, const Army& army) {
    const Id gov = w.planets[at(keeper)].governor;
    std::vector<Id> pool = army.members;
    for (Id k : w.characters[at(gov)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (ct.alive && ct.kind == ContractKind::Service && ct.employer == gov) pool.push_back(ct.employee);
    }
    Id leader = kNone;
    double strongest = -1;
    for (Id m : army.members) {
        const double p = ships::fleet(w, w.characters[at(m)]).combat;
        if (p > strongest) strongest = p, leader = m;
    }
    Id best = kNone;
    double best_score = -std::numeric_limits<double>::max();
    for (Id id : pool) {
        const Character& c = w.characters[at(id)];
        if (!c.alive || c.governs != kNone) continue;
        const double score = c.reputation + opinion::get(w, gov, id) + (id == leader ? w.config.fealty.capture_bonus : 0);
        if (score > best_score || (score == best_score && id < best)) best_score = score, best = id;
    }
    return best;
}

void capture(World& w, Id siege, Observer* observer) {
    const Siege s = w.sieges[at(siege)];
    end_siege(w, siege);
    Army& army = w.armies[at(s.army)];
    const Id t = s.planet;
    Planet& planet = w.planets[at(t)];
    const auto& cap = w.config.capture;

    // FAC-10: capture costs prosperity.
    planet.happiness *= 1 - cap.happiness_loss;
    for (int g = 0; g < kGoodCount; ++g) {
        w.ledger.destroyed[at(g)] += planet.stock[at(g)] * cap.stock_loss;
        planet.stock[at(g)] *= 1 - cap.stock_loss;
    }
    planet.population *= 1 - cap.pop_loss;

    const Id old_root = fealty::root_of(w, t);
    const Id winner_root = fealty::root_of(w, army.home);
    diplomacy::record_loss(w, old_root, winner_root, 1, 0);

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
    }

    const Id keeper = keep_or_grant(w, army.home, t, observer);
    const Id chosen = choose_governor(w, keeper, army);
    fealty::swear(w, t, keeper);
    if (chosen != kNone) {
        Character& g = w.characters[at(chosen)];
        for (Id k : std::vector<Id>(g.contracts)) {
            if (w.contracts[at(k)].employee == chosen) contracts::end(w, k);
        }
        leave(w, s.army, chosen);
        g.army = kNone;
        g.battle = kNone;
        g.governs = t;
        g.planet = t;
        g.last_planet = t;
        g.route.clear();
        ++g.travel_seq;
        g.activity = Activity::Governing;
        planet.governor = chosen;
        planet.growth_factor = w.config.pop.growth_factor_min +
                               g.risk * (w.config.pop.growth_factor_max - w.config.pop.growth_factor_min);
    }
    planet.security = 0;
    ++w.ledger.captures;
    if (observer) {
        observer->log(w.now, "planet.captured", t,
                      "by army " + std::to_string(s.army) + " kept by " + std::to_string(keeper) + " governor " + std::to_string(chosen));
    }
    disband(w, s.army, observer);
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
    const std::vector<Id> defenders = ready(w, t);
    if (!defenders.empty()) {
        combat::start(w, a.members, defenders, t, true, id, kNone, observer);
        return;
    }
    w.events.push({.time = w.now + kDay, .kind = EventKind::SiegeRound, .record = siege});
}

}  // namespace

double power(const World& w, const Army& a) { return fleet_power(w, a.members); }

std::pair<double, double> position(const World& w, const Army& a) {
    if (a.planet != kNone) return {w.planets[at(a.planet)].x, w.planets[at(a.planet)].y};
    if (a.route.empty()) return {0, 0};
    const Leg& l = a.route[at(a.leg)];
    const double span = static_cast<double>(std::max<Time>(1, l.arrive - l.depart));
    const double t = std::clamp(static_cast<double>(w.now - l.depart) / span, 0.0, 1.0);
    return {l.x0 + (l.x1 - l.x0) * t, l.y0 + (l.y1 - l.y0) * t};
}

Plan best_invasion(const World& w, Id planet) {
    Plan best;
    const Id root = fealty::root_of(w, planet);
    const std::vector<Id> enemies = diplomacy::enemies(w, root);
    if (enemies.empty()) return best;
    const Id gov = w.planets[at(planet)].governor;
    for (const Army& a : w.armies) {
        if (a.alive && a.commissioner == gov && !a.relief) return best;  // one campaign at a time
    }
    std::vector<Id> own = ready(w, planet), called;
    for (Id v : fealty::vassals(w, planet)) {
        for (Id id : ready(w, v)) called.push_back(id);
    }
    const double attack = fleet_power(w, own) + 0.5 * fleet_power(w, called);
    if (attack <= 0) return best;
    const double pay = stipends(w, own) + stipends(w, called);
    const auto& v = w.config.value;
    double best_ratio = 0;
    for (Id t = 0; t < static_cast<Id>(w.planets.size()); ++t) {
        const Id troot = fealty::root_of(w, t);
        if (std::find(enemies.begin(), enemies.end(), troot) == enemies.end()) continue;
        if (w.derived.travel(planet, t) > w.config.dip.border_days) continue;
        if (w.planets[at(t)].siege != kNone) continue;
        const double defence = w.planets[at(t)].defence + fleet_power(w, ready(w, t));
        const double p = win_chance(attack, defence);
        const double gain = income(w, t) * (v.invade_months + (governor::holds_gates(w, t) ? v.gate_months : 0) - v.aftermath_months);
        const double days = w.derived.travel(planet, t) + 3 + 10;  // muster, travel, and the fight
        const double cost = std::max(1.0, pay * days / static_cast<double>(w.config.time.payment_days));
        const double value = p * gain;
        if (value / cost > best_ratio) best_ratio = value / cost, best = {t, value, cost};
    }
    return best;
}

Id commission(World& w, Id planet, Id target, bool relief, Observer* observer) {
    const Id gov = w.planets[at(planet)].governor;
    w.armies.push_back({.commissioner = gov, .home = planet, .target = target, .relief = relief, .formed = w.now, .planet = planet});
    const auto id = static_cast<Id>(w.armies.size() - 1);
    for (Id m : ready(w, planet)) {
        attach(w, id, m);
        w.armies[at(id)].members.push_back(m);
    }
    if (!relief) {
        // The call to arms (MIL-9): each vassal answers by its opinion of its liege.
        for (Id v : fealty::vassals(w, planet)) {
            const Id vgov = w.planets[at(v)].governor;
            const bool answers = opinion::get(w, vgov, gov) >= 0;
            opinion::deed(w, gov, vgov, answers ? DeedKind::CallAnswered : DeedKind::CallRefused, observer);
            if (!answers) continue;
            const Time delay = std::max<Time>(1, static_cast<Time>(w.derived.travel(planet, v) * kDay));  // by courier
            for (Id m : ready(w, v)) {
                attach(w, id, m);
                w.armies[at(id)].called.push_back(m);
                chars::wake(w, m, w.now + delay);
            }
        }
    }
    Army& a = w.armies[at(id)];
    if (a.members.empty() && a.called.empty()) {
        a.alive = false;
        return kNone;
    }
    if (observer) {
        observer->log(w.now, relief ? "army.relief" : "army.raised", id,
                      "from " + std::to_string(planet) + " against " + std::to_string(target) + ", " +
                          std::to_string(a.members.size()) + " now, " + std::to_string(a.called.size()) + " called");
    }
    if (relief || a.called.empty()) {
        a.action = ArmyAction::Move;
        go(w, id, target);
    } else {
        w.events.push({.time = w.now + w.config.army.muster_days * kDay, .kind = EventKind::ArmyMuster, .record = id});
    }
    return id;
}

void member_arrived(World& w, Id id, Observer*) {
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
    if (a.called.empty()) {
        a.action = ArmyAction::Move;
        if (a.target == kNone) {
            disband(w, c.army, nullptr);
        } else {
            go(w, c.army, a.target);
        }
    }
}

void muster_deadline(World& w, Id id, Observer* observer) {
    Army& a = w.armies[at(id)];
    if (!a.alive || a.action != ArmyAction::Muster) return;
    // Those not there yet are left behind.
    for (Id m : std::vector<Id>(a.called)) release(w, m, w.characters[at(m)].planet != kNone ? w.characters[at(m)].planet : a.home);
    a.called.clear();
    if (a.members.empty() || a.target == kNone) {
        disband(w, id, observer);
        return;
    }
    a.action = ArmyAction::Move;
    go(w, id, a.target);
}

void on_arrive(World& w, Id id, std::uint32_t seq, Observer* observer) {
    Army& a = w.armies[at(id)];
    if (!a.alive || seq != a.travel_seq || a.route.empty()) return;
    if (a.leg + 1 < static_cast<std::int32_t>(a.route.size())) {
        ++a.leg;
        start_leg(w, id);
        return;
    }
    a.planet = a.route.back().to_planet;
    a.route.clear();
    a.leg = 0;
    for (Id m : a.members) w.characters[at(m)].last_planet = a.planet;
    if (a.action == ArmyAction::Retreat || a.target == kNone || a.planet != a.target) {
        if (a.planet == a.home || a.target == kNone) {
            disband(w, id, observer);
        } else {
            go(w, id, a.target);
        }
        return;
    }
    const Id t = a.target;
    if (a.relief) {
        const Id siege = w.planets[at(t)].siege;
        if (siege == kNone || !w.sieges[at(siege)].alive) {
            retreat(w, id, observer);
            return;
        }
        // Relief joins the defenders against the besiegers (MIL-11).
        const Id besiegers = w.sieges[at(siege)].army;
        std::vector<Id> defenders = a.members;
        for (Id d : ready(w, t)) defenders.push_back(d);
        combat::start(w, w.armies[at(besiegers)].members, defenders, t, true, besiegers, id, observer);
        return;
    }
    if (!diplomacy::planets_at_war(w, a.home, t)) {
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
    if (!a.alive || a.members.empty() || a.target == kNone) {
        end_siege(w, id);
        if (a.alive) retreat(w, s.army, observer);
        return;
    }
    // A battle at the planet (a relief) pauses the siege.
    for (Id m : a.members) {
        if (w.characters[at(m)].battle != kNone) {
            w.events.push({.time = w.now + kDay, .kind = EventKind::SiegeRound, .record = id});
            return;
        }
    }
    Planet& p = w.planets[at(s.planet)];
    const auto& c = w.config.combat;
    Rng& rng = w.rng[Stream::Combat];
    const double attack = power(w, a);
    if (p.defence > 0) {
        const double hit = c.lethality * attack * rng.uniform(1 - c.noise, 1 + c.noise);
        const double back = c.lethality * p.defence * rng.uniform(1 - c.noise, 1 + c.noise);
        p.defence = std::max(0.0, p.defence - hit);
        double carried = 0;
        const double lost = combat::destroy(w, a.members, back, carried, observer);
        diplomacy::record_loss(w, fealty::root_of(w, a.home), fealty::root_of(w, s.planet), 0, lost);
    }
    if (a.members.empty()) {
        end_siege(w, id);
        disband(w, s.army, observer);
        return;
    }
    if (p.defence <= 0 && ready(w, s.planet).empty()) {
        capture(w, id, observer);
        return;
    }
    double risk = 0, strongest = -1;
    for (Id m : a.members) {
        const double f = ships::fleet(w, w.characters[at(m)]).combat;
        if (f > strongest) strongest = f, risk = w.characters[at(m)].risk;
    }
    if (power(w, a) < (c.retreat_base - c.retreat_risk * risk) * p.defence) {
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
    const std::vector<Id> free = ready(w, liege);
    const double relief = fleet_power(w, free);
    const double besiegers = power(w, w.armies[at(s.army)]);
    // Relief goes when it stands a fair chance (proposed): P(win) of at least a third
    // against the besiegers.
    if (relief > 0 && win_chance(relief, besiegers) >= 1.0 / 3 && fealty::same_faction(w, liege, s.planet)) {
        if (commission(w, liege, s.planet, true, observer) != kNone) {
            opinion::deed(w, asking, gov, DeedKind::HelpSent, observer);
            return;
        }
    }
    opinion::deed(w, asking, gov, DeedKind::HelpMissed, observer);
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
    a.alive = false;
    a.action = ArmyAction::Disband;
    const Id where = a.planet != kNone ? a.planet : a.home;
    for (Id m : std::vector<Id>(a.members)) release(w, m, where);
    for (Id m : std::vector<Id>(a.called)) release(w, m, w.characters[at(m)].planet != kNone ? w.characters[at(m)].planet : a.home);
    a.members.clear();
    a.called.clear();
    if (observer) observer->log(w.now, "army.disbanded", id, "at " + std::to_string(where));
}

}  // namespace lattice::armies
