#include <doctest.h>

#include "chars/characters.hpp"
#include "chars/missions.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "lattice/market.hpp"
#include "trade/merchants.hpp"

using namespace lattice;

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

Config quiet() {
    Config c;
    c.chars.target = 0;
    c.chars.spawn_per_day = 0;
    c.courier.mission_share = 0;
    c.dip.declare_threshold = 1e9;
    c.gen.pirate_planets = 0;
    c.value.invade_months = 0;
    c.raid.share = 0;
    c.rebel.scale = 0;
    c.passenger.per_planet = 0;
    return c;
}

// A lawful planet short of food, that has heard every market, with a surplus of food at
// another planet.
Id short_planet(World& w) {
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        Planet& pl = w.planets[at(p)];
        if (pl.pirate || pl.governor == kNone) continue;
        const Id q = p == 0 ? 1 : 0;
        w.planets[at(q)].stock[kFood] = 3 * market::reserve(w.config, w.planets[at(q)], kFood);
        for (Id r = 0; r < static_cast<Id>(w.planets.size()); ++r) info::observe_market(w, info::planet_facts(w, p), r);
        pl.stock[kFood] = 0.1 * market::reserve(w.config, pl, kFood);
        pl.treasury = 1e7;
        return p;
    }
    return kNone;
}

Id open_mission(const World& w, Id planet, MissionKind kind, Good g) {
    for (Id m : w.planets[at(planet)].missions) {
        const Mission& mi = w.missions[at(m)];
        if (mi.kind == kind && mi.good == g && mi.state == MissionState::Open) return m;
    }
    return kNone;
}

}  // namespace

TEST_CASE("a short planet orders from the nearest surplus it has heard of, and the order arrives untaken (MSN-15)") {
    World w = new_world(quiet(), 41);
    const Id p = short_planet(w);
    REQUIRE(p != kNone);
    missions::post_orders(w, p, nullptr);
    const Id order = open_mission(w, p, MissionKind::Order, kFood);
    REQUIRE(order != kNone);
    const Mission o = w.missions[at(order)];
    const Planet& pl = w.planets[at(p)];
    CHECK(o.qty == doctest::Approx(market::reserve(w.config, pl, kFood) - pl.stock[kFood]));
    CHECK(pl.on_order[kFood] == doctest::Approx(o.qty));
    CHECK(o.unit == doctest::Approx(w.config.order.price_share * market::price(w.config, pl, kFood)));
    // The source: a surplus as the planet heard it, the nearest such.
    const PriceFact* f = info::find(info::planet_facts(w, p), o.target);
    REQUIRE(f != nullptr);
    CHECK(f->price[kFood] < w.config.goods[kFood].base_price);
    // What's on order counts: no second order for the same shortfall.
    missions::post_orders(w, p, nullptr);
    CHECK(w.ledger.orders == static_cast<std::int64_t>(1 + (open_mission(w, p, MissionKind::Order, kWater) != kNone) +
                                                       (open_mission(w, p, MissionKind::Order, kOxygen) != kNone)));
    // No one carries it: it arrives anyway, and the producer posts the delivery.
    advance(w, o.expires - w.now + kHour);
    CHECK(w.missions[at(order)].state == MissionState::Expired);
    CHECK(w.ledger.orders_auto >= 1);
    const Id sup = open_mission(w, o.target, MissionKind::Supply, kFood);
    REQUIRE(sup != kNone);
    CHECK(w.missions[at(sup)].target == p);
    CHECK(w.missions[at(sup)].unit == doctest::Approx(o.unit));
}

TEST_CASE("a courier without a Lattice Drive carries an order and the issuer pays it (MSN-15)") {
    World w = new_world(quiet(), 42);
    const Id p = short_planet(w);
    REQUIRE(p != kNone);
    missions::post_orders(w, p, nullptr);
    const Id order = open_mission(w, p, MissionKind::Order, kFood);
    REQUIRE(order != kNone);
    const Id id = chars::spawn(w, p);
    Character& ch = w.characters[at(id)];
    ch.wealth = 0;
    ch.ships = {ships::variant(w.map, ch.culture, ShipClass::Courier, 1, false)};
    const bool open = w.derived.travel(p, w.missions[at(order)].target, false) < std::numeric_limits<double>::max();
    CHECK(missions::qualifies(w, id, w.missions[at(order)]) == open);
    if (!open) return;
    const double treasury = w.planets[at(p)].treasury;
    missions::take(w, id, order, nullptr);
    const Id target = w.missions[at(order)].target;
    advance(w, static_cast<Time>((w.derived.travel(p, target, false) + 1) * kDay));
    CHECK(w.missions[at(order)].state == MissionState::Done);
    CHECK(w.ledger.orders_carried == 1);
    CHECK(w.characters[at(id)].wealth > 0);
    CHECK(w.planets[at(p)].treasury < treasury);
    CHECK(open_mission(w, target, MissionKind::Supply, kFood) != kNone);
}

TEST_CASE("a merchant at the producer weighs the delivery as a trip and sells at the order's price (MSN-15)") {
    World w = new_world(quiet(), 43);
    const Id p = short_planet(w);
    REQUIRE(p != kNone);
    missions::post_orders(w, p, nullptr);
    const Id order = open_mission(w, p, MissionKind::Order, kFood);
    REQUIRE(order != kNone);
    const Id producer = w.missions[at(order)].target;
    advance(w, w.missions[at(order)].expires - w.now + kHour);
    const Id sup = open_mission(w, producer, MissionKind::Supply, kFood);
    REQUIRE(sup != kNone);
    const double ordered = w.missions[at(sup)].qty;
    const double unit = w.missions[at(sup)].unit;
    const Id id = chars::spawn(w, producer);
    Character& ch = w.characters[at(id)];
    ch.ships = {ships::variant(w.map, ch.culture, ShipClass::Freighter, 1)};
    ch.wealth = 1e6;
    for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) info::observe_market(w, ch.facts, q);
    const merchant::Plan plan = merchant::best_trip(w, ch, kNone);
    REQUIRE(plan.supply == sup);
    const double hold = ships::fleet(w, ch).cargo;
    merchant::start(w, id, plan, nullptr);
    // Its hold takes part of the order; the rest stays posted.
    if (hold < ordered) {
        CHECK(w.missions[at(sup)].qty <= hold + 1e-6);
        const Id rest = open_mission(w, producer, MissionKind::Supply, kFood);
        REQUIRE(rest != kNone);
        CHECK(w.missions[at(rest)].qty == doctest::Approx(ordered - w.missions[at(sup)].qty));
    }
    const double carried = w.characters[at(id)].cargo_qty;
    CHECK(carried > 0);
    const double on_order = w.planets[at(p)].on_order[kFood];
    const double stock = w.planets[at(p)].stock[kFood];
    advance(w, static_cast<Time>((w.derived.travel(producer, p) + 0.5) * kDay));
    CHECK(w.missions[at(sup)].state == MissionState::Done);
    CHECK(w.ledger.supplies == 1);
    CHECK(w.ledger.supply_paid == doctest::Approx(carried * unit));
    CHECK(w.planets[at(p)].on_order[kFood] == doctest::Approx(on_order - carried));
    CHECK(w.planets[at(p)].stock[kFood] > stock);
}
