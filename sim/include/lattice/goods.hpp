#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace lattice {

// The v1 goods (PRD-1): three life support, three luxuries and one contraband. The
// luxury and contraband names are flavour. Append only: the values are stored in saves.
enum Good : std::uint8_t {
    kFood = 0,
    kWater = 1,
    kOxygen = 2,
    kWine = 3,
    kSpice = 4,
    kSilk = 5,
    kStims = 6,
    kGoodCount = 7,
};

enum class GoodKind : std::uint8_t { LifeSupport, Luxury, Contraband };

inline constexpr int kLawfulGoodCount = 6;  // every good but contraband
inline constexpr std::array<Good, 3> kLifeSupport = {kFood, kWater, kOxygen};
inline constexpr std::array<Good, 3> kLuxuries = {kWine, kSpice, kSilk};

constexpr GoodKind kind_of(Good g) {
    return g <= kOxygen ? GoodKind::LifeSupport : g <= kSilk ? GoodKind::Luxury : GoodKind::Contraband;
}

constexpr std::string_view good_name(Good g) {
    constexpr std::array<std::string_view, kGoodCount> names = {"food", "water", "oxygen", "wine",
                                                                "spice", "silk", "stims"};
    return names[g];
}

// A planet makes each of its three goods at one of these (PRD-7). Append only.
enum Tier : std::uint8_t { kHigh = 0, kMedium = 1, kLow = 2 };

}  // namespace lattice
