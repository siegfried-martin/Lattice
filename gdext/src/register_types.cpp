#include <gdextension_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "lattice_sim.hpp"

using namespace godot;

namespace {

void initialize(ModuleInitializationLevel level) {
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
    GDREGISTER_CLASS(LatticeSim);
}

void uninitialize(ModuleInitializationLevel) {}

}  // namespace

extern "C" {

// The entry_symbol named in bin/lattice.gdextension.
GDExtensionBool GDE_EXPORT lattice_library_init(GDExtensionInterfaceGetProcAddress get_proc_address,
                                                GDExtensionClassLibraryPtr library,
                                                GDExtensionInitialization* initialization) {
    GDExtensionBinding::InitObject init(get_proc_address, library, initialization);
    init.register_initializer(initialize);
    init.register_terminator(uninitialize);
    init.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init.init();
}

}
