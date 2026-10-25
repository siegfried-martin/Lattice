#include "politics/relief.hpp"

#include <algorithm>

#include "politics/opinion.hpp"
#include "politics/roles.hpp"

namespace lattice::relief {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

Time courier(const World& w, Id a, Id b) { return std::max<Time>(1, static_cast<Time>(w.derived.travel(a, b) * kDay)); }

double monthly_net(const World& w, const Planet& p) { return p.net_avg * static_cast<double>(w.config.time.payment_days); }

}  // namespace

bool in_need(const World& w, Id planet) {
    const auto& r = w.config.relief;
    const Planet& p = w.planets[at(planet)];
    if (p.shortfall >= r.shortfall && r.shortfall > 0) return true;
    if (p.population > 0 && monthly_net(w, p) / p.population < r.income_per_head) return true;
    return p.captured_at != kNever &&
           static_cast<double>(w.now - p.captured_at) <= r.capture_months * static_cast<double>(w.config.time.payment_days * kDay);
}

void clear(Planet& p) {
    p.relief_from = -1;
    p.relief_pending = kNever;
    p.relief_needed = kNever;
}

void weekly(World& w, Id planet, Observer* observer) {
    Planet& p = w.planets[at(planet)];
    if (p.liege == kNone) {
        clear(p);
        return;
    }
    const auto& r = w.config.relief;
    const bool need = in_need(w, planet);
    if (p.relief_from >= 0) {
        // On relief: it ends on its own terms, with no deed (INF-16).
        if (need) {
            p.relief_needed = w.now;
        } else if (w.now - p.relief_needed >= r.clear_weeks * 7 * kDay) {
            p.tribute_rate = p.relief_from;
            p.relief_from = -1;
            p.relief_needed = kNever;
            ++w.ledger.relief_ended;
            if (observer) observer->log(w.now, "relief.ended", planet, "rate back to " + std::to_string(p.tribute_rate));
        }
        return;
    }
    if (p.relief_pending != kNever) {
        if (w.now <= p.relief_pending) return;
        // No answer by the deadline is a refusal.
        p.relief_pending = kNever;
        ++w.ledger.relief_refused;
        opinion::deed(w, p.governor, w.planets[at(p.liege)].governor, DeedKind::ReliefRefused, observer);
        return;
    }
    if (!need || p.tribute_rate <= r.rate) return;
    const Time trip = courier(w, planet, p.liege);
    p.relief_pending = w.now + 2 * trip + w.config.time.budget_days * kDay;
    w.events.push({.time = w.now + trip, .kind = EventKind::ReliefRequest, .record = planet, .a = p.liege});
    ++w.ledger.relief_asked;
    if (observer) observer->log(w.now, "relief.asked", planet, "of " + std::to_string(p.liege));
}

void request(World& w, Id vassal, Id liege, Observer* observer) {
    const Planet& v = w.planets[at(vassal)];
    if (v.liege != liege || v.relief_pending == kNever) return;  // it swore elsewhere meanwhile
    const Planet& l = w.planets[at(liege)];
    const Config& c = w.config;
    bool granted = false;
    if (!roles::vacant(w, liege)) {
        // SYS-5: the tribute given up for the shortest relief, against what keeping the
        // vassal flowing is worth, as patronage is valued.
        const double cost = std::max(0.0, v.tribute_rate - c.relief.rate) * std::max(0.0, monthly_net(w, v)) *
                            static_cast<double>(c.relief.clear_weeks) / 4;
        const double favor = opinion::get(w, l.governor, v.governor);
        const double value = v.tribute_paid * c.value.patronage_months * std::max(0.0, 1 + c.fealty.w_favor * favor / 100);
        const double money = l.treasury + c.budget.credit_risk * std::max(0.0, monthly_net(w, l)) * roles::governor_risk(w, liege);
        granted = value >= cost && money >= cost;
    }
    if (observer) observer->log(w.now, granted ? "relief.granted" : "relief.refused", vassal, "by " + std::to_string(liege));
    // The answer is a courtesy; the vassal's deadline covers its being late (INF-16).
    w.events.push({.time = w.now + courier(w, liege, vassal), .kind = EventKind::ReliefAnswer, .record = vassal, .a = granted, .b = liege});
}

void answer(World& w, Id vassal, bool granted, Id liege, Observer* observer) {
    Planet& v = w.planets[at(vassal)];
    if (v.liege != liege || v.relief_pending == kNever) return;
    v.relief_pending = kNever;
    const Id lgov = w.planets[at(liege)].governor;
    if (!granted) {
        ++w.ledger.relief_refused;
        opinion::deed(w, v.governor, lgov, DeedKind::ReliefRefused, observer);
        return;
    }
    v.relief_from = v.tribute_rate;
    v.tribute_rate = w.config.relief.rate;
    v.relief_needed = w.now;
    ++w.ledger.relief_granted;
    opinion::deed(w, v.governor, lgov, DeedKind::ReliefGranted, observer);
}

}  // namespace lattice::relief
