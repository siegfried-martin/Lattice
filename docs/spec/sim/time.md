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
   order of time, then phase (this list's order), then sequence number.
2. **Decisions on a fixed cadence per role** (TIM-3), staggered so that not every
   governor decides on the same tick.
3. **A coarse kinematic step** for ships in open space (TIM-6).

All three run through the one queue: cadence decisions and the open-space step are
recurring events, so `advance(dt)` is a single loop. It runs every event due in
`[now, now + dt)`, in order of time, phase and sequence number, then sets `now` to
`now + dt`. The interval is half-open, so an event at exactly `now + dt` runs in the
next call, and splitting one advance into several gives the same world.

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

*As built (MS-3):* a character decides when it lands (after `merchant.dock_hours` on a
trip) and, when idle, again a day later. Only its latest scheduled decision runs.

*As built (MS-2):* each planet's day, labour shift and payments are its own recurring
events, staggered across their period by planet. The world's daily tick, at the start
of each day, samples metrics, rolls events and runs osmosis.

### TIM-4 Travel times `v1`

Planning uses travel times from a precomputed graph:

- **Lattice:** shortest paths over the Lattice graph, with gate permissions as an edge
  filter for whoever is travelling (WLD-12). Travel time is distance over Lattice speed.
- **Open space:** straight-line distance between sector positions over the ship's speed,
  plus the hazard penalty (TIM-7).
- **Hop lanes:** a short fixed time.

*As built (MS-2):* reference times use `travel.space_speed`, `travel.lattice_speed` and
`travel.hop_days`, in sector widths per day, ignoring gate permissions. They are the
shortest path over every planet and Lattice node, with open space joining any two. The
starting speeds follow the prototype's scale: a freighter crosses a sector of open space
in about a week of game time, and the Lattice in about half a day *(proposed)*.

A merchant or army plans a route as: open space to the nearest entrance it may use,
Lattice to the exit nearest the destination that will let it out, open space to the
destination (WLD-13).

### TIM-5 The Lattice is a scheduled transit `v1`

Entering the Lattice schedules an arrival at the exit at time *t*. Position in transit
is interpolated along the path when anything needs it. Nothing can happen to a ship in
transit, because there's no combat on the Lattice (WLD-2). Fuel is charged at entry
(ECO-11).

*As built (MS-3):* a journey is a route of legs, each scheduling its own arrival. The
legs are open space, the Lattice (consecutive Lattice edges merged into one transit) and
hop lanes. Every route follows the reference route (TIM-4); gate permissions will filter
routes when they first refuse anyone (MS-5).

### TIM-6 Open space is cheap kinematics `v1`

In open space a ship has a position (sector plus local offset, for precision) and a
velocity, advanced on the coarse step with no collisions. Diversions are free: re-decide
on the next step. **Encounters** are proximity checks with a **spatial hash per sector**:
two ships within detection range on a step get a detection roll from patrol coverage,
hostility, suspicion and stealth; a detected hostile meeting starts a battle (SYS-14) or
an evasion. Sightings (INF-3) come from the same check.

*As built (MS-3):*
- An open-space leg is a straight line with known start and end times; the step
  interpolates positions rather than integrating velocity, which comes to the same thing
  for straight legs. A diversion is a new leg from the current position.
- The step hashes only the ships something could happen to, in cells of
  `space.detect_range`, and checks neighbouring cells. In MS-3 that is members who can
  share facts. Hashing every ship cost a hundred times more at busy systems, where
  traffic crowds the short hops between planets and interchanges.
- *MS-5:* also raiders, patrols, scouts on missions, members of factions at war, and
  armies moving in open space.
  - **Detection:** a hostile pair in range is engaged with chance `space.engage_p` a step.
    A quarry faster than its attacker slips away with chance `space.evade` × its speed
    edge, which stands in for the full detection roll.
  - **Battles in open space** stop both sides' journeys; survivors resume their legs
    afterwards.
  - Armies don't meet each other in the field yet.

### TIM-7 Hazards cost time `v1` `proposed`

A planned path across an asteroid field or nebula costs extra time or risk, or goes by
one waypoint, rather than simulating avoidance.

### TIM-8 Speed target `v1` `proposed`

A full v1 world (WLD-3, about 2,000 deciding characters) should run **a game year in
under a minute on one core**, so an experiment of 50 seeds × 10 years finishes in tens
of minutes across a machine's cores. If it doesn't, profile before changing the design.
