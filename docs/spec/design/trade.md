# Trade (TRD)

Who moves goods: merchant characters, the background osmosis, and the player. How
merchants plan trips and hire escorts, what the player sees around them, and the trade
missions that come out of it.

## Principles

- The world feels alive: space near the player is busy with ships doing things that
  make sense.
- The economy is visible: busy routes, escorted freighters, quiet space.
- Economic warfare has targets: hurting a faction's trade means attacking something
  real.
- Don't simulate what nobody can see or feel.

## Who moves goods

### TRD-1 Merchants, osmosis and the player `v1`

Goods move three ways: **merchant characters** (independent and faction merchants), the
background **osmosis** (TRD-13), and the **player**. Planets don't run merchant fleets of
their own.

*See:* REJ-25 (planet merchant fleets).

### TRD-2 Price is the only motivator `v1`

A governor who wants a good lets its price rise, and merchants come. Governments don't
hire traders for tasks or match convoys with escorts.

*See:* REJ-26 (the planet as matchmaker).

## Merchants

### TRD-3 Faction merchants `v1`

**Only a faction's root pays merchants a salary**, because the root has the widest span
of planets. It's a contract (REL-8).

- **The merchant gets the faction's information:** faction governors share with it when
  it lands at their planets (INF-4).
- **The faction gets in-faction trade:** a faction merchant only runs trips that start
  and end at a faction planet, in three shapes:
  1. **inside:** a single trip between two faction planets;
  2. **out and back:** a faction planet to an outside planet not at war with the faction,
     and back, carrying cargo both ways;
  3. **triangle:** a faction planet to outside planet X, then outside planet Y, then home,
     trading at each stop.

  It picks the best trip among the three shapes, with no extra weighting.
- **The salary is the root's lever:** the information is the main attraction, so it can
  be modest, and the root raises or lowers it to get more or fewer merchants. Tribute
  flowing up pays for it (FAC-6).
- Because they work in their faction's territory, faction merchants are **targetable by
  the faction's enemies**. That's where economic warfare's targets come from (TRD-7).

*Consequence:* faction planets buy or sell at each end of every trip, and merchants coming
home from outside bring outside prices into the faction's information. More merchants
mean better information, which attracts more merchants.

### TRD-4 Independent merchants `v1`

An independent merchant makes **single trips** (buy at A, sell at B) from its own
information. If it can't find a good trade, it moves to a nearby system and looks there.

### TRD-5 Who can run what `v1`

Independent single-trip trade is open to **everyone except pirates**. Faction trips need
the faction's merchant contract.

### TRD-6 War stops trade `v1`

Faction merchants don't trade with planets of a faction they're at war with, merchants in
transit there turn for home when war breaks out, and osmosis stops between warring
factions (TRD-13).

### TRD-7 Economic damage comes from killing merchants `v1`

