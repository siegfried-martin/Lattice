GODOT ?= godot

.PHONY: run tour tour-hop tour-combat tour-jct tour-collide tour-land check check-godot editor \
	sim sim-test sim-release gdext

run:
	$(GODOT) --path .

# Scripted fly-through (space -> Lattice -> dock -> junction -> exit -> hop lane).
# Screenshots land in ~/.local/share/godot/app_userdata/Lattice/tour/.
tour:
	$(GODOT) --path . -- --tour

# Combat: drone, turret, blocker, missile, a hostile freighter and fighter.
tour-combat:
	$(GODOT) --path . -- --tour --combat

# Free flight through junction gates: HWY 1 onto HWY 2, then HWY 2's end onto HWY 1.
tour-jct:
	$(GODOT) --path . -- --tour --jct

# Ships forced into each other in open space, then docked Lattice traffic at 4x; prints overlaps.
tour-collide:
	$(GODOT) --path . -- --tour --collide

# Landing menu and refuelling at a station and a planet, moons, asteroid hits, running out of Lattice fuel.
tour-land:
	$(GODOT) --path . -- --tour --land

# Just the hop-lane part of the tour.
tour-hop:
	$(GODOT) --path . -- --tour --hop-only

# Everything that must pass before a change is done: the simulation's tests, the
# binding, and the Godot project.
check: sim-test gdext check-godot
	@out=$$($(GODOT) --headless --path . --script res://tests/sim_binding.gd 2>&1); \
	echo "$$out" | grep "^sim binding:"; echo "$$out" | grep -q "^sim binding: ok" || exit 1
	@echo "check: ok"

# Parse every script and build both worlds headless; fails on any script error.
check-godot:
	@out=$$($(GODOT) --headless --path . --quit-after 30 2>&1); \
	echo "$$out" | grep -E "SCRIPT ERROR|Parse Error|Failed to load" && exit 1; \
	echo "$$out" | grep "^Galaxy:"; echo "check-godot: ok"

# The simulation core (docs/spec/sim), built with ASan and UBSan, and its tests.
sim:
	@cmake --preset dev -S sim >/dev/null
	@cmake --build sim/build/dev -- --quiet

sim-test: sim
	@sim/build/dev/lattice-tests --no-version

# The optimised runner, for real runs: sim/build/release/lattice-run --help
sim-release:
	@cmake --preset release -S sim >/dev/null
	@cmake --build sim/build/release -- --quiet

# The Godot binding, into bin/. The first build compiles godot-cpp (a few minutes).
# The extension is registered in .godot/extension_list.cfg here rather than left for
# Godot to discover: Godot 4.7 crashes on exit in the session that first discovers a
# godot-cpp extension.
gdext:
	@test -f gdext/godot-cpp/CMakeLists.txt || git submodule update --init gdext/godot-cpp
	@test -f gdext/build/debug/build.ninja || cmake --preset debug -S gdext >/dev/null
	@cmake --build gdext/build/debug -- --quiet
	@mkdir -p .godot; grep -qx "res://bin/lattice.gdextension" .godot/extension_list.cfg 2>/dev/null \
		|| echo "res://bin/lattice.gdextension" >> .godot/extension_list.cfg

editor:
	$(GODOT) --path . --editor
