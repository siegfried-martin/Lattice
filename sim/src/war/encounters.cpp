#include "war/encounters.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>

#include "chars/characters.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "pirates/pirates.hpp"
#include "pirates/smuggling.hpp"
#include "politics/fealty.hpp"
#include "war/armies.hpp"
#include "war/combat.hpp"
#include "war/diplomacy.hpp"
#include "war/risk.hpp"

namespace lattice::encounters {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

struct Entry {
    std::int64_t cx, cy;
    Id id;         // a character, or an army
    bool army;
    Id faction;
    double x, y;
    bool share, raider, patrol, at_war;
};

bool in_space(const Character& c) {
    return c.alive && c.planet == kNone && !c.route.empty() && c.battle == kNone && c.route[at(c.leg)].kind == LegKind::Space;
}

// The merchant a group is led by: an escort's merchant, else itself.
Id leader(const World& w, Id id) {
    const Id m = w.characters[at(id)].escorting;
    return m != kNone ? m : id;
}

void engage(World& w, Id attacker, Id quarry, double x, double y, bool raid, Observer* observer) {
    Rng& rng = w.rng[Stream::Combat];
    if (!rng.chance(w.config.space.engage_p)) return;
    quarry = leader(w, quarry);
    Character& q = w.characters[at(quarry)];
    Character& a = w.characters[at(attacker)];
    if (q.battle != kNone || a.battle != kNone) return;
    // After a fight neither side is set on again for a while: the quarry gets clear, the
    // raider sees to its prize.
    const Time cooldown = static_cast<Time>(w.config.space.engage_cooldown_hours * kHour);
    if ((a.last_engaged != kNever && w.now - a.last_engaged < cooldown) || (q.last_engaged != kNever && w.now - q.last_engaged < cooldown)) {
        return;
    }
    const double vq = ships::fleet(w, q).speed, va = ships::fleet(w, a).speed;
    if (vq > va && rng.chance(w.config.space.evade * (vq - va) / vq)) return;  // it slips away
    a.last_engaged = q.last_engaged = w.now;
    std::vector<Id> defenders{quarry};
    if (q.escort != kNone) defenders.push_back(q.escort);
    combat::start_in_space(w, {attacker}, defenders, x, y, observer);
    if (raid) {
        risk::add_raid(w, x, y);
        ++w.ledger.raids;
    }
}

void sight(World& w, Id who, const Army& a, Id army, double x, double y) {
    Character& c = w.characters[at(who)];
    const Sighting s{.army = army, .time = w.now, .x = x, .y = y, .target = a.target, .faction = fealty::root_of(w, a.home),
                     .power = armies::power(w, a)};
    info::add_sighting(c.facts, s);
    if (c.mission != kNone && w.missions[at(c.mission)].kind == MissionKind::Scout) info::add_sighting(w.missions[at(c.mission)].packet, s);
    ++w.ledger.sightings;
}

}  // namespace

