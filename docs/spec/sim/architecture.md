# Architecture (ARC)

How the simulation is built and how it talks to Godot. Chosen on 2026-10-18 after
researching Rust (gdext), C++ (godot-cpp), compiled Python and staying in GDScript.

### ARC-1 C++20 core with no engine types `v1`

The simulation core is **plain C++20 and the standard library**, with no Godot types,
built with **CMake as a static library**. It knows nothing about rendering.

*Why:* godot-cpp is the official binding and the toolchains are mature on Windows, the
main ship target. Typed GDScript is roughly 40–100× slower for this kind of work. gdext
(Rust) is 0.x with breaking changes and few shipped games (REJ-27).

### ARC-2 Records are integer IDs in vectors `v1`

Every record (MOD) lives in a vector and is referred to by an integer ID, never by
pointer. *(proposed)* IDs are never reused within a world, so a stale ID in a fact or an
event can't silently point at a new record; a dead record keeps its slot with an
`alive = false` flag.

### ARC-3 Runner and tests link the core `v1`

- A **headless command-line runner** loads a config and a seed (or a save), runs N days
  and writes metrics (ARC-8).
- **doctest** unit tests link the same library. On Linux, tests run under **ASan and
  UBSan**.
- `make check` grows a target that builds and runs the core's tests.

### ARC-4 The Godot binding is thin `v1`

A **GDExtension built with godot-cpp v10**, targeting `api_version` 4.7 (extensions are
forward-compatible across 4.x minors), wraps the core:

- **in:** `advance(dt)` and player commands (CHR-17);
- **out:** per-frame bulk reads as packed arrays (for example, the ships in the current
  sector with IDs, positions and types), event-driven UI reads as a Dictionary, and an
  event list since the last call;
- **the simulation never calls the engine.**

The binding is built in milestone 0 to prove the toolchain (MS-1), and connected
properly to the rendered game after tuning.

*Note:* on Windows a loaded extension's PDB is locked under the debugger, hot reload is
flaky, and new classes need an editor restart. Plan for restarts.

### ARC-5 One world, one thread, deterministic `v1`

A world runs on one thread. **Given the same seed, config and command log on the same
platform, it produces the same result.** Parallelism comes from running many worlds at
once (one per core) for experiments (EXP).

### ARC-6 Determinism rules `v1`

- **Iterate only vectors or `std::map`.** Never iterate an `unordered_map` (lookups are
  fine).
- **No fast-math** compiler flags.
- **Random numbers:** a standard engine (`std::mt19937_64`), *(proposed)* one stream per
  system seeded from the world seed, so adding a draw in one system doesn't shift every
  other. Standard distribution helpers are implementation-defined, which only matters for
  regenerating the same world across platforms; the world is stored in saves (ARC-9), so
  it doesn't arise. If cross-platform reproduction is ever needed, replace the two or
  three distributions used with small portable ones.
- **Event ties** break by a sequence number (TIM-2).

### ARC-7 Config `v1`

Every tunable (`tunables.md`) is read from a **config file loaded at startup** (TOML or
JSON), with the defaults in `tunables.md`. A run's config is written into its metrics
output so results are reproducible. Experiment switches (EXP) are config too.

### ARC-8 Metrics `v1`

The runner writes **CSV**: one row per day (or per N days) per tracked series (per
planet, per faction, world totals), plus an event log. A **Python script** plots runs and
compares experiment groups. The series are listed by check (`checks.md`).

### ARC-9 Saves `v1`

**Versioned binary from day one.** A save holds the whole world, including the generated
map, not a seed to regenerate it from.

### ARC-10 World generation lives in C++ `v1`

The simulation generates the world (`worldgen.md`). The rendered game reads the map from
the simulation through the binding, replacing `scripts/galaxy.gd`'s generation, when
the binding is connected.

### ARC-11 Toolchain `v1`

- **Linux (now):** `build-essential cmake ninja-build ccache python3-venv`, and SCons
  through pip for godot-cpp. Installing needs the human's go-ahead.
- **Windows (later):** GitHub Actions jobs for Linux and Windows; MinGW cross-compiling
  or VS Build Tools. Official Godot builds use MinGW.

### ARC-12 Code layout `v1` `proposed`

```
sim/                    C++ core (static library), runner and tests
  CMakeLists.txt
  include/lattice/      public headers: World, Config, commands, snapshots
  src/                  one directory per system (SYS)
  runner/               headless CLI
  tests/                doctest
  tools/                Python plotting
gdext/                  godot-cpp binding (SCons), links the core
```

### ARC-13 Handing entities to the rendered world `later`

The simulation stays authoritative. The area around the player (current sector plus
neighbours) is a view that reads simulation entities, runs real physics and combat, and
writes results back: damage, lost cargo, deaths, positions. "One brain, two bodies": the
same decision logic drives rendered steering near the player and cheap kinematics away
from it, so handing an entity over is spawning it where it already is. *(open)* The size
of the manifested area and the handoff details (OPN-26).
