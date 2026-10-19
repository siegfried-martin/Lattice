#include "politics/opinion.hpp"

#include <algorithm>
#include <cmath>

#include "politics/fealty.hpp"

namespace lattice::opinion {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

bool by_target(const Opinion& o, Id about) { return o.of < about; }

// The weekly fade, (1 - rel.fade) per week, applied for the time since the last update.
double faded(const World& w, const Opinion& o, double rest_value) {
    const double weeks = static_cast<double>(w.now - o.updated) / static_cast<double>(7 * kDay);
    return rest_value + (o.value - rest_value) * std::pow(1 - w.config.rel.fade, std::max(0.0, weeks));
}

}  // namespace

double rest(const World& w, Id holder, Id about) {
    const Character& h = w.characters[at(holder)];
    const Character& a = w.characters[at(about)];
    if (h.governs == kNone || a.governs == kNone || w.planets[at(h.governs)].liege != a.governs) return 0;
    const std::vector<Id> faction = fealty::faction_planets(w, fealty::root_of(w, h.governs));
    std::size_t same = 0;
    for (Id p : faction) same += w.planets[at(p)].region == h.culture;
    const double share = faction.empty() ? 0 : static_cast<double>(same) / static_cast<double>(faction.size());
    return w.config.rel.culture_rest * (share - 0.5);
}

double get(const World& w, Id holder, Id about) {
    const auto& ops = w.characters[at(holder)].opinions;
    auto it = std::lower_bound(ops.begin(), ops.end(), about, by_target);
    const double r = rest(w, holder, about);
    if (it == ops.end() || it->of != about) return r;
    return faded(w, *it, r);
}

void deed(World& w, Id holder, Id about, DeedKind kind, Observer* observer) {
    if (holder == about || holder == kNone || about == kNone) return;
    const double r = rest(w, holder, about);
    auto& ops = w.characters[at(holder)].opinions;
    auto it = std::lower_bound(ops.begin(), ops.end(), about, by_target);
    if (it == ops.end() || it->of != about) it = ops.insert(it, {.of = about, .value = r, .updated = w.now});
    const double amount = w.config.deed.amount[static_cast<std::size_t>(kind)];
    it->value = std::clamp(faded(w, *it, r) + amount, -100.0, 100.0);
    it->updated = w.now;
    it->recent.push_back({kind, amount, w.now});
    const auto keep = static_cast<std::size_t>(w.config.rel.recent_deeds);
    if (it->recent.size() > keep) it->recent.erase(it->recent.begin(), it->recent.end() - static_cast<std::ptrdiff_t>(keep));
    if (observer) {
        observer->log(w.now, "deed", holder,
                      "kind " + std::to_string(static_cast<int>(kind)) + " about " + std::to_string(about) + " now " +
                          std::to_string(it->value));
    }
}

}  // namespace lattice::opinion
