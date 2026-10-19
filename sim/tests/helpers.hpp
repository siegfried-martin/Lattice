#pragma once

#include "lattice/config.hpp"

namespace lattice::test {

// A development world with few characters, so year-long runs stay quick under the
// sanitizers.
inline Config small() {
    Config c;
    c.chars.target = 60;
    return c;
}

}  // namespace lattice::test
