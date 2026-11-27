#include "chars/characters.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string_view>

#include "chars/contracts.hpp"
#include "chars/missions.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "lattice/market.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"
#include "politics/roles.hpp"
#include "trade/merchants.hpp"
#include "pirates/pirates.hpp"
#include "pirates/smuggling.hpp"
#include "pirates/suspicion.hpp"
#include "war/armies.hpp"
#include "war/spies.hpp"
#include "war/raids.hpp"
#include "war/diplomacy.hpp"
#include "war/threats.hpp"

namespace lattice::chars {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

std::string make_name(Rng& rng) {
    static constexpr std::array<std::string_view, 24> first = {
        "Ka", "Ve", "Or", "Ta", "Mi", "Sel", "Dra", "Ny", "Bo", "Ar", "Li", "Que",
        "Zan", "Ro", "Ish", "Fen", "Ula", "Cor", "Ei", "Ham", "Jo", "Pel", "Sa", "Tor"};
    static constexpr std::array<std::string_view, 24> second = {
        "ran", "lis", "dek", "mar", "sin", "vo", "tha", "rel", "kan", "dus", "mi", "lor",
        "ven", "ra", "sk", "bel", "ion", "ta", "gar", "nis", "por", "al", "ek", "um"};
    std::string name;
    name += first[static_cast<std::size_t>(rng.uniform_int(0, first.size() - 1))];
    name += second[static_cast<std::size_t>(rng.uniform_int(0, second.size() - 1))];
    name += ' ';
    name += first[static_cast<std::size_t>(rng.uniform_int(0, first.size() - 1))];
    name += second[static_cast<std::size_t>(rng.uniform_int(0, second.size() - 1))];
    return name;
}

Id add(World& w, Id planet) {
    Rng& rng = w.rng[Stream::Characters];
    Character c;
    c.name = make_name(rng);
    c.culture = w.planets[at(planet)].region;
    c.risk = rng.uniform01();  // char.risk_dist: uniform 0-1
    c.planet = planet;
    c.last_planet = planet;
    c.born = w.now;
    c.idle_since = w.now;
    info::observe_market(w, c.facts, planet);
    w.characters.push_back(std::move(c));
    return static_cast<Id>(w.characters.size() - 1);
}

// Its best standing (REL-5) with a faction that could vouch for it here: the faction of
// the planet it's at, or its own.
double best_standing(const World& w, Id id) {
    const Character& c = w.characters[at(id)];
    const Id here = c.planet != kNone ? fealty::root_of(w, c.planet) : kNone;
    return std::max(here >= 0 ? opinion::standing(w, here, id) : -100.0, opinion::standing(w, faction_of(w, c), id));
}

// The free hull a ruined character starts over in (CHR-16) follows who it was, all tier 1
// and uninsured: a pirate's scout (PIR-16); a courier with a Lattice Drive for one a
// faction trusts; a small freighter without one for a merchant with a record; else a
// shuttle, a courier without a drive (TRD-18). Standing wins over the record.
Id bottom_hull(const World& w, Id id) {
    const Character& c = w.characters[at(id)];
    const auto& s = w.config.ship;
    const Id culture = c.planet != kNone ? w.planets[at(c.planet)].region : c.culture;
    // A hull without a drive only where open space gets out (TRD-18): a free hull can
    // always leave the planet it's given at, for a pirate planet if it's a pirate's.
    bool open = c.planet == kNone;
    for (Id q = 0; q < static_cast<Id>(w.planets.size()) && !open; ++q) {
        if (q == c.planet || w.planets[at(q)].pirate != c.pirate) continue;
        const double days = w.derived.travel(c.planet, q, false);
        open = c.pirate ? days < std::numeric_limits<double>::max() : days <= w.config.passenger.max_days;  // max(): no route
    }
    if (c.pirate) return ships::variant(w.map, culture, ShipClass::Scout, 1, !open);
    if (best_standing(w, id) >= s.trusted_opinion) return ships::variant(w.map, culture, ShipClass::Courier, 1, true);
    if (c.career_cargo >= s.merchant_record) return ships::variant(w.map, culture, ShipClass::Freighter, 1, !open);
    return ships::variant(w.map, culture, ShipClass::Courier, 1, !open);
}

// A free hull given, counted by its rung on the ladder.
void count_free(World& w, Id variant) {
    const ShipVariant& v = w.map.variants[at(variant)];
    const std::size_t k = v.cls == ShipClass::Scout ? 0 : v.lattice_drive ? 1 : v.cls == ShipClass::Freighter ? 2 : 3;
    ++w.ledger.free_hulls;
    ++w.ledger.free_by_hull[k];
}

// Ships and munitions are parts (MIL-4, ECO-6, money round): worth so many at the parts'
// base price, bought from the planet's stock at its ask and tax and paid to its treasury.
// What the stock lacks is rushed in at price.max of the base price, a sink.
double parts_in(const Config& cfg, double value) { return value / cfg.goods[kParts].base_price; }

double rushed_unit(const Config& cfg) { return cfg.goods[kParts].base_price * cfg.price.max; }

double parts_quote(const World& w, Id planet, double qty) {
    constexpr double any = std::numeric_limits<double>::max();
    const market::Fill f = market::quote_buy(w.planets[at(planet)], w.config, kParts, qty, any, any);
    return f.money + std::max(0.0, qty - f.qty) * rushed_unit(w.config);
}

double buy_parts(World& w, Character& c, double qty) {
    constexpr double any = std::numeric_limits<double>::max();
    Planet& p = w.planets[at(c.planet)];
    p.parts_today += qty;  // the yard's demand, met or not, sizes its reserve
    const market::Fill f = market::buy(p, w.config, kParts, qty, any, any);
    const double rushed = std::max(0.0, qty - f.qty) * rushed_unit(w.config);
    w.ledger.consumed[kParts] += f.qty;  // built into the ship or fired
    c.wealth -= f.money + rushed;
    w.ledger.parts_paid += f.money;
    w.ledger.parts_rushed += rushed;
    w.ledger.money_sunk += rushed;
    return f.money + rushed;
}

// What a ship costs its buyer at the planet it's at: its parts, and the insurance
// premium on its price (CHR-21), a sink.
double ship_cost(const World& w, const Character& c, const ShipVariant& v) {
    return parts_quote(w, c.planet, parts_in(w.config, v.price)) + ships::buy_cost(w.config, v) - v.price;
}

void pay_for(World& w, Character& c, Id variant, double resale) {
    const ShipVariant& v = w.map.variants[at(variant)];
    const double premium = ships::buy_cost(w.config, v) - v.price;
    const double parts = buy_parts(w, c, parts_in(w.config, v.price));
    c.wealth -= premium - resale;
    w.ledger.ships_bought += parts + premium - resale;
    w.ledger.money_sunk += premium - resale;
    w.ledger.insurance_premiums += premium;
    ships::insure(w, c, variant);
}

// SYS-8: spend savings on a ship, following what has paid: more cargo or more combat; a
// governor's fleet is gunships (MIL-20). The next ship is the focus class (freighter or
// gunship): added while the fleet has room, otherwise replacing the cheapest ship one
// tier up. It's bought when wealth passes spend.threshold x its cost and leaves the
// cautious reserve untouched.
void maybe_buy_ship(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    // A planet under siege sells no ships (and gives no free hull) until it ends.
    if (c.planet == kNone || w.planets[at(c.planet)].siege != kNone) return;
    const auto& cfg = w.config;
    // A pirate's trade is raiding (PIR-16), so it buys gunships.
    const ShipClass cls = c.governs != kNone || c.pirate || c.earned_combat > c.earned_cargo ? ShipClass::Gunship : ShipClass::Freighter;
    const Id culture = w.planets[at(c.planet)].region;  // ships come from the planet's culture (MIL-4)
    const double threshold = cfg.spend.threshold_base - cfg.spend.threshold_risk * c.risk;
    if (c.ships.empty()) {
        // A lost fleet is rebuilt from the insurance and savings (CHR-16, CHR-21): the
        // highest tier of the work it did that it can afford with a margin, else any hull
        // of it, else the bottom rung. Any hull beats none.
        Id pick = kNone;
        const Id bottom = bottom_hull(w, id);  // the bottom rung (CHR-16)
        for (int tier = kTierCount; tier >= 1 && pick == kNone; --tier) {
            const Id v = ships::variant(w.map, culture, cls, tier);
            if (c.wealth >= threshold * ship_cost(w, c, w.map.variants[at(v)])) pick = v;
        }
        for (const Id v : {ships::variant(w.map, culture, cls, 1), bottom}) {
            if (pick == kNone && c.wealth >= ship_cost(w, c, w.map.variants[at(v)])) pick = v;
        }
        if (pick == kNone) {
            // Ruined: it starts over at the bottom (CHR-16), in the hull that follows who it
            // was. Nobody bought it, so it carries no insurance.
            c.ships.push_back(bottom);
            count_free(w, bottom);
            if (observer) observer->log(w.now, "ship.free", id, std::to_string(c.wealth));
            return;
        }
        pay_for(w, c, pick, 0);
        c.ships.push_back(pick);
        if (observer) {
            const ShipVariant& v = w.map.variants[at(pick)];
            observer->log(w.now, "ship.rebuilt", id, std::to_string(static_cast<int>(v.cls)) + " tier " + std::to_string(v.tier));
        }
        return;
    }
    int best_tier = 0;
    std::size_t cheapest = 0;
    std::size_t bare = c.ships.size();  // a hull without a Lattice Drive
    for (std::size_t i = 0; i < c.ships.size(); ++i) {
        const ShipVariant& v = w.map.variants[at(c.ships[i])];
        if (v.cls == cls) best_tier = std::max<int>(best_tier, v.tier);
        if (v.price < w.map.variants[at(c.ships[cheapest])].price) cheapest = i;
        if (!v.lattice_drive && bare == c.ships.size()) bare = i;
    }
    // A hull without a drive holds the whole fleet off the Lattice (TRD-18), so a lawful
    // buyer replaces it first, with the same hull with a drive: the first step back up.
    const bool swap_bare = bare < c.ships.size() && !c.pirate;
    if (swap_bare) cheapest = bare;
    const bool room = !swap_bare && static_cast<std::int64_t>(c.ships.size()) < cfg.fleet.max_ships;
    const int tier = room ? std::max(1, best_tier) : std::min(kTierCount, best_tier + 1);
    const ShipVariant& old_hull = w.map.variants[at(c.ships[cheapest])];
    const Id pick = swap_bare ? ships::variant(w.map, culture, old_hull.cls, old_hull.tier) : ships::variant(w.map, culture, cls, tier);
    const ShipVariant& v = w.map.variants[at(pick)];
    const double resale = room ? 0 : w.map.variants[at(c.ships[cheapest])].price * cfg.ship.resale;
    if (!room && v.price <= w.map.variants[at(c.ships[cheapest])].price) return;
    const double price = ship_cost(w, c, v);
    if (c.wealth < threshold * price || c.wealth - (price - resale) < reserve(w, c)) return;
    pay_for(w, c, pick, resale);
    if (room) {
        c.ships.push_back(pick);
    } else {
        ships::lapse(w, c, c.ships[cheapest]);
        c.ships[cheapest] = pick;
    }
    if (observer) observer->log(w.now, "ship.bought", id, std::to_string(static_cast<int>(cls)) + " tier " + std::to_string(tier));
}

void sell(World& w, Character& c, std::vector<Id>::iterator it) {
    const double value = w.map.variants[at(*it)].price * w.config.ship.resale;
    c.wealth += value;
    w.ledger.ships_bought -= value;
    w.ledger.money_sunk -= value;
    ships::lapse(w, c, *it);
    c.ships.erase(it);
}

// Not solvent for char.broke_days: sell ships, smallest first, until solvent (PIR-4's
// lawful branch). *(tuning pass part 4)* Still in debt with one hull left, it's bankrupt
// (CHR-16): the last hull is sold unless it's already the bottom rung's, the debt is
// written off (ECO-13), and it starts over on the bottom rung.
void go_broke(World& w, Id id) {
    Character& c = w.characters[at(id)];
    while (c.wealth < 0 && c.ships.size() > 1) {
        sell(w, c, std::min_element(c.ships.begin(), c.ships.end(), [&](Id a, Id b) {
                 return w.map.variants[at(a)].price < w.map.variants[at(b)].price;
             }));
    }
    if (c.wealth >= 0) return;
    const Id bottom = bottom_hull(w, id);
    // Swapped for the bottom rung's if that's cheaper, or if it can leave and this can't.
    const ShipVariant& last = w.map.variants[at(c.ships.empty() ? bottom : c.ships[0])];
    if (!c.ships.empty() && (last.price > w.map.variants[at(bottom)].price ||
                             (!last.lattice_drive && w.map.variants[at(bottom)].lattice_drive))) {
        sell(w, c, c.ships.begin());
        c.ships.push_back(bottom);
        count_free(w, bottom);
    }
    if (c.wealth < 0) {
        w.ledger.money_created -= c.wealth;
        w.ledger.debt_written_off -= c.wealth;
        c.wealth = 0;
    }
    c.broke_since = kNever;
    ++w.ledger.bankruptcies;
    if (c.bankruptcies++ == 0) ++w.ledger.bankrupt_chars;
}

// In debt past char.broke_days (PIR-4, CHR-16), at a planet.
bool broke_due(const World& w, const Character& c) {
    return c.wealth < 0 && c.broke_since != kNever && w.now - c.broke_since >= w.config.chars.broke_days * kDay &&
           c.activity != Activity::Renouncing && c.planet != kNone;
}

// PIR-4: a commander who knows a pirate planet may head there, weighed once a broke
// spell; else it sells ships. Either way the spell ends: go_broke leaves it solvent or
// bankrupt (CHR-16). True if it set off to turn pirate.
bool settle_broke(World& w, Id id) {
    if (pirates::try_turn(w, id, nullptr)) return true;
    go_broke(w, id);
    return false;
}

}  // namespace

