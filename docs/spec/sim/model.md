# Data model (MOD)

Every record the simulation stores, with its main fields. Records live in vectors
indexed by integer ID (ARC-2). Derived values (a planet's faction, a fleet's power) are
computed, not stored, unless noted. Field lists are the v1 minimum; add fields as systems
need them, and note them here.

### MOD-1 Static world data `v1`

Generated once (GEN) and never changed by the simulation.

| Record | Fields |
|---|---|
| **Sector** | hex coordinates, centre, culture region, its system (at most one); neighbours are computed |
| **Region** | culture (its index), seed sector, production weights per lawful good, population scale |
| **System** | sector, centre, fringe flag, planets and moons |
| **Group** | system, member planets, holder planet (WLD-5), interchange node (if on the network) |
| **Lattice node** | interchange or junction, position, the group an interchange serves, edges |
| **Lattice edge** | two nodes, length (sets transit time) |
| **Hop lane** | two planets, length |
| **Seats of power** | the planets chosen as seats (GEN-8) |
| **Ship variant** | culture, class (courier, scout, freighter, gunship), tier 1–3, combat power, trade power (cargo, tons), speed in open space, Lattice Drive (yes/no), price; every culture has every class at every tier (MS-3). Names wait for the content round (OPN-35) |
| **Good** | kind (life support, luxury, contraband), base price, reserve days, price exponent, consumption per capita, happiness value |

Goods are a fixed list in code (food, water, oxygen; wine, spice, silk; stims, the
names flavour); their numbers are config (`good.<name>.*`), not stored records.

**Derived, not saved:** reference travel times between planets (TIM-4), the osmosis
pairs (SYS-6) and each region's planets are rebuilt from the map after generation and
after loading.

### MOD-2 Planet `v1`

| Field | Notes |
|---|---|
| sector, region, system, group, moon of, position | static |
| culture | static (WLD-16): its region |
| fringe | static: off the network (WLD-6) |
| governor | character ID (FAC-1) |
| population, happiness | PRD-2, PRD-4 |
| prosperity | derived index, stored for capture effects (FAC-10, SYS-13) |
| stock per good | the market |
| goods made | three goods, each with base efficiency tier (high, medium, low) |
| efficiency modifiers | tier shift per good made, and festivals running, from active events (PRD-11) |
| labour share per good made | PRD-9 |
| productivity, contraband average | SYS-1 |
| treasury | may be negative (ECO-8) |
| income average, income today | for the credit limit (SYS-3) |
| tax rate | ECO-3 |
| tribute rate | set by its liege (FAC-6) |
| income this month, tribute received this month, last tribute paid | for tribute (FAC-6) and patronage (SYS-5) |
| low since | when its governor's opinion of the liege fell below the defection threshold (FAC-11) |
| growth factor | from governor's risk tolerance (PRD-3) |
| patrol upkeep, policing spend | ECO-4, PIR-15 |
| static defence | current strength, maximum (MIL-5) |
| fuel price | fixed at world build (ECO-11) |
| liege | the planet whose governor this one's is sworn to, or none for a root; fealty is kept per planet because governor and planet are one to one (FAC-1) |
| pirate | flag, plus the goods-made set to restore on recapture (PIR-3); *as built*, also the tiers, and the contraband its population bought today |
| deficit days | days in deficit, for turning pirate (ECO-13) |
| mission board | list of mission IDs posted here |

### MOD-3 Character `v1`

| Field | Notes |
|---|---|
| name, culture, alive | |
| risk tolerance | 0–1 (CHR-4) |
| wealth | credits (CHR-8) |
| ships | list of variant IDs; the first is the flagship |
| location | at planet, in Lattice transit (edge path, depart and arrive times), or in open space (sector, offset, velocity) (TIM-5, TIM-6) |
| skills | per mission kind: courier, scout, trade, escort, command, smuggle (CHR-10) |
| reputation | REL-7 |
| contracts | contract IDs held (REL-8) |
| opinions | sparse map: other character ID → opinion value, plus recent deeds for the UI (REL-2) |
| facts | the character's information (MOD-8) |
| pirate status | none, full pirate (PIR-11); *as built*, also the pirate planet it's heading to when turning (PIR-4) and its last fight in open space |
| suspicion | per faction root, plus Authority (INF-14); *as built*, a sorted list of (faction, value, last updated), fading lazily; and a pirate run offered on contact (PIR-13) |
| goal, action | current long-term goal and short-term action (CHR-2) |
| broke since | for running out of money (PIR-4) |

*As built (MS-3)*, also: the planet it's at, the planet where it last landed (and
refuelled), its route and current leg, what it governs, its mission, cargo (one good,
quantity, cost), its trip's remaining stops and profit so far, running averages of cargo
and combat earnings (SYS-8), missions completed, when it last shared in open space, and
when it last weighed a salary. Opinions, suspicion and pirate status arrive with their
systems.

The pirate faction's shared opinion of each character (PIR-9) and the Authority's
opinion (FAC-14) are stored as two fields on the character.

### MOD-4 Contract `v1`

| Field | Notes |
|---|---|
| kind | fealty (governor to governor), service (commander to governor), merchant salary, escort, courier, scout, security, mercenary task, pirate hire |
| parties | employer, employee |
| pay | amount and schedule (per period, or on completion) |
| mission | the mission a one-job contract carries out |
| terms | trip, target or region, deadline |
| start time | |

Ending a contract removes it from both parties (REL-8).

