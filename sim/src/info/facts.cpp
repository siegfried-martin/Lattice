#include "info/facts.hpp"

#include <algorithm>
#include <utility>

#include "chars/missions.hpp"
#include "lattice/market.hpp"
#include "war/threats.hpp"

namespace lattice::info {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

bool by_planet(const PriceFact& f, Id planet) { return f.planet < planet; }

}  // namespace

void observe_market(const World& w, Facts& facts, Id planet) {
    const Planet& p = w.planets[at(planet)];
    PriceFact f{.planet = planet, .time = w.now};
    for (int g = 0; g < kLawfulGoodCount; ++g) {
        f.price[at(g)] = market::price(w.config, p, static_cast<Good>(g));
        f.stock[at(g)] = p.stock[at(g)];
    }
    auto it = std::lower_bound(facts.prices.begin(), facts.prices.end(), planet, by_planet);
    if (it != facts.prices.end() && it->planet == planet) {
        *it = f;
    } else {
        facts.prices.insert(it, f);
    }
}

const PriceFact* find(const Facts& facts, Id planet) {
    auto it = std::lower_bound(facts.prices.begin(), facts.prices.end(), planet, by_planet);
    return it != facts.prices.end() && it->planet == planet ? &*it : nullptr;
}

namespace {

// Merges two lists sorted by key, keeping the newer of each key.
template <class T, class Key>
void merge_list(std::vector<T>& dst, const std::vector<T>& src, Key key) {
    if (src.empty()) return;
    std::vector<T> out;
    out.reserve(dst.size() + src.size());
    auto a = dst.cbegin();
    auto b = src.cbegin();
    while (a != dst.cend() || b != src.cend()) {
        if (b == src.cend() || (a != dst.cend() && key(*a) < key(*b))) {
            out.push_back(*a++);
        } else if (a == dst.cend() || key(*b) < key(*a)) {
            out.push_back(*b++);
        } else {
            out.push_back(b->time > a->time ? *b : *a);
            ++a, ++b;
        }
    }
    dst = std::move(out);
}

// Inserts or replaces by key in a sorted list.
template <class T, class Key>
void put(std::vector<T>& list, const T& item, Key key) {
    auto it = std::lower_bound(list.begin(), list.end(), key(item), [&](const T& x, const auto& k) { return key(x) < k; });
    if (it != list.end() && key(*it) == key(item)) {
        if (item.time >= it->time) *it = item;
    } else {
        list.insert(it, item);
    }
}

auto risk_key = [](const RiskFact& f) { return f.sector; };
auto dip_key = [](const DipFact& f) { return std::pair{f.a, f.b}; };
auto sighting_key = [](const Sighting& f) { return f.army; };

}  // namespace

void merge_into(Facts& dst, const Facts& src) {
    merge_list(dst.prices, src.prices, [](const PriceFact& f) { return f.planet; });
    merge_list(dst.risks, src.risks, risk_key);
    merge_list(dst.dips, src.dips, dip_key);
    merge_list(dst.sightings, src.sightings, sighting_key);
}

void observe_risk(const World& w, Facts& facts, Id sector) {
    if (sector == kNone || w.risk.empty()) return;
    put(facts.risks, RiskFact{.sector = sector, .time = w.now, .risk = w.risk[at(sector)].risk}, risk_key);
}

double believed_risk(const Facts& facts, Id sector) {
    auto it = std::lower_bound(facts.risks.begin(), facts.risks.end(), sector,
                               [](const RiskFact& f, Id s) { return f.sector < s; });
    return it != facts.risks.end() && it->sector == sector ? it->risk : 0;
}

void learn_state(Facts& facts, Id a, Id b, DipState state, Time time) {
    if (a > b) std::swap(a, b);
    put(facts.dips, DipFact{.a = a, .b = b, .time = time, .state = static_cast<std::uint8_t>(state)}, dip_key);
}

DipState believed_state(const Facts& facts, Id a, Id b) {
    if (a > b) std::swap(a, b);
    auto it = std::lower_bound(facts.dips.begin(), facts.dips.end(), std::pair{a, b},
                               [](const DipFact& f, const std::pair<Id, Id>& k) { return std::pair{f.a, f.b} < k; });
    return it != facts.dips.end() && it->a == a && it->b == b ? static_cast<DipState>(it->state) : DipState::Peace;
}

bool believes_war(const Facts& facts, Id a, Id b) {
    return a != kNone && b != kNone && a != b && believed_state(facts, a, b) == DipState::War;
}

void add_sighting(Facts& facts, const Sighting& s) { put(facts.sightings, s, sighting_key); }

void share(Facts& a, Facts& b) {
    merge_into(a, b);
    b = a;
}

Facts& governor_facts(World& w, Id planet) {
    Character& g = w.characters[at(w.planets[at(planet)].governor)];
    observe_market(w, g.facts, planet);
    return g.facts;
}

void schedule_couriers(World& w) {
    const Time period = w.config.courier.interval_days * kDay;
    const auto n = static_cast<std::int64_t>(w.planets.size());
    for (std::int64_t p = 0; p < n; ++p) {
        // Every planet, sworn or not: a root that swears fealty later starts receiving
        // runs, and a due run with no liege does nothing.
        w.events.push({.time = stagger(period, p, n), .phase = Phase::Cadence, .kind = EventKind::CourierDue, .record = p,
                       .period = period});
    }
}

void courier_due(World& w, Id vassal, Observer* observer) {
    const Id liege = w.planets[at(vassal)].liege;
    if (liege == kNone) return;  // no longer sworn; the event stays harmless
    if (w.rng[Stream::Missions].chance(w.config.courier.mission_share)) {
        missions::post_courier(w, liege, vassal, observer);
    } else {
        send_courier(w, vassal);
    }
}

void send_courier(World& w, Id vassal) {
    const Id liege = w.planets[at(vassal)].liege;
    if (liege == kNone) return;
    Planet& from = w.planets[at(liege)];
    const double cost = w.config.courier.run_cost;
    from.treasury -= cost;
    w.ledger.money_sunk += cost;
    w.ledger.courier_runs += cost;
    w.courier_runs.push_back({.from = liege, .to = vassal, .packet = governor_facts(w, liege)});
    const auto run = static_cast<std::int64_t>(w.courier_runs.size() - 1);
    const Time trip = std::max<Time>(1, static_cast<Time>(w.derived.travel(liege, vassal) * kDay));
    w.events.push({.time = w.now + trip, .kind = EventKind::CourierArrive, .record = run});
}

void courier_arrive(World& w, Id id, Observer* observer) {
    CourierRun& run = w.courier_runs[at(id)];
    if (!run.alive) return;
    if (!run.returning) {
        Facts& vassal = governor_facts(w, run.to);
        merge_into(vassal, run.packet);
        run.packet = vassal;
        threats::check(w, run.to, observer);
        run.returning = true;
        const Time trip = std::max<Time>(1, static_cast<Time>(w.derived.travel(run.to, run.from) * kDay));
        w.events.push({.time = w.now + trip, .kind = EventKind::CourierArrive, .record = id});
    } else {
        merge_into(governor_facts(w, run.from), run.packet);
        threats::check(w, run.from, observer);
        run.alive = false;
        run.packet = {};
    }
}

}  // namespace lattice::info
