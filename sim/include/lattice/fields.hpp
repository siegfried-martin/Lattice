#pragma once

#include <concepts>
#include <type_traits>

namespace lattice {

// Each saved record lists its fields once, in a fields(ar, record) function next to
// its definition; the save writer and reader both walk that list (ARC-9). R is the
// record type, possibly const.
template <class S, class R>
concept record_of = std::same_as<std::remove_const_t<S>, R>;

}  // namespace lattice
