#include "gen/worldgen.hpp"

#include "chars/ships.hpp"
#include "politics/fealty.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <numbers>
#include <numeric>

namespace lattice::gen {

namespace {

double dist(double ax, double ay, double bx, double by) { return std::hypot(ax - bx, ay - by); }

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

// ---- GEN-1, GEN-2: sectors and regions ----

void make_sectors(Map& m) {
    const double row_height = std::sqrt(3.0) / 2;
    for (std::int32_t row = 0; row < m.height; ++row) {
        for (std::int32_t col = 0; col < m.width; ++col) {
            Sector s;
            s.col = col;
            s.row = row;
            s.x = col + 0.5 * (row & 1);
            s.y = row * row_height;
            m.sectors.push_back(s);
        }
    }
}

// One seed sector per region, spread far apart (farthest-point sampling), then a
// breadth-first fill taking turns, so regions come out compact and about equal.
void make_regions(World& w) {
    Map& m = w.map;
    Rng& rng = w.rng[Stream::WorldGen];
    const auto sector_count = static_cast<Id>(m.sectors.size());
    const auto region_count = static_cast<Id>(w.config.gen.regions);

    std::vector<Id> seeds{static_cast<Id>(rng.uniform_int(0, sector_count - 1))};
    while (static_cast<Id>(seeds.size()) < region_count) {
        Id best = kNone;
        double best_d = -1;
        for (Id s = 0; s < sector_count; ++s) {
            double d = std::numeric_limits<double>::max();
            for (Id seed : seeds) d = std::min(d, dist(m.sectors[at(s)].x, m.sectors[at(s)].y, m.sectors[at(seed)].x, m.sectors[at(seed)].y));
            if (d > best_d) best_d = d, best = s;
        }
        seeds.push_back(best);
    }

    std::vector<std::deque<Id>> frontier(at(region_count));
    auto claim = [&](Id s, Id r) {
        m.sectors[at(s)].region = r;
        std::vector<Id> next = neighbours(m, s);
        rng.shuffle(next);
        for (Id n : next) {
            if (m.sectors[at(n)].region == kNone) frontier[at(r)].push_back(n);
        }
    };
    m.regions.resize(at(region_count));
    for (Id r = 0; r < region_count; ++r) {
        m.regions[at(r)].seed_sector = seeds[at(r)];
        claim(seeds[at(r)], r);
    }
    Id claimed = region_count;
    while (claimed < sector_count) {
        bool any = false;
        for (Id r = 0; r < region_count && claimed < sector_count; ++r) {
            auto& f = frontier[at(r)];
            while (!f.empty() && m.sectors[at(f.front())].region != kNone) f.pop_front();
            if (f.empty()) continue;
            const Id s = f.front();
            f.pop_front();
            claim(s, r);
            ++claimed;
            any = true;
        }
        if (!any) break;  // can't happen on a connected grid
    }

    // Production weights (GEN-6): every lawful good near 1, one favoured good higher,
    // so neighbouring planets tend to share strengths (PRD-7).
    const auto& g = w.config.gen;
    for (Region& region : m.regions) {
        for (double& weight : region.weights) weight = 1 + g.weight_spread * rng.uniform(-1, 1);
        region.weights[at(static_cast<Id>(rng.uniform_int(0, kLawfulGoodCount - 1)))] *= g.favour_mult;
        region.pop_scale = std::exp(g.region_pop_spread * rng.normal());
    }
}

// ---- GEN-3: systems and planets ----

Id add_planet(World& w, Id system, double x, double y, Id moon_of) {
    const auto& g = w.config.gen;
    Rng& rng = w.rng[Stream::WorldGen];
    const System& sys = w.map.systems[at(system)];
    Planet p;
    p.sector = sys.sector;
    p.region = w.map.sectors[at(sys.sector)].region;
    p.system = system;
    p.moon_of = moon_of;
    p.x = x;
    p.y = y;
    p.population = g.pop_median * w.map.regions[at(p.region)].pop_scale * std::exp(g.pop_spread * rng.normal()) *
                   (moon_of == kNone ? 1.0 : g.moon_pop);
    p.happiness = g.start_happiness;
    p.productivity = w.config.pop.p_base + w.config.pop.p_happy * p.happiness;
    // PRD-3: the governor's risk tolerance (CHR-4, char.risk_dist uniform 0-1) sets the
    // growth factor. There are no governors before MS-4, so the planet draws it.
    p.growth_factor = w.config.pop.growth_factor_min +
                      rng.uniform01() * (w.config.pop.growth_factor_max - w.config.pop.growth_factor_min);
    w.planets.push_back(p);
    const auto id = static_cast<Id>(w.planets.size() - 1);
    w.map.systems[at(system)].planets.push_back(id);
    return id;
}

// Systems go into sectors weighted by a smoothed random density, so some stretches are
// crowded and some empty. Regions take turns, the one with the fewest planets first, so
// every region has enough planets for GEN-6's producer guarantee.
void make_systems(World& w) {
    Map& m = w.map;
    Rng& rng = w.rng[Stream::WorldGen];
    const auto& g = w.config.gen;

    std::vector<double> raw(m.sectors.size());
    for (double& r : raw) r = rng.uniform01();
    std::vector<double> weight(m.sectors.size());
    for (Id s = 0; s < static_cast<Id>(m.sectors.size()); ++s) {
        double sum = raw[at(s)];
        const auto next = neighbours(m, s);
        for (Id n : next) sum += raw[at(n)];
        const double density = sum / static_cast<double>(next.size() + 1);
        weight[at(s)] = density * density * density + 1e-6;
    }

    std::vector<std::int64_t> bodies(m.regions.size(), 0);
    std::vector<std::vector<Id>> free(m.regions.size());
    for (Id s = 0; s < static_cast<Id>(m.sectors.size()); ++s) free[at(m.sectors[at(s)].region)].push_back(s);

    std::int64_t total = 0;
    while (total < g.planets) {
        Id region = kNone;
        for (Id r = 0; r < static_cast<Id>(m.regions.size()); ++r) {
            if (free[at(r)].empty()) continue;
            if (region == kNone || bodies[at(r)] < bodies[at(region)]) region = r;
        }
        if (region == kNone) break;  // every sector has a system

        auto& options = free[at(region)];
        std::vector<double> w_options;
        for (Id s : options) w_options.push_back(weight[at(s)]);
        const std::size_t pick = rng.weighted(w_options);
        const Id sector = options[pick];
        options.erase(options.begin() + static_cast<std::ptrdiff_t>(pick));

        const Sector& sec = m.sectors[at(sector)];
        System sys;
        sys.sector = sector;
        const double r = 0.25 * std::sqrt(rng.uniform01()), a = rng.uniform(0, 2 * std::numbers::pi);
        sys.x = sec.x + r * std::cos(a);
        sys.y = sec.y + r * std::sin(a);
        m.systems.push_back(sys);
        const auto system = static_cast<Id>(m.systems.size() - 1);
        m.sectors[at(sector)].system = system;

        const auto primaries = rng.uniform_int(1, 4);
        for (std::int64_t i = 0; i < primaries; ++i) {
            const double orbit = 0.05 * static_cast<double>(i + 1), angle = rng.uniform(0, 2 * std::numbers::pi);
            const double px = sys.x + orbit * std::cos(angle), py = sys.y + orbit * std::sin(angle);
            const Id planet = add_planet(w, system, px, py, kNone);
            std::int64_t added = 1;
            if (rng.chance(g.moon_share)) {
                const auto moons = rng.uniform_int(1, 3);
                for (std::int64_t j = 0; j < moons; ++j) {
                    const double mo = 0.012 * static_cast<double>(j + 1), ma = rng.uniform(0, 2 * std::numbers::pi);
                    add_planet(w, system, px + mo * std::cos(ma), py + mo * std::sin(ma), planet);
                }
                added += moons;
            }
            bodies[at(region)] += added;
            total += added;
        }
    }
}

// Each system is one group, or two when it has gen.split_planets planets or more,
// split into inner and outer halves; moons go with their planet (WLD-4).
void make_groups(World& w) {
    Map& m = w.map;
    for (Id s = 0; s < static_cast<Id>(m.systems.size()); ++s) {
        std::vector<Id> primaries;
        for (Id p : m.systems[at(s)].planets) {
            if (w.planets[at(p)].moon_of == kNone) primaries.push_back(p);
        }
        const bool split = static_cast<std::int64_t>(primaries.size()) >= w.config.gen.split_planets;
        const std::size_t inner = split ? (primaries.size() + 1) / 2 : primaries.size();
        const auto first = static_cast<Id>(m.groups.size());
        m.groups.push_back({.system = s});
        if (split) m.groups.push_back({.system = s});
        for (Id p : m.systems[at(s)].planets) {
            const Planet& planet = w.planets[at(p)];
            const Id primary = planet.moon_of == kNone ? p : planet.moon_of;
            const auto index = static_cast<std::size_t>(std::find(primaries.begin(), primaries.end(), primary) - primaries.begin());
            const Id group = first + (index < inner ? 0 : 1);
            m.groups[at(group)].planets.push_back(p);
            w.planets[at(p)].group = group;
        }
    }
}

// About one system in gen.fringe_share is off the network, chosen toward the map's
// edges and region borders (GEN-3, WLD-6).
void choose_fringe(World& w) {
    Map& m = w.map;
    Rng& rng = w.rng[Stream::WorldGen];
    const auto n = static_cast<Id>(m.systems.size());
    std::vector<std::pair<double, Id>> scored;
    for (Id s = 0; s < n; ++s) {
        const Sector& sec = m.sectors[at(m.systems[at(s)].sector)];
        const auto edge_dist = std::min({sec.col, m.width - 1 - sec.col, sec.row, m.height - 1 - sec.row});
        double score = 1 - std::min(edge_dist, 2) / 2.0;
        for (Id nb : neighbours(m, m.systems[at(s)].sector)) {
            if (m.sectors[at(nb)].region != sec.region) {
                score += 0.5;
                break;
            }
        }
        score += rng.uniform(0, 0.5);
        scored.emplace_back(-score, s);
    }
    std::sort(scored.begin(), scored.end());
    auto count = static_cast<Id>(std::llround(w.config.gen.fringe_share * n));
    count = std::min(count, std::max<Id>(0, n - 2));  // keep a network to speak of
    for (Id i = 0; i < count; ++i) {
        const Id s = scored[at(i)].second;
        m.systems[at(s)].fringe = true;
        for (Id p : m.systems[at(s)].planets) w.planets[at(p)].fringe = true;
    }
}

// ---- GEN-6: what planets make ----

int medium_or_better(const World& w, Id region, Good g) {
    int count = 0;
    for (const Planet& p : w.planets) {
        if (p.region != region) continue;
        for (int s = 0; s < 3; ++s) count += p.makes[at(s)] == g && p.tiers[at(s)] != kLow;
    }
    return count;
}

void assign_goods(World& w) {
    Rng& rng = w.rng[Stream::WorldGen];
    for (Planet& p : w.planets) {
        auto weights = w.map.regions[at(p.region)].weights;
        for (int s = 0; s < 3; ++s) {
            const auto g = static_cast<Good>(rng.weighted(weights));
            p.makes[at(s)] = g;
            weights[g] = 0;
        }
        p.tiers = {kHigh, kMedium, kLow};
        rng.shuffle(p.tiers);
    }

    // Repair (PRD-8): every lawful good needs gen.min_producers producers per region at
    // medium or better. Each fix takes a medium-or-better slot from a good with more
    // than enough, so it never creates a new shortfall and the loop ends. It can fail
    // only if a region has fewer than 3 x min_producers planets.
    const int need = static_cast<int>(w.config.gen.min_producers);
    for (Id r = 0; r < static_cast<Id>(w.map.regions.size()); ++r) {
        for (;;) {
            Good short_good = kGoodCount;
            for (int g = 0; g < kLawfulGoodCount; ++g) {
                if (medium_or_better(w, r, static_cast<Good>(g)) < need) {
                    short_good = static_cast<Good>(g);
                    break;
                }
            }
            if (short_good == kGoodCount) break;

            struct Fix {
                Id planet;
                int slot;          // the medium-or-better slot given up
                int low_slot;      // the slot already making short_good at low, or -1
            };
            std::vector<Fix> fixes;
            for (Id pi = 0; pi < static_cast<Id>(w.planets.size()); ++pi) {
                const Planet& p = w.planets[at(pi)];
                if (p.region != r) continue;
                int has = -1;
                for (int s = 0; s < 3; ++s) {
                    if (p.makes[at(s)] == short_good) has = s;
                }
                if (has >= 0 && p.tiers[at(has)] != kLow) continue;
                for (int s = 0; s < 3; ++s) {
                    if (s == has || p.tiers[at(s)] == kLow) continue;
                    if (medium_or_better(w, r, p.makes[at(s)]) > need) fixes.push_back({pi, s, has});
                }
            }
            if (fixes.empty()) break;  // region too small; checked in tests
            const Fix f = fixes[static_cast<std::size_t>(rng.uniform_int(0, static_cast<std::int64_t>(fixes.size()) - 1))];
            Planet& p = w.planets[at(f.planet)];
            if (f.low_slot >= 0) {
                std::swap(p.tiers[at(f.low_slot)], p.tiers[at(f.slot)]);
            } else {
                p.makes[at(f.slot)] = short_good;
            }
        }
    }
}

// ---- GEN-4, GEN-5: holders, the Lattice and hop lanes ----

void choose_holders(World& w) {
    for (Group& g : w.map.groups) {
        for (Id p : g.planets) {
            const Planet& a = w.planets[at(p)];
            if (g.holder == kNone) {
                g.holder = p;
                continue;
            }
            const Planet& b = w.planets[at(g.holder)];
            if (a.population * a.productivity > b.population * b.productivity) g.holder = p;
        }
    }
}

// A minimum spanning tree over the interchanges, plus the shortest edges of the
// relative neighbourhood graph until there are gen.lattice_loops x (n - 1) extra
// edges (GEN-4). Both graphs are planar, so roads never cross. Where three or more
// roads meet an interchange, a junction takes them and the interchange hangs off it.
void make_lattice(World& w) {
    Map& m = w.map;
    for (Id g = 0; g < static_cast<Id>(m.groups.size()); ++g) {
        Group& group = m.groups[at(g)];
        if (m.systems[at(group.system)].fringe) continue;
        const Planet& holder = w.planets[at(group.holder)];
        m.nodes.push_back({.kind = NodeKind::Interchange, .x = holder.x + 0.02, .y = holder.y + 0.02, .group = g});
        group.node = static_cast<Id>(m.nodes.size() - 1);
    }
    const auto n = static_cast<Id>(m.nodes.size());
    if (n < 2) return;

    auto d = [&](Id a, Id b) { return dist(m.nodes[at(a)].x, m.nodes[at(a)].y, m.nodes[at(b)].x, m.nodes[at(b)].y); };
    std::vector<std::pair<Id, Id>> chosen;

    // Prim's algorithm.
    std::vector<bool> in_tree(at(n), false);
    std::vector<double> best(at(n), std::numeric_limits<double>::max());
    std::vector<Id> from(at(n), kNone);
    best[0] = 0;
    for (Id step = 0; step < n; ++step) {
        Id u = kNone;
        for (Id i = 0; i < n; ++i) {
            if (!in_tree[at(i)] && (u == kNone || best[at(i)] < best[at(u)])) u = i;
        }
        in_tree[at(u)] = true;
        if (from[at(u)] != kNone) chosen.emplace_back(std::min(u, from[at(u)]), std::max(u, from[at(u)]));
        for (Id v = 0; v < n; ++v) {
            if (!in_tree[at(v)] && d(u, v) < best[at(v)]) best[at(v)] = d(u, v), from[at(v)] = u;
        }
    }

    // Relative neighbourhood graph edges not in the tree.
    std::vector<std::pair<double, std::pair<Id, Id>>> extra;
    for (Id a = 0; a < n; ++a) {
        for (Id b = a + 1; b < n; ++b) {
            if (std::find(chosen.begin(), chosen.end(), std::pair{a, b}) != chosen.end()) continue;
            const double ab = d(a, b);
            bool empty_lune = true;
            for (Id c = 0; c < n && empty_lune; ++c) {
                if (c != a && c != b && std::max(d(a, c), d(b, c)) < ab) empty_lune = false;
            }
            if (empty_lune) extra.push_back({ab, {a, b}});
        }
    }
    std::sort(extra.begin(), extra.end());
    const auto loops = std::min<std::size_t>(extra.size(), static_cast<std::size_t>(std::llround(w.config.gen.lattice_loops * (n - 1))));
    for (std::size_t i = 0; i < loops; ++i) chosen.push_back(extra[i].second);
    std::sort(chosen.begin(), chosen.end());

    for (auto [a, b] : chosen) m.edges.push_back({a, b, d(a, b)});

    // Junctions.
    for (Id i = 0; i < n; ++i) {
        int degree = 0;
        for (const LatticeEdge& e : m.edges) degree += (e.a == i) + (e.b == i);
        if (degree < 3) continue;
        m.nodes.push_back({.kind = NodeKind::Junction, .x = m.nodes[at(i)].x, .y = m.nodes[at(i)].y});
        const auto j = static_cast<Id>(m.nodes.size() - 1);
        for (LatticeEdge& e : m.edges) {
            if (e.a == i) e.a = j;
            if (e.b == i) e.b = j;
        }
        m.edges.push_back({i, j, 0});
    }
    for (Id e = 0; e < static_cast<Id>(m.edges.size()); ++e) {
        m.nodes[at(m.edges[at(e)].a)].edges.push_back(e);
        m.nodes[at(m.edges[at(e)].b)].edges.push_back(e);
    }
}

// Within a group: each moon to its planet, and the planets in a chain by orbit.
void make_hop_lanes(World& w) {
    for (const Group& g : w.map.groups) {
        Id previous = kNone;
        for (Id p : g.planets) {
            const Planet& planet = w.planets[at(p)];
            const Id link = planet.moon_of != kNone ? planet.moon_of : previous;
            if (planet.moon_of == kNone) previous = p;
            if (link == kNone) continue;
            const Planet& other = w.planets[at(link)];
            w.map.hop_lanes.push_back({link, p, dist(planet.x, planet.y, other.x, other.y)});
        }
    }
}

// ---- GEN-7, GEN-8: seats of power and fuel prices ----

// High-population gate holders on the network, far apart, in different regions where
// possible: score = population share x distance share from the seats already chosen.
void choose_seats(World& w) {
    std::vector<Id> candidates;
    for (const Group& g : w.map.groups) {
        if (g.node != kNone) candidates.push_back(g.holder);
    }
    std::sort(candidates.begin(), candidates.end());
    if (candidates.empty()) return;
    const auto count = std::min<std::size_t>(candidates.size(), static_cast<std::size_t>(w.config.gen.seats));
    auto& seats = w.map.seats;

    double max_pop = 0;
    for (Id p : candidates) max_pop = std::max(max_pop, w.planets[at(p)].population);
    while (seats.size() < count) {
        std::vector<bool> region_taken(w.map.regions.size(), false);
        for (Id s : seats) region_taken[at(w.planets[at(s)].region)] = true;
        bool any_free_region = false;
        for (Id p : candidates) {
            if (!region_taken[at(w.planets[at(p)].region)]) any_free_region = true;
        }
        auto nearest_seat = [&](Id p) {
            double d = std::numeric_limits<double>::max();
            for (Id s : seats) d = std::min(d, dist(w.planets[at(p)].x, w.planets[at(p)].y, w.planets[at(s)].x, w.planets[at(s)].y));
            return seats.empty() ? 1.0 : d;
        };
        double max_d = 0;
        for (Id p : candidates) max_d = std::max(max_d, nearest_seat(p));
        Id best = kNone;
        double best_score = -1;
        for (Id p : candidates) {
            if (std::find(seats.begin(), seats.end(), p) != seats.end()) continue;
            if (any_free_region && region_taken[at(w.planets[at(p)].region)]) continue;
            const double score = w.planets[at(p)].population / max_pop * (max_d > 0 ? nearest_seat(p) / max_d : 1);
            if (score > best_score) best_score = score, best = p;
        }
        seats.push_back(best);
    }
}

// Fuel costs more the more Lattice hops a planet is from the nearest seat of power,
// and more again off the network (GEN-7, ECO-11).
void set_fuel_prices(World& w) {
    const Map& m = w.map;
    const auto inf = std::numeric_limits<std::int64_t>::max();
    std::vector<std::int64_t> hops(m.nodes.size(), inf);
    std::deque<Id> queue;  // 0-1 BFS: a junction's stub costs no hop
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
                cost ? queue.push_back(v) : queue.push_front(v);
            }
        }
    }

    std::vector<double> planet_hops(w.planets.size(), 0);
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        const Planet& planet = w.planets[at(p)];
        const Id node = m.groups[at(planet.group)].node;
        if (node != kNone) {
            planet_hops[at(p)] = hops[at(node)] == inf ? 0 : static_cast<double>(hops[at(node)]);
            continue;
        }
        Id nearest = kNone;
        double best = std::numeric_limits<double>::max();
        for (Id n = 0; n < static_cast<Id>(m.nodes.size()); ++n) {
            if (m.nodes[at(n)].kind != NodeKind::Interchange || hops[at(n)] == inf) continue;
            const double d = dist(planet.x, planet.y, m.nodes[at(n)].x, m.nodes[at(n)].y);
            if (d < best) best = d, nearest = n;
        }
        planet_hops[at(p)] = nearest == kNone ? 0 : static_cast<double>(hops[at(nearest)] + 1);
    }
    const double max_hops = *std::max_element(planet_hops.begin(), planet_hops.end());
    const auto& f = w.config.fuel;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        Planet& planet = w.planets[at(p)];
        const double share = max_hops > 0 ? planet_hops[at(p)] / max_hops : 0;
        planet.fuel_price = f.base * (1 + f.fringe * share) * (planet.fringe ? f.fringe_mult : 1);
    }
}

}  // namespace

