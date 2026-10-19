#pragma once

#include "lattice/world.hpp"

// Markets and prices (SYS-3, PRD-10). Every planet is a state trader (ECO-2): traders
// buy from and sell to its stock, and the money goes through its treasury.
namespace lattice::market {

// The reserve target: good.reserve_days of the planet's own consumption. Luxuries use
// their share of lux.consumption at a fill of 1. Lawful markets hold no contraband
// (PIR-14), so its reserve there is 0.
double reserve(const Config& c, const Planet& p, Good g);

// base x clamp((reserve / stock)^k, price.min, price.max); a market with no reserve
// (no population) sits at price.min.
double price(const Config& c, const Planet& p, Good g);
inline double ask(const Config& c, const Planet& p, Good g) { return price(c, p, g) * (1 + c.price.spread / 2); }
inline double bid(const Config& c, const Planet& p, Good g) { return price(c, p, g) * (1 - c.price.spread / 2); }

// A planet whose treasury is below its credit limit (econ.credit_limit months of
// income in the red) stops buying from traders.
bool can_buy(const Config& c, const Planet& p);

// The planet's own production at its current efficiency, events included (PRD-7,
// SYS-4): prod.efficiency by tier, one tier up from high is x event.bumper_mult, one
// tier down from low is low x (low / medium).
double efficiency(const Config& c, const Planet& p, int slot);

}  // namespace lattice::market
