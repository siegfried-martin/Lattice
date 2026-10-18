# Simulation (SIM)

The headless simulation that runs the world. The design rules it implements are in
`../design/`; this folder says how v1 implements them. Every formula and number here is
a starting point for tuning.

## SIM-1 What the simulation is `v1`

The world exists and acts whether or not it's rendered (VISION). The simulation is
authoritative: it owns every planet, character, fleet, market and fact. The rendered game
is a window onto the part near the player, which hands entities to the rendered world
and takes them back (ARC-13). The player is one more character, acting through commands
(CHR-17).

## SIM-2 The goals `v1`

**Phase A: stability.** Tune values until the world is stable and every mechanic works
roughly as intended. "Stable and working" is defined by the checks in `checks.md`, which
merge the "what a headless prototype should check" lists of all the drafts.

**Phase B: does each mechanic earn its cost?** Add deliberate variance: switch one
mechanic off for one faction (stop raiding enemy merchants, stop scouting, stop sending
couriers, no spies), run against an unchanged control over many seeds, and compare
outcomes (`experiments.md`). A faction that skips a mechanic and does just as well shows
the mechanic isn't worth its cost: tweak it, or remove it if it can't be made to matter.

**All parts before judging.** The simulation gives no useful data until all its parts
run. Milestones (`milestones.md`) set the build order, but Phase A starts only when every
v1 system runs, and verdicts are taken at full v1 scale (WLD-3). A small world with three
factions is fine for building.

## SIM-3 v1 scope `v1`

**In:**
- the economy: v1 goods, population, production and labour, prices, efficiency events,
  treasuries, taxes, operating costs, sinks, fixed fuel prices (PRD, ECO);
- trade: independent and faction merchants, trip shapes, escorts, osmosis, the risk
  number (TRD);
- characters: generation, careers, contracts, skills, fleet power, running costs, pods
  (CHR, REL);
- politics: fealty trees, tribute, patronage, keep-or-grant, choosing governors,
  relationships and deeds, defection, the Trade Authority and its dues (FAC);
- war: diplomacy states, armies, invasions, sieges, calls for help, off-screen combat,
  planetary defence, security missions (DIP, MIL);
- information: per-character facts, sharing within factions, couriers (mostly
  automated), scouts, missions travelling by courier (INF);
- piracy: the pirate faction, pirate planets, broke commanders, raids and loot, the
  pirate relation, smugglers, policing (PIR);
- suspicion and spies, spies last (INF-13, INF-14);
- world generation (GEN).

**Out of v1:** prisoners (never), academy worlds (dropped), aliens, the full supply chain and
industrial goods, fuel as cargo, facilities and efficiency investment, loans, the
information market, Mk variants, faction stations, danger throttling, calibration
against rendered combat. Each is tagged `later` in the design or listed in
`DECISIONS.md`.

**Rendered-only, not simulated:** ambient ships (TRD-15), escort doctrine (TRD-11), ramp
boosts (WLD-9), bailing out (the player), pod flight in range, a commanderless fleet's
behaviour (MIL-14). The simulation exposes what they read (regional risk, flows).

## SIM-4 Three layers `v1`

1. **Strategic:** intent, fealty, diplomacy, conquest decisions. Changes rarely. Fully
   headless.
2. **Operational:** characters, armies and merchants acting on the travel graph (the
   Lattice with gate permissions as an edge filter, hop lanes, open-space sectors).
   Headless.
3. **Tactical:** combat and flight, rendered only near the player.

The sensor-range handoff is the boundary between layers 2 and 3 (ARC-13). Intent never
leaves the simulation.

## Index

| File | Prefix | Covers |
|---|---|---|
| `architecture.md` | ARC | C++ core, Godot binding, determinism, config, saves, metrics, toolchain |
| `model.md` | MOD | Records and fields |
| `time.md` | TIM | Clock, event queue, cadences, movement, encounters |
| `systems.md` | SYS | Each system's state, step and formulas |
| `worldgen.md` | GEN | World generation |
| `tunables.md` | TUN | Config keys and starting values |
| `checks.md` | CHK | Phase A checks |
| `experiments.md` | EXP | Phase B experiments |
| `milestones.md` | MS | Build order |
