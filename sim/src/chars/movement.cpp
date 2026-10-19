#include "chars/movement.hpp"

#include <algorithm>
#include <cmath>

#include "chars/characters.hpp"
#include "chars/ships.hpp"
#include "core/travel.hpp"
#include "politics/fealty.hpp"
#include "info/facts.hpp"
#include "war/diplomacy.hpp"
#include "war/gates.hpp"
#include "war/risk.hpp"

namespace lattice::move {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

std::pair<double, double> vertex_pos(const World& w, std::int32_t v) {
    const auto np = static_cast<std::int32_t>(w.planets.size());
    if (v < np) return {w.planets[at(v)].x, w.planets[at(v)].y};
    const LatticeNode& n = w.map.nodes[at(v - np)];
    return {n.x, n.y};
}

Time to_time(double days) { return std::max<Time>(1, static_cast<Time>(std::ceil(days * static_cast<double>(kDay)))); }

double leg_days(const World& w, const Leg& l, double speed) {
    switch (l.kind) {
        case LegKind::Space: return std::hypot(l.x1 - l.x0, l.y1 - l.y0) / speed;
        case LegKind::Lattice: return l.lattice_length / w.config.travel.lattice_speed;
        case LegKind::Hop: return w.config.travel.hop_days;
    }
    return 0;
}

}  // namespace

namespace {

// Open-space and hop-lane steps one by one, consecutive Lattice edges merged into one
// transit (TIM-5).
std::vector<Leg> build(const World& w, const std::vector<travel::Step>& steps) {
    std::vector<Leg> out;
    if (steps.size() < 2) return out;
    const auto np = static_cast<std::int32_t>(w.planets.size());
    for (std::size_t i = 1; i < steps.size(); ++i) {
        const std::int32_t u = steps[i - 1].vertex, v = steps[i].vertex;
        const LegKind kind = steps[i].via;
        const auto [x0, y0] = vertex_pos(w, u);
        const auto [x1, y1] = vertex_pos(w, v);
        const Id to_planet = v < np ? v : kNone;
        const Id to_node = v < np ? kNone : v - np;
        if (kind == LegKind::Lattice && !out.empty() && out.back().kind == LegKind::Lattice) {
            Leg& l = out.back();
            l.x1 = x1, l.y1 = y1, l.to_node = to_node, l.to_planet = to_planet;
            l.lattice_length += std::hypot(x1 - x0, y1 - y0);
            continue;
        }
        out.push_back({.kind = kind, .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1, .to_planet = to_planet, .to_node = to_node,
                       .lattice_length = kind == LegKind::Lattice ? std::hypot(x1 - x0, y1 - y0) : 0});
    }
    return out;
}

}  // namespace

std::vector<Leg> route_legs(const World& w, Id a, Id b) {
    const std::vector<std::int32_t> verts = travel::path(w, a, b);
    const std::size_t n = w.planets.size() + w.map.nodes.size();
    std::vector<travel::Step> steps;
    for (std::int32_t v : verts) steps.push_back({v, static_cast<LegKind>(w.derived.route_via[at(a) * n + at(v)])});
    return build(w, steps);
}

namespace {

// The gates shut to a traveller of this faction (WLD-12, MIL-17).
std::vector<bool> shut_gates(const World& w, Id faction, const Facts* facts) {
    const std::vector<Id> enemies = diplomacy::enemies(w, faction);
    std::vector<bool> closed(w.map.nodes.size(), false);
    for (Id n = 0; n < static_cast<Id>(w.map.nodes.size()); ++n) {
        const LatticeNode& node = w.map.nodes[at(n)];
        // The Authority refuses full pirates at every gate (WLD-12, PIR-2); junctions have
        // only its key (OPN-22).
        if (faction == kPirates) {
            closed[at(n)] = true;
        } else if (node.kind == NodeKind::Interchange) {
            const Id holder = w.map.groups[at(node.group)].holder;
            const Id holder_root = fealty::root_of(w, holder);
            // A pirate planet's gates are shut with the Authority's key (WLD-12, proposed);
            // an enemy's with its own, unless the traveller's faction holds it (MIL-17).
            const Id captor = gates::captor_faction(w, n);
            if (captor != kNone && captor == faction) continue;
            closed[at(n)] = w.planets[at(holder)].pirate ||
                            std::find(enemies.begin(), enemies.end(), holder_root) != enemies.end() ||
                            (facts ? gates::believed_shut(w, *facts, n, faction) : gates::shut_to(w, n, faction));
        }
    }
    return closed;
}

}  // namespace

