# Time and movement (TIM)

How simulation time advances, when each kind of decision is made, and how things move
and meet.

### TIM-1 The clock `v1`

Simulation time is an **integer count of game seconds** *(proposed)*, so time
comparisons are exact. One game day is one real minute when rendered (WLD-15); headless,
the runner advances as fast as it can. When the player lands, the binding stops calling
`advance` (WLD-15).

### TIM-2 A hybrid of three mechanisms `v1`

1. **A scheduled-event queue** (MOD-11) for things that happen at a known time: Lattice
   arrivals, siege rounds, contract deadlines, event ends, courier deliveries. Popped in
   order of time, then sequence number.
2. **Decisions on a fixed cadence per role** (TIM-3), staggered so that not every
   governor decides on the same tick.
3. **A coarse kinematic step** for ships in open space (TIM-6).

`advance(dt)` runs everything due up to `now + dt`, in that order within each step.

### TIM-3 Cadences `v1` `proposed`

| What | When |
|---|---|
| Economy: production, consumption, prices, growth, happiness (SYS-1 to SYS-3) | Daily, per planet |
| Osmosis (SYS-6) | Daily |
| Labour shift (SYS-2) | Weekly, per planet |
| Governor budget: tribute rates, patronage, policing, hiring, missions (SYS-5) | Weekly, staggered |
| Root decisions: merchant salary, war vote, suing for peace (SYS-12) | Weekly |
| Treasury payments: stipends, salaries, tribute, operating costs, dues | Monthly (30 days) |
| Opinion fade, defection check (SYS-10, SYS-11) | Weekly |
| Character decisions: next mission, next trip, spend savings | On arrival or when idle, at most daily |
| Commander actions within an army | On events and at most every few hours |
| Open-space step (TIM-6) | Every 10 game minutes |
| Battle and siege rounds (SYS-14) | Hourly for battles, daily for sieges |
| Events: start and end (SYS-4) | Daily roll |

All of these are tunables (TUN).

### TIM-4 Travel times `v1`

Planning uses travel times from a precomputed graph:

- **Lattice:** shortest paths over the Lattice graph, with gate permissions as an edge
  filter for whoever is travelling (WLD-12). Travel time is distance over Lattice speed.
- **Open space:** straight-line distance between sector positions over the ship's speed,
  plus the hazard penalty (TIM-7).
- **Hop lanes:** a short fixed time.

A merchant or army plans a route as: open space to the nearest entrance it may use,
Lattice to the exit nearest the destination that will let it out, open space to the
destination (WLD-13).

### TIM-5 The Lattice is a scheduled transit `v1`

Entering the Lattice schedules an arrival at the exit at time *t*. Position in transit
is interpolated along the path when anything needs it. Nothing can happen to a ship in
transit, because there's no combat on the Lattice (WLD-2). Fuel is charged at entry
(ECO-11).

### TIM-6 Open space is cheap kinematics `v1`

In open space a ship has a position (sector plus local offset, for precision) and a
velocity, advanced on the coarse step with no collisions. Diversions are free: re-decide
on the next step. **Encounters** are proximity checks with a **spatial hash per sector**:
two ships within detection range on a step get a detection roll from patrol coverage,
hostility, suspicion and stealth; a detected hostile meeting starts a battle (SYS-14) or
an evasion. Sightings (INF-3) come from the same check.

### TIM-7 Hazards cost time `v1` `proposed`

A planned path across an asteroid field or nebula costs extra time or risk, or goes by
one waypoint, rather than simulating avoidance.

### TIM-8 Speed target `v1` `proposed`

A full v1 world (WLD-3, about 2,000 deciding characters) should run **a game year in
under a minute on one core**, so an experiment of 50 seeds × 10 years finishes in tens
of minutes across a machine's cores. If it doesn't, profile before changing the design.
