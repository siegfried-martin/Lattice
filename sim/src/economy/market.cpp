#include "lattice/market.hpp"

#include <algorithm>
#include <cmath>

namespace lattice::market {

double reserve(const Config& c, const Planet& p, Good g) {
    const auto& gp = c.goods[g];
    switch (kind_of(g)) {
        case GoodKind::LifeSupport:
            return gp.reserve_days * p.population * gp.consumption;
        case GoodKind::Luxury:
            return gp.reserve_days * p.population * c.lux.consumption / static_cast<double>(kLuxuries.size());
        case GoodKind::Contraband:
            return 0;
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

}  // namespace lattice::market
