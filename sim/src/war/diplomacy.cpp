#include "war/diplomacy.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "chars/contracts.hpp"
#include "chars/ships.hpp"
#include "economy/economy.hpp"
#include "info/facts.hpp"
#include "politics/roles.hpp"
#include "politics/fealty.hpp"
#include "war/prizes.hpp"

namespace lattice::diplomacy {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

// A lawful root: pirate planets take no part in diplomacy (DIP-7).
bool is_root(const World& w, Id p) { return p >= 0 && w.planets[at(p)].liege == kNone && !w.planets[at(p)].pirate; }

const Relation* find(const World& w, Id a, Id b) {
    if (a == b || a == kNone || b == kNone) return nullptr;
    if (a > b) std::swap(a, b);
    for (const Relation& r : w.relations) {
        if (r.a == a && r.b == b) return &r;
    }
    return nullptr;
}

Relation& get(World& w, Id a, Id b) {
    if (a > b) std::swap(a, b);
    for (Relation& r : w.relations) {
        if (r.a == a && r.b == b) return r;
    }
    w.relations.push_back({.a = a, .b = b, .peace_income = {faction_income(w, a), faction_income(w, b)}});
    return w.relations.back();
}

int side_of(const Relation& r, Id root) { return root == r.a ? 0 : 1; }

// Both roots know at once; everyone else hears by courier and sharing (DIP-5).
void announce(World& w, const Relation& r) {
    for (Id root : {r.a, r.b}) {
        if (w.planets[at(root)].governor != kNone) {
            info::learn_state(w.characters[at(roles::in_charge(w, root))].facts, r.a, r.b, r.state, w.now);
        }
    }
}

double planet_income(const World& w, Id p) {
    return std::max(0.0, w.planets[at(p)].income_avg) * static_cast<double>(w.config.time.payment_days);
}

double sworn_power(const World& w, Id governor, bool serving_only) {
    double power = 0;
    for (Id k : w.characters[at(governor)].contracts) {
        const Contract& ct = w.contracts[at(k)];
        if (!ct.alive || ct.kind != ContractKind::Service || ct.employer != governor) continue;
        const Character& c = w.characters[at(ct.employee)];
        if (serving_only && c.activity != Activity::Serving) continue;
        power += ships::fleet(w, c).combat;
    }
    return power;
}

// Planets in each planet's branch, itself included.
std::vector<std::int64_t> branch_sizes(const World& w) {
    std::vector<std::int64_t> size(w.planets.size(), 0);
    for (Id p = 0; p < static_cast<Id>(w.planets.size()); ++p) {
        std::size_t guard = 0;
        for (Id q = p; q != kNone && guard++ <= w.planets.size(); q = w.planets[at(q)].liege) ++size[at(q)];
    }
    return size;
}

// The monthly income of target's planets within dip.border_days of any of root's: what a
// war with it could take (DIP-4).
double border_income(const World& w, Id root, Id target) {
    const std::vector<Id> mine = fealty::faction_planets(w, root), theirs = fealty::faction_planets(w, target);
    double total = 0;
    for (Id q : theirs) {
        for (Id p : mine) {
            if (w.derived.travel(p, q) <= w.config.dip.border_days) {
                total += planet_income(w, q);
                break;
            }
        }
    }
    return total;
}

// SYS-12's war vote of root's faction against target, from each governor's own position.
// *(proposed)* Strength and wealth are read from the world, not from beliefs, until
// facts carry them.
double war_vote(const World& w, Id root, Id target) {
    const auto& d = w.config.dip;
    const Relation* rel = find(w, root, target);
    const double power_own = faction_power(w, root), power_target = std::max(1.0, faction_power(w, target));
    const double income_now = faction_income(w, root);
    const double income_then = rel ? rel->peace_income[at(side_of(*rel, root))] : income_now;
    const double peace = income_then > 0 ? income_now / income_then - 1 : 0;
    const auto wars = static_cast<double>(enemies(w, root).size());
    double losses = 0;
    for (const Relation& r : w.relations) {
        if ((r.a == root || r.b == root) && r.state == DipState::War) losses += r.planets_lost[at(side_of(r, root))];
    }
    const std::vector<Id> mine = fealty::faction_planets(w, root), theirs = fealty::faction_planets(w, target);
    const std::vector<std::int64_t> size = branch_sizes(w);
    double total = 0, weights = 0;
    for (Id p : mine) {
        const Planet& planet = w.planets[at(p)];
        const Character& g = w.characters[at(planet.governor)];
        double near = 0;
        for (Id q : theirs) {
            if (w.derived.travel(p, q) <= d.border_days) near += planet_income(w, q);
        }
        const double idle = sworn_power(w, planet.governor, true) / w.config.value.power_ref;
        // Money piled up past a reserve wants spending (DIP-4).
        const auto& b = w.config.budget;
        const double months = planet.treasury / std::max(1.0, economy::monthly_costs(w.config, planet));
        const double hoard = std::clamp(months / b.reserve_months - 1, 0.0, b.hoard_cap);
        const double vote = (d.w_idle * idle + d.w_targets * near / std::max(1.0, planet_income(w, p)) +
                             d.w_strength * (power_own / power_target - 1) + d.w_peace * peace +
                             d.w_treasury * hoard - d.w_wars * wars - d.w_losses * losses) *
                            (0.5 + g.risk);
        const auto weight = static_cast<double>(size[at(p)]);
        total += weight * vote;
        weights += weight;
    }
    return weights > 0 ? total / weights : 0;
}

}  // namespace

