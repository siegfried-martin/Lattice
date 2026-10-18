# Milestones (MS)

The build order. Every milestone is built headless with tests, and each lands on a
development-size world (GEN-1). **Nothing is judged until all of them run** (SIM-2): the
runner's metrics exist from MS-2 to catch bugs, not to tune.

### MS-1 Skeleton and toolchain `v1`

CMake core library, runner, doctest with sanitizers, config loading, the event queue and
clock (TIM-1, TIM-2), metrics output, versioned saves, and a minimal godot-cpp binding
(`advance`, a snapshot) loaded by the Godot project to prove the toolchain (ARC-4).

*Done 2026-10-18.* The world has no systems yet: a daily cadence event and the
`debug.pings` scaffolding exercise the queue, the RNG streams and saves. Tests cover
event order, RNG streams, config errors, save round trips and rejection, determinism,
and that splitting an advance changes nothing.

### MS-2 World and economy `v1`

World generation (GEN-1 to GEN-7), population, production, labour, markets and prices,
events, treasuries and operating costs, osmosis (SYS-1 to SYS-6). Planets with no
characters yet: osmosis is the only trade.

*Built 2026-10-18.* World generation makes the map, regions, systems, groups, the
Lattice, hop lanes, what planets make, the seats of power (for fuel prices) and fuel
prices; each planet runs its daily economy, weekly labour shift and monthly costs; world
events and osmosis run daily. The runner writes the map and per-planet, per-good and
world series. Tests cover every generation promise over many seeds at both sizes, the
SYS-1 to SYS-6 formulas, and that money and goods add up over a year (MOD-12).

*Found:* with the starting values the world can't feed itself, so populations shrink
from the first day. A thousand people need 3 units of life support a day; at most they
make `prod.labour_per_capita` × productivity, 3.5 × 0.9 ≈ 3.2, and only if all their
labour is at high efficiency. Prices then climb to `price.max`, happiness falls to 0
and productivity to `pop.p_base`. Planets missing a life-support good live on osmosis
alone. Not tuned here: merchants (MS-3) change the picture, and tuning waits for Phase A
(SIM-2).

### MS-3 Characters, movement, information and trade `v1`

Characters and generation (SYS-18), ship variants, running costs and fuel (SYS-21),
Lattice transit and open-space movement with encounters (TIM-4 to TIM-6), facts and
sharing (SYS-15), independent and faction merchants (SYS-7), missions and careers
(SYS-8), contracts (SYS-9), automated couriers.

*Built 2026-10-18.*
- **What runs:** characters spawn toward `char.target` and generate ship catalogues
  (MOD-1). They move along routes over open space, the Lattice and hop lanes, paying
  fuel and running costs. They observe markets and share facts within their faction.
- **Work:** they trade as independents or salaried faction merchants (all three trip
  shapes), take courier and scout missions, buy ships with their savings, and sell them
  when broke. Automated couriers carry facts down and up the fealty trees.
- **Moved in:** fealty trees (GEN-8) and governors (part of GEN-9) came forward from
  MS-4, because faction merchants, sharing and couriers need them.
- **Moved out:** escorts move to MS-6, where routes first carry risk; sightings move to
  MS-5, where armies are what they're for.
- **Speed:** a v1-size world with 2,000 characters runs a year in about 7 s.
- **Found:** merchants move more life support than the planets make (goods change hands
  more than once) but can't fix MS-2's structural shortage: populations still shrink
  under the starting values. With `prod.labour_per_capita` at 7 a world settles at about
  60% of its start. Happiness then flips between 0 and 1 across planets: the happiness,
  productivity and budget loop reinforces itself. Both are for Phase A.

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
