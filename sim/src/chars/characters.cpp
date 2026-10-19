#include "chars/characters.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

#include "chars/contracts.hpp"
#include "chars/missions.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "politics/fealty.hpp"
#include "trade/merchants.hpp"
#include "pirates/pirates.hpp"
#include "pirates/smuggling.hpp"
#include "pirates/suspicion.hpp"
#include "war/armies.hpp"
#include "war/spies.hpp"
#include "war/raids.hpp"
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

// SYS-8: spend savings on a ship, following what has paid: more cargo or more combat.
// The next ship is the focus class (freighter or gunship): added while the fleet has room,
// otherwise replacing the cheapest ship one tier up. It's bought when wealth passes
// spend.threshold x its price and leaves the cautious reserve untouched.
void maybe_buy_ship(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    if (c.planet == kNone || c.ships.empty()) return;
    const auto& cfg = w.config;
    const ShipClass cls = c.earned_combat > c.earned_cargo ? ShipClass::Gunship : ShipClass::Freighter;
    int best_tier = 0;
    std::size_t cheapest = 0;
    for (std::size_t i = 0; i < c.ships.size(); ++i) {
        const ShipVariant& v = w.map.variants[at(c.ships[i])];
        if (v.cls == cls) best_tier = std::max<int>(best_tier, v.tier);
        if (v.price < w.map.variants[at(c.ships[cheapest])].price) cheapest = i;
    }
    const bool room = static_cast<std::int64_t>(c.ships.size()) < cfg.fleet.max_ships;
    const int tier = room ? std::max(1, best_tier) : std::min(kTierCount, best_tier + 1);
    const Id culture = w.planets[at(c.planet)].region;  // ships come from the planet's culture (MIL-4)
    const Id pick = ships::variant(w.map, culture, cls, tier);
    const ShipVariant& v = w.map.variants[at(pick)];
    const double resale = room ? 0 : w.map.variants[at(c.ships[cheapest])].price * cfg.ship.resale;
    if (!room && v.price <= w.map.variants[at(c.ships[cheapest])].price) return;
    const double cost = v.price - resale;
    const double threshold = cfg.spend.threshold_base - cfg.spend.threshold_risk * c.risk;
    if (c.wealth < threshold * v.price || c.wealth - cost < reserve(w, c)) return;
    c.wealth -= cost;
    w.ledger.ships_bought += cost;
    w.ledger.money_sunk += cost;
    if (room) {
        c.ships.push_back(pick);
    } else {
        c.ships[cheapest] = pick;
    }
    if (observer) observer->log(w.now, "ship.bought", id, std::to_string(static_cast<int>(cls)) + " tier " + std::to_string(tier));
}

// Not solvent for char.broke_days: sell ships, smallest first, until solvent (PIR-4's
// lawful branch; turning pirate arrives with piracy, MS-6). The last hull is kept.
void go_broke(World& w, Id id) {
    Character& c = w.characters[at(id)];
    while (c.wealth < 0 && c.ships.size() > 1) {
        auto it = std::min_element(c.ships.begin(), c.ships.end(),
                                   [&](Id a, Id b) { return w.map.variants[at(a)].price < w.map.variants[at(b)].price; });
        const double value = w.map.variants[at(*it)].price * w.config.ship.resale;
        c.wealth += value;
        w.ledger.ships_bought -= value;
        w.ledger.money_sunk -= value;
        c.ships.erase(it);
    }
}

}  // namespace

Id spawn(World& w, Id planet) {
    const Id id = add(w, planet);
    Character& c = w.characters[at(id)];
    c.wealth = w.config.chars.start_wealth;
    w.ledger.money_spawned += c.wealth;
    const bool armed = w.rng[Stream::Characters].chance(w.config.chars.start_armed_p);
    c.ships.push_back(ships::variant(w.map, c.culture, armed ? ShipClass::Scout : ShipClass::Courier, 1));
    wake(w, id, w.now);
    return id;
}

Id make_governor(World& w, Id planet) {
    const Id id = add(w, planet);
    Character& c = w.characters[at(id)];
    c.governs = planet;
    c.activity = Activity::Governing;
    Planet& p = w.planets[at(planet)];
    p.governor = id;
    // PRD-3: the governor's risk tolerance sets the planet's growth factor.
    p.growth_factor = w.config.pop.growth_factor_min + c.risk * (w.config.pop.growth_factor_max - w.config.pop.growth_factor_min);
    return id;
}

void populate(World& w) {
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) make_governor(w, p);
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

void earn(Character& c, double amount, bool combat_work) {
    c.wealth += amount;
    (combat_work ? c.earned_combat : c.earned_cargo) += amount;
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
        } else if (w.now - c.broke_since >= cfg.chars.broke_days * kDay && c.activity != Activity::Renouncing) {
            // PIR-4: a commander who knows a pirate planet may head there; else it sells ships.
            if (!pirates::try_turn(w, id, nullptr)) go_broke(w, id);
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

void decide(World& w, Id id, std::uint32_t seq, Observer* observer) {
    Character& c = w.characters[at(id)];
    if (seq != c.wake_seq || !c.alive || c.governs != kNone || c.planet == kNone) return;
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
        // Called to an army: go to the muster point and join (MIL-9).
        const Army& a = w.armies[at(c.army)];
        if (a.alive && a.action == ArmyAction::Muster && c.planet != a.home) {
            if (!move::travel_to(w, id, a.home)) armies::member_arrived(w, id, observer);
            return;
        }
        armies::member_arrived(w, id, observer);
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
    if (offer.mission != kNone && offer.score > 0 && offer.score >= trade) {
        missions::take(w, id, offer.mission, observer);
        return;
    }
    if (plan.score >= w.config.merchant.min_score || (c.cargo_qty > 0 && plan.value > 0)) {
        merchant::start(w, id, plan, observer);
        return;
    }
    // Nothing worth doing here: look at another market (TRD-4), if it can afford to.
    const Id target = merchant::explore_target(w, c);
    if (target != kNone && c.wealth - move::fuel_cost(w, c, c.planet, target) > reserve(w, c) &&
        move::travel_to(w, id, target)) {
        c.activity = Activity::Exploring;
        return;
    }
    wake(w, id, w.now + kDay);  // at most daily (TIM-3)
}

void landed(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    info::observe_market(w, c.facts, c.planet);
    pirates::observe_system(w, c.facts, c.planet);
    // INF-4: a member landing at a planet of its faction shares with the governor.
    const Id faction = faction_of(w, c);
    if (faction != kNone && fealty::root_of(w, c.planet) == faction) {
        info::share(c.facts, info::governor_facts(w, c.planet));
        threats::check(w, c.planet, observer);
    }
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
