# Trade: design

*Written 2026-10-12 from a design session following `ECONOMY.md`. This covers who moves
goods around the world, how planet merchant fleets work, the ambient traffic that makes
the world feel alive, and the trade-related missions that come out of it. It also
records some decisions about how NPCs move at scale, which belong in a simulation
document still to be written.*

*Items marked **(proposed)** came up in the session and weren't explicitly signed off.
Items marked **(later)** are decided as worth doing, but not in the first version.
Everything else is decided. Numbers are tuning, not design.*

## Principles this serves

- **The world feels alive and rich.** Space near the player is busy with ships of every
  kind, doing things that make sense.
- **The economy is visible.** Players can read what's happening by looking around: busy
  routes, convoys, quiet space.
- **Economic warfare has targets.** Hurting a planet's trade means attacking something
  real.
- **Don't simulate what nobody can see or feel.** Detail exists where it changes
  outcomes or experiences, and nowhere else.

## Three layers of trade

| Layer | What it is | Persistence |
|---|---|---|
| **Named merchants** | Merchant characters with their own ships | Persistent |
| **Planet merchant fleets** | A planet's bulk trade, sent out on missions | Persistent while on a mission |
| **Ambient traders** | Independent background trade ("osmosis") | Never persisted; generated near the player |

## Ambient trade (osmosis)

### The flow

Ambient trade is a **slow osmosis of goods** from producers to nearby planets and
systems. It is not done by named NPCs.

- It's a flow between nearby markets, **proportional to the price difference**, falling
  off with travel time and faster along the Lattice.
- **It closes only part of a gap.** Osmosis moderates extremes and quietly corrects
  small world-generation errors before role switching is needed, but it must leave a
  margin. Otherwise named merchants, planet fleets and the player have nothing to
  profit from. Osmosis handles the background; real ships make the big moves.
- **It runs as real market trades:** buying at the source, selling at the destination.
  Prices move and the transaction tax applies like any other trade.
- **The independent traders' margin vanishes,** a small money sink that scales with
  trade volume.

### What the player sees

Within the player's detection range, **non-persisted ships of all types are generated
procedurally.** The traders among them are the visible face of osmosis.

- **Ambient traders match the flow.** Direction, density and cargo type reflect what's
  actually moving on that route. Busy routes are full of freighters heading from
  producer to consumer.
- **Persistence on contact.** An ambient ship stays real while the player is
  interacting with it (scanning, hailing, chasing). It's only promoted to persistent if
  something lasting happens, such as being hired or escaping as someone with a grudge.
  Otherwise it despawns out of range, and a player who returns sees different ships.
- **Killing one doesn't touch the global economy.** It's too small to be worth
  calculating. Loot, suspicion and reputation apply as normal, and reputation is what
  stops it being free money.

### Danger throttling (later)

Osmosis could slow through dangerous space, scaled by pirate presence, active war and
patrol coverage. Not in the first version. When it's added, it should have a **floor
rather than a cutoff**, so war can't choke the whole trade economy.

## Planet merchant fleets

### Hulls persist; the fleet doesn't

Part of a planet's wealth is its merchant fleet.

- **Between missions,** the planet owns merchant ships as a count of hulls. No fleet
  entity exists.
- **On a mission,** a fleet entity is created, persists until the mission is complete,
  and then dissolves back into the count.
- **Losses come off the count.** Replacing them is the rebuild cost.

### Missions are governor decisions

Sending a fleet is a decision a governor makes with the planet's resources, through
the budget model in `ECONOMY.md`: operating cost plus escort cost plus risk, against
expected return.

- **Missions are a money sink.** Fleet operating costs are paid to nobody.
- **A stretched governor sends fewer or shorter missions,** which cuts income and feeds
  the pressure toward piracy.
- **Missions are planned on believed prices,** which may be stale. A mission can
  disappoint because the market moved or the goods were already bought up. Some
  missions fail for understandable reasons.

### Mission shapes

Mission length and objective are for experimenting with. The starting shape is a
**triangle**: carry exports to sell at A, buy needed imports at B, return home. That
covers both exporting and importing in one trip.

**(proposed)** Other shapes to try: a simple there-and-back, and a standing route that
repeats until cancelled.

### Escorts

- **Missions need protection, and the planet pays for it.** Escort is a budget line.
  The governor chooses the escort: the planet's own gunships, hired mercenaries, the
  player, or none. A stretched planet sends thin escorts and a rich one sends a wall,
  so the player can read a planet's health from its convoys.
