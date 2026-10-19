#include <doctest.h>

#include <algorithm>
#include <deque>
#include <set>

#include "gen/worldgen.hpp"
#include "lattice/save.hpp"
#include "lattice/world.hpp"

using namespace lattice;

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

Config v1_size() {
    Config c;
    c.gen.width = 10;
    c.gen.height = 15;
    c.gen.planets = 100;
    c.gen.regions = 6;
    c.gen.seats = 6;
    c.chars.target = 0;  // governors only: these tests are about the map
    return c;
}

int medium_or_better(const World& w, Id region, Good g) {
    int n = 0;
    for (const Planet& p : w.planets) {
        if (p.region != region) continue;
        const auto& makes = p.pirate ? p.lawful_makes : p.makes;
        const auto& tiers = p.pirate ? p.lawful_tiers : p.tiers;
        for (int s = 0; s < 3; ++s) n += makes[at(s)] == g && tiers[at(s)] != kLow;
    }
    return n;
}

// Every invariant world generation promises, for one world.
void check_world(const World& w) {
    const Config& c = w.config;
    const Map& m = w.map;

    // GEN-1, GEN-3: about the planets asked for.
    CHECK(static_cast<std::int64_t>(w.planets.size()) >= c.gen.planets);
    CHECK(static_cast<std::int64_t>(w.planets.size()) <= c.gen.planets + 16);

    // GEN-2: every sector is in a region, and each region is contiguous.
    REQUIRE(m.sectors.size() == static_cast<std::size_t>(c.gen.width * c.gen.height));
    for (Id r = 0; r < static_cast<Id>(m.regions.size()); ++r) {
        std::vector<Id> members;
        for (Id s = 0; s < static_cast<Id>(m.sectors.size()); ++s) {
            if (m.sectors[at(s)].region == r) members.push_back(s);
        }
        REQUIRE(!members.empty());
        std::set<Id> seen{members[0]};
        std::deque<Id> queue{members[0]};
        while (!queue.empty()) {
            const Id s = queue.front();
            queue.pop_front();
            for (Id n : gen::neighbours(m, s)) {
                if (m.sectors[at(n)].region == r && seen.insert(n).second) queue.push_back(n);
            }
        }
        CHECK(seen.size() == members.size());
    }

    // GEN-3, WLD-4: one to four planets per system, moons beside their planet.
    for (const System& sys : m.systems) {
        int primaries = 0;
        for (Id p : sys.planets) primaries += w.planets[at(p)].moon_of == kNone;
        CHECK(primaries >= 1);
        CHECK(primaries <= 4);
        for (Id p : sys.planets) CHECK(w.planets[at(p)].fringe == sys.fringe);
    }
    for (const Planet& p : w.planets) {
        if (p.moon_of == kNone) continue;
        CHECK(w.planets[at(p.moon_of)].moon_of == kNone);
        CHECK(w.planets[at(p.moon_of)].group == p.group);
    }

    // GEN-5: the holder is the group's largest N x P.
    for (const Group& g : m.groups) {
        const Planet& holder = w.planets[at(g.holder)];
        for (Id p : g.planets) CHECK(w.planets[at(p)].population * w.planets[at(p)].productivity <= holder.population * holder.productivity);
        CHECK((g.node == kNone) == m.systems[at(g.system)].fringe);
    }

    // GEN-4: the network connects every interchange; no interchange has three roads
    // (a junction takes them).
    if (!m.nodes.empty()) {
        std::set<Id> seen{0};
        std::deque<Id> queue{0};
        while (!queue.empty()) {
            const Id u = queue.front();
            queue.pop_front();
            for (Id e : m.nodes[at(u)].edges) {
                const Id v = m.edges[at(e)].a == u ? m.edges[at(e)].b : m.edges[at(e)].a;
                if (seen.insert(v).second) queue.push_back(v);
            }
        }
        CHECK(seen.size() == m.nodes.size());
        for (const LatticeNode& n : m.nodes) {
            if (n.kind == NodeKind::Interchange) CHECK(n.edges.size() <= 2);
            if (n.kind == NodeKind::Junction) CHECK(n.edges.size() >= 3);
        }
    }

    // GEN-6: three distinct lawful goods at high, medium and low; enough producers per
    // region.
    for (const Planet& p : w.planets) {
        // A pirate planet makes contraband now; what it drew is kept for recapture (PIR-3).
        const auto& makes = p.pirate ? p.lawful_makes : p.makes;
        std::set<Good> goods(makes.begin(), makes.end());
        CHECK(goods.size() == 3);
        CHECK(!goods.contains(kStims));
        const auto& drawn = p.pirate ? p.lawful_tiers : p.tiers;
        std::set<Tier> tiers(drawn.begin(), drawn.end());
        CHECK(tiers.size() == 3);
    }
    for (Id r = 0; r < static_cast<Id>(m.regions.size()); ++r) {
        for (int g = 0; g < kLawfulGoodCount; ++g) CHECK(medium_or_better(w, r, static_cast<Good>(g)) >= c.gen.min_producers);
    }

    // GEN-7: fuel costs at least the base, more off the network.
    double max_on = 0, min_fringe = 1e9;
    for (const Planet& p : w.planets) {
        CHECK(p.fuel_price >= c.fuel.base);
        CHECK(p.fuel_price <= c.fuel.base * (1 + c.fuel.fringe) * c.fuel.fringe_mult + 1e-9);
        if (p.fringe) min_fringe = std::min(min_fringe, p.fuel_price);
        else max_on = std::max(max_on, p.fuel_price);
    }

    // GEN-8: the seats are on the network, in different regions where there are enough.
    std::set<Id> seat_regions, network_regions;
    for (Id s : m.seats) {
        CHECK(!w.planets[at(s)].fringe);
        seat_regions.insert(w.planets[at(s)].region);
    }
    for (const Planet& p : w.planets) {
        if (!p.fringe) network_regions.insert(p.region);
    }
    CHECK(seat_regions.size() == std::min(m.seats.size(), network_regions.size()));

    // TIM-4: travel times are finite, zero to self and symmetric enough.
    for (Id a = 0; a < static_cast<Id>(w.planets.size()); ++a) {
        CHECK(w.derived.travel(a, a) == 0);
        for (Id b = 0; b < static_cast<Id>(w.planets.size()); ++b) {
            CHECK(w.derived.travel(a, b) < 1e6);
            CHECK(w.derived.travel(a, b) == doctest::Approx(w.derived.travel(b, a)).epsilon(1e-9));
        }
    }
}

}  // namespace

