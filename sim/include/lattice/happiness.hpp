#pragma once

#include <algorithm>
#include <cmath>

#include "lattice/config.hpp"

// Happiness's effects (PRD-4, PRD-5, SYS-1). Happiness is signed, 0 neutral; its effects
// grow with distance from neutral and diminish, through u(H) = tanh(|H| / happy.scale).
namespace lattice::happiness {

inline double u(const Config& c, double h) { return std::tanh(std::abs(h) / c.happy.scale); }

// u of unhappiness: 0 at neutral and above, toward 1 as happiness falls.
inline double misery(const Config& c, double h) { return h < 0 ? u(c, h) : 0; }

// Productivity's factor F(H): 1 at neutral, toward 1 + p_up and 1 - p_down.
inline double productivity(const Config& c, double h) {
    return h >= 0 ? 1 + c.happy.p_up * u(c, h) : 1 - c.happy.p_down * u(c, h);
}

// Growth's factor G(H): 1 at neutral; unhappiness cuts it, to nothing at g_down 1.
inline double growth(const Config& c, double h) {
    return h >= 0 ? 1 + c.happy.g_up * u(c, h) : std::max(0.0, 1 - c.happy.g_down * u(c, h));
}

// Contraband demand's factor (SYS-16): 1 when miserable, 1/2 at neutral, toward 0 when
// content. The signed form of what was (1 - H) on the old 0-to-1 scale.
inline double craving(const Config& c, double h) {
    const double s = h >= 0 ? u(c, h) : -u(c, h);
    return (1 - s) / 2;
}

}  // namespace lattice::happiness
