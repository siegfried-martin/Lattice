#include "war/prizes.hpp"

#include <algorithm>

#include "chars/characters.hpp"
#include "chars/ships.hpp"
#include "politics/opinion.hpp"
#include "war/diplomacy.hpp"

namespace lattice::prizes {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

// Kept sorted by character, so tallies are found and paid in a fixed order (ARC-6).
void add(std::vector<Owed>& list, Id character, double amount) {
    auto it = std::lower_bound(list.begin(), list.end(), character, [](const Owed& o, Id c) { return o.character < c; });
    if (it == list.end() || it->character != character) it = list.insert(it, {character, 0});
    it->amount += amount;
}

}  // namespace

std::vector<Owed> shares(const World& w, const std::vector<Id>& side, double static_power) {
    std::vector<Owed> out;
    double total = std::max(0.0, static_power);
    for (Id id : side) {
        const double p = ships::fleet(w, w.characters[at(id)]).combat;
        out.push_back({id, p});
        total += p;
    }
    for (Owed& o : out) o.amount = total > 0 ? o.amount / total : 0;
    return out;
}

void credit(World& w, const std::vector<Owed>& by, Id victim_faction, double price) {
    if (victim_faction == kNone || price <= 0) return;
    for (const Owed& s : by) {
        const Character& c = w.characters[at(s.character)];
        if (!c.alive || s.amount <= 0) continue;
        const Id root = chars::faction_of(w, c);
        if (root == kNone || root == kPirates || w.planets[at(root)].pirate) continue;
        if (victim_faction != kPirates && !diplomacy::at_war(w, root, victim_faction)) continue;
        const double amount = w.config.bounty.share * price * s.amount;
        add(w.planets[at(root)].bounties, s.character, amount);
        w.ledger.bounties_owed += amount;
    }
}

double owed(const World& w, Id root) {
    double sum = 0;
    for (const Owed& o : w.planets[at(root)].bounties) sum += o.amount;
    return sum;
}

void pay(World& w, Id root, Observer* observer) {
    Planet& p = w.planets[at(root)];
    std::vector<Owed> left;
    for (const Owed& o : p.bounties) {
        Character& c = w.characters[at(o.character)];
        if (!c.alive || c.pirate) continue;  // nobody to pay
        const double paid = std::min(o.amount, std::max(0.0, p.treasury));
        p.treasury -= paid;
        p.month_military += paid;
        chars::earn(c, paid, true);
        w.ledger.bounties_paid += paid;
        if (o.amount - paid > 1e-9) {
            left.push_back({o.character, o.amount - paid});
            ++w.ledger.bounties_unpaid;
            if (p.governor != kNone) opinion::deed(w, o.character, p.governor, DeedKind::BountyUnpaid, observer);
        }
    }
    p.bounties = std::move(left);
}

void move(World& w, Id from, Id to) {
    if (from == to) return;
    std::vector<Owed> tallies = std::move(w.planets[at(from)].bounties);
    w.planets[at(from)].bounties.clear();
    if (to == kNone) return;
    for (const Owed& o : tallies) add(w.planets[at(to)].bounties, o.character, o.amount);
}

double spoils_of(const World& w, Id planet) {
    const Planet& p = w.planets[at(planet)];
    return std::min(std::max(0.0, p.treasury), w.config.spoils.per_prosperity * p.population * p.productivity);
}

void siege_day(World& w, Id siege) {
    Siege& s = w.sieges[at(siege)];
    for (Id m : w.armies[at(s.army)].members) add(s.power_days, m, ships::fleet(w, w.characters[at(m)]).combat);
}

namespace {

// Each member's part of a prize: by power x days in the siege, or by power now when it has
// none (taken on the first day, or no siege). Shares sum to 1; empty if nobody has any.
std::vector<Owed> parts(const World& w, const Army& a, const Siege* s) {
    std::vector<Owed> part;
    double total = 0;
    for (Id m : a.members) {
        double served = 0;
        if (s) {
            for (const Owed& o : s->power_days) {
                if (o.character == m) served = o.amount;
            }
        }
        part.push_back({m, served});
        total += served;
    }
    if (total <= 0) {
        part.clear();
        for (Id m : a.members) {
            part.push_back({m, ships::fleet(w, w.characters[at(m)]).combat});
            total += part.back().amount;
        }
    }
    if (total <= 0) return {};
    for (Owed& o : part) o.amount /= total;
    return part;
}

}  // namespace

void take_spoils(World& w, Id siege, Observer* observer) {
    const Siege& s = w.sieges[at(siege)];
    const Army& a = w.armies[at(s.army)];
    const double amount = spoils_of(w, s.planet);
    if (amount <= 0) return;
    const std::vector<Owed> part = parts(w, a, &s);
    if (part.empty()) return;
    w.planets[at(s.planet)].treasury -= amount;
    for (const Owed& o : part) chars::earn(w.characters[at(o.character)], amount * o.amount, true);
    w.ledger.spoils += amount;
    if (observer) observer->log(w.now, "planet.spoils", s.planet, std::to_string(amount) + " to army " + std::to_string(s.army));
}

void pay_purse(World& w, Id army, Id siege, Observer* observer) {
    Army& a = w.armies[at(army)];
    const double amount = a.purse;
    if (amount <= 0) return;
    const std::vector<Owed> part = parts(w, a, siege != kNone ? &w.sieges[at(siege)] : nullptr);
    if (part.empty()) return;
    a.purse = 0;
    Planet& home = w.planets[at(a.home)];
    home.treasury -= amount;
    home.month_military += amount;
    for (const Owed& o : part) chars::earn(w.characters[at(o.character)], amount * o.amount, true);
    w.ledger.objective_rewards += amount;
    if (observer) observer->log(w.now, "army.purse", army, std::to_string(amount) + " from " + std::to_string(a.home));
}

}  // namespace lattice::prizes
