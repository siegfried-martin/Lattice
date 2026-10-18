#include "core/travel.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace lattice::travel {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

}  // namespace

// Dense Dijkstra over planets and Lattice nodes. Open space joins every pair, so the
// graph is complete; at v1 size that is a few hundred vertices, cheap to run per planet.
Routes planet_routes(const World& w) {
    const auto& c = w.config.travel;
    const std::size_t np = w.planets.size(), nn = w.map.nodes.size(), n = np + nn;
    std::vector<double> x(n), y(n);
    for (std::size_t i = 0; i < np; ++i) x[i] = w.planets[i].x, y[i] = w.planets[i].y;
    for (std::size_t i = 0; i < nn; ++i) x[np + i] = w.map.nodes[i].x, y[np + i] = w.map.nodes[i].y;

    // Faster links than open space: the Lattice and hop lanes.
    struct Link {
        std::size_t to;
        double t;
        LegKind kind;
    };
    std::vector<std::vector<Link>> links(n);
    for (const LatticeEdge& e : w.map.edges) {
        const double t = e.length / c.lattice_speed;
        links[np + at(e.a)].push_back({np + at(e.b), t, LegKind::Lattice});
        links[np + at(e.b)].push_back({np + at(e.a), t, LegKind::Lattice});
    }
    for (const HopLane& h : w.map.hop_lanes) {
        links[at(h.a)].push_back({at(h.b), c.hop_days, LegKind::Hop});
        links[at(h.b)].push_back({at(h.a), c.hop_days, LegKind::Hop});
    }

    Routes out;
    out.days.resize(np * np);
    out.pred.resize(np * n);
    out.via.resize(np * n);
    std::vector<double> best(n);
    std::vector<bool> done(n);
    for (std::size_t src = 0; src < np; ++src) {
        std::fill(best.begin(), best.end(), std::numeric_limits<double>::max());
        std::fill(done.begin(), done.end(), false);
        best[src] = 0;
        std::int32_t* pred = &out.pred[src * n];
        std::uint8_t* via = &out.via[src * n];
        std::fill(pred, pred + n, -1);
        std::fill(via, via + n, static_cast<std::uint8_t>(LegKind::Space));
        for (std::size_t step = 0; step < n; ++step) {
            std::size_t u = n;
            for (std::size_t i = 0; i < n; ++i) {
                if (!done[i] && (u == n || best[i] < best[u])) u = i;
            }
            done[u] = true;
            for (std::size_t v = 0; v < n; ++v) {
                if (done[v]) continue;
                const double t = best[u] + std::hypot(x[u] - x[v], y[u] - y[v]) / c.space_speed;
                if (t < best[v]) {
                    best[v] = t;
                    pred[v] = static_cast<std::int32_t>(u);
                    via[v] = static_cast<std::uint8_t>(LegKind::Space);
                }
            }
            for (const Link& l : links[u]) {
                if (!done[l.to] && best[u] + l.t < best[l.to]) {
                    best[l.to] = best[u] + l.t;
                    pred[l.to] = static_cast<std::int32_t>(u);
                    via[l.to] = static_cast<std::uint8_t>(l.kind);
                }
            }
        }
        for (std::size_t dst = 0; dst < np; ++dst) out.days[src * np + dst] = best[dst];
    }
    return out;
}

std::vector<Step> path_avoiding(const World& w, Id a, Id b, const std::vector<bool>& closed, bool board_at_start) {
    const auto& c = w.config.travel;
    const std::size_t np = w.planets.size(), nn = w.map.nodes.size(), n = np + nn;
    auto pos = [&](std::size_t v) {
        return v < np ? std::pair{w.planets[v].x, w.planets[v].y} : std::pair{w.map.nodes[v - np].x, w.map.nodes[v - np].y};
    };
    const std::size_t start = at(a);
    auto shut = [&](std::size_t v) { return v >= np && closed[v - np] && (board_at_start || v != start); };
    std::vector<std::vector<std::pair<std::size_t, LegKind>>> links(n);
    for (const LatticeEdge& e : w.map.edges) {
        links[np + at(e.a)].emplace_back(np + at(e.b), LegKind::Lattice);
        links[np + at(e.b)].emplace_back(np + at(e.a), LegKind::Lattice);
    }
    for (const HopLane& h : w.map.hop_lanes) {
        links[at(h.a)].emplace_back(at(h.b), LegKind::Hop);
        links[at(h.b)].emplace_back(at(h.a), LegKind::Hop);
    }
    std::vector<double> best(n, std::numeric_limits<double>::max());
    std::vector<std::int32_t> pred(n, -1);
    std::vector<LegKind> via(n, LegKind::Space);
    std::vector<bool> done(n, false);
    best[at(a)] = 0;
    for (std::size_t step = 0; step < n; ++step) {
        std::size_t u = n;
        for (std::size_t i = 0; i < n; ++i) {
            if (!done[i] && (u == n || best[i] < best[u])) u = i;
        }
        if (u == n || best[u] == std::numeric_limits<double>::max()) break;
        done[u] = true;
        const auto [ux, uy] = pos(u);
        for (std::size_t v = 0; v < n; ++v) {
            if (done[v] || shut(u) || shut(v)) continue;  // no getting on or off at a shut gate
            const auto [vx, vy] = pos(v);
            const double t = best[u] + std::hypot(ux - vx, uy - vy) / c.space_speed;
            if (t < best[v]) best[v] = t, pred[v] = static_cast<std::int32_t>(u), via[v] = LegKind::Space;
        }
        for (auto [v, kind] : links[u]) {
            if (done[v] || (u == start && !board_at_start && kind == LegKind::Lattice)) continue;
            const auto [vx, vy] = pos(v);
            const double t = best[u] + (kind == LegKind::Hop ? c.hop_days : std::hypot(ux - vx, uy - vy) / c.lattice_speed);
            if (t < best[v]) best[v] = t, pred[v] = static_cast<std::int32_t>(u), via[v] = kind;
        }
    }
    if (pred[at(b)] < 0 && a != b) return {};
    std::vector<Step> out{{b, via[at(b)]}};
    while (out.back().vertex != a) {
        const std::int32_t p = pred[at(out.back().vertex)];
        out.push_back({p, via[at(p)]});
    }
    std::reverse(out.begin(), out.end());
    return out;
}

std::vector<std::int32_t> path(const World& w, Id a, Id b) {
    const std::size_t n = w.planets.size() + w.map.nodes.size();
    const std::int32_t* pred = &w.derived.route_pred[at(a) * n];
    std::vector<std::int32_t> out{b};
    while (out.back() != a) {
        const std::int32_t p = pred[at(out.back())];
        if (p < 0) return {};
        out.push_back(p);
    }
    std::reverse(out.begin(), out.end());
    return out;
}

}  // namespace lattice::travel
