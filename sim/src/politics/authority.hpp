#pragma once

#include "lattice/world.hpp"

// What the Trade Authority does with its surplus (FAC-14, SYS-20).
namespace lattice::authority {

// Weekly (proposed): for up to authority.contracts of the riskiest sectors with an
// on-network planet, above authority.risk_post, it posts a clearing contract there: a
// patrol of authority.clear_days at authority.pay a day, paid from its treasury. A
// mercenary who completes one raises the Authority's opinion of it.
void weekly(World& world, Observer* observer);

}  // namespace lattice::authority
