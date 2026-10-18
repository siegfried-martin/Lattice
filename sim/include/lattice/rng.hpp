#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <iterator>
#include <string>
#include <utility>

namespace lattice {

// One random stream per system, seeded from the world seed, so a new draw in one
// system doesn't shift the others (ARC-6). Append only: adding a stream in the
// middle would reseed every stream after it.
enum class Stream : std::uint8_t {
    WorldGen = 0,
    Debug = 1,
    Events = 2,  // world events (SYS-4)
    Count
};

// A 64-bit engine with distributions written here rather than taken from <random>,
// whose distributions differ between standard libraries.
class Rng {
public:
    Rng() = default;
    explicit Rng(std::uint64_t seed) : engine_(seed) {}

    std::uint64_t next() { return engine_(); }
    // Uniform in [0, 1).
    double uniform01();
    // Uniform in [lo, hi], inclusive. Requires lo <= hi.
    std::int64_t uniform_int(std::int64_t lo, std::int64_t hi);
    // Uniform in [lo, hi).
    double uniform(double lo, double hi) { return lo + (hi - lo) * uniform01(); }
    bool chance(double p) { return uniform01() < p; }
    // Standard normal, by Box-Muller (one value per call).
    double normal();
    // An index drawn with probability proportional to weights[i]. Requires a positive
    // total; zero weights are never drawn.
    template <class Weights>
    std::size_t weighted(const Weights& weights) {
        double total = 0;
        for (double w : weights) total += w;
        double r = uniform01() * total;
        std::size_t last = 0;
        for (std::size_t i = 0; i < std::size(weights); ++i) {
            if (weights[i] <= 0) continue;
            last = i;
            if (r < weights[i]) return i;
            r -= weights[i];
        }
        return last;  // rounding at the top end
    }
    // Fisher-Yates.
    template <class Vec>
    void shuffle(Vec& v) {
        for (std::size_t i = std::size(v); i > 1; --i) {
            const auto j = static_cast<std::size_t>(uniform_int(0, static_cast<std::int64_t>(i) - 1));
            std::swap(v[i - 1], v[j]);
        }
    }

    // The engine state as text; the format is fixed by the standard.
    std::string state() const;
    void set_state(const std::string& text);

    friend bool operator==(const Rng&, const Rng&) = default;

private:
    std::mt19937_64 engine_;
};

struct RngStreams {
    std::array<Rng, static_cast<std::size_t>(Stream::Count)> streams;

    static RngStreams from_seed(std::uint64_t world_seed);
    Rng& operator[](Stream s) { return streams[static_cast<std::size_t>(s)]; }

    friend bool operator==(const RngStreams&, const RngStreams&) = default;
};

}  // namespace lattice