std::vector<Leg> plan(const World& w, Id faction, Id a, Id b, const Facts* facts) {
    std::vector<Leg> legs = route_legs(w, a, b);
    const std::vector<bool> closed = shut_gates(w, faction, facts);
    if (std::none_of(closed.begin(), closed.end(), [](bool x) { return x; })) return legs;
    // Does the reference route get on or off at a shut gate?
    bool blocked = faction == kPirates && std::any_of(legs.begin(), legs.end(), [](const Leg& l) { return l.kind == LegKind::Lattice; });
    for (std::size_t i = 0; i < legs.size(); ++i) {
        if (legs[i].kind != LegKind::Space) continue;
        if (legs[i].to_node != kNone && closed[at(legs[i].to_node)]) blocked = true;
        if (i > 0 && legs[i - 1].kind == LegKind::Lattice && legs[i - 1].to_node != kNone && closed[at(legs[i - 1].to_node)]) blocked = true;
    }
    if (!blocked) return legs;
    return build(w, travel::path_avoiding(w, a, b, closed));
}

std::vector<Leg> plan_between(const World& w, Id faction, std::int32_t a, std::int32_t b, const Facts* facts,
                              bool board_at_start) {
    std::vector<bool> closed = shut_gates(w, faction, facts);
    const auto np = static_cast<std::int32_t>(w.planets.size());
    if (b >= np) closed[at(b - np)] = false;
    return build(w, travel::path_avoiding(w, a, b, closed, board_at_start));
}

Time leg_time(const World& w, const Leg& l, double speed) {
    return to_time(leg_days(w, l, speed > 0 ? speed : w.config.travel.space_speed)) + l.wait;
}

std::vector<Leg> sortie(const World& w, Id faction, Id home, Id target, double standoff, Time hold) {
    if (home == target) {
        // Patrolling its own space: a short loop out and back.
        const Planet& p = w.planets[at(home)];
        const double x = p.x + standoff, y = p.y;
        return {{.kind = LegKind::Space, .x0 = p.x, .y0 = p.y, .x1 = x, .y1 = y},
                {.kind = LegKind::Space, .x0 = x, .y0 = y, .x1 = x, .y1 = y, .wait = hold},
                {.kind = LegKind::Space, .x0 = x, .y0 = y, .x1 = p.x, .y1 = p.y, .to_planet = home}};
    }
    std::vector<Leg> out = plan(w, faction, home, target);
    std::vector<Leg> back = plan(w, faction, target, home);
    if (out.empty() || back.empty()) return {};
    // Stop short on the last leg in: on its line, standoff before the planet.
    Leg& last = out.back();
    const double dx = last.x0 - last.x1, dy = last.y0 - last.y1, len = std::hypot(dx, dy);
    const double k = len > 0 ? std::min(1.0, standoff / len) : 0;
    const double wx = last.x1 + dx * k, wy = last.y1 + dy * k;
    if (last.kind == LegKind::Space) {
        last.x1 = wx, last.y1 = wy;
        last.to_planet = kNone, last.to_node = kNone;
    } else {
        out.push_back({.kind = LegKind::Space, .x0 = last.x1, .y0 = last.y1, .x1 = wx, .y1 = wy});
    }
    out.push_back({.kind = LegKind::Space, .x0 = wx, .y0 = wy, .x1 = wx, .y1 = wy, .wait = hold});
    // Home from where it waited, joining the way back after its first step.
    Leg first = back.front();
    if (first.kind == LegKind::Space) {
        first.x0 = wx, first.y0 = wy;
        out.push_back(first);
    } else {
        out.push_back({.kind = LegKind::Space, .x0 = wx, .y0 = wy, .x1 = first.x0, .y1 = first.y0});
        out.push_back(first);
    }
    for (std::size_t i = 1; i < back.size(); ++i) out.push_back(back[i]);
    return out;
}

namespace {

void start_leg(World& w, Id id) {
    Character& c = w.characters[at(id)];
    Leg& l = c.route[at(c.leg)];
    const ships::Fleet f = ships::fleet(w, c);
    if (l.kind == LegKind::Lattice && c.last_planet != kNone) {
        // Fuel is charged at entry, at the price where the fleet last refuelled (TIM-5).
        const double fuel = l.lattice_length * w.config.fuel.burn * w.planets[at(c.last_planet)].fuel_price * f.count;
        c.wealth -= fuel;
        w.ledger.fuel += fuel;
        w.ledger.money_sunk += fuel;
    }
    l.depart = w.now;
    l.arrive = w.now + to_time(leg_days(w, l, f.speed > 0 ? f.speed : w.config.travel.space_speed));
    w.events.push({.time = l.arrive, .kind = EventKind::CharArrive, .record = id, .a = c.travel_seq});
}

}  // namespace

