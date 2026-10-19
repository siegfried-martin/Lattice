#include "chars/missions.hpp"

#include <algorithm>

#include "chars/characters.hpp"
#include "chars/contracts.hpp"
#include "chars/movement.hpp"
#include "chars/ships.hpp"
#include "info/facts.hpp"
#include "politics/fealty.hpp"
#include "politics/opinion.hpp"

namespace lattice::missions {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

Id post(World& w, Mission m, Observer* observer) {
    w.missions.push_back(std::move(m));
    const auto id = static_cast<Id>(w.missions.size() - 1);
    const Mission& posted = w.missions.back();
    w.planets[at(posted.at)].missions.push_back(id);
    w.events.push({.time = posted.expires, .kind = EventKind::MissionExpire, .record = id});
    ++w.ledger.missions_posted;
    if (observer) {
        observer->log(w.now, "mission.posted", id,
                      std::string(posted.kind == MissionKind::Courier ? "courier" : "scout") + " at " + std::to_string(posted.at) +
                          " to " + std::to_string(posted.target));
    }
    return id;
}

void unpost(World& w, Id id) {
    auto& board = w.planets[at(w.missions[at(id)].at)].missions;
    board.erase(std::remove(board.begin(), board.end(), id), board.end());
}

double round_trip(const World& w, Id a, Id b) { return w.derived.travel(a, b) + w.derived.travel(b, a); }

ContractKind contract_kind(MissionKind k) { return k == MissionKind::Courier ? ContractKind::Courier : ContractKind::Scout; }

}  // namespace

void post_courier(World& w, Id liege, Id vassal, Observer* observer) {
    const auto& c = w.config;
    post(w,
         {.kind = MissionKind::Courier,
          .level = MissionLevel::Planet,
          .poster = w.planets[at(liege)].governor,
          .at = liege,
          .target = vassal,
          .base_pay = c.mission.courier_pay * round_trip(w, liege, vassal),
          .posted = w.now,
          .expires = w.now + c.courier.interval_days * kDay},
         observer);
}

// A faction scouts for trade: prices in neutral territory no single planet would pay to
// learn (INF-9). The root posts a run to the nearest outside market it knows least
// about. *(proposed)* Posted as a planet-level mission at the root's own planet, so open
// to anyone, since peacetime has no members to take faction-level work.
void root_weekly(World& w, Id root, Observer* observer) {
    const Planet& planet = w.planets[at(root)];
    if (planet.liege != kNone || planet.governor == kNone) return;
    for (Id m : planet.missions) {
        if (w.missions[at(m)].kind == MissionKind::Scout) return;  // one at a time
    }
    const Facts& known = info::governor_facts(w, root);
    const double stale = w.config.scout.stale_days * kDay;
    Id target = kNone;
    double best = 0;
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        if (fealty::same_faction(w, p, root)) continue;
        const PriceFact* f = info::find(known, p);
        if (f && static_cast<double>(w.now - f->time) < stale) continue;
        const double d = w.derived.travel(root, p);
        if (target == kNone || d < best) target = p, best = d;
    }
    if (target == kNone) return;
    const auto& c = w.config;
    post(w,
         {.kind = MissionKind::Scout,
          .level = MissionLevel::Planet,
          .poster = planet.governor,
          .at = root,
          .target = target,
          .base_pay = c.mission.scout_pay * round_trip(w, root, target),
          .min_combat = c.scout.min_combat,
          .posted = w.now,
          .expires = w.now + c.mission.expiry_days * kDay},
         observer);
}

double pay_now(const World& w, const Mission& m) {
    const double waited = static_cast<double>(w.now - m.posted) / kDay;
    return m.base_pay * (1 + w.config.mission.pay_growth * waited);
}

bool qualifies(const World& w, Id id, const Mission& m) {
    const Character& c = w.characters[at(id)];
    if (c.governs != kNone || c.ships.empty() || c.cargo_qty > 0) return false;
    const ships::Fleet f = ships::fleet(w, c);
    if (!f.lattice_drive || f.combat < m.min_combat || f.cargo < m.min_cargo) return false;
    if (m.level == MissionLevel::Faction) {
        const Id faction = chars::faction_of(w, c);
        if (faction == kNone || faction != fealty::root_of(w, m.at)) return false;  // INF-10
    }
    return true;
}

