#pragma once

#include <array>
#include <cstdint>
#include <string>
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

// A preset ship (MOD-1, CHR-13). Each culture has every class at tiers 1 to 3.
// Append only.
enum class ShipClass : std::uint8_t { Courier = 0, Scout = 1, Freighter = 2, Gunship = 3 };
inline constexpr int kShipClassCount = 4;
inline constexpr int kTierCount = 3;

struct ShipVariant {
    Id culture = kNone;
    ShipClass cls = ShipClass::Courier;
    std::int8_t tier = 1;     // 1 to 3
    double combat = 0;        // combat power (CHR-12)
    double cargo = 0;         // trade power: tons of cargo
    double speed = 0;         // open space, sector widths per day
    double price = 0;
    bool lattice_drive = true;
};
template <class Ar, record_of<ShipVariant> V>
void fields(Ar& ar, V& v) {
    ar(v.culture, v.cls, v.tier, v.combat, v.cargo, v.speed, v.price, v.lattice_drive);
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
    std::vector<ShipVariant> variants;  // index: (culture x classes + class) x tiers + tier - 1
};
template <class Ar, record_of<Map> M>
void fields(Ar& ar, M& m) {
    ar(m.width, m.height, m.sectors, m.regions, m.systems, m.groups, m.nodes, m.edges, m.hop_lanes, m.seats,
       m.variants);
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

    // Politics (FAC-1, FAC-2).
    Id governor = kNone;  // character
    Id liege = kNone;     // the planet whose governor this one's governor is sworn to; none for a root
    std::vector<Id> missions{};  // mission board (MOD-9): open missions posted here
    double tribute_rate = 0;     // set by its liege (FAC-6)
    double income_month = 0;     // own income since its last payment (sales and tax)
    double tribute_month = 0;    // tribute received since its last payment
    double tribute_paid = 0;     // its last payment's tribute, as its liege saw it
    Time low_since = kNever;     // its governor's opinion of the liege below fealty.defect_opinion since

    // Defence (MIL-5).
    double security = 0;     // monthly spend on static defence and patrols (ECO-4): a sink
    double defence = 0;      // static defence now, in combat power
    Id siege = kNone;        // the siege it's under
};
template <class Ar, record_of<Planet> P>
void fields(Ar& ar, P& p) {
    ar(p.sector, p.region, p.system, p.group, p.moon_of, p.x, p.y, p.fringe, p.makes, p.tiers, p.fuel_price);
    ar(p.population, p.happiness, p.productivity, p.growth_factor, p.contraband_avg);
    ar(p.stock, p.labour, p.tier_shift, p.festivals);
    ar(p.treasury, p.tax_rate, p.income_avg, p.income_today, p.deficit_days);
    ar(p.governor, p.liege, p.missions, p.tribute_rate, p.income_month, p.tribute_month, p.tribute_paid, p.low_since);
    ar(p.security, p.defence, p.siege);
}

// ---- Relationships (REL-2, REL-3, SYS-10) ----

// What happened (REL-3). Append only.
enum class DeedKind : std::uint8_t {
    HelpSent = 0, HelpMissed = 1, PlanetGranted = 2, Patronage = 3, TributeRaised = 4, PassedOver = 5,
    CallAnswered = 6, CallRefused = 7, BorderHeld = 8, JobDone = 9, Attacked = 10, TributeLowered = 11,
};
inline constexpr int kDeedKindCount = 12;

struct Deed {
    DeedKind kind = DeedKind::JobDone;
    double amount = 0;
    Time time = 0;
};
template <class Ar, record_of<Deed> D>
void fields(Ar& ar, D& d) {
    ar(d.kind, d.amount, d.time);
}

// One character's opinion of another. The value drifts toward its resting point as
// time passes; it's brought up to date whenever it's read or moved (SYS-10).
struct Opinion {
    Id of = kNone;  // the character it's about
    double value = 0;
    Time updated = 0;
    std::vector<Deed> recent{};  // the last few, for the UI (REL-12)
};
template <class Ar, record_of<Opinion> O>
void fields(Ar& ar, O& o) {
    ar(o.of, o.value, o.updated, o.recent);
}

// ---- Facts (MOD-8, INF-2) ----

// What a character believes about one market: every lawful good's price and stock when
// it was observed. Landing at a market observes all of them at once (INF-3).
struct PriceFact {
    Id planet = kNone;
    Time time = 0;
    std::array<double, kLawfulGoodCount> price{};
    std::array<double, kLawfulGoodCount> stock{};
};
template <class Ar, record_of<PriceFact> F>
void fields(Ar& ar, F& f) {
    ar(f.planet, f.time, f.price, f.stock);
}

