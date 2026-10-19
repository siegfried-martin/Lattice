# Working notes for AI-assisted sessions

Read `docs/VISION.md` first. It is the intent everything else serves.

## The prototype and the plan

- `docs/spec/` is the source of truth for the full game: `design/` holds the rules and
  `sim/` the simulation spec. Every rule has a stable ID (`FAC-8`, `SYS-3`); cite IDs in
  code comments, tests and commits. Start from `docs/spec/README.md`, which indexes
  everything and explains the tags (`v1`, `later`, `proposed`, `open`, …).
- `docs/planning/` holds the planning agent's ideation drafts, which aren't
  authoritative. Once a round is settled, write its decisions into `docs/spec/` with IDs,
  log them in `docs/spec/DECISIONS.md`, and delete the drafts. See
  `docs/planning/README.md`.
- The other files directly in `docs/` describe the current prototype (POC) and the brief.
  The POC existed to prove out the feel of travel and combat. Its conveniences (unlimited
  missiles, numpad ship switching, starting in a freighter, the small fixed 4×4 map) are
  testing scaffolding, not design.
- Don't read a POC shortcut as a design decision, and don't propose prototyping systems
  whose feel isn't in question (a market screen, say) ahead of the plan.

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

## The simulation

- `sim/` is the C++20 core (`docs/spec/sim/`), with no Godot types; `gdext/` is the
  thin binding, which builds into `bin/`. Both build with CMake and Ninja.
- `make sim-test` builds and runs the tests; `make sim-release` builds the runner,
  `sim/build/release/lattice-run --help`. The default config is a development world;
  `--config sim/configs/v1.toml` runs a v1-size one.
- Keep it deterministic (ARC-5, ARC-6): iterate only vectors or `std::map`, draw random
  numbers from the system's own stream, and schedule through the event queue.
- A new record field or event kind changes the save layout: add it to the record's
  `fields()` list (and to the save's reference checks for an event kind), and bump
  `kSaveVersion`. A new tunable goes in `Config`, `for_each_field`, `validate` and
  `tunables.md`.
- Run `make gdext` before opening the editor on a fresh checkout (ARC-4).

## Checking your work

- `make check` must pass before anything is called done. It runs the simulation's tests
  under ASan and UBSan, builds the Godot binding, loads it from Godot, and runs the
  project headless. The first run compiles godot-cpp, which takes a few minutes.
- For anything visual, run `make tour` (or `make tour-hop`, `make tour-combat`, `make tour-jct`, `make tour-collide`, `make tour-land`) and look at the
  screenshots before asking the human to. They land in
  `~/.local/share/godot/app_userdata/Lattice/tour/`. The tour turns vsync off, so it
  keeps running while its window is hidden.
- Feel verdicts belong to the human. Build it and show it; don't report on how it
  feels.

## Git

- Work on a branch, `feat/…`, `fix/…` or `docs/…`, and merge through a PR.
- Update `docs/` in the same change when a design decision changes: the rule in
  `docs/spec/`, plus a line in `docs/spec/DECISIONS.md`.
  `docs/BRIEF.md` is the code-free summary the human uses for brainstorming; keep it current too.