### MOD-5 Faction relations `v1`

Factions aren't stored (FAC-2); a planet's faction is its root, found by walking up the
tree (cache per tick). Between each pair of neighbouring roots:

| Field | Notes |
|---|---|
| state | war, peace, ceasefire (DIP-1) |
| since | start time |
| ceasefire terms | payer, payee, amount per period, end time (DIP-2) |
| war ledger | planets and fleet power lost by each side since the war began, and the enemy income near each side's border at the declaration, for suing for peace (DIP-3) |

When a root changes (defection, capture of the root's planet), relations carry over to
the new root *(proposed)*: the breakaway starts at peace with everyone except the pirates.

*As built (MS-5):* relations are records between two root planets; a pair with no
record is at peace. A record also keeps when the last war ended and each side's income
then (for the war vote), and each side's planets and power at the war's start (for
suing).

### MOD-6 Army `v1`

| Field | Notes |
|---|---|
| commissioner | governor ID (MIL-9) |
| members | commander IDs |
| intent | target planet (MIL-7), or a gate (MIL-17) with its holder as the target |
| action | muster, move, besiege, retreat, disband |
| location | as a character's, moving at the slowest member's speed |
| called | commanders on their way to the muster; also its home planet, whether it is a relief, its battle and siege (MS-5) |

### MOD-7 Siege and battle `v1`

| Record | Fields |
|---|---|
| **Battle** | location, sides (character IDs per side), start time, rounds fought, losses so far (MIL-13) |
| **Siege** | planet, attacking army, start time, remaining static defence, defenders present (MIL-10) |
| **Gate** *(proposed, round 2)* | per Lattice node: static defence, the attack it's under, captor, since, garrison (MIL-17) |

*As built (MS-5):* a battle keeps each side's characters, armies, damage carried over
and power lost, and whether side 1 has the planet's static defence. The static defence is
kept on the planet with its security spending (MOD-2). A siege keeps whether its call
for help has gone out and whether it was relieved.

### MOD-8 Fact `v1`

A fact is `(subject, value, observed time)`. *(Round 2: also each gate's captor, MIL-17.)*
*As built (MS-5):* prices per market, risk
per sector, the state between two roots, and the latest sighting of each army (position,
target, faction, power), and whether a planet is a pirate planet (MS-6). Each is a sorted
list merged newest-wins. Subjects in v1:

| Subject | Value |
|---|---|
| price and stock of good g at planet p | price, stock |
| risk of sector s | risk |
| sighting of character or army x | sector, offset, time; army intent if seen |
| state between roots a and b | war, peace, ceasefire |
| pirate planet p | exists, location |
| threat to planet p | the army sighted, for calls for help |

*(proposed)* Storage: each character holds a small sorted vector of facts keyed by
subject; governors and merchants hold full price tables for the planets they know, about
100 planets × 7 goods. At 2,000 characters that's about 1.4 million price entries if
everyone held everything; in practice merchants and governors do, and most others hold
little. Sharing is a merge of two sorted vectors keeping the newer of each key (INF-4).

### MOD-9 Mission `v1`

| Field | Notes |
|---|---|
| kind | courier, scout, security, smuggling, legal goods, trade delivery; *as built*, a pirate run also names its good and quantity |
| level | planet or faction (INF-10) |
| poster | governor ID (or pirate planet) |
| at | planet where it's posted |
| target | planet, sector or region |
| pay | grows the longer it sits (CHR-10) |
| requirements | minimum combat or trade power, faction membership |
| posted, expires | times |
| state, taker, agreed pay, leg | open, taken, done or expired; the pay is fixed when taken; outbound or returning |
| packet | the facts a courier or scout carries |

### MOD-10 World event `v1`

`(kind, planets or region, good, efficiency or consumption change, start, end)` for
PRD-11. *As built:* kind, region (regional events), the planets it affects, the good,
start, end and an alive flag; the change follows from the kind (SYS-4). Ended events
keep their slot (ARC-2).

### MOD-11 The scheduled-event queue `v1`

| Field | Notes |
|---|---|
| time | game seconds (TIM-1) |
| phase | scheduled, cadence or kinematic: the order among events at the same time (TIM-2) |
| sequence number | assigned on push; breaks the remaining ties, so the order is total |
| kind | what to run |
| record ID | the record it concerns, or none |
| period | above zero: rescheduled this far ahead after it runs (cadences, TIM-3) |
| payload | two integers, their meaning set by the kind |

Ordered by time, then phase, then sequence number (TIM-2).

### MOD-12 World ledger `v1`

Cumulative world totals kept for the checks (CHK-7): money created (population spending,
ECO-1), money sunk (operating costs, the osmosis margin and later every other sink),
goods produced and consumed per good, osmosis volume and margin. The total of all money
held always equals the starting total plus money created minus money sunk; a test holds
the simulation to it.

### MOD-13 Automated courier run `v1`

A scheduled transit, not a ship (INF-8): from and to planets (the liege's and the
vassal's), whether it is returning, and the copy of the facts it carries. Runs that have
ended keep their slot with `alive = false` (ARC-2).

### MOD-14 The Trade Authority `v1`

Not a fealty tree and holds no planets (FAC-13): a treasury fed by dues, less what it
spends on the network. Its opinion of each character is a field on the character
(MOD-3).

### MOD-15 Sector risk `v1`

Per sector (SYS-19): the risk now, and what feeds today's update: raids, hostile
ship-days and patrol days. The truth that characters' risk facts approximate.
