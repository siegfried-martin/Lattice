#include "war/combat.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/missions.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "politics/fealty.hpp"
#include "politics/roles.hpp"
#include "war/armies.hpp"
#include "war/diplomacy.hpp"
#include "war/prizes.hpp"

namespace lattice::combat {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

double ships_power(const World& w, const std::vector<Id>& side) {
    double p = 0;
    for (Id id : side) p += ships::fleet(w, w.characters[at(id)]).combat;
    return p;
}

// The faction a fighter fights for: its contract's, or its own planet's for a governor.
Id fighter_root(const World& w, Id id) { return chars::faction_of(w, w.characters[at(id)]); }

// A pod lands at the nearest planet that isn't at war with its pilot (CHR-15), or dies
// with pod.death_p.
void lose_fleet(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    // Any contract that needed the fleet ends (CHR-16).
    for (ContractKind k : {ContractKind::Service, ContractKind::Escort}) {
        const Id ct = contracts::held(w, id, k);
        if (ct != kNone) contracts::end(w, ct);
    }
    // Cargo: combat.cargo_destroyed of it is destroyed; the rest is the winner's to take,
    // and what it can't carry is lost (SYS-14).
    if (c.cargo_good != kGoodCount && c.cargo_qty > 0) {
        double lost = c.cargo_qty;
        if (c.battle != kNone) {
            Battle& b = w.battles[at(c.battle)];
            if (b.loot_good == kGoodCount || b.loot_good == c.cargo_good) {
                const double saved = c.cargo_qty * (1 - w.config.combat.cargo_destroyed);
                b.loot_good = c.cargo_good;
                b.loot_qty += saved;
                lost -= saved;
            }
        }
        w.ledger.destroyed[c.cargo_good] += lost;
    }
    c.cargo_qty = 0, c.cargo_cost = 0, c.cargo_good = kGoodCount;
    // A mission that needed the fleet, or its cargo, has failed (MSN-3).
    if (c.mission != kNone) missions::fail(w, c.mission, observer);
    // An escort and its merchant part (TRD-8): the job no longer exists.
    if (c.escort != kNone) {
        Character& e = w.characters[at(c.escort)];
        contracts::end(w, contracts::held(w, c.escort, ContractKind::Escort));
        e.escorting = kNone;
        if (e.planet == kNone && e.route.empty() && e.battle == kNone) {
            e.planet = e.last_planet = c.last_planet;
            e.activity = Activity::Idle;
            chars::wake(w, c.escort, w.now + kHour);
        }
        c.escort = kNone;
    }
    if (c.escorting != kNone) {
        w.characters[at(c.escorting)].escort = kNone;
        c.escorting = kNone;
    }
    c.trip.clear();
    if (c.army != kNone) armies::leave(w, c.army, id);
    c.army = kNone;
    c.battle = kNone;
    c.route.clear();
    c.leg = 0;
    ++c.travel_seq;
    c.paused_at = kNever;
    if (w.rng[Stream::Combat].chance(w.config.combat.pod_death_p)) {
        c.alive = false;
        ++w.ledger.deaths;
        if (observer) observer->log(w.now, "character.died", id, "pod lost");
        if (c.governs != kNone) {
            // A governor's contracts as employer wait for its successor (FAC-17).
            for (Id k : std::vector<Id>(c.contracts)) {
                if (w.contracts[at(k)].employee == id) contracts::end(w, k);
            }
            roles::governor_died(w, c.governs, observer);
            return;
        }
        for (Id k : std::vector<Id>(c.contracts)) contracts::end(w, k);
        return;
    }
    ++w.ledger.pods;
    if (c.governs != kNone) {
        // A governor's pod makes for home (CHR-15), where it governs on without a fleet.
        c.planet = c.last_planet = c.governs;
        c.activity = Activity::Idle;
        chars::wake(w, id, w.now + kDay);
        if (observer) observer->log(w.now, "character.pod", id, "home to " + std::to_string(c.planet));
        return;
    }
    const Id own = fighter_root(w, id);
    Id best = kNone;
    double best_d = std::numeric_limits<double>::max();
    const auto [px, py] = std::pair{c.last_planet != kNone ? w.planets[at(c.last_planet)].x : 0.0,
                                    c.last_planet != kNone ? w.planets[at(c.last_planet)].y : 0.0};
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (own != kNone && diplomacy::at_war(w, own, fealty::root_of(w, p))) continue;
        const double d = std::hypot(w.planets[at(p)].x - px, w.planets[at(p)].y - py);
        if (d < best_d) best_d = d, best = p;
    }
    c.planet = best == kNone ? c.last_planet : best;
    c.last_planet = c.planet;
    c.activity = Activity::Idle;
    c.idle_since = w.now;
    chars::wake(w, id, w.now + kDay);
    if (observer) observer->log(w.now, "character.pod", id, "landed at " + std::to_string(c.planet));
}

}  // namespace