Id spawn(World& w, Id planet) {
    const Id id = add(w, planet);
    // Born into a faction (REL-5): standing enough for its courier work, the way in (MSN-2).
    if (!w.planets[at(planet)].pirate) {
        const Id root = fealty::root_of(w, planet);
        if (root >= 0) opinion::seed(w, w.planets[at(root)].governor, id, w.config.chars.start_standing);
    }
    Character& c = w.characters[at(id)];
    c.wealth = w.config.chars.start_wealth;
    w.ledger.money_spawned += c.wealth;
    const bool armed = w.rng[Stream::Characters].chance(w.config.chars.start_armed_p);
    c.ships.push_back(ships::variant(w.map, c.culture, armed ? ShipClass::Scout : ShipClass::Courier, 1));
    wake(w, id, w.now);
    return id;
}

Id spawn_ruler(World& w, Id planet) {
    const Id id = add(w, planet);
    Character& c = w.characters[at(id)];
    c.wealth = w.config.chars.start_wealth;
    w.ledger.money_spawned += c.wealth;
    for (std::int64_t i = 0; i < w.config.gen.governor_ships; ++i) {
        c.ships.push_back(ships::variant(w.map, c.culture, ShipClass::Gunship, 1));  // MIL-20
        ships::insure(w, c, c.ships.back());  // seeded ships are insured (CHR-21)
    }
    return id;
}

