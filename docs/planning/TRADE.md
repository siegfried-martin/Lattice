# Trade: design

*Written 2026-10-12 from a design session following `ECONOMY.md`. This covers who moves
goods around the world, how planet merchant fleets work, the ambient traffic that makes
the world feel alive, and the trade-related missions that come out of it. It also
records some decisions about how NPCs move at scale, which belong in a simulation
document still to be written. Revised on 2026-10-16 and 2026-10-17 with the human's
answers to reviews (see "Revised after review").*

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

## Named merchants

Static characters who chose to be merchants rather than politicians or commanders.
They're the least critical of the three kinds of character, but they add flavour: the
player meets the same merchant several times. **(proposed)** Replacing all procedurally
generated characters with static ones is an idea to play with and iterate on, though it
may be too hard to manage.

## Ambient trade (osmosis)

### The flow

Ambient trade is a **slow osmosis of goods** from producers to nearby planets and
systems. It is not done by named NPCs. It's a very slow trickle that keeps any market
from becoming a complete desert. **Most goods are moved by merchants.** Osmosis has
little influence on prices beyond taking the edge off extreme spikes in supply and
demand.

- It's a flow between nearby markets, **proportional to the difference in amounts**
  (one market's surplus against another's shortage), not to price. It reacts
  instantly. Like real osmosis, a difference in amounts is a reasonable proxy for price
  without needing it. The flow falls off with travel time and is faster along the
  Lattice.
- **It closes only part of a gap.** Osmosis moderates extremes and quietly corrects
  small world-generation errors before role switching is needed, but it must leave a
  margin. Otherwise named merchants, planet fleets and the player have nothing to
  profit from. Osmosis handles the background; real ships make the big moves.
- **It runs as real market trades:** buying at the source, selling at the destination.
  Prices move and the transaction tax applies like any other trade.
- **It stops between factions at war,** like merchant fleets (see Planet merchant
  fleets).
- **Contraband moves by osmosis too. (proposed)** Either at a smaller rate, or with a
  higher rate of consumption.
- **The independent traders' margin vanishes,** a small money sink that scales with
  trade volume.

### What the player sees

Within the player's detection range, **non-persisted ships of all types are generated
procedurally.** The traders among them are the visible face of osmosis.

- **Spawned from the region and events, as in Escape Velocity.** Ambient ships aren't
  backed by simulation state. What spawns follows the region and what's happening
  there: pirates where piracy is common, patrols in policed space, warships near a war.
  That keeps the world immersive, and it's good enough. Like most games of this kind,
  Lattice mixes named, persistent NPCs with procedurally generated ones; making
  everything persistent would be too hard at scale and wouldn't make the game more fun.
- **How spawning works.** Now and then a random chance triggers a spawn event near the
  player, at the edge of their radar. The chance of anything spawning depends on the
  area's population and activity. The number, size and faction of the ships come from
  regional strength nearby.
- **Regional piracy is a level that spawns read from.** Authority mercenary contracts
  and the player clearing pirates lower it.
- **Ambient traders match the flow.** Direction, density and cargo type reflect what's
  actually moving on that route. Busy routes are full of freighters heading from
  producer to consumer.
- **Persistence on contact.** An ambient ship stays real while the player is
  interacting with it (scanning, hailing, chasing). It's only promoted to persistent if
  something lasting happens, such as being hired or escaping as someone with a grudge.
  Otherwise it despawns out of range, and a player who returns sees different ships.
- **Killing one doesn't touch the global economy.** It's too small to be worth
  calculating. Loot, suspicion and reputation apply as normal: killing merchants still
  makes local governments angry, and reputation is what stops it being free money.

### Economic damage comes from merchant fleets

Osmosis doesn't react to raids or blockades, by design. It would be hard to approximate
well, and choking trade completely would crush a planet's economy and money supply,
which is too drastic. Economic warfare hits **planet merchant fleets** instead: killing
one does a lot of damage, because it removes the income the planet was going to make
and destroys its merchant ships.

The balance to find: rebuilding and resending merchant fleets stays cost-effective for
planets despite the risk of piracy, but in rare cases a planet still loses money and
has to turn to piracy.

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
- **Idle hulls cost nothing.** An idle hull is a trade not being made, which is cost
  enough. A standing cost makes sense for a defence force (`ECONOMY.md`), not for
  merchants.
- **No trade with a faction at war.** A planet doesn't send merchant fleets to a faction
  it's at war with, and convoys in transit there turn for home when war breaks out.

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

### Where convoys are attacked

**In open space.** There's no combat on the Lattice, and an enemy generally isn't let
onto the highway in the region where a convoy is travelling. The exception is a convoy
trading in a third faction's territory that isn't at war with the raider, where both
can use the highway. They'd have to be on the same stretch at the same time, which is
rare enough to ignore for now.

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
- **Game time** runs at **one real minute per in-game day** to start. It's a value to
  play with, not a design.
- **The clock stops while the player is landed.** Anything else is too stressful; the
  player gets as long as they want to plan their next move and shop around.
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
- Rebuilding and resending merchant fleets stays worthwhile despite piracy; only rarely
  does a planet lose enough to it to turn pirate.
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

## Revised after review

The human answered a review on 2026-10-16:

- **Planets are state traders.** The governor decides what the planet produces and
  sends out its merchant fleets (recorded in `ECONOMY.md`).
- **Osmosis doesn't react to raids,** by design. Economic damage comes from killing
  planet merchant fleets.
- **Ambient ships are spawned from region and events,** as in Escape Velocity, not
  backed by simulation state.
- **Osmosis follows the difference in amounts,** instantly, rather than price.
- **Convoys are attacked in open space;** meetings in a third faction's territory are
  ignored for now.
- **Game time** starts at one real minute per in-game day.

And on 2026-10-17:

- **Merchants buy from and sell to the state** (recorded in `ECONOMY.md`).
- **Osmosis is a slow trickle** that prevents deserts and blunts extreme spikes; most
  goods move by merchant.
- **Spawning:** a random spawn event near the player at the edge of radar, with its
  chance set by the area's population and activity and its ships by regional strength.
  A regional piracy level feeds it, lowered by mercenary contracts and by the player.
- **The clock stops while landed.**
- **War stops trade:** no merchant fleets to a faction at war, convoys in transit turn
  home, and osmosis stops. Contraband osmosis is proposed.
- **Idle merchant hulls cost nothing.**
- **Named merchants** are static characters who chose trade; replacing procedural
  characters with static ones is an idea to try.

## For the implementation agent

Not to be acted on while this document is in `docs/planning/`. When it's promoted:

- Add a row for it to `docs/planning/README.md` now.
- Cross-reference `ECONOMY.md` (budget model, money sinks, merchants) and
  `FACTIONS.md` (governors, belief state, mercenaries).
- The scale section is a placeholder for a simulation document; move it there when that
  document exists.
- This is design only. Don't build it yet.