double destroy(World& w, std::vector<Id>& side, double damage, double& carried, Observer* observer, const std::vector<Owed>* by) {
    carried += damage;
    double destroyed = 0;
    for (;;) {
        // The weakest ship that isn't a flagship, else the weakest flagship.
        Id who = kNone;
        std::size_t index = 0;
        double best = std::numeric_limits<double>::max();
        bool flagship = true;
        for (Id id : side) {
            const auto& ships = w.characters[at(id)].ships;
            for (std::size_t i = 0; i < ships.size(); ++i) {
                const double p = w.map.variants[at(ships[i])].combat;
                const bool is_flag = i == 0;
                if (std::tie(is_flag, p) < std::tie(flagship, best)) flagship = is_flag, best = p, who = id, index = i;
            }
        }
        if (who == kNone || carried <= 0 || best > carried) break;
        carried -= best;
        destroyed += best;
        Character& c = w.characters[at(who)];
        if (by) prizes::credit(w, *by, chars::faction_of(w, c), w.map.variants[at(c.ships[index])].price);
        c.ships.erase(c.ships.begin() + static_cast<std::ptrdiff_t>(index));
        if (c.ships.empty()) {
            side.erase(std::find(side.begin(), side.end(), who));
            lose_fleet(w, who, observer);
        }
    }
    w.ledger.power_destroyed += destroyed;
    return destroyed;
}

double power(const World& w, const Battle& b, int side) {
    double p = ships_power(w, b.sides[at(side)]);
    if (side == 1 && b.planet_side && b.planet != kNone) p += w.planets[at(b.planet)].defence;
    return p;
}

Id start(World& w, std::vector<Id> attackers, std::vector<Id> defenders, Id planet, bool planet_side, Id army0, Id army1,
         Observer* observer) {
    Battle b{.planet = planet, .planet_side = planet_side, .start = w.now};
    if (planet != kNone) b.x = w.planets[at(planet)].x, b.y = w.planets[at(planet)].y;
    b.sides = {std::move(attackers), std::move(defenders)};
    b.armies = {army0, army1};
    w.battles.push_back(std::move(b));
    const auto id = static_cast<Id>(w.battles.size() - 1);
    for (const auto& side : w.battles.back().sides) {
        for (Id c : side) {
            w.characters[at(c)].battle = id;
            move::pause(w, c);
        }
    }
    ++w.ledger.battles;
    w.events.push({.time = w.now + w.config.time.battle_round_hours * kHour, .kind = EventKind::BattleRound, .record = id});
    if (observer) observer->log(w.now, "battle.start", id, "at " + std::to_string(planet));
    return id;
}

Id start_in_space(World& w, std::vector<Id> attackers, std::vector<Id> defenders, double x, double y, Observer* observer) {
    const Id id = start(w, std::move(attackers), std::move(defenders), kNone, false, kNone, kNone, observer);
    w.battles[at(id)].x = x;
    w.battles[at(id)].y = y;
    return id;
}