// A character's information. Sorted by planet, so sharing is a merge of two sorted
// lists keeping the newer of each (INF-4).
struct Facts {
    std::vector<PriceFact> prices{};
};
template <class Ar, record_of<Facts> F>
void fields(Ar& ar, F& f) {
    ar(f.prices);
}

// ---- Character (MOD-3) ----

// One leg of a journey (TIM-4 to TIM-6). Append only.
enum class LegKind : std::uint8_t { Space = 0, Lattice = 1, Hop = 2 };

struct Leg {
    LegKind kind = LegKind::Space;
    double x0 = 0, y0 = 0, x1 = 0, y1 = 0;  // straight-line ends (the Lattice's are drawn by node)
    Time depart = 0, arrive = 0;           // depart is set when the leg starts
    Id to_planet = kNone;                  // where it ends: a planet, or
    Id to_node = kNone;                    // a Lattice node
    double lattice_length = 0;             // for fuel (SYS-21)
};
template <class Ar, record_of<Leg> L>
void fields(Ar& ar, L& l) {
    ar(l.kind, l.x0, l.y0, l.x1, l.y1, l.depart, l.arrive, l.to_planet, l.to_node, l.lattice_length);
}

// What a character is doing now (CHR-2's short-term action). Append only.
enum class Activity : std::uint8_t {
    Idle = 0,
    Governing = 1,  // a governor stays at its planet
    Trading = 2,    // on a trip (SYS-7)
    Exploring = 3,  // moving to look at another market (TRD-4)
    Mission = 4,    // on a mission (SYS-8)
    Serving = 5,    // a sworn commander at its post (MIL-2)
    Army = 6,       // with an army, mustering or moving with it (MIL-7)
};
inline constexpr int kActivityCount = 7;

// The kinds of mission and skill (CHR-10). Append only.
enum class MissionKind : std::uint8_t { Courier = 0, Scout = 1, Trade = 2, Escort = 3, Command = 4, Smuggle = 5 };
inline constexpr int kSkillCount = 6;

// One stop of a merchant's planned trip: sell what's aboard, then buy `buy` (or
// nothing) for the next stop.
struct TripStop {
    Id planet = kNone;
    Good buy = kGoodCount;  // kGoodCount: buy nothing
};
template <class Ar, record_of<TripStop> T>
void fields(Ar& ar, T& t) {
    ar(t.planet, t.buy);
}

struct Character {
    std::string name;
    Id culture = kNone;
    bool alive = true;
    double risk = 0.5;  // risk tolerance, 0-1 (CHR-4)
    double wealth = 0;  // credits (CHR-8)
    std::vector<Id> ships{};  // variants; the first is the flagship
    Time born = 0;

    // Where it is. At a planet, or travelling a route of legs.
    Id planet = kNone;       // the planet it's at, or none while travelling
    Id last_planet = kNone;  // where it last landed (and refuelled)
    std::vector<Leg> route{};
    std::int32_t leg = 0;    // the leg in progress
    std::uint32_t travel_seq = 0;  // bumped on every new route, to drop stale arrivals

    // Work.
    Activity activity = Activity::Idle;
    std::vector<Opinion> opinions{};  // sorted by who it's about (REL-2)
    double authority_opinion = 0;     // the Trade Authority's opinion of it: its standing (FAC-14)
    Id governs = kNone;                    // the planet it governs (FAC-1)
    std::vector<Id> contracts{};           // MOD-4
    Id mission = kNone;                    // the mission it's on
    std::array<double, kSkillCount> skills{};  // by MissionKind (CHR-10)
    double reputation = 0;                 // REL-7
    std::int64_t missions_done = 0;

    // Trade.
    Good cargo_good = kGoodCount;  // kGoodCount: empty
    double cargo_qty = 0;
    double cargo_cost = 0;         // what it paid, for the trip's profit
    std::vector<TripStop> trip{};  // remaining stops
    double trip_profit = 0;        // so far on this trip
    Time trip_start = 0;

    // Spending (SYS-8): what has paid recently, as running averages per day.
    double earned_cargo = 0;   // trade and courier work
    double earned_combat = 0;  // scout, escort and command work