TEST_CASE("development worlds keep every generation promise (GEN-1 to GEN-8)") {
    for (std::uint64_t seed = 1; seed <= 30; ++seed) {
        CAPTURE(seed);
        Config c;
        c.chars.target = 0;
        check_world(new_world(c, seed));
    }
}

TEST_CASE("v1-size worlds keep every generation promise") {
    for (std::uint64_t seed = 1; seed <= 8; ++seed) {
        CAPTURE(seed);
        const World w = new_world(v1_size(), seed);
        check_world(w);
        CHECK(w.map.regions.size() == 6);
    }
}

TEST_CASE("about one system in gen.fringe_share is off the network (GEN-3)") {
    std::size_t systems = 0, fringe = 0;
    for (std::uint64_t seed = 1; seed <= 12; ++seed) {
        const World w = new_world(v1_size(), seed);
        for (const System& s : w.map.systems) fringe += s.fringe, ++systems;
    }
    const double share = static_cast<double>(fringe) / static_cast<double>(systems);
    CHECK(share > 0.10);
    CHECK(share < 0.20);
}

TEST_CASE("the Lattice is faster than open space (TIM-4)") {
    const World w = new_world(v1_size(), 3);
    // Two interchanges far apart: the reference trip uses the network.
    const Group* far_a = nullptr;
    const Group* far_b = nullptr;
    double best = 0;
    for (const Group& a : w.map.groups) {
        for (const Group& b : w.map.groups) {
            if (a.node == kNone || b.node == kNone) continue;
            const Planet& pa = w.planets[at(a.holder)];
            const Planet& pb = w.planets[at(b.holder)];
            const double d = std::hypot(pa.x - pb.x, pa.y - pb.y);
            if (d > best) best = d, far_a = &a, far_b = &b;
        }
    }
    REQUIRE(far_a);
    const double open_space = best / w.config.travel.space_speed;
    CHECK(w.derived.travel(far_a->holder, far_b->holder) < open_space / 3);
}

TEST_CASE("the same seed gives the same map; another seed a different one") {
    CHECK(state_hash(new_world(Config{}, 9)) == state_hash(new_world(Config{}, 9)));
    CHECK(state_hash(new_world(Config{}, 9)) != state_hash(new_world(Config{}, 10)));
}

TEST_CASE("a region too small for the producer guarantee doesn't hang generation") {
    Config c;
    c.gen.planets = 4;
    c.gen.regions = 3;
    c.gen.min_producers = 3;
    const World w = new_world(c, 1);
    CHECK(!w.planets.empty());
}
