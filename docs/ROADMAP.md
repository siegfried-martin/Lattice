# Roadmap

*Reset 2026-09-27; updated 2026-10-18.*

Done so far: the travel foundation (`WORLD_AND_TRAVEL.md`), a first combat prototype
(`COMBAT.md`), and four rounds of design, now formalised as the spec (`docs/spec/`).

**Current step:** the headless simulation, built in C++ in the order of
`docs/spec/sim/milestones.md` (MS-1 to MS-9): every v1 system first, then tuning until
the world is stable (Phase A), then experiments testing whether each mechanic earns its
cost (Phase B). Connecting the simulation to the rendered game follows.

MS-1, the skeleton and toolchain, is done: the core, runner, tests, saves and a Godot
binding the project loads. MS-2, world generation and the economy, and MS-3, characters,
movement, information and trade, are built. Next is MS-4, politics.