bool start_route(World& w, Id id, std::vector<Leg> route) {
    Character& c = w.characters[at(id)];
    if (c.planet == kNone || route.empty()) return false;
    c.route = std::move(route);
    c.leg = 0;
    ++c.travel_seq;
    c.planet = kNone;
    if (c.escort != kNone) w.characters[at(c.escort)].planet = kNone;  // carried along (TRD-8)
    start_leg(w, id);
    return true;
}

bool travel_to(World& w, Id id, Id dest) {
    const Character& c = w.characters[at(id)];
    if (c.planet == kNone || c.planet == dest) return false;
    return start_route(w, id, plan(w, chars::faction_of(w, c), c.planet, dest, &c.facts));
}

void pause(World& w, Id id) {
    Character& c = w.characters[at(id)];
    if (c.planet != kNone || c.route.empty() || c.paused_at != kNever) return;
    c.paused_at = w.now;
    ++c.travel_seq;  // the leg's arrival no longer stands
}

void resume(World& w, Id id) {
    Character& c = w.characters[at(id)];
    if (c.paused_at == kNever) return;
    const Time lost = w.now - c.paused_at;
    c.paused_at = kNever;
    if (!c.alive || c.route.empty() || c.planet != kNone) return;
    Leg& l = c.route[at(c.leg)];
    l.depart += lost;
    l.arrive += lost;
    w.events.push({.time = std::max(l.arrive, w.now), .kind = EventKind::CharArrive, .record = id, .a = c.travel_seq});
}

double trip_days(const World& w, const Character& c, Id from, Id to) {
    if (from == to) return 0;
    const Derived::RouteSum& r = w.derived.route(from, to);
    double speed = 0;
    for (Id v : c.ships) {
        const double s = w.map.variants[at(v)].speed;
        speed = speed == 0 ? s : std::min(speed, s);
    }
    if (speed <= 0) speed = w.config.travel.space_speed;
    const auto& t = w.config.travel;
    return r.space / speed + r.lattice / t.lattice_speed + r.hops * t.hop_days;
}

double fuel_cost(const World& w, const Character& c, Id from, Id to) {
    if (from == to) return 0;
    return w.derived.route(from, to).lattice * w.config.fuel.burn * w.planets[at(from)].fuel_price *
           static_cast<double>(c.ships.size());
}

void on_arrive(World& w, Id id, std::uint32_t seq, Observer* observer) {
    Character& c = w.characters[at(id)];
    if (!c.alive || seq != c.travel_seq || c.route.empty()) return;  // a stale arrival
    // Passing through a sector shows its risk (INF-6).
    const Leg& done = c.route[at(c.leg)];
    info::observe_risk(w, c.facts, risk::sector_at(w.map, done.x1, done.y1));
    if (c.leg + 1 < static_cast<std::int32_t>(c.route.size())) {
        // At a gate it meant to get on or off at: is it blockaded (MIL-17)? Then it knows,
        // and goes another way.
        const Leg& next = c.route[at(c.leg + 1)];
        const bool off = done.kind == LegKind::Lattice && next.kind != LegKind::Lattice;
        const bool on = done.kind != LegKind::Lattice && next.kind == LegKind::Lattice;
        const Id faction = chars::faction_of(w, c);
        if (done.to_node != kNone && (on || off) && gates::shut_to(w, done.to_node, faction)) {
            const Id node = done.to_node;
            gates::observe(w, c.facts, node);
            const Id dest = c.route.back().to_planet;
            const auto np = static_cast<std::int32_t>(w.planets.size());
            std::vector<Leg> again = dest == kNone ? std::vector<Leg>{}
                                                   : plan_between(w, faction, np + node, dest, &c.facts, off);
            if (!again.empty()) {
                c.route = std::move(again);
                c.leg = 0;
                ++c.travel_seq;
                start_leg(w, id);
                return;
            }
        }
        ++c.leg;
        start_leg(w, id);
        return;
    }
    const Id dest = c.route.back().to_planet;
    c.route.clear();
    c.leg = 0;
    c.planet = dest;
    c.last_planet = dest;
    if (c.escort != kNone) {
        Character& e = w.characters[at(c.escort)];
        e.planet = e.last_planet = dest;
    }
    chars::landed(w, id, observer);
}

std::pair<double, double> position(const World& w, const Character& c) {
    if (c.planet != kNone) return {w.planets[at(c.planet)].x, w.planets[at(c.planet)].y};
    if (c.route.empty()) return {0, 0};
    const Leg& l = c.route[at(c.leg)];
    const double span = static_cast<double>(std::max<Time>(1, l.arrive - l.depart));
    const double t = std::clamp(static_cast<double>(w.now - l.depart) / span, 0.0, 1.0);
    return {l.x0 + (l.x1 - l.x0) * t, l.y0 + (l.y1 - l.y0) * t};
}

}  // namespace lattice::move
