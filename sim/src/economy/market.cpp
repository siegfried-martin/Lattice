#include "lattice/market.hpp"

#include <algorithm>
#include <cmath>

#include "lattice/happiness.hpp"

namespace lattice::market {

double reserve(const Config& c, const Planet& p, Good g) {
    const auto& gp = c.goods[g];
    switch (kind_of(g)) {
        case GoodKind::LifeSupport:
            return gp.reserve_days * p.population * gp.consumption;
        case GoodKind::Luxury:
            // An unhappy planet stocks more, so it bids more at the same stock (PRD-10, SYS-3).
            return gp.reserve_days * p.population * c.happy.need * (1 + c.happy.want * happiness::misery(c, p.happiness));
        case GoodKind::Contraband:
            return p.pirate ? gp.reserve_days * p.population * gp.consumption : 0;
    }
    return 0;
}

double price(const Config& c, const Planet& p, Good g) {
    const double r = reserve(c, p, g);
    const auto& gp = c.goods[g];
    if (r <= 0) return gp.base_price * c.price.min;
    const double stock = std::max(p.stock[g], r * 1e-9);
    return gp.base_price * std::clamp(std::pow(r / stock, gp.k), c.price.min, c.price.max);
}

bool can_buy(const Config& c, const Planet& p) {
    const double monthly_income = p.income_avg * static_cast<double>(c.time.payment_days);
    return p.treasury >= -c.econ.credit_limit * monthly_income;
}

double efficiency(const Config& c, const Planet& p, int slot) {
    const auto s = static_cast<std::size_t>(slot);
    const int tier = static_cast<int>(p.tiers[s]) + p.tier_shift[s];
    const auto& e = c.prod;
    if (tier <= -1) return e.efficiency_high * c.event.bumper_mult;
    if (tier == kHigh) return e.efficiency_high;
    if (tier == kMedium) return e.efficiency_medium;
    if (tier == kLow) return e.efficiency_low;
    return e.efficiency_low * e.efficiency_low / e.efficiency_medium;
}

Fill buy(Planet& p, const Config& c, Good g, double max_qty, double budget, double max_unit) {
    Fill f;
    const double r = reserve(c, p, g);
    if (r <= 0) return f;
    const double chunk = c.price.chunk * r;
    while (f.qty < max_qty - 1e-9) {
        const double unit = ask(c, p, g) * (1 + p.tax_rate);
        if (unit > max_unit) break;
        double q = std::min({chunk, max_qty - f.qty, p.stock[g], (budget - f.money) / unit});
        if (q <= 1e-9) break;
        p.stock[g] -= q;
        receive(p, q * unit);
        f.qty += q;
        f.money += q * unit;
    }
    return f;
}

Fill quote_buy(Planet p, const Config& c, Good g, double max_qty, double budget, double max_unit) {
    return buy(p, c, g, max_qty, budget, max_unit);
}

Fill sell(Planet& p, const Config& c, Good g, double qty, double min_unit) {
    Fill f;
    const double r = reserve(c, p, g);
    if (r <= 0) return f;
    const double chunk = c.price.chunk * r;
    while (f.qty < qty - 1e-9 && can_buy(c, p)) {
        const double unit = bid(c, p, g) * (1 - p.tax_rate);
        if (unit < min_unit) break;
        const double q = std::min(chunk, qty - f.qty);
        p.stock[g] += q;
        p.treasury -= q * unit;  // pays the bid and keeps the tax (ECO-3)
        f.qty += q;
        f.money += q * unit;
    }
    return f;
}

}  // namespace lattice::market