DipState state(const World& w, Id a, Id b) {
    if (a < 0 || b < 0 || !is_root(w, a) || !is_root(w, b)) return DipState::Peace;
    const Relation* r = find(w, a, b);
    return r ? r->state : DipState::Peace;
}

bool at_war(const World& w, Id a, Id b) { return state(w, a, b) == DipState::War; }

bool planets_at_war(const World& w, Id a, Id b) { return at_war(w, fealty::root_of(w, a), fealty::root_of(w, b)); }

bool hostile_planets(const World& w, Id a, Id b) {
    return w.planets[at(a)].pirate != w.planets[at(b)].pirate || planets_at_war(w, a, b);
}

std::vector<Id> enemies(const World& w, Id root) {
    std::vector<Id> out;
    if (root < 0) return out;
    for (const Relation& r : w.relations) {
        if (r.state != DipState::War || !is_root(w, r.a) || !is_root(w, r.b)) continue;
        if (r.a == root) out.push_back(r.b);
        if (r.b == root) out.push_back(r.a);
    }
    return out;
}

bool neighbours(const World& w, Id a, Id b) {
    const std::vector<Id> pa = fealty::faction_planets(w, a), pb = fealty::faction_planets(w, b);
    for (Id p : pa) {
        for (Id q : pb) {
            if (w.derived.travel(p, q) <= w.config.dip.neighbour_days) return true;
        }
    }
    return false;
}

double faction_income(const World& w, Id root) {
    double income = 0;
    for (Id p : fealty::faction_planets(w, root)) income += planet_income(w, p);
    return income;
}

double faction_power(const World& w, Id root) {
    double power = 0;
    for (Id p : fealty::faction_planets(w, root)) {
        power += w.planets[at(p)].defence + sworn_power(w, w.planets[at(p)].governor, false);
    }
    return power;
}

void declare_war(World& w, Id a, Id b, Observer* observer) {
    Relation& r = get(w, a, b);
    r.state = DipState::War;
    r.since = w.now;
    r.planets_lost = r.power_lost = {0, 0};
    r.planets_at_start = {static_cast<double>(fealty::faction_planets(w, r.a).size()),
                          static_cast<double>(fealty::faction_planets(w, r.b).size())};
    r.power_at_start = {faction_power(w, r.a), faction_power(w, r.b)};
    r.hoped = {border_income(w, r.a, r.b), border_income(w, r.b, r.a)};
    announce(w, r);
    ++w.ledger.wars;
    if (observer) observer->log(w.now, "war.declared", a, "on " + std::to_string(b));
}

void make_ceasefire(World& w, Id payer, Id payee, double per_month, Observer* observer) {
    Relation& r = get(w, payer, payee);
    r.state = DipState::Ceasefire;
    r.payer = payer;
    r.payee = payee;
    r.amount = per_month;
    r.until = w.now + w.config.dip.ceasefire_days * kDay;
    announce(w, r);
    ++w.ledger.ceasefires;
    // Armies sent against each other stand down (DIP-2).
    for (Army& army : w.armies) {
        if (!army.alive || army.target == kNone) continue;
        const Id own = fealty::root_of(w, army.home), theirs = fealty::root_of(w, army.target);
        if ((own == payer && theirs == payee) || (own == payee && theirs == payer)) army.target = kNone;
    }
    if (observer) observer->log(w.now, "war.ceasefire", payer, "pays " + std::to_string(payee) + " " + std::to_string(per_month));
}

