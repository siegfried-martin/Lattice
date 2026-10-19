#include "politics/fealty.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>

namespace lattice::fealty {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

}  // namespace

Id root_of(const World& w, Id planet) {
    Id p = planet;
    for (std::size_t guard = 0; guard <= w.planets.size(); ++guard) {
        const Id liege = w.planets[at(p)].liege;
        if (liege == kNone) return p;
        p = liege;
    }
    return p;  // a cycle would be a bug; make_trees and later rules never make one
}

bool same_faction(const World& w, Id a, Id b) { return root_of(w, a) == root_of(w, b); }

std::vector<Id> faction_planets(const World& w, Id root) {
    std::vector<Id> out;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (root_of(w, p) == root) out.push_back(p);
    }
    return out;
}

std::vector<Id> vassals(const World& w, Id planet) {
    std::vector<Id> out;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (w.planets[at(p)].liege == planet) out.push_back(p);
    }
    return out;
}

void make_trees(World& w) {
    const Map& m = w.map;
    // Breadth-first over the Lattice from the seats' interchanges: each node learns the
    // node it was reached from, one step closer to a seat (junction links are free, as
    // for fuel).
    const auto inf = std::numeric_limits<std::int64_t>::max();
    std::vector<std::int64_t> hops(m.nodes.size(), inf);
    std::vector<Id> from(m.nodes.size(), kNone);
    std::deque<Id> queue;
    for (Id s : m.seats) {
        const Id node = m.groups[at(w.planets[at(s)].group)].node;
        if (node != kNone && hops[at(node)] != 0) hops[at(node)] = 0, queue.push_back(node);
    }
    while (!queue.empty()) {
        const Id u = queue.front();
        queue.pop_front();
        for (Id e : m.nodes[at(u)].edges) {
            const LatticeEdge& edge = m.edges[at(e)];
            const Id v = edge.a == u ? edge.b : edge.a;
            const std::int64_t cost = edge.length > 0 ? 1 : 0;
            if (hops[at(u)] + cost < hops[at(v)]) {
                hops[at(v)] = hops[at(u)] + cost;
                from[at(v)] = u;
                cost ? queue.push_back(v) : queue.push_front(v);
            }
        }
    }

    auto is_seat = [&](Id p) { return std::find(m.seats.begin(), m.seats.end(), p) != m.seats.end(); };

    // On the network: a group's holder swears to the holder of the next interchange
    // toward its seat; the group's other planets swear to their holder.
    for (const Group& g : m.groups) {
        for (Id p : g.planets) {
            if (p != g.holder) w.planets[at(p)].liege = g.holder;
        }
        if (g.node == kNone || is_seat(g.holder)) continue;
        Id n = from[at(g.node)];
        while (n != kNone && m.nodes[at(n)].kind != NodeKind::Interchange) n = from[at(n)];
        if (n != kNone) w.planets[at(g.holder)].liege = m.groups[at(m.nodes[at(n)].group)].holder;
    }
    // Off the network: to the holder of the nearest group on it.
    for (const Group& g : m.groups) {
        if (g.node != kNone) continue;
        const Planet& h = w.planets[at(g.holder)];
        Id best = kNone;
        double best_d = std::numeric_limits<double>::max();
        for (const Group& other : m.groups) {
            if (other.node == kNone) continue;
            const Planet& o = w.planets[at(other.holder)];
            const double d = std::hypot(h.x - o.x, h.y - o.y);
            if (d < best_d) best_d = d, best = other.holder;
        }
        w.planets[at(g.holder)].liege = best;
    }
    for (Id s : m.seats) w.planets[at(s)].liege = kNone;

    // Independents: planets with no vassals, so trees stay whole.
    std::vector<Id> leaves;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (!is_seat(p) && vassals(w, p).empty()) leaves.push_back(p);
    }
    Rng& rng = w.rng[Stream::WorldGen];
    rng.shuffle(leaves);
    const auto count = std::min<std::size_t>(
        leaves.size(), static_cast<std::size_t>(std::llround(w.config.gen.independent_share * static_cast<double>(w.planets.size()))));
    for (std::size_t i = 0; i < count; ++i) w.planets[at(leaves[i])].liege = kNone;
}

}  // namespace lattice::fealty
