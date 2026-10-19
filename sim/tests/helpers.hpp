#pragma once

#include "lattice/config.hpp"

namespace lattice::test {

// A development world with few characters, so year-long runs stay quick under the
// sanitizers.
inline Config small() {
    Config c;
    c.chars.target = 100;
    c.gen.commanders_per_income = 3e-6;  // a commander or so per planet
    return c;
}

}  // namespace lattice::test
