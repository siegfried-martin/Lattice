# Information (INF)

There is no instant communication. What every character knows, how it spreads, the
couriers and scouts who carry it, spies, and the suspicion that catches them.

## Facts and beliefs

### INF-1 News moves at the speed of ships `v1`

Nothing is instant except at zero distance. This applies to the player exactly as to
NPCs. The player's map shows what they've seen or been told.

### INF-2 Every character has its own information `v1`

Every character, governors included, holds a set of **facts**: prices and stock at
markets, sightings of fleets and armies, per-sector risk, army intents, the state between
factions, and *(proposed)* which gates are blockaded (MIL-17). **Each fact records when it was observed.** Characters decide on what they
believe, which may be stale. **"The planet's information" is its governor's
information.**

### INF-3 Sightings `v1`

A sighting happens when something comes within sensor range of an observer (the 4 km
rule is the rendered primitive; the simulation uses its encounter check, TIM-6). Landing
at a market observes its prices.

### INF-4 Sharing keeps the newest `v1`

When two characters share, **both keep the newest version of each fact**, so afterwards
both hold the freshest of what either knew.

- **v1: sharing happens only within a faction,** between characters sworn to it or
  holding a contract with it (faction merchants), when they're in contact: landing at a
  faction planet shares with its governor, and meeting another member shares with them.
- Independents work only from what they see themselves.
- *(later)* Information brokers, buying information, and high-standing independents
  sharing with each other. Maybe player-only, maybe never (INF-15).

*Consequences:* outside prices reach a government automatically when its merchants come
home. Faction membership is the way into the network, which is what gives the merchant
contract its value. Joining a faction gets you its intelligence; a spy sworn to a faction
gets its information when they land.

### INF-5 Facts decay at different rates `v1`

An army's position goes stale fast; its intent stays valid a long time (MIL-7). Prices
go stale as fast as merchants move them.

### INF-6 Risk is a belief `v1`

Each character holds its own copy of the per-sector risk (TRD-16), updated by visiting
the sector in open space and by what its faction shares. A merchant with stale data can
skip an escort it needed or buy one it didn't.

## Couriers and scouts

### INF-7 Two different jobs `v1`

| | Courier | Scout |
|---|---|---|
| **Where** | Inside the faction, on the Lattice | Outside the faction, mostly open space |
| **Ship** | Fast, unarmed | Some combat capability |
| **Risk and pay** | Low | Higher, longer range |

They're the bottom rungs of the two career paths (CHR-11).

### INF-8 Couriers carry the faction `v1`

Couriers carry three things: **information, orders and missions, and calls for help**.
They travel **down the fealty tree hop by hop**: the root's couriers go to its direct
vassals, and each governor with vassals runs its own courier network below it.
Information flows back up the same way.

- **A planet many hops from the root learns and is commissioned more slowly.** Loyalty
  and information lag share a cause: distance in courier time.
- **A faction that spends more on couriers** has fresher information, learns of danger
  sooner and commissions a response sooner.
- **Most courier runs are automated:** scheduled transits on the Lattice rather than
  simulated ships, with some left as missions for NPCs and the player. Couriers travel
  the Lattice, where there's no combat (WLD-2), so automation loses nothing.

### INF-16 Messages go one way `v1` `proposed`

**No exchange waits for an acknowledgement.** A message is never lost on the Lattice,
only late, and it can be stopped only where the Lattice can be: at a blockaded gate
(MIL-17), or on an open-space leg to a planet off the network. Three rules keep every
exchange working when a message is late or stopped:

1. **Contracts end on their own terms.** Both sides hold the contract, so both know
   when it ends. A lent commander goes home when its loan ends (MIL-3); the lender
   learns it's back when it lands.
2. **A request carries a deadline and a default.** "Lend me fleets, muster at X by day
   T." No answer by T is a refusal. The sender never waits: at T it acts on what it has.
3. **The fleet is the reply.** Accepting means the fleet sets out, and its arrival is
   the acknowledgement. A reply sent by courier is a courtesy, never a condition.

So a stale belief costs time or an opportunity, never a stuck state. Disagreement is
fine: each side acts on its own facts (INF-2) and the contract settles itself.

- *Consequence:* a blockade can make a loyal vassal look disloyal: its fleet is stuck
  behind the gate, the deadline passes, and the liege records a refusal. That
  misunderstanding is intended. If it proves too harsh, a fleet that arrives late can
  turn the deed into a milder "answered late".
- *Why not acknowledgements and resends:* they need protocol state that every character
  tracks, and a design for every lost reply. Couriers that nothing could stop, even a
  blockade, would remove the fragility that makes information interesting.

### INF-9 Scouts `v1`

