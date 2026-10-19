#include <doctest.h>

#include <algorithm>
#include <set>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/missions.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "helpers.hpp"
#include "info/facts.hpp"
#include "lattice/market.hpp"
#include "lattice/save.hpp"
#include "politics/fealty.hpp"
#include "trade/merchants.hpp"

using namespace lattice;

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

// Governors only, no random courier missions: characters are added by hand.
Config quiet() {
    Config c;
    c.chars.target = 0;
    c.courier.mission_share = 0;
    return c;
}

// A world and two planets in different groups joined by the Lattice.
struct Lab {
    World w;
    Id a = kNone, b = kNone;
};
Lab lab(std::uint64_t seed = 21) {
    Lab l{new_world(quiet(), seed)};
    const Map& m = l.w.map;
    for (const Group& g : m.groups) {
        if (g.node == kNone) continue;
        if (l.a == kNone) {
            l.a = g.holder;
        } else if (l.b == kNone && l.w.derived.route(l.a, g.holder).lattice > 1) {
            l.b = g.holder;
        }
    }
    REQUIRE(l.b != kNone);
    return l;
}

Id add_trader(World& w, Id planet, ShipClass cls, int tier, double wealth) {
    const Id id = chars::spawn(w, planet);
    Character& c = w.characters[at(id)];
    c.ships = {ships::variant(w.map, c.culture, cls, tier)};
    c.wealth = wealth;
    return id;
}

void set_fill(World& w, Id planet, Good g, double fill) {
    Planet& p = w.planets[at(planet)];
    p.stock[g] = fill * market::reserve(w.config, p, g);
}

// Advances in hours until done() or the limit.
template <class F>
bool run_until(World& w, F done, Time limit) {
    const Time end = w.now + limit;
    while (w.now < end) {
        advance(w, kHour);
        if (done()) return true;
    }
    return false;
}

}  // namespace

TEST_CASE("every culture has every ship class at three tiers (MOD-1, CHR-12)") {
    const World w = new_world(quiet(), 2);
    CHECK(w.map.variants.size() == w.map.regions.size() * kShipClassCount * kTierCount);
    for (Id culture = 0; culture < static_cast<Id>(w.map.regions.size()); ++culture) {
        for (int k = 0; k < kShipClassCount; ++k) {
            for (int tier = 1; tier <= kTierCount; ++tier) {
                const ShipVariant& v = w.map.variants[at(ships::variant(w.map, culture, static_cast<ShipClass>(k), tier))];
                CHECK(v.culture == culture);
                CHECK(static_cast<int>(v.cls) == k);
                CHECK(v.tier == tier);
                if (tier > 1) {
                    const ShipVariant& below = w.map.variants[at(ships::variant(w.map, culture, static_cast<ShipClass>(k), tier - 1))];
                    CHECK(v.price > below.price);
                    CHECK(v.cargo > below.cargo);
                }
            }
        }
        // Freighters carry, gunships fight (CHR-3).
        const auto& freighter = w.map.variants[at(ships::variant(w.map, culture, ShipClass::Freighter, 1))];
        const auto& gunship = w.map.variants[at(ships::variant(w.map, culture, ShipClass::Gunship, 1))];
        CHECK(freighter.cargo > gunship.cargo);
        CHECK(gunship.combat > freighter.combat);
    }
}

TEST_CASE("fealty trees grow from the seats (GEN-8, FAC-2)") {
    for (std::uint64_t seed = 1; seed <= 10; ++seed) {
        CAPTURE(seed);
        const World w = new_world(quiet(), seed);
        std::set<Id> roots;
        for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) roots.insert(fealty::root_of(w, p));
        for (Id s : w.map.seats) {
            CHECK(w.planets[at(s)].liege == kNone);
            CHECK(roots.contains(s));
        }
        for (Id r : roots) CHECK(w.planets[at(r)].liege == kNone);
        // Each group's planets are sworn to their holder, unless independent.
        for (const Group& g : w.map.groups) {
            for (Id p : g.planets) {
                if (p != g.holder && w.planets[at(p)].liege != kNone) CHECK(w.planets[at(p)].liege == g.holder);
            }
        }
        // The seats' factions hold most planets, and trees have depth.
        std::size_t in_seat_trees = 0;
        int deepest = 0;
        for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
            const Id r = fealty::root_of(w, p);
            in_seat_trees += std::find(w.map.seats.begin(), w.map.seats.end(), r) != w.map.seats.end();
            int depth = 0;
            for (Id q = p; w.planets[at(q)].liege != kNone; q = w.planets[at(q)].liege) ++depth;
            deepest = std::max(deepest, depth);
        }
        CHECK(in_seat_trees >= w.planets.size() * 3 / 4);
        CHECK(deepest >= 2);
    }
}

