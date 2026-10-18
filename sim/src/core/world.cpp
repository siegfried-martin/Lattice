#include "lattice/world.hpp"

#include <algorithm>
#include <cassert>
#include <utility>
#include <cmath>
#include <numeric>
#include <string>

#include "chars/characters.hpp"
#include "chars/missions.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "core/travel.hpp"
#include "info/facts.hpp"
#include "politics/fealty.hpp"
#include "pirates/pirates.hpp"
#include "politics/authority.hpp"
#include "politics/governor.hpp"
#include "politics/opinion.hpp"
#include "trade/merchants.hpp"
#include "economy/economy.hpp"
#include "economy/events.hpp"
#include "lattice/market.hpp"
#include "gen/worldgen.hpp"
#include "trade/osmosis.hpp"
#include "war/armies.hpp"
#include "war/combat.hpp"
#include "war/diplomacy.hpp"
#include "war/encounters.hpp"
#include "war/risk.hpp"
#include "war/threats.hpp"

namespace lattice {

namespace {

constexpr Time kPingMin = kHour;
constexpr Time kPingMax = 2 * kDay;

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

// The day's metrics (ARC-8), sampled at the start of each day, before anything runs.
void sample_metrics(const World& w, Observer& o) {
    const Config& c = w.config;
    const Time t = w.now;
    o.sample(t, "world.events_processed", -1, static_cast<double>(w.events_processed));
    o.sample(t, "world.queue_size", -1, static_cast<double>(w.events.size()));
    if (!w.ping_fires.empty()) {
        const auto total = std::accumulate(w.ping_fires.begin(), w.ping_fires.end(), std::int64_t{0});
        o.sample(t, "debug.ping_fires", -1, static_cast<double>(total));
    }
    if (w.planets.empty()) return;

    static const auto price_series = [] {
        std::array<std::string, kGoodCount> s;
        for (int g = 0; g < kGoodCount; ++g) s[at(g)] = "planet.price." + std::string(good_name(static_cast<Good>(g)));
        return s;
    }();
    static const auto labour_series = [] {
        std::array<std::string, kGoodCount> s;
        for (int g = 0; g < kGoodCount; ++g) s[at(g)] = "planet.labour." + std::string(good_name(static_cast<Good>(g)));
        return s;
    }();

    double population = 0, money = 0, index = 0;
    std::array<double, kGoodCount> stock{}, price_sum{}, price_sq{};
    for (Id id = 0; id < static_cast<Id>(w.planets.size()); ++id) {
        const Planet& p = w.planets[at(id)];
        o.sample(t, "planet.population", id, p.population);
        o.sample(t, "planet.happiness", id, p.happiness);
        o.sample(t, "planet.productivity", id, p.productivity);
        o.sample(t, "planet.treasury", id, p.treasury);
        for (int gi = 0; gi < kLawfulGoodCount; ++gi) {
            const auto g = static_cast<Good>(gi);
            const double price = market::price(c, p, g);
            o.sample(t, price_series[g], id, price);
            stock[g] += p.stock[g];
            price_sum[g] += price;
            price_sq[g] += price * price;
            index += price / c.goods[g].base_price;
        }
        for (int s = 0; s < 3; ++s) o.sample(t, labour_series[p.makes[at(s)]], id, p.labour[at(s)]);
        population += p.population;
        money += p.treasury;
    }
    const double n = static_cast<double>(w.planets.size());
    o.sample(t, "world.population", -1, population);
    o.sample(t, "world.money", -1, money);
    o.sample(t, "world.money_created", -1, w.ledger.money_created);
    o.sample(t, "world.money_sunk", -1, w.ledger.money_sunk);
    o.sample(t, "world.osmosis_volume", -1, w.ledger.osmosis_volume);
    o.sample(t, "world.osmosis_margin", -1, w.ledger.osmosis_margin);
    o.sample(t, "world.price_index", -1, index / (n * kLawfulGoodCount));
    std::int64_t active = 0;
    for (const WorldEvent& e : w.world_events) active += e.alive;
    o.sample(t, "world.events_active", -1, static_cast<double>(active));

    // Characters (CHK-16 to CHK-18).
    std::array<std::int64_t, kActivityCount> by_activity{};
    std::int64_t alive = 0, faction_merchants = 0, ships = 0;
    double wealth = 0, cargo = 0;
    for (Id id = 0; id < static_cast<Id>(w.characters.size()); ++id) {
        const Character& ch = w.characters[at(id)];
        if (!ch.alive) continue;
        ++alive;
        wealth += ch.wealth;
        ++by_activity[static_cast<std::size_t>(ch.activity)];
        faction_merchants += merchant::employer_root(w, id) != kNone;
        const ships::Fleet f = ships::fleet(w, ch);
        ships += f.count;
        cargo += f.cargo;
    }
    o.sample(t, "world.characters", -1, static_cast<double>(alive));
    o.sample(t, "world.char_wealth", -1, wealth);
    o.sample(t, "world.money_total", -1, money + wealth + w.authority.treasury);
    o.sample(t, "world.money_spawned", -1, w.ledger.money_spawned);
    o.sample(t, "world.ships", -1, static_cast<double>(ships));
    o.sample(t, "world.cargo_capacity", -1, cargo);
    o.sample(t, "world.chars_idle", -1, static_cast<double>(by_activity[static_cast<std::size_t>(Activity::Idle)]));
    o.sample(t, "world.chars_trading", -1, static_cast<double>(by_activity[static_cast<std::size_t>(Activity::Trading)]));
    o.sample(t, "world.chars_exploring", -1, static_cast<double>(by_activity[static_cast<std::size_t>(Activity::Exploring)]));
    o.sample(t, "world.chars_mission", -1, static_cast<double>(by_activity[static_cast<std::size_t>(Activity::Mission)]));
    o.sample(t, "world.faction_merchants", -1, static_cast<double>(faction_merchants));
    o.sample(t, "world.chars_serving", -1, static_cast<double>(by_activity[static_cast<std::size_t>(Activity::Serving)]));
    o.sample(t, "world.chars_army", -1, static_cast<double>(by_activity[static_cast<std::size_t>(Activity::Army)]));

    // Politics (CHK-1, CHK-3).
    std::int64_t roots = 0, factions = 0, sworn = 0;
    double loyalty = 0;
    std::vector<std::int64_t> size(w.planets.size(), 0);
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) ++size[at(fealty::root_of(w, p))];
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        const Planet& pl = w.planets[at(p)];
        if (pl.liege == kNone) {
            if (pl.pirate) continue;  // the pirate faction is not a root (PIR-1)
            ++roots;
            factions += size[at(p)] >= 2;
        } else {
            ++sworn;
            loyalty += opinion::get(w, pl.governor, w.planets[at(pl.liege)].governor);
        }
    }
    std::int64_t commanders = 0;
    for (const Contract& k : w.contracts) commanders += k.alive && k.kind == ContractKind::Service;
    o.sample(t, "world.roots", -1, static_cast<double>(roots));
    o.sample(t, "world.factions", -1, static_cast<double>(factions));
    o.sample(t, "world.loyalty", -1, sworn ? loyalty / static_cast<double>(sworn) : 0);
    o.sample(t, "world.commanders", -1, static_cast<double>(commanders));
    o.sample(t, "world.defections", -1, static_cast<double>(w.ledger.defections));
    o.sample(t, "world.tribute", -1, w.ledger.tribute);
    o.sample(t, "world.patronage", -1, w.ledger.patronage);
    o.sample(t, "world.stipends", -1, w.ledger.stipends);
    o.sample(t, "world.salaries", -1, w.ledger.salaries);
    o.sample(t, "world.dues", -1, w.ledger.dues);
    o.sample(t, "world.authority_treasury", -1, w.authority.treasury);
    o.sample(t, "world.hires", -1, static_cast<double>(w.ledger.hires));

