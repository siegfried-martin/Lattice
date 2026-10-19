#include "war/gates.hpp"

#include <algorithm>
#include <string>

#include "politics/fealty.hpp"
#include "war/diplomacy.hpp"

namespace lattice::gates {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

bool by_node(const GateFact& f, Id node) { return f.node < node; }

void put(Facts& facts, const GateFact& f) {
    auto it = std::lower_bound(facts.gates.begin(), facts.gates.end(), f.node, by_node);
    if (it != facts.gates.end() && it->node == f.node) {
        if (f.time >= it->time) *it = f;
    } else {
        facts.gates.insert(it, f);
    }
}

// Those who see a gate change hands: everyone at the planets it serves, and the
// characters given (the army that did it).
void tell(World& w, Id node, const std::vector<Id>& also) {
    const LatticeNode& n = w.map.nodes[at(node)];
    if (n.group == kNone) return;
    const std::vector<Id>& planets = w.map.groups[at(n.group)].planets;
    for (Character& c : w.characters) {
        if (c.alive && c.planet != kNone && std::find(planets.begin(), planets.end(), c.planet) != planets.end()) {
            observe(w, c.facts, node);
        }
    }
    for (Id id : also) observe(w, w.characters[at(id)].facts, node);
}

void end_blockade(World& w, Id node) {
    Gate& g = w.gates[at(node)];
    g.captor = kNone;
    g.since = kNever;
    g.garrison = kNone;
    g.defence = 0;  // it rebuilds from nothing
}

}  // namespace

Id holder(const World& w, Id node) {
    const LatticeNode& n = w.map.nodes[at(node)];
    if (n.kind != NodeKind::Interchange || n.group == kNone) return kNone;
    return w.map.groups[at(n.group)].holder;
}

Id held_by(const World& w, Id planet) {
    const Planet& p = w.planets[at(planet)];
    if (p.group == kNone) return kNone;
    const Group& g = w.map.groups[at(p.group)];
    return g.holder == planet ? g.node : kNone;
}

Id captor_faction(const World& w, Id node) {
    if (w.gates.empty()) return kNone;
    const Id captor = w.gates[at(node)].captor;
    return captor == kNone ? kNone : fealty::root_of(w, captor);
}

bool shut_to(const World& w, Id node, Id faction) {
    const Id captor = captor_faction(w, node);
    return captor != kNone && captor != faction;
}

bool believed_shut(const World& w, const Facts& facts, Id node, Id faction) {
    const GateFact* f = find(facts, node);
    return f && f->captor != kNone && fealty::root_of(w, f->captor) != faction;
}

void observe(const World& w, Facts& facts, Id node) {
    put(facts, {.node = node, .time = w.now, .captor = w.gates[at(node)].captor});
}

const GateFact* find(const Facts& facts, Id node) {
    auto it = std::lower_bound(facts.gates.begin(), facts.gates.end(), node, by_node);
    return it != facts.gates.end() && it->node == node ? &*it : nullptr;
}

double max_defence(const World& w, Id node) {
    const Id h = holder(w, node);
    if (h == kNone || w.gates[at(node)].captor != kNone) return 0;
    return w.config.blockade.defence_share * w.config.defence.per_spend * w.planets[at(h)].security;
}

void regen(World& w, Id node) {
    Gate& g = w.gates[at(node)];
    if (g.siege != kNone || g.captor != kNone) return;
    const double top = max_defence(w, node);
    g.defence = std::min(top, g.defence + w.config.defence.regen * top);
}

double group_income(const World& w, Id node) {
    const LatticeNode& n = w.map.nodes[at(node)];
    if (n.group == kNone) return 0;
    double total = 0;
    for (Id p : w.map.groups[at(n.group)].planets) {
        total += std::max(0.0, w.planets[at(p)].income_avg) * static_cast<double>(w.config.time.payment_days);
    }
    return total;
}

void capture(World& w, Id node, Id army, Observer* observer) {
    Gate& g = w.gates[at(node)];
    const Army& a = w.armies[at(army)];
    g.captor = a.home;
    g.since = w.now;
    g.defence = 0;
    g.garrison = army;
    ++w.ledger.blockades;
    std::vector<Id> seen = a.members;
    seen.push_back(a.commissioner);
    tell(w, node, seen);
    if (observer) {
        observer->log(w.now, "gate.captured", node,
                      "holder " + std::to_string(holder(w, node)) + " by " + std::to_string(fealty::root_of(w, a.home)));
    }
}

void retake(World& w, Id node, Observer* observer) {
    const Id was = captor_faction(w, node);
    end_blockade(w, node);
    ++w.ledger.retakes;
    tell(w, node, {});
    if (observer) observer->log(w.now, "gate.retaken", node, "from " + std::to_string(was));
}

void check(World& w, Id node, Observer* observer) {
    const Gate& g = w.gates[at(node)];
    if (g.captor == kNone) return;
    const Id h = holder(w, node);
    if (!w.planets[at(g.captor)].pirate && diplomacy::hostile_planets(w, g.captor, h)) return;
    const Id was = captor_faction(w, node);
    end_blockade(w, node);
    ++w.ledger.lifts;
    tell(w, node, {});
    if (observer) observer->log(w.now, "gate.lifted", node, "by " + std::to_string(was));
}

}  // namespace lattice::gates