TEST_CASE("the starting world has a governor per planet and fills char.target (GEN-9, FAC-1)") {
    const World w = new_world(test::small(), 4);
    CHECK(static_cast<std::int64_t>(w.characters.size()) == w.config.chars.target);
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        const Planet& planet = w.planets[at(p)];
        REQUIRE(planet.governor != kNone);
        const Character& g = w.characters[at(planet.governor)];
        CHECK(g.governs == p);
        CHECK(g.planet == p);
        CHECK(g.ships.empty());
        // PRD-3: the growth factor follows the governor's risk tolerance.
        const auto& pop = w.config.pop;
        CHECK(planet.growth_factor == doctest::Approx(pop.growth_factor_min + g.risk * (pop.growth_factor_max - pop.growth_factor_min)));
    }
    for (const Character& c : w.characters) {
        if (c.governs != kNone) continue;
        CHECK(c.ships.size() == 1);  // a first-tier hull (CHR-6)
        CHECK(w.map.variants[at(c.ships[0])].tier == 1);
        CHECK(c.wealth == w.config.chars.start_wealth);
        CHECK(info::find(c.facts, c.planet) != nullptr);  // it knows where it starts
    }
}

TEST_CASE("sharing keeps the newest of each fact (INF-4)") {
    World w = new_world(quiet(), 5);
    Facts a, b;
    info::observe_market(w, a, 0);
    info::observe_market(w, a, 2);
    advance(w, kDay);
    info::observe_market(w, b, 2);
    info::observe_market(w, b, 3);
    info::share(a, b);
    REQUIRE(a.prices.size() == 3);
    CHECK(a.prices[0].planet == 0);
    CHECK(a.prices[1].planet == 2);
    CHECK(a.prices[1].time == kDay);
    CHECK(a.prices[2].planet == 3);
    CHECK(b.prices.size() == 3);
    CHECK(info::find(b, 0)->time == 0);
}

TEST_CASE("a character travels the route, pays Lattice fuel and lands (TIM-4, TIM-5, SYS-21)") {
    Lab l = lab();
    const Id id = add_trader(l.w, l.a, ShipClass::Courier, 1, 10000);
    Character& c0 = l.w.characters[at(id)];
    c0.activity = Activity::Exploring;  // so its first decision doesn't start a trade
    const double days = move::trip_days(l.w, c0, l.a, l.b);
    const double fuel = move::fuel_cost(l.w, c0, l.a, l.b);
    CHECK(fuel > 0);
    const Time start = l.w.now;
    REQUIRE(move::travel_to(l.w, id, l.b));
    CHECK(l.w.characters[at(id)].planet == kNone);
    bool saw_lattice = false;
    for (const Leg& leg : l.w.characters[at(id)].route) saw_lattice |= leg.kind == LegKind::Lattice;
    CHECK(saw_lattice);
    // last_planet changes only on landing; the character may set off again at once.
    REQUIRE(run_until(l.w, [&] { return l.w.characters[at(id)].last_planet == l.b; }, 30 * kDay));
    const double took = static_cast<double>(l.w.now - start) / kDay;
    CHECK(took >= days - 1.0 / 24);
    CHECK(took <= days + 2.0 / 24);
    CHECK(l.w.ledger.fuel == doctest::Approx(fuel));
    CHECK(info::find(l.w.characters[at(id)].facts, l.b) != nullptr);  // landing observes (INF-3)
}

