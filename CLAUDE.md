# Working notes for AI-assisted sessions

Read `docs/VISION.md` first. It is the intent everything else serves.

## Engine

- Godot **4.7**, GDScript. Write Godot 4 APIs only: `Node3D` not `Spatial`, `await`
  not `yield`, `@export`, `instantiate()`, `Callable`-based `connect`.
- When unsure of an API, check the engine's own reference (`godot --doctool <dir>`
  dumps it) rather than recalling it.
- After adding a new `class_name`, run `godot --headless --path . --import` once so
  the class cache picks it up.

## How the code is organised

- **Scenes are shells.** Each `.tscn` is a root node plus a script, and everything is
  built in code. Nothing that matters should only be findable by opening a scene in
  the editor.
- `scripts/galaxy.gd` (autoload `Galaxy`) generates the whole world layout:
  sectors, systems, bodies, the Lattice roads, links and gates, hop lanes and
  asteroid clusters. Both worlds read from it.
- `scripts/main.gd` owns the ship, the camera and the HUD, and swaps between the open
  space world (`space_world.gd`) and the Lattice (`highway_world.gd`). In code the
  Lattice is still called the highway.
- Flight: `space_flight.gd` (free flight, both worlds) and `highway_drive.gd` (docked).
- Combat: `combat.gd` (child of the space world, world coordinates; tuning in `Combat.T`).
  `main.gd` owns the player's station (pilot / turret / missile) and applies combat events.

## Checking your work

- `make check` must pass before anything is called done.
- For anything visual, run `make tour` (or `make tour-hop`, `make tour-combat`) and look at the
  screenshots before asking the human to. They land in
  `~/.local/share/godot/app_userdata/Lattice/tour/`. The tour turns vsync off, so it
  keeps running while its window is hidden.
- Feel verdicts belong to the human. Build it and show it; don't report on how it
  feels.

## Git

- Work on a branch, `feat/…`, `fix/…` or `docs/…`, and merge through a PR.
- Update `docs/` in the same change when a design decision changes.