void populate(World& w) {
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) roles::seat(w, p, spawn_ruler(w, p), nullptr);
    // Commanders sworn to each governor, with combat power in proportion to the planet's
    // income (GEN-9): one gunship each.
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        const Planet& planet = w.planets[at(p)];
        const double target = w.config.gen.commanders_per_income * planet.income_avg * static_cast<double>(w.config.time.payment_days);
        double power = 0;
        while (power < target && static_cast<std::int64_t>(w.characters.size()) < w.config.chars.target) {
            const Id id = spawn(w, p);
            Character& c = w.characters[at(id)];
            c.ships = {ships::variant(w.map, c.culture, ShipClass::Gunship, 1)};
            c.cover = 0;
            ships::insure(w, c, c.ships.back());  // CHR-21
            const ships::Fleet f = ships::fleet(w, c);
            power += f.combat;
            contracts::sign(w, ContractKind::Service, planet.governor, id, w.config.hire.stipend_per_power * f.combat,
                            w.config.time.payment_days * kDay);
        }
    }
    pirates::seed(w);  // the starting pirate planets and their crews (GEN-8, GEN-9)
    std::vector<double> weights;
    for (const Planet& p : w.planets) weights.push_back(p.pirate ? 0 : p.population);
    Rng& rng = w.rng[Stream::Characters];
    while (static_cast<std::int64_t>(w.characters.size()) < w.config.chars.target) {
        spawn(w, static_cast<Id>(rng.weighted(weights)));
    }
}

