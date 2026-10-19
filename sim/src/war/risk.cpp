#include "war/risk.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "chars/movement.hpp"
#include "info/facts.hpp"

namespace lattice::risk {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

}  // namespace

Id sector_at(const Map& m, double x, double y) {
    const double row_height = std::sqrt(3.0) / 2;
    const auto row = static_cast<std::int32_t>(std::lround(y / row_height));
    Id best = kNone;
    double best_d = std::numeric_limits<double>::max();
    for (std::int32_t r = row - 1; r <= row + 1; ++r) {
        if (r < 0 || r >= m.height) continue;
        const auto col = static_cast<std::int32_t>(std::lround(x - 0.5 * (r & 1)));
        for (std::int32_t c = col - 1; c <= col + 1; ++c) {
            if (c < 0 || c >= m.width) continue;
            const Sector& s = m.sectors[at(r * m.width + c)];
            const double d = std::hypot(s.x - x, s.y - y);
            if (d < best_d) best_d = d, best = r * m.width + c;
        }
    }
    if (best == kNone) {
        // Off the map: clamp to the nearest edge sector.
        const auto r = std::clamp<std::int32_t>(row, 0, m.height - 1);
        const auto c = std::clamp<std::int32_t>(static_cast<std::int32_t>(std::lround(x)), 0, m.width - 1);
        best = r * m.width + c;
    }
    return best;
}

void daily(World& w) {
    const auto& c = w.config.risk;
    for (SectorRisk& s : w.risk) {
        const double x = s.raids + c.presence * s.hostile_days - c.clear * s.clear_days;
        s.risk = std::max(0.0, s.risk + (x - s.risk) / c.window);
        s.raids = s.hostile_days = s.clear_days = 0;
    }
}

void add_raid(World& w, double x, double y) { w.risk[at(sector_at(w.map, x, y))].raids += 1; }
void add_hostile(World& w, double x, double y, double days) { w.risk[at(sector_at(w.map, x, y))].hostile_days += days; }
void add_patrol(World& w, double x, double y, double days) { w.risk[at(sector_at(w.map, x, y))].clear_days += days; }

std::vector<Id> route_sectors(const World& w, Id a, Id b) {
    std::vector<Id> out;
    for (const Leg& l : move::route_legs(w, a, b)) {
        if (l.kind != LegKind::Space) continue;
        out.push_back(sector_at(w.map, l.x0, l.y0));
        out.push_back(sector_at(w.map, l.x1, l.y1));
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

double believed_route_risk(const World& w, const Facts& facts, Id a, Id b) {
    if (a == b || facts.risks.empty()) return 0;
    double r = 0;
    for (Id s : w.derived.route_risk_sectors[at(a) * w.derived.planet_count + at(b)]) r += info::believed_risk(facts, s);
    return r;
}

}  // namespace lattice::risk
