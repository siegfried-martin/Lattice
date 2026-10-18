# Milestones (MS)

The build order. Every milestone is built headless with tests, and each lands on a
development-size world (GEN-1). **Nothing is judged until all of them run** (SIM-2): the
runner's metrics exist from MS-2 to catch bugs, not to tune.

### MS-1 Skeleton and toolchain `v1`

CMake core library, runner, doctest with sanitizers, config loading, the event queue and
clock (TIM-1, TIM-2), metrics output, versioned saves, and a minimal godot-cpp binding
(`advance`, a snapshot) loaded by the Godot project to prove the toolchain (ARC-4).

### MS-2 World and economy `v1`

World generation (GEN-1 to GEN-7), population, production, labour, markets and prices,
events, treasuries and operating costs, osmosis (SYS-1 to SYS-6). Planets with no
characters yet: osmosis is the only trade.

### MS-3 Characters, movement, information and trade `v1`

Characters and generation (SYS-18), ship variants, running costs and fuel (SYS-21),
Lattice transit and open-space movement with encounters (TIM-4 to TIM-6), facts and
sharing (SYS-15), independent and faction merchants (SYS-7), missions and careers
(SYS-8), contracts (SYS-9), automated couriers.

### MS-4 Politics `v1`

Fealty trees and starting factions (GEN-8, GEN-9), relationships and deeds (SYS-10),
tribute, patronage and defection (SYS-11), the governor's budget model (SYS-5), the Trade
Authority and dues (SYS-20).

### MS-5 War `v1`

Diplomacy (SYS-12), combat resolution (SYS-14), escorts, armies, invasions, sieges,
calls for help, relief and capture with keep-or-grant (SYS-13), scouts, planetary
defence and security missions, risk (SYS-19).

### MS-6 Piracy and suspicion `v1`

Pirate planets and flips, broke commanders, raiding and loot, pirate missions and
smugglers, the pirate relation, contraband in lawful markets, policing and suspicion
(SYS-16, SYS-17).

### MS-7 Spies `v1`

Spies (INF-13, SYS-17). Last because suspicion already exists for smuggling, so a spy
only adds the role and what it can learn behind enemy lines.

### MS-8 Full scale and Phase A `v1`

v1-size worlds, the parallel experiment harness (many worlds across cores), plotting,
and tuning until the checks pass (`checks.md`).

### MS-9 Phase B `v1`

The experiments (`experiments.md`), and the spec changes that follow from them.

After MS-9 the roadmap continues with connecting the simulation to the rendered game
(ARC-10, ARC-13).
