#include "lattice/rng.hpp"

#include <cmath>
#include <numbers>
#include <sstream>
#include <stdexcept>

namespace lattice {

namespace {

// SplitMix64, to turn (world seed, stream) into well-spread engine seeds.
std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

}  // namespace

double Rng::uniform01() {
    // The top 53 bits, so every value is exactly representable.
    return static_cast<double>(next() >> 11) * 0x1.0p-53;
}

std::int64_t Rng::uniform_int(std::int64_t lo, std::int64_t hi) {
    const std::uint64_t span = static_cast<std::uint64_t>(hi) - static_cast<std::uint64_t>(lo);
    if (span == UINT64_MAX) return static_cast<std::int64_t>(next());
    const std::uint64_t n = span + 1;
    // Reject the top partial block so every value is equally likely.
    const std::uint64_t limit = UINT64_MAX - UINT64_MAX % n;
    std::uint64_t r;
    do {
        r = next();
    } while (r >= limit);
    return static_cast<std::int64_t>(static_cast<std::uint64_t>(lo) + r % n);
}

double Rng::normal() {
    const double u1 = 1.0 - uniform01();  // (0, 1], so the log is finite
    const double u2 = uniform01();
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * std::numbers::pi * u2);
}

std::string Rng::state() const {
    std::ostringstream out;
    out << engine_;
    return out.str();
}

void Rng::set_state(const std::string& text) {
    std::istringstream in(text);
    in >> engine_;
    if (in.fail()) throw std::runtime_error("invalid RNG state");
}

RngStreams RngStreams::from_seed(std::uint64_t world_seed) {
    RngStreams r;
    for (std::size_t i = 0; i < r.streams.size(); ++i) {
        r.streams[i] = Rng(splitmix64(world_seed ^ splitmix64(i + 1)));
    }
    return r;
}

}  // namespace lattice
