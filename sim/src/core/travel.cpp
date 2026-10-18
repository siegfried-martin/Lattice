#include "core/travel.hpp"

#include <cmath>
#include <limits>

namespace lattice::travel {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

}  // namespace

// Dense Dijkstra over planets and Lattice nodes. Open space joins every pair, so the
// graph is complete; at v1 size that is a few hundred vertices, cheap to run per planet.
std::vector<double> planet_times(const World& w) {
    const auto& c = w.config.travel;
    const std::size_t np = w.planets.size(), nn = w.map.nodes.size(), n = np + nn;
    std::vector<double> x(n), y(n);
    for (std::size_t i = 0; i < np; ++i) x[i] = w.planets[i].x, y[i] = w.planets[i].y;
    for (std::size_t i = 0; i < nn; ++i) x[np + i] = w.map.nodes[i].x, y[np + i] = w.map.nodes[i].y;

    // Faster links than open space: the Lattice and hop lanes.
    std::vector<std::vector<std::pair<std::size_t, double>>> links(n);
    for (const LatticeEdge& e : w.map.edges) {
        const double t = e.length / c.lattice_speed;
        links[np + at(e.a)].emplace_back(np + at(e.b), t);
        links[np + at(e.b)].emplace_back(np + at(e.a), t);
    }
    for (const HopLane& h : w.map.hop_lanes) {
        links[at(h.a)].emplace_back(at(h.b), c.hop_days);
        links[at(h.b)].emplace_back(at(h.a), c.hop_days);
    }

    std::vector<double> out(np * np);
    std::vector<double> best(n);
    std::vector<bool> done(n);
    for (std::size_t src = 0; src < np; ++src) {
        std::fill(best.begin(), best.end(), std::numeric_limits<double>::max());
        std::fill(done.begin(), done.end(), false);
        best[src] = 0;
        for (std::size_t step = 0; step < n; ++step) {
            std::size_t u = n;
            for (std::size_t i = 0; i < n; ++i) {
                if (!done[i] && (u == n || best[i] < best[u])) u = i;
            }
            done[u] = true;
            for (std::size_t v = 0; v < n; ++v) {
                if (done[v]) continue;
                const double t = best[u] + std::hypot(x[u] - x[v], y[u] - y[v]) / c.space_speed;
                if (t < best[v]) best[v] = t;
            }
            for (auto [v, t] : links[u]) {
                if (!done[v] && best[u] + t < best[v]) best[v] = best[u] + t;
            }
        }
        for (std::size_t dst = 0; dst < np; ++dst) out[src * np + dst] = best[dst];
    }
    return out;
}

}  // namespace lattice::travel