    Facts facts;
    Time last_meet = -kDay;  // last fact merge in open space (SYS-15)
    Time idle_since = 0;
    Time broke_since = kNever;  // below zero since (PIR-4); kNever when solvent
    std::uint32_t wake_seq = 0;  // only the latest scheduled decision runs
    Time salary_checked = -kDay * 365;  // last weighed a merchant salary (SYS-9)
    Id army = kNone;    // the army it serves in
    Id battle = kNone;  // the battle it's fighting
};
template <class Ar, record_of<Character> C>
void fields(Ar& ar, C& c) {
    ar(c.name, c.culture, c.alive, c.risk, c.wealth, c.ships, c.born);
    ar(c.planet, c.last_planet, c.route, c.leg, c.travel_seq);
    ar(c.activity, c.opinions, c.authority_opinion, c.governs, c.contracts, c.mission, c.skills, c.reputation,
       c.missions_done);
    ar(c.cargo_good, c.cargo_qty, c.cargo_cost, c.trip, c.trip_profit, c.trip_start);
    ar(c.earned_cargo, c.earned_combat, c.facts, c.last_meet, c.idle_since, c.broke_since, c.wake_seq, c.salary_checked, c.army, c.battle);
}

// ---- Contract (MOD-4, REL-8) ----

// Append only.
enum class ContractKind : std::uint8_t {
    Fealty = 0, Service = 1, MerchantSalary = 2, Escort = 3, Courier = 4, Scout = 5, Security = 6, Mercenary = 7,
    PirateHire = 8,
};

struct Contract {
    ContractKind kind = ContractKind::Courier;
    Id employer = kNone, employee = kNone;  // characters
    double pay = 0;      // per period, or on completion when period is 0
    Time period = 0;
    Id mission = kNone;  // the mission it carries out
    Time start = 0;
    bool alive = true;
};
template <class Ar, record_of<Contract> C>
void fields(Ar& ar, C& c) {
    ar(c.kind, c.employer, c.employee, c.pay, c.period, c.mission, c.start, c.alive);
}

// ---- Mission (MOD-9) ----

enum class MissionLevel : std::uint8_t { Planet = 0, Faction = 1 };
// Append only.
enum class MissionState : std::uint8_t { Open = 0, Taken = 1, Done = 2, Expired = 3 };

struct Mission {
    MissionKind kind = MissionKind::Courier;
    MissionLevel level = MissionLevel::Planet;
    Id poster = kNone;  // governor (character)
    Id at = kNone;      // planet where it's posted
    Id target = kNone;  // planet
    double base_pay = 0;
    double min_combat = 0, min_cargo = 0;  // requirements
    Time posted = 0, expires = 0;
    MissionState state = MissionState::Open;
    Id taker = kNone;
    double pay = 0;     // agreed when taken (it grows while open)
    bool outbound = true;  // a round trip's first half
    Facts packet{};     // what a courier or scout carries
};
template <class Ar, record_of<Mission> M>
void fields(Ar& ar, M& m) {
    ar(m.kind, m.level, m.poster, m.at, m.target, m.base_pay, m.min_combat, m.min_cargo, m.posted, m.expires);
    ar(m.state, m.taker, m.pay, m.outbound, m.packet);
}

// ---- An automated courier run (INF-8, SYS-15) ----

// A scheduled transit on the Lattice, not a simulated ship: the facts both ways are
// what it carries.
struct CourierRun {
    Id from = kNone, to = kNone;  // planets: the liege's and the vassal's
    bool returning = false;
    bool alive = true;
    Facts packet{};
};
template <class Ar, record_of<CourierRun> R>
void fields(Ar& ar, R& r) {
    ar(r.from, r.to, r.returning, r.alive, r.packet);
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
    double money_spawned = 0;  // new characters' starting wealth (SYS-18)
    // Sinks by kind, each also counted in money_sunk.
    double running_costs = 0, fuel = 0, ships_bought = 0, courier_runs = 0;
    std::array<double, kGoodCount> traded{};  // goods bought by characters at markets
    double trade_profit = 0;   // merchants' realised trip profit
    std::int64_t trips = 0;
    std::int64_t detections = 0, meetings_shared = 0;
    std::int64_t missions_posted = 0, missions_done = 0, missions_expired = 0;
    // Politics (MS-4).
    double tribute = 0, patronage = 0, stipends = 0, salaries = 0, dues = 0, maintenance = 0;
    std::int64_t defections = 0, hires = 0, fires = 0, tribute_changes = 0;
    // War (MS-5).
    double security = 0, ceasefire_tribute = 0, power_destroyed = 0;
    std::array<double, kGoodCount> destroyed{};  // goods lost in capture and battle
    std::int64_t wars = 0, ceasefires = 0, battles = 0, sieges = 0, captures = 0, reliefs = 0, deaths = 0, pods = 0;
};
template <class Ar, record_of<Ledger> L>
void fields(Ar& ar, L& l) {
    ar(l.money_created, l.money_sunk, l.osmosis_volume, l.osmosis_margin, l.produced, l.consumed);
    ar(l.money_spawned, l.running_costs, l.fuel, l.ships_bought, l.courier_runs, l.traded, l.trade_profit, l.trips);
    ar(l.detections, l.meetings_shared, l.missions_posted, l.missions_done, l.missions_expired);
    ar(l.tribute, l.patronage, l.stipends, l.salaries, l.dues, l.maintenance, l.defections, l.hires, l.fires,
       l.tribute_changes);
    ar(l.security, l.ceasefire_tribute, l.power_destroyed, l.destroyed, l.wars, l.ceasefires, l.battles, l.sieges, l.captures,
       l.reliefs, l.deaths, l.pods);
}