A scout is a character generating sightings in open space, and a real, killable ship.

- **A planet scouts out of self-preservation,** when it expects an attack: border and
  fringe planets scout the enemy and send couriers home when they find troops near.
- **A faction scouts for strategy and trade:** enemy troop movements, prices in neutral
  territory. No single planet would pay for that alone.
- **Sightings aren't private:** anyone near enough to see what a scout sees gets the
  sighting too.
- A scout is a vessel of its faction and an enemy to anyone at war with it.

### INF-10 Four mission types `v1`

Planet-level and faction-level couriers, and planet-level and faction-level scouts.
**Planet-level missions are open to anyone; faction-level ones need faction
membership.**

*(proposed, roles round)* These are four kinds of the one mission primitive (MSN-2),
beside spy missions (MSN-11) and the rest of MSN.

### INF-11 Missions travel at courier speed `v1`

A faction-level mission appears at a planet when a courier delivers it. A governor who
wants a region scouted sends a courier to the relevant planet, which also brings back
that planet's information. So missions spread out through the world instead of piling up
at the capital, a player on the fringe finds jobs from whichever governor is nearest, and
the hierarchy is readable from the traffic and the mission boards.

### INF-12 The defence timing race `v1`

The invader's open-space crawl from the nearest allowed exit runs against the defender's
detection, scout return, courier run and reinforcement travel (MIL-11). Scouting earlier
means reacting earlier; **killing scouts buys surprise**.

## Spies and suspicion

### INF-13 Spies `v1`

Planets employ **agents who pose as neutral traders.** Neutral standing lets them ride the
Lattice deep into enemy territory, which is their advantage over scouts: **scouts catch
armies in transit at the border, spies catch them forming at home.** A spy who is
discovered is attacked; there is no capture (CHR-14). Spies are the last v1 milestone
(MS-7), and the no-spies experiment (EXP-4) decides whether they earn their place.

*(proposed, roles round; built in the spy round)* **Spies take missions; planets don't
employ them.** A spy is a freelancer paid per job (MSN-11, REL-8), working for any
governor or root, for the Authority, or for a pirate planet. A scout risks combat; a
spy risks detection.

### INF-14 Suspicion `v1`

One detection mechanism, shared by spies and smugglers:

- **A suspicion meter per character, per faction,** and one for the Authority.
- **Raised by behaviour:** lingering near fleets or musters, visiting military systems
  without a trade reason, cargo that doesn't fit the route, repeat visits, and a high
  pirate relation (PIR-9).
- **Lowered by genuine trading.** The cover has to be real, so good spies are good
  traders.
- **Inspection intensity** scales with how paranoid or at war a planet is and with its
  policing spend (PIR-15).
- **The player can see their own meter,** so the risk is legible and they can back off.
- *(proposed)* An inspection that finds contraband or catches a spy is a bad deed (REL-3)
  and confiscates the contraband.

*(proposed, roles round; replaces the meter above when the spy round is built)*
**Stealth against detection, counted per trip:**

- **Stealth** is a character rating. It grows a very little with each exposure survived,
  with a cap and diminishing returns. The ship adds a small modifier: a covert hull is
  stealthier (OPN-44).
- **Detection** is a stat on **planets** (set by policing, PIR-15), on **sectors**
  (derived from the patrols the open-space step finds there, TIM-6, never stored) and
  on **ships**, which matters when ships meet.
- **An exposure** is landing at an enemy or inspecting planet, or an encounter in an
  enemy sector. It adds to the character's **suspicion** by how far the place's
  detection beats the character's stealth. A high pirate relation (PIR-9), cargo that
  doesn't fit the route, and lingering all add to it.
- **Suspicion is one number per character, per trip.** It resets on landing at a planet
  not hostile to the character. Landing at the planet being infiltrated is an exposure,
  so it doesn't reset there. Working deep in enemy territory with many enemy ships
  around is harder, which is the point.
- **Past the threshold:**
  - **in enemy territory,** the mission fails and the character is **flagged hostile**
    to the detecting faction until it lands at a planet not hostile to it. There's no
    capture (CHR-14). A flagged ship that's caught is a combat loss, and the pod rolls;
  - **in lawful space,** an inspection that finds contraband confiscates it, and it's a
    bad deed.
- **The player sees their own counter,** so the risk is readable and they can back off.
- **Exposures happen** at landings and at encounters in enemy sectors, not on every
  sector entered, so the risk doesn't compound unrealistically.
- A second, lasting suspicion stat is the upgrade if Phase A shows spies never fail.

### INF-15 An information market `later`

Buying and selling intelligence is deferred. For now planets want information and hand
out scouting missions to get it. Revisit if the world doesn't feel dynamic enough.