Killing a merchant destroys the cargo and the ships, which the merchant has to buy again
(or can't, and drops a rung), and raises everyone's belief about the risk in that sector
(TRD-16). Fewer merchants then choose that route, and trade there drops. No extra system
is needed.

*See:* CHK-15 (the merchant-kill test).

## Escorts

### TRD-8 Merchants hire their own escorts `v1`

- **Only merchants who can afford it hire escorts;** poorer ones take the risk.
- **The escort is a private contract** between two characters (REL-8). No planet or
  governor is involved.
- **Hired for one trip, never on retainer,** and the job covers every leg of the trip.
- **For a faction merchant's trip,** the job is open to characters in good standing with
  that faction. *(proposed)* An independent merchant's has no standing gate.
- **It's the second rung of the warrior path** (CHR-11).

### TRD-9 When a merchant hires `v1`

The map carries a **risk number per sector** (TRD-16). A merchant sums its believed risk
over the open-space sectors on its route (the Lattice is safe) and weighs it against the
cargo's value and the price of an escort. A cheap load through safe space never bothers;
a rich load through pirate-adjacent space does. Wealth only decides whether the merchant
can make the offer. *(proposed)* The comparison runs for every trip, so hiring, hiring a
stronger escort and going without are one decision.

*Consequence:* bad information makes bad decisions, which is wanted. Joining a faction
gets you its risk beliefs; killing a faction's scouts degrades its merchants' choices.

### TRD-10 How the offer reaches an escort `v1`

- **The merchant broadcasts the offer over comms** to anyone in sensor range along the
  whole route, Lattice included. The player can accept or decline.
- **NPCs in range** accept by fit and pay (CHR-10). Warrior characters don't wait at
  planets; they travel and take what they come across.
- **A merchant who loses an escort keeps trading** and posts a new offer for the legs
  that remain. A merchant alone in dangerous space is likelier to be picked up than one
  in a quiet core system, which is emergent.

### TRD-11 Escort doctrine `v1` `rendered`

Defend the merchant and drive threats off, but don't pursue an enemy that runs. Escorts
stay on a leash. *(open)* Leash distance and escort ratio (OPN-24).

### TRD-12 Escorted merchants are a sight `v1` `rendered`

A rich merchant's escorted freighters are large and visible: six cargo ships and four
gunships in a line on the Lattice tell you someone is backing that trade. NPC loading and
unloading takes time. *(proposed)* A scan names the merchant and their faction.

## Background trade

### TRD-13 Osmosis `v1`

A **slow trickle of goods** between nearby markets keeps any market from becoming a
complete desert. It isn't done by characters, and most goods are moved by merchants.

- The flow is **proportional to the difference in amounts** (one market's surplus against
  another's shortage), reacts instantly, falls off with travel time and is faster along
  the Lattice.
- **It closes only part of a gap,** leaving margin for merchants and the player.
- **It runs as real market trades,** buying at the source and selling at the destination,
  with the transaction tax. The independent traders' margin vanishes, a small sink.
- **It stops between factions at war.** It doesn't react to raids, by design.
- *(proposed)* Contraband moves by osmosis too, at a smaller rate (OPN-33).

### TRD-14 Danger throttling `later`

Osmosis slows through dangerous space, scaled by piracy, war and patrol coverage, with a
**floor rather than a cutoff** so war can't choke the whole trade economy (OPN-32).

### TRD-15 Ambient ships `v1` `rendered`

Within the player's detection range, non-persisted ships of all kinds are **spawned from
the region and events,** as in Escape Velocity. They aren't backed by simulation state.

- **Spawn events** fire by chance near the player, at the edge of radar. The chance
  depends on the area's population and activity; the number, size and faction of ships
  come from regional strength.
- **Ambient traders match the flow:** direction, density and cargo reflect what's really
  moving on that route.
- **Regional piracy** is read from the risk number (TRD-16).
- **Persistence on contact:** an ambient ship stays real while the player interacts with
  it, and is promoted to a character only if something lasting happens (hired, or
  escaping with a grudge). Otherwise it despawns out of range.
- **Killing one doesn't touch the global economy,** but suspicion and opinion apply as
  normal, so it isn't free money.

### TRD-16 One risk number `v1`

Each sector has one **risk** value: the truth that drives ambient pirate spawns
(TRD-15), and that every character's per-sector risk belief approximates (INF-6). It
rises with pirate presence and raids, and falls with Authority mercenary contracts
(FAC-14) and the player clearing pirates.

## Missions for the player

### TRD-17 Trade-related missions `v1`

- **Escort** a merchant for pay, through comms offers in range (TRD-10).
- **Raid** a merchant for loot, or to wage economic war on its faction.
- **Trade around** a big merchant, since its arrival is about to move prices.
- **Intercept** a particular merchant: time-sensitive, worked out from sightings and
  reports to predict where it will be (INF-2). *(open)* How the player finds targets and
  what they start with (OPN-25).

Merchants are attacked **in open space**; a meeting on the Lattice in a third faction's
territory is rare enough to ignore.
