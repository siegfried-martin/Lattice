#include "chars/contracts.hpp"

#include <algorithm>

#include "chars/characters.hpp"
#include "politics/roles.hpp"

namespace lattice::contracts {

namespace {

std::size_t at(Id i) { return static_cast<std::size_t>(i); }

void unlist(std::vector<Id>& list, Id id) { list.erase(std::remove(list.begin(), list.end(), id), list.end()); }

}  // namespace

Id sign(World& w, ContractKind kind, Id employer, Id employee, double pay, Time period, Id mission) {
    w.contracts.push_back({.kind = kind, .employer = employer, .employee = employee, .pay = pay, .period = period,
                           .mission = mission, .start = w.now});
    const auto id = static_cast<Id>(w.contracts.size() - 1);
    w.characters[at(employer)].contracts.push_back(id);
    w.characters[at(employee)].contracts.push_back(id);
    return id;
}

void end(World& w, Id id) {
    if (id == kNone) return;  // already ended (a deposed employer, a lost fleet)
    Contract& k = w.contracts[at(id)];
    if (!k.alive) return;
    k.alive = false;
    unlist(w.characters[at(k.employer)].contracts, id);
    unlist(w.characters[at(k.employee)].contracts, id);
    if (k.kind == ContractKind::Steward) {
        // The post is empty again (FAC-16): its governor was deposed, or it quit.
        Character& s = w.characters[at(k.employee)];
        if (s.stewards != kNone) {
            const Id planet = s.stewards;
            if (w.planets[at(planet)].steward == k.employee) {
                w.planets[at(planet)].steward = kNone;
                roles::set_growth(w, planet);
            }
            s.stewards = kNone;
            if (s.activity == Activity::Stewarding) s.activity = Activity::Idle;
            if (s.alive) chars::wake(w, k.employee, w.now);
        }
    }
}

Id held(const World& w, Id character, ContractKind kind) {
    for (Id id : w.characters[at(character)].contracts) {
        const Contract& k = w.contracts[at(id)];
        if (k.alive && k.kind == kind && k.employee == character) return id;
    }
    return kNone;
}

}  // namespace lattice::contracts
