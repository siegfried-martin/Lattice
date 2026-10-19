#include "economy/events.hpp"

#include <algorithm>
#include <string>

namespace lattice::events {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

int slot_of(const Planet& p, Good g) {
    for (int s = 0; s < 3; ++s) {
        if (p.makes[at(s)] == g) return s;
    }
    return -1;
}

bool is_drought(WorldEventKind k) { return k == WorldEventKind::Drought || k == WorldEventKind::RegionalDrought; }

// Whether an alive event like this already covers the planet. Only asked after a roll
// succeeds, which is rare, so a scan of every event is cheap.
bool covered(const World& w, Id planet, WorldEventKind kind, Good good) {
    for (const WorldEvent& e : w.world_events) {
        if (!e.alive || std::find(e.planets.begin(), e.planets.end(), planet) == e.planets.end()) continue;
        if (is_drought(kind) && is_drought(e.kind)) return true;
        if (kind == e.kind && (kind == WorldEventKind::Festival || e.good == good)) return true;
    }
    return false;
}

// Applies (sign +1) or undoes (sign -1) an event's effect on its planets.
void apply(World& w, const WorldEvent& e, int sign) {
    for (Id id : e.planets) {
        Planet& p = w.planets[at(id)];
        if (e.kind == WorldEventKind::Festival) {
            p.festivals += sign;
            continue;
        }
        const int s = slot_of(p, e.good);
        if (s < 0) continue;
        const int shift = e.kind == WorldEventKind::Bumper ? -1 : 1;  // up or down a tier
        p.tier_shift[at(s)] = static_cast<std::int8_t>(p.tier_shift[at(s)] + sign * shift);
    }
}

}  // namespace

void reshift(World& w, Id planet, int sign) {
    Planet& p = w.planets[at(planet)];
    for (const WorldEvent& e : w.world_events) {
        if (!e.alive || e.kind == WorldEventKind::Festival) continue;
        if (std::find(e.planets.begin(), e.planets.end(), planet) == e.planets.end()) continue;
        const int s = slot_of(p, e.good);
        if (s < 0) continue;
        const int shift = e.kind == WorldEventKind::Bumper ? -1 : 1;
        p.tier_shift[at(s)] = static_cast<std::int8_t>(p.tier_shift[at(s)] + sign * shift);
    }
}

namespace {

const char* kind_name(WorldEventKind k) {
    switch (k) {
        case WorldEventKind::Drought: return "drought";
        case WorldEventKind::Bumper: return "bumper";
        case WorldEventKind::Festival: return "festival";
        case WorldEventKind::RegionalDrought: return "regional_drought";
    }
    return "?";
}

void start(World& w, WorldEvent e, std::int64_t days, Observer* observer) {
    if (e.planets.empty()) return;
    e.start = w.now;
    e.end = w.now + days * kDay;
    apply(w, e, +1);
    w.world_events.push_back(e);
    const auto id = static_cast<Id>(w.world_events.size() - 1);
    w.events.push({.time = e.end, .phase = Phase::Scheduled, .kind = EventKind::WorldEventEnd, .record = id});
    if (observer) {
        std::string detail = std::string(kind_name(e.kind)) + " " + std::string(good_name(e.good)) + " planets";
        for (Id p : e.planets) detail += " " + std::to_string(p);
        if (e.region != kNone) detail += " region " + std::to_string(e.region);
        detail += " days " + std::to_string(days);
        observer->log(w.now, "event.start", id, detail);
    }
}

}  // namespace

void roll(World& w, Observer* observer) {
    const auto& c = w.config.event;
    Rng& rng = w.rng[Stream::Events];
    for (Id id = 0; id < static_cast<Id>(w.planets.size()); ++id) {
        const Planet& p = w.planets[at(id)];
        if (slot_of(p, kFood) >= 0 && rng.chance(c.drought_p) && !covered(w, id, WorldEventKind::Drought, kFood)) {
            start(w, {.kind = WorldEventKind::Drought, .planets = {id}, .good = kFood}, c.drought_days, observer);
        }
        for (int s = 0; s < 3; ++s) {
            const Good g = p.makes[at(s)];
            if (rng.chance(c.bumper_p) && !covered(w, id, WorldEventKind::Bumper, g)) {
                start(w, {.kind = WorldEventKind::Bumper, .planets = {id}, .good = g}, c.bumper_days, observer);
            }
        }
        if (rng.chance(c.festival_p) && !covered(w, id, WorldEventKind::Festival, kFood)) {
            start(w, {.kind = WorldEventKind::Festival, .planets = {id}, .good = kFood}, c.festival_days, observer);
        }
    }
    for (Id r = 0; r < static_cast<Id>(w.map.regions.size()); ++r) {
        if (!rng.chance(c.region_drought_p)) continue;
        WorldEvent e{.kind = WorldEventKind::RegionalDrought, .region = r, .good = kFood};
        for (Id id : w.derived.region_planets[at(r)]) {
            if (slot_of(w.planets[at(id)], kFood) >= 0 && !covered(w, id, WorldEventKind::Drought, kFood)) {
                e.planets.push_back(id);
            }
        }
        start(w, e, c.drought_days, observer);
    }
}

void end(World& w, Id id, Observer* observer) {
    WorldEvent& e = w.world_events[at(id)];
    if (!e.alive) return;
    e.alive = false;
    apply(w, e, -1);
    if (observer) observer->log(w.now, "event.end", id, kind_name(e.kind));
}

}  // namespace lattice::events
