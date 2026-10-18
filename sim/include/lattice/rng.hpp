#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>

namespace lattice {

// One random stream per system, seeded from the world seed, so a new draw in one
// system doesn't shift the others (ARC-6). Append only: adding a stream in the
// middle would reseed every stream after it.
enum class Stream : std::uint8_t {
    WorldGen = 0,
    Debug = 1,
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