void root_weekly(World& w, Id root, Observer* observer) {
    if (!is_root(w, root)) return;
    const auto& d = w.config.dip;
    // Ceasefires that have run out become peace.
    for (Relation& r : w.relations) {
        if (r.state == DipState::Ceasefire && (r.a == root || r.b == root) && w.now >= r.until) {
            r.state = DipState::Peace;
            r.peace_since = w.now;
            r.peace_income = {faction_income(w, r.a), faction_income(w, r.b)};
            announce(w, r);
        }
    }

    // Suing for peace (DIP-3), from the root's own losses and weariness.
    for (Id enemy : enemies(w, root)) {
        const Relation& r = *find(w, root, enemy);
        const std::size_t me = at(side_of(r, root)), them = 1 - me;
        const double planets = r.planets_at_start[me] > 0 ? r.planets_lost[me] / r.planets_at_start[me] : 0;
        const double power = r.power_at_start[me] > 0 ? r.power_lost[me] / r.power_at_start[me] : 0;
        const double income = std::max(1.0, faction_income(w, root));
        const double treasury = std::min(0.0, w.planets[at(root)].treasury) / income;
        const double months = static_cast<double>(w.now - r.since) / (30.0 * kDay);
        const double score = -d.s_planets * planets - d.s_power * power + d.s_treasury * treasury - d.s_months * months;
        if (score >= -d.sue_threshold) continue;
        // The winner hears no offer until its armies could have reached the front.
        if (w.now - r.since < d.min_war_days * kDay) continue;
        const double offer = d.tribute_rate * std::min(-score, 3.0) * income;
        // The winner's view of carrying on: what it has been taking a month, or, if more,
        // what it hoped to take, by its share of the two sides' power, fading as the war
        // drags on.
        const std::vector<Id> mine = fealty::faction_planets(w, root);
        double avg = 0;
        for (Id p : mine) avg += planet_income(w, p);
        avg = mine.empty() ? 0 : avg / static_cast<double>(mine.size());
        const double taken = r.planets_lost[me] / std::max(1.0, months) * avg * w.config.value.invade_months / 12;
        const double own = faction_power(w, enemy), other = faction_power(w, root);
        const double share = own + other > 0 ? own / (own + other) : 0.5;
        const double hope = d.hope_share * r.hoped[them] * share * std::pow(0.5, months / d.hope_halflife_months);
        if (offer >= std::max(taken, hope)) make_ceasefire(w, root, enemy, offer, observer);
    }

    // The war vote against each neighbour at peace (DIP-4); none during the warm-up, so
    // markets and labour settle first (GEN-10).
    if (w.now < w.config.gen.warmup_days * kDay) return;
    for (Id other = 0; other < static_cast<Id>(w.planets.size()); ++other) {
        if (other == root || !is_root(w, other) || state(w, root, other) != DipState::Peace) continue;
        if (fealty::faction_planets(w, other).size() < 1 || !neighbours(w, root, other)) continue;
        if (war_vote(w, root, other) > d.declare_threshold) declare_war(w, root, other, observer);
    }
}

void pay_ceasefire(World& w, Id root, Observer* observer) {
    for (const Relation& r : w.relations) {
        if (r.state != DipState::Ceasefire || r.payer != root || !is_root(w, r.payee)) continue;
        // Only what the treasury holds, as with bounties (MIL-18): a ceasefire can't put a
        // planet into debt (tuning pass, part 2).
        const double paid = std::min(r.amount, std::max(0.0, w.planets[at(root)].treasury));
        w.planets[at(root)].treasury -= paid;
        w.planets[at(r.payee)].treasury += paid;
        w.ledger.ceasefire_tribute += paid;
        if (paid < r.amount) {
            w.ledger.ceasefire_unpaid += r.amount - paid;
            if (observer) observer->log(w.now, "ceasefire.short", root, std::to_string(r.amount - paid) + " to " + std::to_string(r.payee));
        }
    }
}

void forget(World& w, Id planet) {
    std::erase_if(w.relations, [&](const Relation& r) { return r.a == planet || r.b == planet; });
}

void move_root(World& w, Id old_root, Id new_root) {
    forget(w, new_root);  // anything left from when it was last a root
    prizes::move(w, old_root, new_root);  // its bounties owed (MIL-18)
    for (Relation& r : w.relations) {
        if (r.a == old_root) r.a = new_root;
        if (r.b == old_root) r.b = new_root;
        if (r.payer == old_root) r.payer = new_root;
        if (r.payee == old_root) r.payee = new_root;
        if (r.a > r.b) {
            std::swap(r.a, r.b);
            std::swap(r.peace_income[0], r.peace_income[1]);
            std::swap(r.planets_lost[0], r.planets_lost[1]);
            std::swap(r.power_lost[0], r.power_lost[1]);
            std::swap(r.planets_at_start[0], r.planets_at_start[1]);
            std::swap(r.power_at_start[0], r.power_at_start[1]);
        }
    }
    // A faction can't be in a relation with itself.
    std::erase_if(w.relations, [](const Relation& r) { return r.a == r.b; });
}

void record_loss(World& w, Id loser, Id winner, double planets, double power) {
    const Relation* found = find(w, loser, winner);
    if (!found || found->state != DipState::War) return;
    Relation& r = get(w, loser, winner);
    const std::size_t side = at(side_of(r, loser));
    r.planets_lost[side] += planets;
    r.power_lost[side] += power;
}

}  // namespace lattice::diplomacy
