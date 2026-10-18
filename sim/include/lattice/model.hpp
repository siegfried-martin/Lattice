#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "lattice/fields.hpp"
#include "lattice/goods.hpp"
#include "lattice/time.hpp"

// The records the simulation stores (docs/spec/sim/model.md). Records live in vectors
// and refer to each other by index (ARC-2). Each record's fields() lists what a save
// holds; a change to any of them changes the save layout, so bump kSaveVersion.
//
// Positions are in sector widths: neighbouring sector centres are 1 apart.

namespace lattice {

using Id = std::int32_t;
inline constexpr Id kNone = -1;

// ---- Static world data (MOD-1), generated once (GEN) ----

struct Sector {
    std::int32_t col = 0, row = 0;  // offset coordinates; odd rows shift half a sector east
    double x = 0, y = 0;            // centre
    Id region = kNone;
    Id system = kNone;  // at most one system per sector
};
template <class Ar, record_of<Sector> S>
void fields(Ar& ar, S& s) {
    ar(s.col, s.row, s.x, s.y, s.region, s.system);
}

// A culture region (WLD-16, GEN-2). Its index is its culture.
struct Region {
    Id seed_sector = kNone;
    std::array<double, kLawfulGoodCount> weights{};  // production weights (GEN-6)
    double pop_scale = 1;
};
template <class Ar, record_of<Region> R>
void fields(Ar& ar, R& r) {
    ar(r.seed_sector, r.weights, r.pop_scale);
}

// One to four planets in a sector, plus their moons (WLD-4, GEN-3).
struct System {
    Id sector = kNone;
    double x = 0, y = 0;
    bool fringe = false;  // off the network (WLD-6)
    std::vector<Id> planets{};
};
template <class Ar, record_of<System> S>
void fields(Ar& ar, S& s) {
    ar(s.sector, s.x, s.y, s.fringe, s.planets);
}

// Planets sharing one Lattice entrance and exit (WLD-4).
struct Group {
    Id system = kNone;
    std::vector<Id> planets{};
    Id holder = kNone;  // holds the gates, fixed forever (WLD-5, GEN-5)
    Id node = kNone;    // its interchange, or none off the network
};
template <class Ar, record_of<Group> G>
void fields(Ar& ar, G& g) {
    ar(g.system, g.planets, g.holder, g.node);
}

enum class NodeKind : std::uint8_t { Interchange = 0, Junction = 1 };

struct LatticeNode {
    NodeKind kind = NodeKind::Interchange;
    double x = 0, y = 0;
    Id group = kNone;  // the group an interchange serves
    std::vector<Id> edges{};
};
template <class Ar, record_of<LatticeNode> N>
void fields(Ar& ar, N& n) {
    ar(n.kind, n.x, n.y, n.group, n.edges);
}

struct LatticeEdge {
    Id a = kNone, b = kNone;
    double length = 0;  // sector widths; a junction's link to its interchange is 0
};
template <class Ar, record_of<LatticeEdge> E>
void fields(Ar& ar, E& e) {
    ar(e.a, e.b, e.length);
}

// Open to everyone, a short fixed time (WLD-11, TIM-4).
struct HopLane {
    Id a = kNone, b = kNone;  // planets
    double length = 0;
};
template <class Ar, record_of<HopLane> H>
void fields(Ar& ar, H& h) {
    ar(h.a, h.b, h.length);
}

struct Map {
    std::int32_t width = 0, height = 0;
    std::vector<Sector> sectors;  // index = row * width + col
    std::vector<Region> regions;
    std::vector<System> systems;
    std::vector<Group> groups;
    std::vector<LatticeNode> nodes;
    std::vector<LatticeEdge> edges;
    std::vector<HopLane> hop_lanes;
    std::vector<Id> seats;  // seats of power (GEN-8): planets
};
template <class Ar, record_of<Map> M>
void fields(Ar& ar, M& m) {
    ar(m.width, m.height, m.sectors, m.regions, m.systems, m.groups, m.nodes, m.edges, m.hop_lanes, m.seats);
}

// ---- Planet (MOD-2) ----

struct Planet {
    // Static (GEN).
    Id sector = kNone, region = kNone, system = kNone, group = kNone;
    Id moon_of = kNone;
    double x = 0, y = 0;
    bool fringe = false;
    std::array<Good, 3> makes{};  // three distinct lawful goods (PRD-7, GEN-6)
    std::array<Tier, 3> tiers{};  // high, medium, low in some order
    double fuel_price = 0;        // ECO-11, GEN-7

    // The population (PRD-2 to PRD-5, SYS-1). Population is in thousands.
    double population = 0;
    double happiness = 0;
    double productivity = 0;
    double growth_factor = 1;   // PRD-3; from the governor's risk tolerance once there are governors
    double contraband_avg = 0;  // C in SYS-1

    // The market (SYS-3) and production (SYS-2).
    std::array<double, kGoodCount> stock{};
    std::array<double, 3> labour{};       // share per good made; sums to 1
    std::array<std::int8_t, 3> tier_shift{};  // from active events: -1 up a tier, +1 down
    std::int32_t festivals = 0;           // active festivals (SYS-4)

    // Money (SYS-5).
    double treasury = 0;      // may be negative (ECO-8)
    double tax_rate = 0;      // ECO-3
    double income_avg = 0;    // daily income, averaged over a payment period
    double income_today = 0;  // since the planet's last day
    std::int64_t deficit_days = 0;  // consecutive days with a negative treasury (ECO-13)
};
template <class Ar, record_of<Planet> P>
void fields(Ar& ar, P& p) {
    ar(p.sector, p.region, p.system, p.group, p.moon_of, p.x, p.y, p.fringe, p.makes, p.tiers, p.fuel_price);
    ar(p.population, p.happiness, p.productivity, p.growth_factor, p.contraband_avg);
    ar(p.stock, p.labour, p.tier_shift, p.festivals);
    ar(p.treasury, p.tax_rate, p.income_avg, p.income_today, p.deficit_days);
}

// ---- World event (MOD-10, SYS-4) ----

// Append only.
enum class WorldEventKind : std::uint8_t { Drought = 0, Bumper = 1, Festival = 2, RegionalDrought = 3 };

struct WorldEvent {
    WorldEventKind kind = WorldEventKind::Drought;
    Id region = kNone;        // regional events
    std::vector<Id> planets{};  // the planets it affects
    Good good = kFood;        // the good whose efficiency it shifts (not festivals)
    Time start = 0, end = 0;
    bool alive = true;
};
template <class Ar, record_of<WorldEvent> E>
void fields(Ar& ar, E& e) {
    ar(e.kind, e.region, e.planets, e.good, e.start, e.end, e.alive);
}

// World totals kept for the money and goods checks (CHK-7). Cumulative.
struct Ledger {
    double money_created = 0;   // population spending, the only new money (ECO-1)
    double money_sunk = 0;      // operating costs and the osmosis margin (ECO-5)
    double osmosis_volume = 0;  // goods moved by osmosis
    double osmosis_margin = 0;  // part of money_sunk
    std::array<double, kGoodCount> produced{};
    std::array<double, kGoodCount> consumed{};
};
template <class Ar, record_of<Ledger> L>
void fields(Ar& ar, L& l) {
    ar(l.money_created, l.money_sunk, l.osmosis_volume, l.osmosis_margin, l.produced, l.consumed);
}

}  // namespace lattice