Id faction_of(const World& w, const Character& c) {
    if (c.pirate) return kPirates;
    if (c.governs != kNone) return fealty::root_of(w, c.governs);
    for (Id k : c.contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.period == 0) continue;  // one-job contracts don't make a member
        if (ct.kind == ContractKind::Spy) continue;  // nor does a secret one, to anyone looking (SYS-17)
        const Character& employer = w.characters[at(ct.employer)];
        if (&employer == &c || employer.governs == kNone) continue;
        return fealty::root_of(w, employer.governs);
    }
    return kNone;
}

double reserve(const World& w, const Character& c) {
    return w.config.spend.reserve_months * (1 - c.risk) * ships::fleet(w, c).run_cost * static_cast<double>(w.config.time.payment_days);
}

// MIL-4: the munitions a battle used are refilled at the first planet that isn't hostile,
// as parts from its stock, paid to it; what it lacks is rushed in (money round).
void refill(World& w, Id id) {
    Character& c = w.characters[at(id)];
    if (c.munitions <= 0 || c.planet == kNone) return;
    const Planet& p = w.planets[at(c.planet)];
    const Id faction = faction_of(w, c);
    if (p.pirate != c.pirate || (faction != kNone && !c.pirate && diplomacy::at_war(w, faction, fealty::root_of(w, c.planet)))) return;
    w.ledger.munitions += buy_parts(w, c, parts_in(w.config, c.munitions));
    c.munitions = 0;
}