void round(World& w, Id id, Observer* observer) {
    Battle& b = w.battles[at(id)];
    if (!b.alive) return;
    const auto& c = w.config.combat;
    Rng& rng = w.rng[Stream::Combat];
    const double p0 = power(w, b, 0), p1 = power(w, b, 1);
    int winner = -1;
    if (b.sides[0].empty()) winner = 1;
    if (b.sides[1].empty()) winner = 0;  // the static defence alone is a siege, not a battle
    if (winner < 0) {
        ++b.rounds;
        const double loss0 = c.lethality * p1 * rng.uniform(1 - c.noise, 1 + c.noise);
        const double loss1 = c.lethality * p0 * rng.uniform(1 - c.noise, 1 + c.noise);
        const Id root0 = b.sides[0].empty() ? kNone : fighter_root(w, b.sides[0][0]);
        const Id root1 = b.planet_side && b.planet != kNone ? fealty::root_of(w, b.planet)
                         : b.sides[1].empty()               ? kNone
                                                            : fighter_root(w, b.sides[1][0]);
        // Kills are credited by each side's power at the round's start (MIL-18).
        const std::array<std::vector<Owed>, 2> by = {prizes::shares(w, b.sides[0], 0),
                                                     prizes::shares(w, b.sides[1], p1 - ships_power(w, b.sides[1]))};
        const double lost0 = destroy(w, b.sides[0], loss0, b.damage[0], observer, &by[1]);
        const double lost1 = destroy(w, b.sides[1], loss1, b.damage[1], observer, &by[0]);
        b.lost[0] += lost0, b.lost[1] += lost1;
        diplomacy::record_loss(w, root0, root1, 0, lost0);
        diplomacy::record_loss(w, root1, root0, 0, lost1);
        if (b.sides[0].empty()) winner = 1;
        else if (b.sides[1].empty()) winner = 0;
        // Retreat: bolder commanders hold longer (SYS-14). The planet's defence never runs.
        for (int s = 0; s < 2 && winner < 0; ++s) {
            const double own = power(w, b, s), enemy = power(w, b, 1 - s);
            double risk = 0, strongest = -1;
            for (Id f : b.sides[at(s)]) {
                const double fp = ships::fleet(w, w.characters[at(f)]).combat;
                if (fp > strongest) strongest = fp, risk = w.characters[at(f)].risk;
            }
            if (own < (c.retreat_base - c.retreat_risk * risk) * enemy) {
                double carried = 0;
                destroy(w, b.sides[at(s)], c.retreat_loss * ships_power(w, b.sides[at(s)]), carried, observer, &by[at(1 - s)]);
                winner = 1 - s;
                if (observer) observer->log(w.now, "battle.retreat", id, "side " + std::to_string(s));
            }
        }
    }
    if (winner < 0) {
        w.events.push({.time = w.now + w.config.time.battle_round_hours * kHour, .kind = EventKind::BattleRound, .record = id});
        return;
    }
    b.alive = false;
    // The winners take what loot they can carry; the rest is lost (SYS-14, MIL-15).
    if (b.loot_qty > 0) {
        for (Id f : b.sides[at(winner)]) {
            Character& taker = w.characters[at(f)];
            if (taker.cargo_qty > 0 && taker.cargo_good != b.loot_good) continue;
            const double take = std::min(b.loot_qty, ships::fleet(w, taker).cargo - taker.cargo_qty);
            if (take <= 0) continue;
            taker.cargo_good = b.loot_good;
            taker.cargo_qty += take;
            b.loot_qty -= take;
            w.ledger.loot += take;
        }
        w.ledger.destroyed[b.loot_good] += b.loot_qty;
        b.loot_qty = 0;
    }
    for (const auto& side : b.sides) {
        for (Id f : side) {
            Character& ch = w.characters[at(f)];
            ch.munitions += w.config.fleet.munitions_per_power * ships::fleet(w, ch).combat;  // refilled on landing (MIL-4)
            ch.battle = kNone;
            move::resume(w, f);
        }
    }
    if (observer) observer->log(w.now, "battle.end", id, "won by side " + std::to_string(winner));
    armies::battle_over(w, id, winner, observer);
}

}  // namespace lattice::combat
