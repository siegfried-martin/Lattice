# World and travel (WLD)

The map, how ships move across it, who may use the Lattice, and the static geography
(cultures and regions) that politics moves across. The prototype's travel feel is
recorded in `docs/WORLD_AND_TRAVEL.md`; this file holds the rules the full game adds.

## The map

### WLD-1 The Lattice is built infrastructure `v1`

The Lattice exists and nobody questions why. It's built (roadway, lamps, docking, a
trucker's-highway feel), not a natural wormhole. Every planet wants to be on it, because
being cut off from interstellar travel and commerce is ruinous, and it sustains itself
on that incentive.

*See:* REJ-1.

### WLD-2 No combat on the Lattice or hop lanes `v1`

Nothing can attack anything on the Lattice or on a hop lane. All fighting happens in
open space.

*Why:* the Lattice is the safe road; danger lives off it.

### WLD-3 Map and scale `v1`

The map is a grid of hexagonal sectors (`docs/WORLD_AND_TRAVEL.md`). The first full
world is **10×15 sectors (150) with about 100 planets**, to be judged by feel; the
prototype's 4×4 map is far too small. Development worlds can be smaller (sim/worldgen
GEN-1), but tuning is judged at full v1 scale.

*See:* OPN-20 (how large the real map gets).

### WLD-4 Systems, groups and gates that serve places `v1`

A **system** is one to four planets in a sector; about a quarter of planets have one to
three moons. Each **group** of nearby planets (a planet with its moons, or a cluster in
a system) has **one Lattice entrance and one exit**, beside it or a short hop-lane trip
away. The exit's boost (WLD-9) is aimed at the group it serves.

### WLD-5 The gate holder is fixed and is the prize `v1`

Each group's gates are held by its **most prosperous planet or moon, chosen at world
build and never changed**. Conquest changes who governs the holder, not which planet
holds. The holder controls every other planet's way on and off the Lattice there, pays
the gate-holding tier of dues (ECO-10), and is usually the most heavily guarded planet
of its group.

### WLD-6 Fringe worlds `v1`

Some planets are off the network, reachable only by open space or hop lanes. They're at
a disadvantage, softened by natural resources in high demand (legal or contraband) or by
being pirate systems. The fringe is where lasting piracy lives (PIR-2).

### WLD-7 Service and faction stations `v1`

Service stations sit beside some interchanges and sell fuel (ECO-11). *(later)* Faction
stations in large empty stretches of a faction's region, owned by the faction.

## Travel

### WLD-8 The Lattice Drive and Lattice fuel `v1`

Entering the Lattice takes a ship fitted with a **Lattice Drive**, which burns Lattice
fuel per distance travelled on the Lattice. Open space costs no Lattice fuel. Ships
refuel only at planets and stations. Running dry drops the ship out of the Lattice into
open space at its mapped position, with its heading and speed. *(proposed)* A small
reserve lets a ship limp rather than sit dead.

*See:* ECO-11 (fuel price), ECO-12 (tank size is a fitting choice).

### WLD-9 Exits give a boost `v1` `rendered`

Leaving by an exit ramp sends the ship out through the gate's frames with a speed boost
aimed at the destination the ramp was built to serve. The direction is fixed by the
ramp. This is what makes exits the normal way off the network.

### WLD-10 Bailing out `v1`

The player can leave the Lattice into open space at any point, at its mapped position,
with its current heading and speed and no boost. Knowing when it pays (a station with no
exit of its own, sneaking round an exit with security scans) is earned map knowledge.
**NPCs never bail out** except when out of fuel; armies never do (MIL-8). Destinations
nobody built a ramp for (new outposts, pirate havens) are reached by bailing out or
flying.

### WLD-11 Hop lanes are open to everyone `v1`

Hop lanes are open to any ship, pirates included, so a slow freighter isn't punished for
visiting planets far from a system's exit. Combat abuse near them is not a concern for
now.

## Access

### WLD-12 Gates have two keys `v1`

A ship passes a gate only if both the **Trade Authority** and the **holding planet's
faction** allow it.