// ---- Diplomacy (MOD-5, DIP-1 to DIP-3) ----

// Append only.
enum class DipState : std::uint8_t { Peace = 0, War = 1, Ceasefire = 2 };

// Between two roots, a < b. Pairs with no record are at peace.
struct Relation {
    Id a = kNone, b = kNone;
    DipState state = DipState::Peace;
    Time since = 0;
    Time peace_since = 0;           // when the last war ended
    std::array<double, 2> peace_income{};  // each root's faction income then, for the war vote
    // A ceasefire's tribute (DIP-2).
    Id payer = kNone, payee = kNone;
    double amount = 0;  // per month
    Time until = 0;
    // The war ledger (DIP-3): per side (a, b), since the war began.
    std::array<double, 2> planets_lost{}, power_lost{};
    std::array<double, 2> planets_at_start{}, power_at_start{};
};
template <class Ar, record_of<Relation> R>
void fields(Ar& ar, R& r) {
    ar(r.a, r.b, r.state, r.since, r.peace_since, r.peace_income, r.payer, r.payee, r.amount, r.until);
    ar(r.planets_lost, r.power_lost, r.planets_at_start, r.power_at_start);
}

// ---- Armies, battles and sieges (MOD-6, MOD-7) ----

// Append only.
enum class ArmyAction : std::uint8_t { Muster = 0, Move = 1, Besiege = 2, Retreat = 3, Disband = 4 };

struct Army {
    Id commissioner = kNone;  // the governor (character) who raised it (MIL-9)
    Id home = kNone;          // its planet: the muster point
    Id target = kNone;        // the planet it's to take: its intent (MIL-7)
    bool relief = false;      // sent to break a siege (MIL-11)
    std::vector<Id> members{};
    std::vector<Id> called{};  // commanders on their way to join
    ArmyAction action = ArmyAction::Muster;
    Time formed = 0;
    // Moving as one, at the slowest member's speed (MIL-8).
    Id planet = kNone;  // where it is, when not moving
    std::vector<Leg> route{};
    std::int32_t leg = 0;
    std::uint32_t travel_seq = 0;
    Id battle = kNone, siege = kNone;
    bool alive = true;
};
template <class Ar, record_of<Army> A>
void fields(Ar& ar, A& a) {
    ar(a.commissioner, a.home, a.target, a.relief, a.members, a.called, a.action, a.formed);
    ar(a.planet, a.route, a.leg, a.travel_seq, a.battle, a.siege, a.alive);
}

// A fight, resolved in hourly rounds (SYS-14). Side 0 attacks.
struct Battle {
    Id planet = kNone;  // where (a planet for sieges and their reliefs)
    double x = 0, y = 0;
    std::array<std::vector<Id>, 2> sides{};  // characters
    std::array<Id, 2> armies{kNone, kNone};
    std::array<double, 2> damage{};  // damage not yet enough to take a ship
    std::array<double, 2> lost{};    // combat power lost so far
    bool planet_side = false;        // side 1 has the planet's static defence
    Time start = 0;
    std::int64_t rounds = 0;
    bool alive = true;
};
template <class Ar, record_of<Battle> B>
void fields(Ar& ar, B& b) {
    ar(b.planet, b.x, b.y, b.sides, b.armies, b.damage, b.lost, b.planet_side, b.start, b.rounds, b.alive);
}

// MOD-7: once no defending ships remain, daily rounds against the static defence.
struct Siege {
    Id planet = kNone;
    Id army = kNone;
    Time start = 0;
    bool called = false;  // the call for help has gone out (MIL-11)
    bool relieved = false;
    bool alive = true;
};
template <class Ar, record_of<Siege> S>
void fields(Ar& ar, S& s) {
    ar(s.planet, s.army, s.start, s.called, s.relieved, s.alive);
}

// ---- The Trade Authority (FAC-13, FAC-14, SYS-20) ----

// Not a fealty tree and holds no planets: a treasury fed by dues.
struct Authority {
    double treasury = 0;
};
template <class Ar, record_of<Authority> A>
void fields(Ar& ar, A& a) {
    ar(a.treasury);
}

}  // namespace lattice