TEST_CASE("buying walks the price up, selling walks it down (SYS-3)") {
    World w = new_world(quiet(), 6);
    Planet& p = w.planets[0];
    const Config& c = w.config;
    set_fill(w, 0, kFood, 1);
    const double before = market::price(c, p, kFood), treasury = p.treasury;
    const double big = 0.5 * market::reserve(c, p, kFood);
    const market::Fill bought = market::buy(p, c, kFood, big, 1e12, 1e12);
    CHECK(bought.qty == doctest::Approx(big));
    CHECK(market::price(c, p, kFood) > before);
    CHECK(bought.money > big * before);  // later chunks cost more
    CHECK(p.treasury - treasury == doctest::Approx(bought.money));
    const market::Fill sold = market::sell(p, c, kFood, big, 0);
    CHECK(sold.qty == doctest::Approx(big));
    CHECK(sold.money < bought.money);  // spread and tax both ways
    // A buyer's limit stops the walk.
    const market::Fill capped = market::buy(p, c, kFood, big, 1e12, market::ask(c, p, kFood) * (1 + p.tax_rate) * 1.05);
    CHECK(capped.qty < big);
    // A planet past its credit limit stops buying (SYS-3).
    p.treasury = -1e12;
    CHECK(market::sell(p, c, kFood, 10, 0).qty == 0);
}

TEST_CASE("an independent merchant finds and runs a profitable trip (SYS-7, TRD-4)") {
    Lab l = lab();
    World& w = l.w;
    for (int g = 0; g < kLawfulGoodCount; ++g) {
        set_fill(w, l.a, static_cast<Good>(g), 1);
        set_fill(w, l.b, static_cast<Good>(g), 1);
    }
    set_fill(w, l.a, kOxygen, 3);    // cheap here
    set_fill(w, l.b, kOxygen, 0.2);  // dear there
    const Id id = add_trader(w, l.a, ShipClass::Freighter, 1, 100000);
    Character& c = w.characters[at(id)];
    info::observe_market(w, c.facts, l.b);

    const merchant::Plan plan = merchant::best_trip(w, c, kNone);
    REQUIRE(plan.stops.size() == 2);
    CHECK(plan.stops[0].planet == l.a);
    CHECK(plan.stops[0].buy == kOxygen);
    CHECK(plan.stops[1].planet == l.b);
    CHECK(plan.value > 0);

    const double oxygen_b = w.planets[at(l.b)].stock[kOxygen];
    merchant::start(w, id, plan, nullptr);
    CHECK(w.characters[at(id)].cargo_good == kOxygen);
    CHECK(w.characters[at(id)].cargo_qty > 0);
    CHECK(w.characters[at(id)].planet == kNone);
    // It may set off on its next trip within the hour, so look at the ledger and B.
    REQUIRE(run_until(w, [&] { return w.ledger.trips >= 1; }, 30 * kDay));
    CHECK(w.ledger.trade_profit > 0);
    CHECK(w.planets[at(l.b)].stock[kOxygen] > oxygen_b);
}

TEST_CASE("a faction merchant's trips start and end in its faction (TRD-3)") {
    World w = new_world(quiet(), 23);
    const Id root = w.map.seats[0];
    const Id id = add_trader(w, root, ShipClass::Freighter, 1, 100000);
    Character& c = w.characters[at(id)];
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) info::observe_market(w, c.facts, p);
    // Make trade pay somewhere: a scarce good at every other planet.
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) set_fill(w, p, kWater, p == root ? 3 : 0.3);
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) info::observe_market(w, c.facts, p);
    const merchant::Plan plan = merchant::best_trip(w, c, root);
    REQUIRE(plan.stops.size() >= 2);
    CHECK(fealty::root_of(w, plan.stops.front().planet) == root);
    CHECK(fealty::root_of(w, plan.stops.back().planet) == root);
    CHECK(plan.stops.size() <= 4);  // inside, out and back, or triangle
}