void earn(Character& c, double amount, bool combat_work) {
    c.wealth += amount;
    (combat_work ? c.earned_combat : c.earned_cargo) += amount;
    if (!combat_work && amount > 0) c.career_cargo += amount;
}

void daily(World& w, Observer*) {
    const auto& cfg = w.config;
    const double decay = 1.0 / static_cast<double>(cfg.time.payment_days);
    std::int64_t alive = 0;
    for (Id id = 0; id < static_cast<Id>(w.characters.size()); ++id) {
        Character& c = w.characters[at(id)];
        if (!c.alive) continue;
        ++alive;
        // SYS-21; suspended on the way to turn pirate (PIR-4).
        const double cost = c.activity == Activity::Renouncing ? 0 : ships::fleet(w, c).run_cost;
        c.wealth -= cost;
        w.ledger.running_costs += cost;
        w.ledger.money_sunk += cost;
        c.earned_cargo -= c.earned_cargo * decay;
        c.earned_combat -= c.earned_combat * decay;
        if (c.wealth >= 0) {
            c.broke_since = kNever;
        } else if (c.broke_since == kNever) {
            c.broke_since = w.now;
        } else if (broke_due(w, c)) {
            settle_broke(w, id);  // also weighed at landing, so the always-moving don't slip it
        }
    }

    // SYS-18: top the population up, at lawful planets weighted by population.
    std::vector<double> weights;
    for (const Planet& p : w.planets) weights.push_back(p.pirate ? 0 : p.population);  // at lawful planets
    Rng& rng = w.rng[Stream::Characters];
    for (std::int64_t i = 0; i < cfg.chars.spawn_per_day && alive < cfg.chars.target; ++i, ++alive) {
        spawn(w, static_cast<Id>(rng.weighted(weights)));
    }
}