Offer best_offer(const World& w, Id id) {
    Offer best;
    const Character& c = w.characters[at(id)];
    if (c.planet == kNone) return best;
    const ships::Fleet f = ships::fleet(w, c);
    for (Id mid : w.planets[at(c.planet)].missions) {
        const Mission& m = w.missions[at(mid)];
        if (m.state != MissionState::Open || !qualifies(w, id, m)) continue;
        const double days = std::max(move::trip_days(w, c, m.at, m.target) + move::trip_days(w, c, m.target, m.at), 1.0 / 24);
        const double fuel = move::fuel_cost(w, c, m.at, m.target) + move::fuel_cost(w, c, m.target, m.at);
        const double skill = c.skills[static_cast<std::size_t>(m.kind)];
        const double score = (pay_now(w, m) / days) * (1 + w.config.skill.bonus * skill) - f.run_cost - fuel / days;
        if (score > best.score) best = {mid, score};
    }
    return best;
}

void take(World& w, Id id, Id mid, Observer* observer) {
    Mission& m = w.missions[at(mid)];
    m.state = MissionState::Taken;
    m.taker = id;
    m.pay = pay_now(w, m);
    unpost(w, mid);
    contracts::sign(w, contract_kind(m.kind), m.poster, id, m.pay, 0, mid);
    if (m.kind == MissionKind::Courier) m.packet = info::governor_facts(w, m.at);
    Character& c = w.characters[at(id)];
    c.mission = mid;
    c.activity = Activity::Mission;
    if (observer) observer->log(w.now, "mission.taken", mid, "by " + std::to_string(id));
    if (!move::travel_to(w, id, m.target)) {
        // Unreachable: give it back.
        m.state = MissionState::Expired;
        contracts::end(w, contracts::held(w, id, contract_kind(m.kind)));
        c.mission = kNone;
        c.activity = Activity::Idle;
        chars::wake(w, id, w.now + kDay);
    }
}

void arrived(World& w, Id id, Observer* observer) {
    Character& c = w.characters[at(id)];
    Mission& m = w.missions[at(c.mission)];
    if (m.outbound && c.planet == m.target) {
        if (m.kind == MissionKind::Courier) {
            Facts& there = info::governor_facts(w, m.target);
            info::merge_into(there, m.packet);
            m.packet = there;
        } else {
            // A scout looks over the whole system it was sent to.
            for (Id p : w.map.systems[at(w.planets[at(m.target)].system)].planets) info::observe_market(w, m.packet, p);
            info::merge_into(c.facts, m.packet);
        }
        m.outbound = false;
        if (move::travel_to(w, id, m.at)) return;
    }
    if (c.planet != m.at) {
        if (!move::travel_to(w, id, m.at)) chars::wake(w, id, w.now + kDay);
        return;
    }
    // Home: deliver, and get paid (REL-8: the contract ends with the job).
    info::merge_into(info::governor_facts(w, m.at), m.packet);
    m.packet = {};
    m.state = MissionState::Done;
    w.planets[at(m.at)].treasury -= m.pay;
    chars::earn(c, m.pay, m.kind != MissionKind::Courier);
    auto& skill = c.skills[static_cast<std::size_t>(m.kind)];
    skill = std::min(w.config.skill.max, skill + w.config.skill.gain);
    ++c.missions_done;
    c.reputation += 1;  // REL-7: grows with missions completed (OPN-11)
    contracts::end(w, contracts::held(w, id, contract_kind(m.kind)));
    opinion::deed(w, m.poster, id, DeedKind::JobDone, observer);  // REL-3: a job done well under contract
    ++w.ledger.missions_done;
    if (observer) observer->log(w.now, "mission.done", c.mission, "by " + std::to_string(id));
    c.mission = kNone;
    c.activity = Activity::Idle;
    c.idle_since = w.now;
    chars::wake(w, id, w.now);
}

void expire(World& w, Id mid, Observer* observer) {
    Mission& m = w.missions[at(mid)];
    if (m.state != MissionState::Open) return;
    m.state = MissionState::Expired;
    unpost(w, mid);
    ++w.ledger.missions_expired;
    if (observer) observer->log(w.now, "mission.expired", mid, "");
    if (m.kind == MissionKind::Courier) info::send_courier(w, m.target);
}

}  // namespace lattice::missions