void step(World& w, Observer* observer) {
    const double r = w.config.space.detect_range;
    const double step_days = static_cast<double>(w.config.time.space_step_minutes * kMinute) / kDay;
    const Time cooldown = static_cast<Time>(w.config.space.meet_cooldown_hours * kHour);
    std::vector<Entry> entries;
    auto cell = [&](double v) { return static_cast<std::int64_t>(std::floor(v / r)); };
    // Everyone in open space, then only those something could happen to. Anyone near a
    // raider is someone something could happen to.
    std::vector<Entry> all;
    std::vector<std::pair<std::int64_t, std::int64_t>> hot;
    for (Id id = 0; id < static_cast<Id>(w.characters.size()); ++id) {
        const Character& c = w.characters[at(id)];
        if (!in_space(c)) continue;
        const Id faction = chars::faction_of(w, c);
        const bool raider = c.activity == Activity::Raiding, patrol = c.activity == Activity::Patrolling;
        const bool share = faction >= 0 && w.now - c.last_meet >= cooldown;
        const bool at_war = faction >= 0 && !diplomacy::enemies(w, faction).empty();
        const bool scout = c.mission != kNone && w.missions[at(c.mission)].kind == MissionKind::Scout;
        const auto [x, y] = move::position(w, c);
        if (raider) {
            risk::add_hostile(w, x, y, step_days);
            for (std::int64_t dx = -1; dx <= 1; ++dx) {
                for (std::int64_t dy = -1; dy <= 1; ++dy) hot.emplace_back(cell(x) + dx, cell(y) + dy);
            }
        }
        if (patrol) risk::add_patrol(w, x, y, step_days);
        all.push_back({cell(x), cell(y), id, false, faction, x, y, share, raider, patrol, at_war || scout});
    }
    std::sort(hot.begin(), hot.end());
    for (const Entry& e : all) {
        const bool near_raider = std::binary_search(hot.begin(), hot.end(), std::pair{e.cx, e.cy});
        if (e.share || e.raider || e.patrol || e.at_war || near_raider) entries.push_back(e);
    }
    for (Id id = 0; id < static_cast<Id>(w.armies.size()); ++id) {
        const Army& a = w.armies[at(id)];
        if (!a.alive || a.planet != kNone || a.route.empty() || a.route[at(a.leg)].kind != LegKind::Space) continue;
        const auto [x, y] = armies::position(w, a);
        entries.push_back({cell(x), cell(y), id, true, fealty::root_of(w, a.home), x, y, false, false, false, false});
    }
    auto key = [](const Entry& e) { return std::tuple{e.cx, e.cy, e.army, e.id}; };
    std::sort(entries.begin(), entries.end(), [&](const Entry& a, const Entry& b) { return key(a) < key(b); });

    for (std::size_t i = 0; i < entries.size(); ++i) {
        const Entry a = entries[i];
        for (std::int64_t dx = -1; dx <= 1; ++dx) {
            for (std::int64_t dy = -1; dy <= 1; ++dy) {
                const std::pair want{a.cx + dx, a.cy + dy};
                auto it = std::lower_bound(entries.begin(), entries.end(), want, [](const Entry& e, const std::pair<std::int64_t, std::int64_t>& k) {
                    return std::pair{e.cx, e.cy} < k;
                });
                for (; it != entries.end() && std::pair{it->cx, it->cy} == want; ++it) {
                    const Entry& b = *it;
                    if (std::tie(b.army, b.id) <= std::tie(a.army, a.id)) continue;  // each pair once
                    if (std::hypot(b.x - a.x, b.y - a.y) > r) continue;
                    if (a.army && b.army) continue;  // armies don't meet in the field yet
                    if (a.army || b.army) {
                        const Entry& ship = a.army ? b : a;
                        const Entry& army = a.army ? a : b;
                        if (ship.faction != army.faction) sight(w, ship.id, w.armies[at(army.id)], army.id, army.x, army.y);
                        continue;
                    }
                    if (a.share && b.share && a.faction == b.faction) chars::detected(w, a.id, b.id);
                    // Hostile meetings: pirates attack anyone below neutral with them (PIR-9),
                    // lawful patrols attack pirates, and at war, as the attacker believes
                    // (DIP-5).
                    auto hostile = [&](const Entry& from, const Entry& to) {
                        const Character& x = w.characters[at(from.id)];
                        const Character& y = w.characters[at(to.id)];
                        if (x.pirate) return pirates::attacks(w, x, y);
                        if (y.pirate) return true;
                        return from.faction >= 0 && to.faction >= 0 && from.faction != to.faction &&
                               info::believes_war(x.facts, from.faction, to.faction);
                    };
                    const double mx = (a.x + b.x) / 2, my = (a.y + b.y) / 2;
                    // A pirate meeting a bold independent offers it a run first (PIR-13).
                    if (w.characters[at(a.id)].pirate && !w.characters[at(b.id)].pirate) smuggling::offer(w, a.id, b.id, observer);
                    if (w.characters[at(b.id)].pirate && !w.characters[at(a.id)].pirate) smuggling::offer(w, b.id, a.id, observer);
                    if (a.patrol && b.raider && hostile(a, b)) engage(w, a.id, b.id, mx, my, false, observer);
                    else if (b.patrol && a.raider && hostile(b, a)) engage(w, b.id, a.id, mx, my, false, observer);
                    else if (a.raider && !b.raider && hostile(a, b)) engage(w, a.id, b.id, mx, my, true, observer);
                    else if (b.raider && !a.raider && hostile(b, a)) engage(w, b.id, a.id, mx, my, true, observer);
                }
            }
        }
    }
}

}  // namespace lattice::encounters
