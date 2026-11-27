#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace lattice {

// The v1 goods (PRD-1): three life support, eight luxuries (six regional, two from the
// fringe), ship parts (materiel, money round) and one contraband. The luxury and
// contraband names are placeholders (OPN-35). The values are stored in saves: changing them bumps kSaveVersion. Lawful
// goods come first, so 0 to kLawfulGoodCount - 1 are the lawful ones.
enum Good : std::uint8_t {
    kFood = 0,
    kWater = 1,
    kOxygen = 2,
    kWine = 3,
    kSpice = 4,
    kSilk = 5,
    kTea = 6,
    kPerfume = 7,
    kGlassware = 8,
    kCrystal = 9,  // fringe
    kAmber = 10,   // fringe
    kParts = 11,   // materiel: ships, munitions, static defence (PRD-1, ECO-6)
    kStims = 12,
    kGoodCount = 13,
};

enum class GoodKind : std::uint8_t { LifeSupport, Luxury, Materiel, Contraband };

inline constexpr int kLawfulGoodCount = 12;  // every good but contraband
inline constexpr std::array<Good, 3> kLifeSupport = {kFood, kWater, kOxygen};
inline constexpr std::array<Good, 8> kLuxuries = {kWine, kSpice, kSilk, kTea, kPerfume, kGlassware, kCrystal, kAmber};
inline constexpr std::array<Good, 6> kRegionalLuxuries = {kWine, kSpice, kSilk, kTea, kPerfume, kGlassware};
inline constexpr std::array<Good, 2> kFringeLuxuries = {kCrystal, kAmber};

constexpr GoodKind kind_of(Good g) {
    return g <= kOxygen ? GoodKind::LifeSupport : g <= kAmber ? GoodKind::Luxury : g == kParts ? GoodKind::Materiel : GoodKind::Contraband;
}

constexpr std::string_view good_name(Good g) {
    constexpr std::array<std::string_view, kGoodCount> names = {
        "food", "water", "oxygen", "wine", "spice", "silk", "tea", "perfume", "glassware", "crystal", "amber", "parts", "stims"};
    return names[g];
}

// A planet makes each of its three goods at one of these (PRD-7). Append only.
enum Tier : std::uint8_t { kHigh = 0, kMedium = 1, kLow = 2 };

}  // namespace lattice
