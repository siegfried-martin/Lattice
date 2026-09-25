GODOT ?= godot

.PHONY: run tour tour-hop tour-combat check editor

run:
	$(GODOT) --path .

# Scripted fly-through (space -> Lattice -> dock -> junction -> exit -> hop lane).
# Screenshots land in ~/.local/share/godot/app_userdata/Lattice/tour/.
tour:
	$(GODOT) --path . -- --tour

# Combat: drone, turret, blocker, missile, a hostile freighter and fighter.
tour-combat:
	$(GODOT) --path . -- --tour --combat

# Just the hop-lane part of the tour.
tour-hop:
	$(GODOT) --path . -- --tour --hop-only

# Parse every script and build both worlds headless; fails on any script error.
check:
	@out=$$($(GODOT) --headless --path . --quit-after 30 2>&1); \
	echo "$$out" | grep -E "SCRIPT ERROR|Parse Error|Failed to load" && exit 1; \
	echo "$$out" | grep "^Galaxy:"; echo "check: ok"

editor:
	$(GODOT) --path . --editor