void wake(World& w, Id id, Time when) {
    Character& c = w.characters[at(id)];
    ++c.wake_seq;
    w.events.push({.time = std::max(when, w.now), .kind = EventKind::CharDecide, .record = id, .a = c.wake_seq});
}

namespace {

// Called to an army: go to the muster point and join (MIL-9).
void join_army(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    const Army& a = w.armies[at(c.army)];
    if (a.alive && a.action == ArmyAction::Muster && c.planet != a.home) {
        if (c.governs == c.planet) roles::handover(w, c.governs);  // the steward runs things meanwhile
        if (!move::travel_to(w, id, a.home)) armies::member_arrived(w, id, observer);
        return;
    }
    armies::member_arrived(w, id, observer);
}

// A governor stays at its planet unless it sails with an army (MIL-20).
void govern(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    if (c.army != kNone) {
        join_army(w, id, observer);
        return;
    }
    if (c.planet != c.governs) {
        c.activity = Activity::Idle;
        if (!move::travel_to(w, id, c.governs)) wake(w, id, w.now + kDay);
        return;
    }
    c.activity = Activity::Governing;
    roles::handover(w, c.governs);
    maybe_buy_ship(w, id, observer);  // a governor rebuilds its own fleet too (MIL-20, CHR-16)
}

}  // namespace

namespace {

// TRD-4: where a character with nothing to do goes to look: a lawful planet within
// merchant.wander_days of it by its own travel time, not at war with its faction,
// weighted by population.
Id wander_target(World& w, Id id) {
    const Character& c = w.characters[at(id)];
    const Id own = faction_of(w, c);
    std::vector<Id> near;
    std::vector<double> weights;
    for (Id q = 0; q < static_cast<Id>(w.planets.size()); ++q) {
        const Planet& p = w.planets[at(q)];
        if (q == c.planet || p.pirate || p.siege != kNone) continue;
        if (move::ref_days(w, c, c.planet, q) > w.config.merchant.wander_days) continue;
        if (own != kNone && diplomacy::at_war(w, own, fealty::root_of(w, q))) continue;
        near.push_back(q);
        weights.push_back(p.population);
    }
    if (near.empty()) return kNone;
    return near[w.rng[Stream::Characters].weighted(weights)];
}

}  // namespace