    // War (CHK-5, CHK-6, CHK-8, CHK-20).
    std::int64_t at_war = 0, armies = 0, besieged = 0;
    double defence = 0;
    for (const Relation& r : w.relations) at_war += diplomacy::state(w, r.a, r.b) == DipState::War;
    for (const Army& a : w.armies) armies += a.alive;
    for (const Planet& pl : w.planets) besieged += pl.siege != kNone, defence += pl.defence;
    o.sample(t, "world.wars_now", -1, static_cast<double>(at_war));
    o.sample(t, "world.armies", -1, static_cast<double>(armies));
    o.sample(t, "world.besieged", -1, static_cast<double>(besieged));
    o.sample(t, "world.static_defence", -1, defence);
    o.sample(t, "world.security", -1, w.ledger.security);
    o.sample(t, "world.wars", -1, static_cast<double>(w.ledger.wars));
    o.sample(t, "world.ceasefires", -1, static_cast<double>(w.ledger.ceasefires));
    o.sample(t, "world.battles", -1, static_cast<double>(w.ledger.battles));
    o.sample(t, "world.sieges", -1, static_cast<double>(w.ledger.sieges));
    o.sample(t, "world.captures", -1, static_cast<double>(w.ledger.captures));
    o.sample(t, "world.reliefs", -1, static_cast<double>(w.ledger.reliefs));
    o.sample(t, "world.deaths", -1, static_cast<double>(w.ledger.deaths));
    o.sample(t, "world.power_destroyed", -1, w.ledger.power_destroyed);
    o.sample(t, "world.raids", -1, static_cast<double>(w.ledger.raids));
    o.sample(t, "world.escorts", -1, static_cast<double>(w.ledger.escorts));
    o.sample(t, "world.escort_pay", -1, w.ledger.escort_pay);
    o.sample(t, "world.loot", -1, w.ledger.loot);
    o.sample(t, "world.sightings", -1, static_cast<double>(w.ledger.sightings));
    o.sample(t, "world.warnings", -1, static_cast<double>(w.ledger.warnings));
    o.sample(t, "world.garrisons", -1, static_cast<double>(w.ledger.garrisons));
    o.sample(t, "world.patrols", -1, static_cast<double>(w.ledger.patrols));
    // Risk (SYS-19, CHK-4): per sector, and its spread.
    double risk_sum = 0, risk_max = 0;
    for (std::size_t s = 0; s < w.risk.size(); ++s) {
        risk_sum += w.risk[s].risk;
        risk_max = std::max(risk_max, w.risk[s].risk);
        if (w.risk[s].risk > 0) o.sample(t, "sector.risk", static_cast<std::int64_t>(s), w.risk[s].risk);
    }
    o.sample(t, "world.mean_risk", -1, w.risk.empty() ? 0 : risk_sum / static_cast<double>(w.risk.size()));
    o.sample(t, "world.max_risk", -1, risk_max);