TEST_CASE("a courier mission carries facts both ways and pays on return (SYS-8, INF-8)") {
    World w = new_world(quiet(), 24);
    Id vassal = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (w.planets[at(p)].liege != kNone && w.derived.travel(w.planets[at(p)].liege, p) > 0.5) {
            vassal = p;
            break;
        }
    }
    REQUIRE(vassal != kNone);
    const Id liege = w.planets[at(vassal)].liege;
    missions::post_courier(w, liege, vassal, nullptr);
    const Id mid = static_cast<Id>(w.missions.size() - 1);
    CHECK(w.planets[at(liege)].missions.back() == mid);

    // Unfilled pay grows (SYS-8).
    const double base = missions::pay_now(w, w.missions[at(mid)]);
    advance(w, 2 * kDay);
    CHECK(missions::pay_now(w, w.missions[at(mid)]) == doctest::Approx(base * (1 + 2 * w.config.mission.pay_growth)));

    const Id id = add_trader(w, liege, ShipClass::Courier, 1, 5000);
    CHECK(missions::best_offer(w, id).mission == mid);
    const double wealth = w.characters[at(id)].wealth;
    missions::take(w, id, mid, nullptr);
    const double pay = w.missions[at(mid)].pay;
    CHECK(w.missions[at(mid)].state == MissionState::Taken);
    CHECK(contracts::held(w, id, ContractKind::Courier) != kNone);
    CHECK(w.planets[at(liege)].missions.empty());
    REQUIRE(run_until(w, [&] { return w.missions[at(mid)].state == MissionState::Done; }, 60 * kDay));
    const Character& c = w.characters[at(id)];
    CHECK(c.skills[static_cast<std::size_t>(MissionKind::Courier)] == w.config.skill.gain);
    CHECK(c.missions_done == 1);
    CHECK(contracts::held(w, id, ContractKind::Courier) == kNone);  // the contract ends with the job
    // The vassal's governor learnt the liege's market, and the liege the vassal's.
    CHECK(info::find(w.characters[at(w.planets[at(vassal)].governor)].facts, liege) != nullptr);
    CHECK(info::find(w.characters[at(w.planets[at(liege)].governor)].facts, vassal) != nullptr);
    CHECK(pay > 0);
    CHECK(c.wealth > wealth + pay - 2000);  // paid, less running costs and fuel
}

TEST_CASE("an untaken courier mission expires and the run goes automated (INF-8)") {
    World w = new_world(quiet(), 24);
    Id vassal = kNone;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (w.planets[at(p)].liege != kNone) vassal = p;
    }
    missions::post_courier(w, w.planets[at(vassal)].liege, vassal, nullptr);
    const Id mid = static_cast<Id>(w.missions.size() - 1);
    const auto runs = w.courier_runs.size();
    advance(w, w.config.courier.interval_days * kDay + 1);
    CHECK(w.missions[at(mid)].state == MissionState::Expired);
    CHECK(w.courier_runs.size() > runs);
}

TEST_CASE("automated couriers bring a vassal its liege's facts (SYS-15)") {
    World w = new_world(quiet(), 25);
    advance(w, 3 * w.config.courier.interval_days * kDay);
    std::size_t knows = 0, sworn = 0;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        const Id liege = w.planets[at(p)].liege;
        if (liege == kNone || w.planets[at(liege)].system == w.planets[at(p)].system) continue;
        ++sworn;
        knows += info::find(w.characters[at(w.planets[at(p)].governor)].facts, liege) != nullptr;
    }
    CHECK(sworn > 0);
    CHECK(knows == sworn);
    CHECK(w.ledger.courier_runs > 0);
}

TEST_CASE("roots pay merchant salaries monthly; members share on landing (SYS-9, INF-4)") {
    Config cfg = quiet();
    cfg.merchant.salary = 1e6;  // an offer nobody refuses
    World w = new_world(cfg, 26);
    const Id root = w.map.seats[0];
    const Id id = add_trader(w, root, ShipClass::Freighter, 1, 10000);
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) info::observe_market(w, w.characters[at(id)].facts, p);
    merchant::consider_salary(w, id, nullptr);
    REQUIRE(merchant::employer_root(w, id) == root);
    CHECK(chars::faction_of(w, w.characters[at(id)]) == root);
    const double before = w.characters[at(id)].wealth;
    advance(w, w.config.time.payment_days * kDay);
    // Rich now, it may have bought ships too.
    CHECK(w.characters[at(id)].wealth + w.ledger.ships_bought > before + 0.9e6);
}

