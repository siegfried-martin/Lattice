# World generation (GEN)

The simulation generates the world in C++ (ARC-10) from a seed and a config. The output
is stored in saves (ARC-9), so a world is never regenerated to load it.

### GEN-1 Sizes `v1`

| | Development | v1 |
|---|---|---|
| Map | about 6×8 sectors | 10×15 sectors (WLD-3) |
| Planets | about 30 | about 100 |
| Culture regions | 3 | *(open)* 5–6 suggested |
| Starting factions | 3 | *(open)* (OPN-5) |
| Characters (target) | about 500 | about 2,000 |

Both are config, so any size in between can be run. Verdicts are taken at v1 size
(SIM-2).

### GEN-2 Sectors and regions `v1` `proposed`

A hex grid of the configured size. Culture regions are contiguous: pick one seed sector
per region spread far apart, then grow regions outward by alternating steps (a
breadth-first fill) so they come out roughly equal and compact. Each region draws its
culture and production weights (GEN-6).

### GEN-3 Systems and planets `v1` `proposed`

Place systems in sectors with a density that varies (some sectors empty), one to four
planets each, a quarter with one to three moons (WLD-4). Each system is one group, or a
large system splits into two. Planets get a starting population drawn per region.
Mark **fringe** systems: about one in six, placed toward the map's edges and region
borders, left off the network (WLD-6).

### GEN-4 The Lattice network `v1` `open`

A generated network connecting every non-fringe group's interchange. *(proposed)* Start
from a minimum spanning tree over interchange positions, then add the edges of a
relative neighbourhood graph until the network has some loops (a share
`gen.lattice_loops` of extra edges), keeping edges roughly along sector directions so the
Lattice stays directionally accurate. Where three or more roads meet, insert a junction.
The rendered Lattice has to draw whatever this makes (right-hand traffic, junction gates,
`docs/WORLD_AND_TRAVEL.md`), which is GDScript work when the binding is connected.

*See:* OPN-6.

### GEN-5 Gates and holders `v1`

Each on-network group gets an interchange. Its holder is the group's most prosperous
planet at build (largest starting `N × P`), fixed forever (WLD-5).

### GEN-6 What planets make `v1` `proposed`

Each region has weights over the v1 goods (a farming region favours food). Each planet
draws **three distinct goods** by those weights and assigns them high, medium and low
efficiency at random (PRD-7). Then repair: every good must have at least
`gen.min_producers` producers per region at medium or better (PRD-8); swap draws until it
holds. Contraband is never drawn for a lawful planet.

### GEN-7 Fuel prices `v1`

Each planet's fuel price is `fuel.base × (1 + fuel.fringe × distance in Lattice hops
from the nearest seat of power, normalised)`, higher again for fringe planets (ECO-11).

### GEN-8 Factions `v1` `proposed`

Pick the configured number of **seats of power**: high-population on-network planets far
apart, each in a different region where possible (FAC-2). Grow each tree outward over the
Lattice graph: each planet swears to a nearby planet one step closer to a seat, so trees
have depth (a seat, regional lords, their vassals). Leave a share `gen.independent_share`
of planets independent (their own roots), and start a few fringe planets as pirate
planets (`gen.pirate_planets`) so the pirate faction exists from the first tick. All
factions start at peace.

### GEN-9 Starting characters `v1` `proposed`

- A governor for every planet (CHR-7), with reputation scaled by tree height.
- Commanders under contract to each governor, with fleet power proportional to the
  planet's income (`gen.commanders_per_income`).
- Faction merchants for each root and independent merchants spread by population.
- A few pirate commanders at each pirate planet.
- The rest of `char.target` as bottom-rung characters (CHR-6).

Risk tolerance for all from `char.risk_dist`.

### GEN-10 Starting state and warm-up `v1` `proposed`

Stocks start at their reserve targets, treasuries at `gen.start_treasury_months` of
costs, and opinions at their resting points. A run first simulates `gen.warmup_days`
with war declarations disabled, so markets and labour settle, and measurement starts
after it.
