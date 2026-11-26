#include "chars/ships.hpp"

#include <algorithm>
#include <cmath>

namespace lattice::ships {

void make_catalogue(World& w) {
    const auto& s = w.config.ship;
    Rng& rng = w.rng[Stream::WorldGen];
    for (Id culture = 0; culture < static_cast<Id>(w.map.regions.size()); ++culture) {
        for (int k = 0; k < kShipClassCount; ++k) {
            const auto& base = s.classes[static_cast<std::size_t>(k)];
            // A culture's flavour: each stat within +- ship.culture_spread, and the price
            // following the stats' average.
            const double fc = 1 + s.culture_spread * rng.uniform(-1, 1);
            const double fg = 1 + s.culture_spread * rng.uniform(-1, 1);
            const double fs = 1 + s.culture_spread * rng.uniform(-1, 1);
            const double fp = (fc + fg + fs) / 3;
            // With a Lattice Drive, then without (TRD-18): cheaper, with the drive's
            // space for cargo.
            for (const bool drive : {true, false}) {
                for (int tier = 1; tier <= kTierCount; ++tier) {
                    const double up = tier - 1;
                    w.map.variants.push_back({
                        .culture = culture,
                        .cls = static_cast<ShipClass>(k),
                        .tier = static_cast<std::int8_t>(tier),
                        .combat = base.combat * fc * std::pow(s.tier_combat, up),
                        .cargo = base.cargo * fg * std::pow(s.tier_cargo, up) * (drive ? 1 : s.no_drive_cargo),
                        .speed = base.speed * fs * std::pow(s.tier_speed, up),
                        .price = base.price * fp * std::pow(s.tier_price, up) * (drive ? 1 : s.no_drive_price),
                        .lattice_drive = drive,
                    });
                }
            }
        }
    }
}

Id variant(const Map&, Id culture, ShipClass cls, int tier, bool drive) {
    return ((culture * kShipClassCount + static_cast<Id>(cls)) * 2 + (drive ? 0 : 1)) * kTierCount + (tier - 1);
}

double upkeep(const Config& c, const ShipVariant& v) {
    const double crew = c.ship.classes[static_cast<std::size_t>(v.cls)].crew * c.fleet.crew_tier[static_cast<std::size_t>(v.tier - 1)];
    return v.price * c.fleet.maintenance / 365 + crew * c.fleet.crew_wage;
}

Fleet fleet(const World& w, const Character& c) {
    Fleet f;
    f.lattice_drive = !c.ships.empty();
    for (Id id : c.ships) {
        const ShipVariant& v = w.map.variants[static_cast<std::size_t>(id)];
        f.combat += v.combat;
        f.cargo += v.cargo;
        f.speed = f.count == 0 ? v.speed : std::min(f.speed, v.speed);
        f.run_cost += upkeep(w.config, v);
        f.value += v.price;
        f.lattice_drive = f.lattice_drive && v.lattice_drive;
        ++f.count;
    }
    return f;
}

double buy_cost(const Config& c, const ShipVariant& v) { return v.price * (1 + c.ship.insurance_premium); }

double cover_of(const Config& c, const ShipVariant& v) { return v.price * c.ship.insurance_payout; }

void insure(const World& w, Character& c, Id variant) { c.cover += cover_of(w.config, w.map.variants[static_cast<std::size_t>(variant)]); }

void lapse(const World& w, Character& c, Id variant) {
    c.cover = std::max(0.0, c.cover - cover_of(w.config, w.map.variants[static_cast<std::size_t>(variant)]));
}

double claim(World& w, Character& c, Id variant) {
    const double paid = std::min(c.cover, cover_of(w.config, w.map.variants[static_cast<std::size_t>(variant)]));
    if (paid <= 0) return 0;
    c.cover -= paid;
    c.wealth += paid;
    w.ledger.insurance_paid += paid;
    w.ledger.free_money += paid;
    ++w.ledger.claims;
    return paid;
}

}  // namespace lattice::ships