- **Escorts follow a doctrine:** defend the convoy and drive threats off, but don't
  pursue an enemy that runs. They stay on a leash. The leash distance is tuning.

### Convoys are visible and exciting

Planet convoys are **large in scale** compared with independent merchants, and the
player sees them. On the highway, six large cargo ships and four gunships travelling
one after another: you know a planet is backing that. Out in open space, a convoy
docked at a planet unloading cargo, an enemy faction ship arriving, the gunships
driving it off but letting it go when it runs. These are the moments that make the
world feel alive.

- **NPC loading and unloading takes time,** unlike the player's.
- **(proposed)** A scan names the planet backing a convoy.

## Missions for the player

Convoys give the player three ways in:

- **Escort** a convoy for pay. Planets need protection, and the player is one way to
  provide it.
- **Raid** a convoy for loot, or to wage economic war on the planet behind it.
- **Trade around** a convoy, since its arrival is about to move prices at its
  destination.

### Intercept a convoy

A mission to **intercept a particular convoy.** It's naturally time sensitive, and it
has a light element of finding and puzzle-solving that fits the physical-information
system: the player works from sightings and reports to predict where the convoy will
be, and gets there in time.

## How NPCs move at scale

At full scale there will be on the order of 100–200 sectors, 100–200 inhabited
planets, and similar numbers of governors, fleets and named merchants: a couple of
thousand entities making individual decisions. Making the decisions is cheap.
Positions are the hard part, because these entities can't all be manifested in the
real world, yet their interactions with it must still affect how they move.
These notes move to a simulation document when one is written.

### Decided

- **NPC short-term decisions** are based on current location, what the NPC knows, and
  distances to points of interest.
- **Highway travel** can use a map of roads and edges.
- **Open space doesn't use edges.** There's no separate decision layer that has to know
  about edges and diversions. Open-space movement works from positions directly.

### Proposed

- **One brain, two bodies.** NPCs use the same decision logic everywhere. Near the
  player, a decision drives real physics and steering. Far away, it drives cheap
  kinematics: position plus velocity on a coarse timestep, with no collisions.
  Diversions are free (re-decide next tick), and handing an NPC into or out of the
  player's area is just spawning it where it already is.
- **Highway travel is a scheduled transit.** Entry at A and exit at B at time *t*, with
  the position in transit interpolated along the road when anything needs it.
- **The abstract state is authoritative.** The area around the player (current sector
  plus neighbours) is a view that writes results back: damage, lost cargo, position.
- **Encounters are proximity checks,** using a spatial hash per sector, followed by a
  detection chance from patrol coverage, hostility, suspicion and stealth.
- **Off-screen combat is auto-resolved** from fleet strength plus randomness, giving
  losses, cargo destroyed or seized, retreat and capture. **Calibrate it against real
  combat** in the headless harness, or the player's presence biases outcomes.
- **Battles have duration,** so a player who arrives mid-fight finds it in progress,
  with the losses so far applied.
- **Hazards in the cheap body:** a planned path crossing an asteroid field or nebula
  costs extra time or risk, or goes via one waypoint, rather than simulating avoidance.
- **Precision:** abstract positions are stored as sector ID plus local offset, not one
  global coordinate.

## What a headless prototype should check

- Osmosis narrows price gaps without closing them; named merchants and planet fleets
  still find profitable trades.
- Planet fleet missions are worth sending in good times and get cut back when a planet
  is stretched.
- Escort spending tracks planet wealth.
- The money supply stays in a band with mission costs and the osmosis margin included
  as sinks.
- Auto-resolved combat matches manifested combat statistically.

## Open questions

- Mission length and objective shapes.
- Escort ratio and leash distance.
- How the player finds intercept targets, and how much information they start with.
- How danger throttling and its floor should work, when it's added.
- Decision tick rates for governors, fleets and merchants.
- How large the manifested area around the player is.

## For the implementation agent

Not to be acted on while this document is in `docs/planning/`. When it's promoted:

- Add a row for it to `docs/planning/README.md` now.
- Cross-reference `ECONOMY.md` (budget model, money sinks, merchants) and
  `FACTIONS.md` (governors, belief state, mercenaries).
- The scale section is a placeholder for a simulation document; move it there when that
  document exists.
- This is design only. Don't build it yet.
