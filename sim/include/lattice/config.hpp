#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace lattice {

// Every tunable the simulation reads (ARC-7). Defaults are the starting values in
// docs/spec/sim/tunables.md; keep the two in step. Keys are added as milestones need
// them.
struct Config {
    struct Clock {
        double real_seconds_per_day = 60;     // rendered clock (WLD-15)
        std::int64_t space_step_minutes = 10;  // open-space step (TIM-6)
        std::int64_t battle_round_hours = 1;   // SYS-14
        std::int64_t labour_days = 7;          // cadences (TIM-3)
        std::int64_t budget_days = 7;
        std::int64_t diplomacy_days = 7;
        std::int64_t payment_days = 30;

        friend bool operator==(const Clock&, const Clock&) = default;
    } time;

    struct Debug {
        // Records that ping at random intervals, to exercise the event queue and the
        // RNG streams before real systems exist. Scaffolding; 0 in real runs.
        std::int64_t pings = 0;

        friend bool operator==(const Debug&, const Debug&) = default;
    } debug;

    friend bool operator==(const Config&, const Config&) = default;
};

// Calls f(key, field) for every field, in a fixed order. Loading, writing and saving
// all go through this list, so a new field needs adding only here.
template <class C, class F>
void for_each_field(C& c, F&& f) {
    f("time.real_seconds_per_day", c.time.real_seconds_per_day);
    f("time.space_step_minutes", c.time.space_step_minutes);
    f("time.battle_round_hours", c.time.battle_round_hours);
    f("time.labour_days", c.time.labour_days);
    f("time.budget_days", c.time.budget_days);
    f("time.diplomacy_days", c.time.diplomacy_days);
    f("time.payment_days", c.time.payment_days);
    f("debug.pings", c.debug.pings);
}

class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Missing keys keep their defaults. Unknown keys, wrong types and invalid values throw
// ConfigError, so a typo in a config file can't pass silently.
Config parse_config(std::string_view toml_text, std::string_view source = "config");
Config load_config_file(const std::string& path);
void validate(const Config& config);

// The full config, defaults included, as TOML (written into each run's output, ARC-7).
std::string to_toml(const Config& config);

}  // namespace lattice
