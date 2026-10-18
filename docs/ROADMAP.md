# Roadmap

*Reset 2026-09-27; updated 2026-10-18.*

Done so far: the travel foundation (`WORLD_AND_TRAVEL.md`), a first combat prototype
(`COMBAT.md`), and four rounds of design, now formalised as the spec (`docs/spec/`).

**Current step:** the headless simulation, built in C++ in the order of
`docs/spec/sim/milestones.md` (MS-1 to MS-9): every v1 system first, then tuning until
the world is stable (Phase A), then experiments testing whether each mechanic earns its
cost (Phase B). Connecting the simulation to the rendered game follows.

MS-1, the skeleton and toolchain, is done: the core, runner, tests, saves and a Godot
binding the project loads. MS-2 to MS-7 are built: world generation and the economy,
characters, movement, information and trade, politics, war, piracy and suspicion, and
spies. Every v1 system runs. MS-8 is in progress: the experiment harness is built, and
Phase A has had three rounds. Round 2 added war pacing, rebellion and gate blockades.
Round 3 rebuilt happiness and productivity on a signed scale, eight regional and fringe
luxuries, rebellion as a weekly chance, garrisoning as a choice, and defence sized to
income; its results and what still fails are in `sim/milestones.md` (MS-8). Round 4,
written into the spec and not built yet, makes production cost money, with efficiency
as that cost.