    // Piracy (CHK-4, CHK-9, CHK-23).
    std::int64_t pirate_planets = 0, fringe_pirates = 0, pirates = 0;
    for (const Planet& pl : w.planets) pirate_planets += pl.pirate, fringe_pirates += pl.pirate && pl.fringe;
    for (const Character& ch : w.characters) pirates += ch.alive && ch.pirate;
    o.sample(t, "world.pirate_planets", -1, static_cast<double>(pirate_planets));
    o.sample(t, "world.fringe_pirate_planets", -1, static_cast<double>(fringe_pirates));
    o.sample(t, "world.pirates", -1, static_cast<double>(pirates));
    o.sample(t, "world.flips", -1, static_cast<double>(w.ledger.flips));
    o.sample(t, "world.recaptures", -1, static_cast<double>(w.ledger.recaptures));
    o.sample(t, "world.turned_pirate", -1, static_cast<double>(w.ledger.turned_pirate));
    o.sample(t, "world.pirate_raids", -1, static_cast<double>(w.ledger.pirate_raids));
    o.sample(t, "world.authority_contracts", -1, static_cast<double>(w.ledger.authority_contracts));
    o.sample(t, "world.fires", -1, static_cast<double>(w.ledger.fires));
    o.sample(t, "world.trips", -1, static_cast<double>(w.ledger.trips));
    o.sample(t, "world.trade_profit", -1, w.ledger.trade_profit);
    o.sample(t, "world.running_costs", -1, w.ledger.running_costs);
    o.sample(t, "world.fuel", -1, w.ledger.fuel);
    o.sample(t, "world.ships_bought", -1, w.ledger.ships_bought);
    o.sample(t, "world.courier_runs", -1, w.ledger.courier_runs);
    o.sample(t, "world.missions_posted", -1, static_cast<double>(w.ledger.missions_posted));
    o.sample(t, "world.missions_done", -1, static_cast<double>(w.ledger.missions_done));
    o.sample(t, "world.missions_expired", -1, static_cast<double>(w.ledger.missions_expired));
    o.sample(t, "world.meetings_shared", -1, static_cast<double>(w.ledger.meetings_shared));
    for (int g = 0; g < kLawfulGoodCount; ++g) {
        const double mean = price_sum[at(g)] / n;
        const double var = std::max(0.0, price_sq[at(g)] / n - mean * mean);
        o.sample(t, "good.price_cv", g, mean > 0 ? std::sqrt(var) / mean : 0);  // dispersion (CHK-14)
        o.sample(t, "good.stock", g, stock[at(g)]);
        o.sample(t, "good.produced", g, w.ledger.produced[at(g)]);
        o.sample(t, "good.consumed", g, w.ledger.consumed[at(g)]);
        o.sample(t, "good.traded", g, w.ledger.traded[at(g)]);
    }
}

