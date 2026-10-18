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
factions. **Each fact records when it was observed.** Characters decide on what they
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

### INF-15 An information market `later`

Buying and selling intelligence is deferred. For now planets want information and hand
out scouting missions to get it. Revisit if the world doesn't feel dynamic enough.
