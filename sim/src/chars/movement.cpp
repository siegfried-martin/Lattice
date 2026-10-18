#include "chars/movement.hpp"

#include <algorithm>
#include <cmath>

#include "chars/characters.hpp"
#include "chars/ships.hpp"
#include "core/travel.hpp"

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

// Open-space and hop-lane steps one by one, consecutive Lattice edges merged into one
// transit (TIM-5).
std::vector<Leg> route_legs(const World& w, Id a, Id b) {
    std::vector<Leg> out;
    const std::vector<std::int32_t> verts = travel::path(w, a, b);
    if (verts.size() < 2) return out;
    const auto np = static_cast<std::int32_t>(w.planets.size());
    const std::size_t n = w.planets.size() + w.map.nodes.size();
    for (std::size_t i = 1; i < verts.size(); ++i) {
        const std::int32_t u = verts[i - 1], v = verts[i];
        const auto kind = static_cast<LegKind>(w.derived.route_via[at(a) * n + at(v)]);
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

bool travel_to(World& w, Id id, Id dest) {
    Character& c = w.characters[at(id)];
    if (c.planet == kNone || c.planet == dest) return false;
    std::vector<Leg> route = route_legs(w, c.planet, dest);
    if (route.empty()) return false;
    c.route = std::move(route);
    c.leg = 0;
    ++c.travel_seq;
    c.planet = kNone;
    start_leg(w, id);
    return true;
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
    if (c.leg + 1 < static_cast<std::int32_t>(c.route.size())) {
        ++c.leg;
        start_leg(w, id);
        return;
    }
    const Id dest = c.route.back().to_planet;
    c.route.clear();
    c.leg = 0;
    c.planet = dest;
    c.last_planet = dest;
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

void kinematic_step(World& w, Observer*) {
    // Only ships that something could happen to are hashed: in MS-3 that is members who
    // may share facts (SYS-15). Hostile meetings (battles, raids) add their own interest.
    const double r = w.config.space.detect_range;
    const Time cooldown = static_cast<Time>(w.config.space.meet_cooldown_hours * kHour);
    struct Entry {
        std::int64_t cx, cy;
        Id id;
        Id faction;
        double x, y;
    };
    std::vector<Entry> in_space;
    for (Id id = 0; id < static_cast<Id>(w.characters.size()); ++id) {
        const Character& c = w.characters[at(id)];
        if (!c.alive || c.planet != kNone || c.route.empty() || c.route[at(c.leg)].kind != LegKind::Space) continue;
        if (w.now - c.last_meet < cooldown) continue;
        const Id faction = chars::faction_of(w, c);
        if (faction == kNone) continue;
        const auto [x, y] = position(w, c);
        in_space.push_back({static_cast<std::int64_t>(std::floor(x / r)), static_cast<std::int64_t>(std::floor(y / r)), id,
                            faction, x, y});
    }
    auto key = [](const Entry& e) { return std::pair{e.cx, e.cy}; };
    std::sort(in_space.begin(), in_space.end(), [&](const Entry& a, const Entry& b) {
        return std::tie(a.cx, a.cy, a.id) < std::tie(b.cx, b.cy, b.id);
    });
    for (const Entry& a : in_space) {
        for (std::int64_t dx = -1; dx <= 1; ++dx) {
            for (std::int64_t dy = -1; dy <= 1; ++dy) {
                const std::pair cell{a.cx + dx, a.cy + dy};
                auto it = std::lower_bound(in_space.begin(), in_space.end(), cell,
                                           [&](const Entry& e, const std::pair<std::int64_t, std::int64_t>& k) { return key(e) < k; });
                for (; it != in_space.end() && key(*it) == cell; ++it) {
                    if (it->id <= a.id || it->faction != a.faction) continue;  // each pair once
                    if (std::hypot(it->x - a.x, it->y - a.y) <= r) chars::detected(w, a.id, it->id);
                }
            }
        }
    }
}

}  // namespace lattice::move