std::vector<Id> neighbours(const Map& m, Id sector) {
    const Sector& s = m.sectors[at(sector)];
    // Odd rows are shifted half a sector east.
    static constexpr int even[6][2] = {{1, 0}, {-1, 0}, {0, -1}, {-1, -1}, {0, 1}, {-1, 1}};
    static constexpr int odd[6][2] = {{1, 0}, {-1, 0}, {1, -1}, {0, -1}, {1, 1}, {0, 1}};
    const auto& offsets = (s.row & 1) ? odd : even;
    std::vector<Id> out;
    for (const auto& o : offsets) {
        const int c = s.col + o[0], r = s.row + o[1];
        if (c >= 0 && c < m.width && r >= 0 && r < m.height) out.push_back(r * m.width + c);
    }
    return out;
}

void generate(World& w) {
    w.map.width = static_cast<std::int32_t>(w.config.gen.width);
    w.map.height = static_cast<std::int32_t>(w.config.gen.height);
    make_sectors(w.map);
    make_regions(w);
    make_systems(w);
    make_groups(w);
    choose_fringe(w);
    assign_goods(w);
    choose_holders(w);
    make_lattice(w);
    make_hop_lanes(w);
    choose_seats(w);
    set_fuel_prices(w);
    ships::make_catalogue(w);
    fealty::make_trees(w);
}

}  // namespace lattice::gen