void decide(World& w, Id id, std::uint32_t seq, Observer* observer) {
    Character& c = w.characters[at(id)];
    if (seq != c.wake_seq || !c.alive || c.planet == kNone) return;
    refill(w, id);
    if (c.governs != kNone) {
        govern(w, id, observer);
        return;
    }
    if (c.stewards != kNone) {
        roles::steward_decide(w, id, observer);
        return;
    }
    if (c.activity == Activity::Trading) {
        merchant::at_stop(w, id, observer);
        return;
    }
    if (c.activity == Activity::Mission || c.activity == Activity::Escorting || c.activity == Activity::Patrolling ||
        c.activity == Activity::Raiding || c.activity == Activity::Renouncing) {
        return;  // arrivals drive these
    }
    if (contracts::held(w, id, ContractKind::Spy) != kNone) {
        spies::next(w, id, observer);
        return;
    }
    if (c.army != kNone) {
        join_army(w, id, observer);
        return;
    }
    // A sworn commander serves at its governor's planet (MIL-2): the governor sets the
    // objective, which in peacetime is the defence of home.
    const Id service = contracts::held(w, id, ContractKind::Service);
    if (service != kNone) {
        maybe_buy_ship(w, id, observer);
        const Id home = w.characters[at(w.contracts[at(service)].employer)].governs;
        if (c.planet != home && move::travel_to(w, id, home)) {
            c.activity = Activity::Idle;
            return;
        }
        c.activity = Activity::Serving;
        wake(w, id, w.now + 7 * kDay);
        return;
    }
    if (c.activity == Activity::Serving) c.activity = Activity::Idle;

    maybe_buy_ship(w, id, observer);
    if (c.ships.empty()) {
        // No hull, and none to be had here yet (a siege): it waits (CHR-16).
        c.activity = Activity::Idle;
        wake(w, id, w.now + kDay);
        return;
    }
    // A pirate run it agreed to on contact (PIR-13), once its hold is empty.
    if (c.offer != kNone) {
        const Mission& run = w.missions[at(c.offer)];
        if (run.state != MissionState::Open) {
            c.offer = kNone;
        } else if (smuggling::qualifies(w, id, run)) {
            smuggling::take(w, id, c.offer, observer);
            if (c.mission != kNone) return;
        }
    }
    merchant::consider_salary(w, id, observer);

    // Fit and pay (CHR-10): the best mission on this board against the best trade.
    const missions::Offer offer = missions::best_offer(w, id);
    const merchant::Plan plan = merchant::best_trip(w, c, merchant::employer_root(w, id));
    const double trade = plan.score;
    if (offer.mission != kNone && offer.score > 0 && (offer.score >= trade || c.pirate)) {
        missions::take(w, id, offer.mission, observer);
        return;
    }
    // A pirate's work is raiding (PIR-16): a mission if there is one, then its hold sold,
    // then out on its own until it's destroyed.
    if (c.pirate && !(c.cargo_qty > 0 && plan.value > 0)) {
        if (!pirates::freelance(w, id, observer)) wake(w, id, w.now + kDay);
        return;
    }
    if (plan.score >= w.config.merchant.min_score || (c.cargo_qty > 0 && plan.value > 0)) {
        merchant::start(w, id, plan, observer);
        return;
    }
    // Nothing worth doing here: look at another market (TRD-4), if it can afford to. The
    // bottom rung (one tier-1 hull) keeps no reserve, since idling costs it as much as
    // looking for work does (CHR-16), but it looks only with money to trade with, and
    // pays its fuel from it: broke, it waits for an errand (MSN-14), the paid way to look.
    const Id target = merchant::explore_target(w, c);
    const bool bottom_rung = c.ships.size() == 1 && w.map.variants[at(c.ships[0])].tier == 1;
    const double explore_fuel = target != kNone ? move::fuel_cost(w, c, c.planet, target) : 0;
    if (target != kNone && (bottom_rung ? c.wealth > 0 && explore_fuel <= c.wealth : c.wealth - explore_fuel > reserve(w, c)) &&
        move::travel_to(w, id, target)) {
        c.activity = Activity::Exploring;
        return;
    }
    // MSN-14: the lawful bottom rung never waits long for work. After a random
    // passenger.wait_min_days to wait_max_days with nothing, its planet posts it an errand:
    // a paid passenger run to a nearby planet. It moves on paid, so it doesn't wander.
    if (bottom_rung && !c.pirate) {
        const auto& pc = w.config.passenger;
        if (c.errand_wait != c.idle_since) {
            c.errand_wait = c.idle_since;
            c.errand_at = c.idle_since + static_cast<Time>(w.rng[Stream::Characters].uniform(pc.wait_min_days, pc.wait_max_days) * kDay);
        }
        if (w.now >= c.errand_at) {
            const Id errand = missions::post_errand(w, c.planet, id, observer);
            if (errand != kNone && missions::qualifies(w, id, w.missions[at(errand)])) {
                missions::take(w, id, errand, observer);
                return;
            }
            wake(w, id, w.now + kDay);  // none to be had here (a siege, say): it looks again tomorrow
            return;
        }
        wake(w, id, std::min(c.errand_at, w.now + kDay));
        return;
    }
    // A day with nothing here and nothing stale to look at: it moves on anyway (TRD-4),
    // to a nearby planet, busier ones likelier, so long as the fuel puts it in no debt.
    // Off the Lattice the fuel is nothing, so even the indebted can go.
    if (w.now - c.idle_since >= static_cast<Time>(w.config.merchant.wander_after_days * kDay)) {
        const Id to = wander_target(w, id);
        if (to != kNone && move::fuel_cost(w, c, c.planet, to) <= std::max(0.0, c.wealth) && move::travel_to(w, id, to)) {
            c.activity = Activity::Exploring;
            ++w.ledger.wanders;
            return;
        }
    }
    wake(w, id, w.now + kDay);  // at most daily (TIM-3)
}

