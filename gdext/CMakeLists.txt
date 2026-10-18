# The Godot binding (ARC-4): a GDExtension that wraps the simulation core. Built with
# godot-cpp's CMake support, targeting the Godot 4.7 API. The library lands in ../bin,
# next to lattice.gdextension, where the Godot project loads it.
cmake_minimum_required(VERSION 3.22)
project(lattice_gdext LANGUAGES CXX)

set(GODOTCPP_API_VERSION "4.7" CACHE STRING "Godot API version to target")
# The core reports bad configs and saves with exceptions, caught here.
set(GODOTCPP_DISABLE_EXCEPTIONS OFF CACHE BOOL "")
# Our strict warnings are for our code, not godot-cpp's headers.
set(GODOTCPP_SYSTEM_HEADERS ON CACHE BOOL "")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(godot-cpp)
add_subdirectory(../sim sim)

add_library(lattice_gdext SHARED
    src/lattice_sim.cpp
    src/register_types.cpp
)
target_link_libraries(lattice_gdext PRIVATE godot-cpp lattice_core)

get_target_property(GODOTCPP_SUFFIX godot-cpp GODOTCPP_SUFFIX)
set(LATTICE_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../bin")
set_target_properties(lattice_gdext
    PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_VISIBILITY_PRESET hidden
        LIBRARY_OUTPUT_DIRECTORY "$<1:${LATTICE_BIN_DIR}>"
        RUNTIME_OUTPUT_DIRECTORY "$<1:${LATTICE_BIN_DIR}>"
        PDB_OUTPUT_DIRECTORY "$<1:${LATTICE_BIN_DIR}>"
        PREFIX "lib"
        OUTPUT_NAME "lattice${GODOTCPP_SUFFIX}"
)
