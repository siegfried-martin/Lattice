#pragma once

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "lattice/world.hpp"

namespace lattice {

// Saves are versioned binary holding the whole world (ARC-9). Bump this whenever the
// layout of the world section changes; a build reads only its own version. Config
// fields are stored by name, so adding a tunable doesn't need a bump.
inline constexpr std::uint32_t kSaveVersion = 12;

class SaveError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

std::vector<std::uint8_t> save_world(const World& world);
World load_world(std::span<const std::uint8_t> bytes);

void save_world_file(const World& world, const std::string& path);
World load_world_file(const std::string& path);

// A hash of the saved bytes: equal hashes mean equal worlds, for determinism tests.
std::uint64_t state_hash(const World& world);

}  // namespace lattice
