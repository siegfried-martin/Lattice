#pragma once

#include "lattice/world.hpp"

// Markets and prices (SYS-3, PRD-10). Every planet is a state trader (ECO-2): traders
// buy from and sell to its stock, and the money goes through its treasury.
namespace lattice::market {

// The reserve target: good.reserve_days of the planet's own consumption. Luxuries use
// their share of lux.consumption at a fill of 1. Lawful markets hold no contraband
// (PIR-14), so its reserve there is 0; pirate markets do.
double reserve(const Config& c, const Planet& p, Good g);

// base x clamp((reserve / stock)^k, price.min, price.max); a market with no reserve
// (no population) sits at price.min.
double price(const Config& c, const Planet& p, Good g);
inline double ask(const Config& c, const Planet& p, Good g) { return price(c, p, g) * (1 + c.price.spread / 2); }
inline double bid(const Config& c, const Planet& p, Good g) { return price(c, p, g) * (1 - c.price.spread / 2); }

// Money the planet earns (sales and tax, ECO-3): into the treasury and its income
// counts (the credit limit's average, and the month's income that tribute is paid on).
inline void receive(Planet& p, double money) {
    p.treasury += money;
    p.income_today += money;
    p.income_month += money;
}

// A planet whose treasury is below its credit limit (econ.credit_limit months of
// income in the red) stops buying from traders.
bool can_buy(const Config& c, const Planet& p);

// The planet's own production at its current efficiency, events included (PRD-7,
// SYS-4): prod.efficiency by tier, one tier up from high is x event.bumper_mult, one
// tier down from low is low x (low / medium).
double efficiency(const Config& c, const Planet& p, int slot);

// What a trade moved: goods, and money paid (buying) or received (selling), tax
// included.
struct Fill {
    double qty = 0;
    double money = 0;
};

// A trader buys from the planet's stock, in chunks of price.chunk x reserve so the
// price walks (SYS-3): at most max_qty, spending at most budget, and only while a
// chunk costs no more than max_unit per unit, tax included. The planet's treasury takes
// the money. quote_buy works the same on a copy and changes nothing.
Fill buy(Planet& planet, const Config& c, Good g, double max_qty, double budget, double max_unit);
Fill quote_buy(Planet planet, const Config& c, Good g, double max_qty, double budget, double max_unit);

// A trader sells to the planet: up to qty, while a chunk brings at least min_unit per
// unit after tax and the planet is within its credit limit.
Fill sell(Planet& planet, const Config& c, Good g, double qty, double min_unit);

}  // namespace lattice::market