// The world's daily cadence (TIM-3): metrics, then the event roll (SYS-4), then
// osmosis (SYS-6). Each planet's own day runs from its PlanetDay event.
void on_daily_tick(World& w, Observer* observer) {
    if (observer) sample_metrics(w, *observer);
    events::roll(w, observer);
    osmosis::run(w);
    chars::daily(w, observer);
    risk::daily(w);
    if (day_of(w.now) % 7 == 0) authority::weekly(w, observer);
}

void on_debug_ping(World& w, const Event& e) {
    ++w.ping_fires[at(static_cast<Id>(e.record))];
    Event next = e;
    next.time = w.now + w.rng[Stream::Debug].uniform_int(kPingMin, kPingMax);
    w.events.push(next);
}

void dispatch(World& w, const Event& e, Observer* observer) {
    const auto record = static_cast<Id>(e.record);
    switch (e.kind) {
        case EventKind::DailyTick:
            on_daily_tick(w, observer);
            break;
        case EventKind::DebugPing:
            on_debug_ping(w, e);
            break;
        case EventKind::PlanetDay:
            economy::planet_day(w, record);
            break;
        case EventKind::PlanetLabour:
            economy::planet_labour(w, record);
            break;
        case EventKind::PlanetPayment:
            economy::planet_payment(w, record);
            governor::monthly(w, record, observer);
            break;
        case EventKind::WorldEventEnd:
            events::end(w, record, observer);
            break;
        case EventKind::CharDecide:
            chars::decide(w, record, static_cast<std::uint32_t>(e.a), observer);
            break;
        case EventKind::CharArrive:
            move::on_arrive(w, record, static_cast<std::uint32_t>(e.a), observer);
            break;
        case EventKind::KinematicStep:
            encounters::step(w, observer);
            break;
        case EventKind::CourierDue:
            info::courier_due(w, record, observer);
            break;
        case EventKind::CourierArrive:
            info::courier_arrive(w, record, observer);
            break;
        case EventKind::MissionExpire:
            missions::expire(w, record, observer);
            break;
        case EventKind::GovernorWeekly:
            governor::weekly(w, record, observer);
            break;
        case EventKind::ArmyMuster:
            armies::muster_deadline(w, record, observer);
            break;
        case EventKind::ArmyArrive:
            armies::on_arrive(w, record, static_cast<std::uint32_t>(e.a), observer);
            break;
        case EventKind::BattleRound:
            combat::round(w, record, observer);
            break;
        case EventKind::SiegeRound:
            armies::siege_round(w, record, observer);
            break;
        case EventKind::CallForHelp:
            armies::call_for_help(w, record, static_cast<Id>(e.a), observer);
            break;
        case EventKind::ThreatWarning:
            threats::warning(w, record, static_cast<Id>(e.a), static_cast<Id>(e.b), observer);
            break;
    }
}

}  // namespace