- **The Authority** decides who may be on the network at all. It admits anyone whose
  standing with it is at least neutral, and refuses known (full) pirates at every
  on-ramp (PIR-11). *(open)* Whether the bar is neutral or positive (OPN-21).
- **The faction** controls the gates its planets hold. A ship needs neutral or better
  standing with the faction (its root, REL-5). Members of a faction at war with it are
  refused (DIP-6).
- *(proposed)* A pirate planet's gates are shut with the Authority's key.
- *(open)* Whether junction gates, which no planet holds, have only the Authority's key
  (OPN-22).

A refused player always has a legible reason, and can still bail out before the gate
and fly in, without the boost.

### WLD-13 What the two keys produce `v1`

- **Enemy forces can't exit in hostile territory.** They leave the Lattice at the
  nearest exit to their target that will let them out and cross the rest in open space,
  slowly and visibly. That's the defender's home-field advantage.
- **Fronts advance gate by gate.** Taking a holder planet flips its gates and gives the
  attacker a new exit one step deeper.
- **A pirate-held interchange is dead in both directions:** pirates can't board, and
  nobody else can exit there.
- *(proposed)* Neutral planets decide whether to let a passing army out, possibly for a
  price.

### WLD-14 Landing permission `v1`

Landing takes the planet's permission, as in Escape Velocity. Neutral or better standing
with the planet (its governor, REL-5) gets permission; below that it's refused.
Permission takes a real wait, shorter with better standing and reputation, with a
different message at each level. Landing is a safety net: the clock stops while the
player is landed (WLD-15), so a ship that gets permission can land its way out of
trouble. The wait is where the tension comes from.

## Time

### WLD-15 The clock `v1`

Game time runs at **one real minute per in-game day** to start (a value to tune). **The
clock stops while the player is landed**, so they can plan and shop without pressure.

*See:* TIM-1.

## Cultures and regions

### WLD-16 Cultures are regional and static `v1`

A **culture** is a region of the map. It stays where it is whoever holds the planets.
Factions are political and move; cultures don't. At the start, cultures and factions
roughly line up, and a large faction may hold several.

### WLD-17 Culture decides hardware availability `v1`

Each culture has its own flavour of ships and weapons. Any market has a chance of
stocking another culture's hardware, falling off with distance from that culture's
region out to about two or three sectors. Availability is **geographic, not political**:
owning a planet doesn't spread its culture's gear to the rest of your faction. Conquest
across a cultural border gains access to that culture's gear through those planets,
which is a conquest incentive alongside wealth.

*See:* PRD-14 (specific equipment is stocked by chance, outside the supply chain).

### WLD-18 Every character has a culture `v1`

Governors, commanders, merchants and the player each have a culture. Planets acquire
commanders of their own culture more easily, and commanders recruit ships of their own
culture more easily; the player is subject to the same effects.

### WLD-19 Culture carries loyalty `v1`

A planet is less likely to defect when more planets of its own culture are in the same
faction. *(proposed)* A commander's culture does the same for the commander.

*Consequences:* a multicultural faction has the broadest arsenal and the weakest
cohesion, so crossing a cultural border is a real decision and works against snowballing.
Faction lines drift from culture lines over time, and the mismatches are where defection
is likeliest. A faction's fleets take on its cultural makeup.

*See:* FAC-11 (how loyalty is modelled).

### WLD-20 Human Lattice technology is uniform `v1`

The Lattice is built the same way across all human factions and cultures. Variety
between regions comes from culture's ships and weapons.

*See:* REJ-5.

### WLD-21 The danger gradient emerges `v1`

Nobody authors where danger is. Near the Lattice, lawful forces arrive fast and pirates
are slow (PIR-2), so the civilised region rarely has pirates. The fringe is pirate-heavy,
less lawful and more dangerous. The player can read the gradient before entering it.

*See:* CHK-4.

## Aliens

### WLD-22 Alien factions `later`

Not in v1. As designed so far: each is a unique species with some unique technology and
none or some human technology. They don't follow the feudal system: a hive mind with a
fixed hierarchy that can't split. Their default transport is larger-scale hop lanes; one
may be connected to the Lattice through an old alliance. They can ally with factions and
the player, and exist mainly to add danger and variety for advanced players.
