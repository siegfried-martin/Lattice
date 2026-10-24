#pragma once

#include <vector>

#include "lattice/world.hpp"

// Governors, stewards and succession (SYS-22, CHR-20, FAC-16, FAC-17, MIL-20).
namespace lattice::roles {

enum class Post { Governor, Steward, Commander };

// How well a candidate suits a post at a planet, as the chooser sees it (CHR-20):
//   fit = w.power x combat / (combat + value.power_ref) + w.trade x cargo / (cargo + fit.cargo_half)
//       + w.rep x rep / (rep + hire.rep_half) + w.opinion x the chooser's opinion / 100
//       + w.culture x (same culture as the planet)
// with the weights fit.<post>.*. Power, cargo and reputation level off.
double fitness(const World& world, Post post, Id chooser, Id candidate, Id planet);

// The planet's governor died and the seat waits for a successor (FAC-17).
bool vacant(const World& world, Id planet);
// Who runs the planet now: the governor when it's there and alive, else the steward,
// else the governor's record (SYS-22). Its facts are the planet's information.
Id in_charge(const World& world, Id planet);
// The risk tolerance behind each half of the budget (SYS-5): the governor's lines read
// the governor's, the steward's lines the steward's. A vacant seat's steward decides
// both; a planet with no steward uses the governor's throughout.
double governor_risk(const World& world, Id planet);
double steward_risk(const World& world, Id planet);
// A governor sails with its own fleet only with a steward at the planet to run it while
// it's away (MIL-20).
bool can_sail(const World& world, Id planet);
// The governor is at its planet and not with an army (SYS-22).
bool home(const World& world, Id planet);
// The governor and the steward share what each knows (INF-4), as when the governor comes
// home or leaves.
void handover(World& world, Id planet);
// PRD-3: the growth factor follows the steward's risk tolerance when there is one, else
// the governor's.
void set_growth(World& world, Id planet);
// Notes what the character mostly earned from, as it takes a post (CHK-27).
void record_path(Character& c);

// Makes the character the planet's governor: its own contracts as an employee end, it
// leaves any army, and it goes home if it's elsewhere.
void seat(World& world, Id planet, Id character, Observer* observer);
// The best of a pool for a planet's seat by the governor weights, as the chooser sees
// it; none if the pool is empty. bonus_for gets fealty.capture_bonus (the army's leader).
Id best_governor(const World& world, const std::vector<Id>& pool, Id chooser, Id planet, Id bonus_for = kNone);
// SYS-22's fallback: anyone within hire.range_days of the planet who governs nothing,
// isn't busy elsewhere, and is a pirate exactly when the planet is.
std::vector<Id> fallback_pool(const World& world, Id planet);
// The last resort: a character generated at the planet, as the starting world seeds one.
Id generate(World& world, Id planet);

// Weekly, at the governor's budget: a planet with no steward offers the post down the
// steward fitness order, at steward.salary_share of monthly net income, until someone
// accepts (it beats their running average of earnings). The salary follows income.
void steward_weekly(World& world, Id planet, Observer* observer);
// A steward's decision: go to its planet, stay docked, and once a payment period weigh
// the salary against its best trade, quitting when trade wins (SYS-9's merchant rule).
void steward_decide(World& world, Id character, Observer* observer);

// A governor died (SYS-18): the seat falls vacant. A root is filled at once: its steward,
// else its most loyal vassal becomes root and fills it, else its best commander or the
// fallback. A vassal waits for its liege to hear (FAC-17).
void governor_died(World& world, Id planet, Observer* observer);
// Weekly: a vacant seat whose chooser has heard is filled. True while the seat is vacant.
bool settle(World& world, Id planet, Observer* observer);

}  // namespace lattice::roles
