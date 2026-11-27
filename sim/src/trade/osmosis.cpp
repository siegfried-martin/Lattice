#include "trade/osmosis.hpp"

#include <algorithm>
#include <cmath>

#include "lattice/market.hpp"
#include "war/diplomacy.hpp"
#include "war/gates.hpp"

namespace lattice::osmosis {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

}  // namespace

std::vector<Derived::Pair> pairs(const World& w) {
    std::vector<Derived::Pair> out;
    const auto n = static_cast<Id>(w.planets.size());
    for (Id a = 0; a < n; ++a) {
        for (Id b = a + 1; b < n; ++b) {
            const double days = std::max(w.derived.travel(a, b), w.derived.travel(b, a));
            if (days <= w.config.osmosis.range_days) {
                out.push_back({a, b, days, std::exp(-days / w.config.osmosis.falloff)});
            }
        }
    }
    return out;
}

// For each pair and lawful good, goods flow from the fuller market to the emptier:
//   flow = osmosis.rate x fill gap x the smaller reserve x exp(-days / osmosis.falloff),
// capped at osmosis.max_close of the amount that would equalise the fills. It runs in
// chunks of price.chunk x the smaller reserve so it walks both prices, while the
// destination's taxed bid covers the source's taxed ask. Each chunk settles at the
// midpoint of the two, so the margin is split between the planets: osmosis moves money
// and neither sinks nor creates any (ECO-1, ECO-5; money round). Wars stop it between
// factions (MS-5).
double run(World& w) {
    const Config& c = w.config;
    double moved = 0;
    // A blockaded gate cuts its group off from trade along the Lattice (MIL-17).
    auto cut = [&](Id p) {
        const Id g = w.planets[at(p)].group;
        const Id node = g == kNone ? kNone : w.map.groups[at(g)].node;
        return node != kNone && !w.gates.empty() && w.gates[at(node)].captor != kNone;
    };
    for (const Derived::Pair& pair : w.derived.osmosis_pairs) {
        if (diplomacy::hostile_planets(w, pair.a, pair.b)) continue;  // war stops osmosis (TRD-6), as do the pirates
        if ((cut(pair.a) || cut(pair.b)) && w.derived.route(pair.a, pair.b).lattice > 0) continue;
        for (int gi = 0; gi < kLawfulGoodCount; ++gi) {
            const auto g = static_cast<Good>(gi);
            Planet* src = &w.planets[at(pair.a)];
            Planet* dst = &w.planets[at(pair.b)];
            double rs = market::reserve(c, *src, g), rd = market::reserve(c, *dst, g);
            if (rs <= 0 || rd <= 0) continue;
            if (src->stock[g] / rs < dst->stock[g] / rd) std::swap(src, dst), std::swap(rs, rd);
            const double gap = src->stock[g] / rs - dst->stock[g] / rd;
            if (gap <= 0) continue;
            const double equalise = (src->stock[g] * rd - dst->stock[g] * rs) / (rs + rd);
            double remaining = std::min(c.osmosis.rate * gap * std::min(rs, rd) * pair.weight, c.osmosis.max_close * equalise);
            const double chunk = c.price.chunk * std::min(rs, rd);
            while (remaining > 1e-12) {
                if (!market::can_buy(c, *dst, g)) break;
                const double q = std::min({remaining, chunk, market::sellable(c, *src, g)});  // the floor (ECO-2)
                if (q <= 1e-12) break;
                const double ask = q * market::ask(c, *src, g) * (1 + src->tax_rate);
                const double bid = q * market::bid(c, *dst, g) * (1 - dst->tax_rate);
                if (bid < ask) break;
                const double settle = (ask + bid) / 2;
                src->stock[g] -= q;
                dst->stock[g] += q;
                market::receive(*src, settle);
                market::pay(*dst, settle);
                w.ledger.osmosis_margin += bid - ask;  // split between the two, not sunk
                w.ledger.osmosis_volume += q;
                moved += q;
                remaining -= q;
            }
        }
    }
    return moved;
}

}  // namespace lattice::osmosis