void rebuild_derived(World& w) {
    Derived& d = w.derived;
    d = Derived{};
    d.planet_count = w.planets.size();
    travel::Routes routes = travel::planet_routes(w);
    d.travel_days = std::move(routes.days);
    d.route_pred = std::move(routes.pred);
    d.route_via = std::move(routes.via);
    d.route_sums.assign(d.planet_count * d.planet_count, {});
    d.route_risk_sectors.assign(d.planet_count * d.planet_count, {});
    for (Id a = 0; a < static_cast<Id>(d.planet_count); ++a) {
        for (Id b = 0; b < static_cast<Id>(d.planet_count); ++b) {
            if (a == b) continue;
            const std::size_t k = at(a) * d.planet_count + at(b);
            Derived::RouteSum& s = d.route_sums[k];
            std::vector<Id>& sectors = d.route_risk_sectors[k];
            for (const Leg& l : move::route_legs(w, a, b)) {
                if (l.kind == LegKind::Space) {
                    s.space += std::hypot(l.x1 - l.x0, l.y1 - l.y0);
                    sectors.push_back(risk::sector_at(w.map, l.x0, l.y0));
                    sectors.push_back(risk::sector_at(w.map, l.x1, l.y1));
                }
                if (l.kind == LegKind::Lattice) s.lattice += l.lattice_length;
                if (l.kind == LegKind::Hop) ++s.hops;
            }
            std::sort(sectors.begin(), sectors.end());
            sectors.erase(std::unique(sectors.begin(), sectors.end()), sectors.end());
        }
    }
    d.osmosis_pairs = osmosis::pairs(w);
    d.region_planets.assign(w.map.regions.size(), {});
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) d.region_planets[at(w.planets[at(p)].region)].push_back(p);
}

World new_world(const Config& config, std::uint64_t seed) {
    validate(config);
    World world;
    world.config = config;
    world.seed = seed;
    world.rng = RngStreams::from_seed(seed);

    gen::generate(world);
    world.risk.assign(world.map.sectors.size(), {});
    economy::init_state(world);
    rebuild_derived(world);
    chars::populate(world);

    world.events.push({.time = 0, .phase = Phase::Cadence, .kind = EventKind::DailyTick, .period = kDay});
    economy::schedule(world);
    const Time step = config.time.space_step_minutes * kMinute;
    world.events.push({.time = step, .phase = Phase::Kinematic, .kind = EventKind::KinematicStep, .period = step});
    info::schedule_couriers(world);
    governor::schedule(world);

    world.ping_fires.assign(static_cast<std::size_t>(config.debug.pings), 0);
    for (std::int64_t i = 0; i < config.debug.pings; ++i) {
        world.events.push({.time = world.rng[Stream::Debug].uniform_int(0, kPingMax - 1),
                           .kind = EventKind::DebugPing,
                           .record = i});
    }
    return world;
}

void advance(World& world, Time dt, Observer* observer) {
    assert(dt >= 0);
    const Time end = world.now + dt;
    while (!world.events.empty() && world.events.top().time < end) {
        Event e = world.events.pop();
        assert(e.time >= world.now);
        world.now = e.time;
        dispatch(world, e, observer);
        ++world.events_processed;
        if (e.period > 0) {
            e.time += e.period;
            world.events.push(e);
        }
    }
    world.now = end;
}

}  // namespace lattice
