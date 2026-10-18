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
(SIM-2). The config defaults build a development world; `sim/configs/v1.toml` sets the
v1 sizes.

*(proposed)* The planet count includes moons: every planet and moon is a settled market,
as in the prototype's body count. That gives about 8 systems in a development world and
about 27 in a v1 world.

### GEN-2 Sectors and regions `v1` `proposed`

A hex grid of the configured size. Culture regions are contiguous: pick one seed sector
per region spread far apart, then grow regions outward by alternating steps (a
breadth-first fill) so they come out roughly equal and compact. Each region draws its
culture and production weights (GEN-6).

*As built:* odd rows sit half a sector east, and neighbouring sector centres are one
unit apart; every distance in the simulation is in these sector widths. Seeds are
chosen by farthest-point sampling from a random first sector; regions take turns
claiming the oldest unclaimed sector on their frontier.

### GEN-3 Systems and planets `v1` `proposed`

Place systems in sectors with a density that varies (some sectors empty), one to four
planets each, a quarter with one to three moons (WLD-4). Each system is one group, or a
large system splits into two. Planets get a starting population drawn per region.
Mark **fringe** systems: about one in six, placed toward the map's edges and region
borders, left off the network (WLD-6).

*As built:*
- At most one system per sector. Sectors are weighted by a random density smoothed
  over each sector's neighbours and cubed, so systems crowd in some stretches and leave
  others empty.
- Regions take turns placing systems, the region with the fewest planets first, so
  each region has enough planets for GEN-6's producer guarantee.
- Population is `gen.pop_median` × the region's scale (log-normal,
  `gen.region_pop_spread`) × the planet's own draw (log-normal, `gen.pop_spread`); a
  moon gets `gen.moon_pop` of that.
- A system with `gen.split_planets` planets or more is two groups, its inner and outer
  halves; moons go with their planet.
- Fringe systems are the top `gen.fringe_share` by a score of closeness to the map's
  edge (within two sectors), a region border in the sector, and a random part. At
  least two systems stay on the network.
- Hop lanes join each moon to its planet and a group's planets in a chain by orbit.

### GEN-4 The Lattice network `v1` `open`

A generated network connecting every non-fringe group's interchange. *(proposed)* Start
from a minimum spanning tree over interchange positions, then add the edges of a
relative neighbourhood graph until the network has some loops (a share
`gen.lattice_loops` of extra edges), keeping edges roughly along sector directions so the
Lattice stays directionally accurate. Where three or more roads meet, insert a junction.
The rendered Lattice has to draw whatever this makes (right-hand traffic, junction gates,
`docs/WORLD_AND_TRAVEL.md`), which is GDScript work when the binding is connected.

*As built:* each on-network group's interchange sits beside its holder. The extra
edges are the shortest relative-neighbourhood edges not in the tree. Both graphs are
planar, so roads never cross; edges are straight lines between interchanges, which is
what keeps the Lattice directionally accurate (aligning roads to hex directions is left
to the rendered layout). An interchange where three or more roads would meet gets a
junction at the same position: the roads move to the junction, and the interchange
hangs off it by a zero-length link, so no interchange has more than two roads.

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

*(proposed, round 3)* **Luxuries are regional:**
- Each of the six regional luxuries has `gen.lux_home_regions` home regions, spread so
  every region is home to about two. A region's weight for a luxury it isn't home to
  is × `gen.lux_away`; for one it is, × `gen.favour_mult`.
- The two fringe luxuries have weight × `gen.lux_away` on network planets, and fringe
  planets weight them × `gen.fringe_lux_mult`.
- The repair guarantees life support in every region, and each luxury only in its home
  regions (each fringe luxury across the fringe as a whole, if there are enough fringe
  planets).

*As built:* a region's weights are `1 ± gen.weight_spread` per lawful good, with one
favoured good × `gen.favour_mult`. The repair takes a medium-or-better slot from a good
that has more producers than it needs: it either swaps tiers with the short good where
the planet already makes it at low, or replaces that slot's good. Neither creates a new
shortfall, so the repair always ends, and it always succeeds when the region has at
least 3 × `gen.min_producers` planets.

### GEN-7 Fuel prices `v1`

Each planet's fuel price is `fuel.base × (1 + fuel.fringe × distance in Lattice hops
from the nearest seat of power, normalised)`, higher again for fringe planets (ECO-11).

*As built:* hops are counted between interchanges (a junction's link is free) and
normalised by the largest in the world. A fringe planet counts the hops of its nearest
interchange plus one, and its price is multiplied by `fuel.fringe_mult`
*(proposed)*. The seats of power are chosen in MS-2 for this, by GEN-8's rule: the most
populous on-network planet first, then each next one by population share × distance
share from the seats already chosen, in a region without a seat while one remains.

### GEN-8 Factions `v1` `proposed`

Pick the configured number of **seats of power**: high-population on-network planets far
apart, each in a different region where possible (FAC-2). Grow each tree outward over the
Lattice graph: each planet swears to a nearby planet one step closer to a seat, so trees
have depth (a seat, regional lords, their vassals). Leave a share `gen.independent_share`
of planets independent (their own roots), and start a few fringe planets as pirate
planets (`gen.pirate_planets`) so the pirate faction exists from the first tick. All
factions start at peace.

*As built (MS-3, ahead of MS-4 because merchants and couriers need factions):*
- Seats are gate holders.
- A breadth-first search over the Lattice from the seats' interchanges gives each
  interchange the next one toward its seat. A group's holder swears to the holder there;
  the group's other planets swear to their holder. An off-network group's holder swears
  to the holder of the nearest on-network group.
- Independents are drawn from planets with no vassals, so trees stay whole.
- *(MS-6)* Pirate planets: `gen.pirate_planets` fringe planets with no vassals, each turned
  pirate with `pirate.commanders` pirate commanders (one gunship each). They are seeded
  after the lawful commanders and before the bottom rung, inside `char.target`.

### GEN-9 Starting characters `v1` `proposed`

- A governor for every planet (CHR-7), with reputation scaled by tree height.
- Commanders under contract to each governor, with fleet power proportional to the
  planet's income (`gen.commanders_per_income`).
- Faction merchants for each root and independent merchants spread by population.
- A few pirate commanders at each pirate planet.
- The rest of `char.target` as bottom-rung characters (CHR-6).

Risk tolerance for all from `char.risk_dist`.

*As built (MS-4):* each governor starts with commanders sworn to it: characters with one
first-tier gunship each, enough that their combat power reaches
`gen.commanders_per_income` × the planet's monthly income, at the full stipend.

*As built (MS-3):* governors and the bottom rung, placed by population. A governor's risk
tolerance sets its planet's growth factor (PRD-3). Commanders, faction merchants and
pirates wait for their milestones; faction merchants are hired from the bottom rung by
salary (SYS-9).

### GEN-10 Starting state and warm-up `v1` `proposed`

Stocks start at their reserve targets, treasuries at `gen.start_treasury_months` of
costs, and opinions at their resting points. A run first simulates `gen.warmup_days`
with war declarations disabled, so markets and labour settle, and measurement starts
after it. *(proposed)* Rebellions still happen, and a secession's war with them (ECO-13):
that war is no vote.

*As built:* labour starts split equally across a planet's three goods, happiness at
`gen.start_happiness`, and each planet's income average at what its population would
spend with its whole budget. The runner runs the warm-up unmeasured on a new world
(`--observe-warmup` measures it too).