void landed(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    info::observe_market(w, c.facts, c.planet);
    pirates::observe_system(w, c.facts, c.planet);
    refill(w, id);
    // The broke clock runs at landing too (PIR-4, CHR-16): a character that's never docked
    // at the daily tick would otherwise stay in debt for months.
    if (c.wealth >= 0) c.broke_since = kNever;
    if (broke_due(w, c) && settle_broke(w, id)) return;
    // INF-4: a member landing at a planet of its faction shares with the governor.
    const Id faction = faction_of(w, c);
    if (faction != kNone && fealty::root_of(w, c.planet) == faction) {
        info::share(c.facts, info::planet_facts(w, c.planet));
        threats::check(w, c.planet, observer);
    }
    if (c.governs == c.planet) roles::handover(w, c.planet);  // home: it and the steward share (SYS-22)
    // Lawful markets inspect (INF-14).
    suspicion::on_landing(w, id, observer);
    switch (c.activity) {
        case Activity::Mission:
            if (c.mission != kNone && (w.missions[at(c.mission)].kind == MissionKind::Smuggle ||
                                       w.missions[at(c.mission)].kind == MissionKind::LegalGoods)) {
                smuggling::landed(w, id, observer);
                return;
            }
            missions::arrived(w, id, observer);
            return;
        case Activity::Patrolling:
            missions::arrived(w, id, observer);
            return;
        case Activity::Raiding:
            raids::home(w, id, observer);
            return;
        case Activity::Renouncing:
            pirates::renounce(w, id, observer);
            return;
        case Activity::Spying:
            spies::landed(w, id, observer);
            return;
        case Activity::Trading:
            wake(w, id, w.now + static_cast<Time>(w.config.merchant.dock_hours * kHour));  // TRD-12: loading takes time
            return;
        case Activity::Army:
        case Activity::Stewarding:
            wake(w, id, w.now);
            return;
        default:
            c.activity = Activity::Idle;
            c.idle_since = w.now;
            wake(w, id, w.now);
            return;
    }
}

void detected(World& w, Id a, Id b) {
    ++w.ledger.detections;
    Character& ca = w.characters[at(a)];
    Character& cb = w.characters[at(b)];
    const Id fa = faction_of(w, ca);
    if (fa == kNone || fa != faction_of(w, cb)) return;
    const Time cooldown = static_cast<Time>(w.config.space.meet_cooldown_hours * kHour);
    if (w.now - ca.last_meet < cooldown || w.now - cb.last_meet < cooldown) return;
    info::share(ca.facts, cb.facts);  // SYS-15: members who meet in open space share
    ca.last_meet = cb.last_meet = w.now;
    ++w.ledger.meetings_shared;
}

}  // namespace lattice::chars
