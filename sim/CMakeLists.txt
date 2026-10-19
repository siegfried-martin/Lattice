# The Lattice simulation core (ARC-1), its headless runner and tests (ARC-3).
# The Godot binding in ../gdext adds this directory with the runner and tests off.
cmake_minimum_required(VERSION 3.22)
project(lattice_sim LANGUAGES CXX)

option(LATTICE_BUILD_TESTS "Build the doctest unit tests" ${PROJECT_IS_TOP_LEVEL})
option(LATTICE_BUILD_RUNNER "Build the headless runner" ${PROJECT_IS_TOP_LEVEL})
option(LATTICE_SANITIZE "Build with ASan and UBSan (ARC-3)" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Flags every Lattice target gets. Determinism (ARC-6): no fast-math, and no FMA
# contraction, so the runner, the tests and the binding compute the same results.
add_library(lattice_options INTERFACE)
target_compile_features(lattice_options INTERFACE cxx_std_20)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lattice_options INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -ffp-contract=off -fno-fast-math)
    if(LATTICE_SANITIZE)
        target_compile_options(lattice_options INTERFACE
            -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
        target_link_options(lattice_options INTERFACE -fsanitize=address,undefined)
    endif()
elseif(MSVC)
    target_compile_options(lattice_options INTERFACE /W4 /fp:precise /permissive-)
endif()

add_library(lattice_core STATIC
    src/core/config.cpp
    src/core/event_queue.cpp
    src/core/rng.cpp
    src/core/save.cpp
    src/core/travel.cpp
    src/core/world.cpp
    src/economy/economy.cpp
    src/economy/events.cpp
    src/economy/market.cpp
    src/gen/worldgen.cpp
    src/trade/osmosis.cpp
)
set_target_properties(lattice_core PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(lattice_core PUBLIC include PRIVATE src)
target_include_directories(lattice_core SYSTEM PRIVATE third_party/tomlplusplus)
target_link_libraries(lattice_core PUBLIC lattice_options)

if(LATTICE_BUILD_RUNNER)
    add_executable(lattice-run runner/main.cpp runner/csv_observer.cpp)
    set_target_properties(lattice-run PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(lattice-run PRIVATE lattice_core)
endif()

if(LATTICE_BUILD_TESTS)
    enable_testing()
    add_executable(lattice-tests
        tests/main.cpp
        tests/test_config.cpp
        tests/test_event_queue.cpp
        tests/test_rng.cpp
        tests/test_save.cpp
        tests/test_world.cpp
        tests/test_worldgen.cpp
        tests/test_economy.cpp
    )
    set_target_properties(lattice-tests PROPERTIES CXX_EXTENSIONS OFF)
    target_include_directories(lattice-tests SYSTEM PRIVATE third_party/doctest)
    target_include_directories(lattice-tests PRIVATE src)
    target_compile_definitions(lattice-tests PRIVATE LATTICE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(lattice-tests PRIVATE lattice_core)
    add_test(NAME lattice-tests COMMAND lattice-tests)
endif()