TEST_CASE("members meeting in open space share facts (SYS-15, TIM-6)") {
    Lab l = lab(27);
    World& w = l.w;
    const Id root = fealty::root_of(w, l.a);
    const Id gov = w.planets[at(root)].governor;
    const Id x = add_trader(w, l.a, ShipClass::Courier, 1, 10000);
    const Id y = add_trader(w, l.a, ShipClass::Courier, 1, 10000);
    contracts::sign(w, ContractKind::MerchantSalary, gov, x, 0, 30 * kDay);
    contracts::sign(w, ContractKind::MerchantSalary, gov, y, 0, 30 * kDay);
    w.characters[at(x)].activity = w.characters[at(y)].activity = Activity::Exploring;
    // Each knows something the other doesn't.
    info::observe_market(w, w.characters[at(x)].facts, l.b);
    const Id other = l.b == 0 ? 1 : 0;
    info::observe_market(w, w.characters[at(y)].facts, other);
    REQUIRE(move::travel_to(w, x, l.b));
    REQUIRE(move::travel_to(w, y, l.b));
    REQUIRE(w.characters[at(x)].route[0].kind == LegKind::Space);
    advance(w, w.config.time.space_step_minutes * kMinute + 1);
    CHECK(w.ledger.meetings_shared == 1);
    CHECK(info::find(w.characters[at(x)].facts, other) != nullptr);
    CHECK(info::find(w.characters[at(y)].facts, l.b) != nullptr);
}

TEST_CASE("savings buy the ship that pays (SYS-8)") {
    World w = new_world(quiet(), 28);
    const Id id = add_trader(w, 0, ShipClass::Courier, 1, 1e6);
    Character& c = w.characters[at(id)];
    c.earned_cargo = 1000;
    advance(w, kHour);
    const auto& ships = w.characters[at(id)].ships;
    REQUIRE(ships.size() == 2);
    CHECK(w.map.variants[at(ships[1])].cls == ShipClass::Freighter);
    CHECK(w.ledger.ships_bought > 0);
}

TEST_CASE("a broke character sells ships until solvent, keeping one (PIR-4)") {
    World w = new_world(quiet(), 29);
    const Id id = add_trader(w, 0, ShipClass::Freighter, 2, -1000);
    Character& c = w.characters[at(id)];
    c.ships.push_back(ships::variant(w.map, c.culture, ShipClass::Courier, 1));
    c.activity = Activity::Mission;  // keep it still
    advance(w, (w.config.chars.broke_days + 2) * kDay);
    CHECK(w.characters[at(id)].ships.size() == 1);
    CHECK(w.characters[at(id)].wealth > 0);
}

TEST_CASE("the population is topped up to char.target (SYS-18)") {
    Config cfg = test::small();
    World w = new_world(cfg, 30);
    int killed = 0;
    for (Character& c : w.characters) {
        if (c.governs == kNone && killed < 10) c.alive = false, ++killed;
    }
    advance(w, kDay);
    std::int64_t alive = 0;
    for (const Character& c : w.characters) alive += c.alive;
    CHECK(alive == cfg.chars.target - 10 + cfg.chars.spawn_per_day);
    advance(w, 5 * kDay);
    alive = 0;
    for (const Character& c : w.characters) alive += c.alive;
    CHECK(alive == cfg.chars.target);
}

TEST_CASE("only a character's latest wake decides") {
    World w = new_world(quiet(), 31);
    const Id id = add_trader(w, 0, ShipClass::Courier, 1, 100);
    chars::wake(w, id, w.now + kDay);
    chars::wake(w, id, w.now + 2 * kDay);
    int pending = 0, current = 0;
    for (const Event& e : w.events.raw()) {
        if (e.kind != EventKind::CharDecide || e.record != id) continue;
        ++pending;
        current += static_cast<std::uint32_t>(e.a) == w.characters[at(id)].wake_seq;
    }
    CHECK(pending == 3);  // spawning woke it too
    CHECK(current == 1);
}

TEST_CASE("characters keep the world deterministic and saveable") {
    World a = new_world(test::small(), 32);
    advance(a, 40 * kDay + 5 * kHour);
    World b = load_world(save_world(a));
    advance(a, 40 * kDay);
    advance(b, 40 * kDay);
    CHECK(state_hash(a) == state_hash(b));
    CHECK(a.ledger.trips > 0);
}
