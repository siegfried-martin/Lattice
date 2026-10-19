#include "politics/authority.hpp"

#include <algorithm>

#include "chars/missions.hpp"

namespace lattice::authority {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

}  // namespace

void weekly(World& w, Observer* observer) {
    const auto& a = w.config.authority;
    // Sectors by risk, those with a lawful planet on the network.
    std::vector<std::pair<double, Id>> risky;
    for (Id s = 0; s < static_cast<Id>(w.map.sectors.size()); ++s) {
        if (w.risk[at(s)].risk < a.risk_post) continue;
        risky.emplace_back(-w.risk[at(s)].risk, s);
    }
    std::sort(risky.begin(), risky.end());
    std::int64_t posted = 0;
    for (auto [neg, s] : risky) {
        if (posted >= a.contracts) break;
        const Id system = w.map.sectors[at(s)].system;
        if (system == kNone || w.map.systems[at(system)].fringe) continue;
        Id planet = kNone;
        for (Id p : w.map.systems[at(system)].planets) {
            if (!w.planets[at(p)].pirate) planet = p;
        }
        if (planet == kNone) continue;
        const double cost = a.pay * static_cast<double>(a.clear_days);
        if (w.authority.treasury < cost) break;
        if (missions::post_clearing(w, planet, observer)) ++posted;
    }
}

}  // namespace lattice::authority
