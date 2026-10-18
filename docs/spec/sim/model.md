# Data model (MOD)

Every record the simulation stores, with its main fields. Records live in vectors
indexed by integer ID (ARC-2). Derived values (a planet's faction, a fleet's power) are
computed, not stored, unless noted. Field lists are the v1 minimum; add fields as systems
need them, and note them here.

### MOD-1 Static world data `v1`

Generated once (GEN) and never changed by the simulation.

| Record | Fields |
|---|---|
| **Sector** | hex coordinates, culture region, neighbours, fuel price base |
| **Region** | culture, production weights per good |
| **Group** | sector, member planets, holder planet (WLD-5), interchange node (if on the network) |
| **Lattice node** | interchange or junction, position, edges |
| **Lattice edge** | two nodes, length (sets transit time) |
| **Hop lane** | two planets, length |
| **Ship variant** | name, culture, class, tier, combat power, trade power (cargo), speed in open space, Lattice Drive (yes/no), price |
| **Good** | kind (life support, luxury, contraband), base price, reserve days, price exponent, consumption per capita, happiness value |

### MOD-2 Planet `v1`

| Field | Notes |
|---|---|
| sector, group | static |
| culture | static (WLD-16) |
| governor | character ID (FAC-1) |
| population, happiness | PRD-2, PRD-4 |
| prosperity | derived index, stored for capture effects (FAC-10, SYS-13) |
| stock per good | the market |
| goods made | three goods, each with base efficiency tier (high, medium, low) |
| efficiency modifiers | active events (PRD-11) |
| labour share per good made | PRD-9 |
| treasury | may be negative (ECO-8) |
| tax rate | ECO-3 |
| tribute rate | set by its liege (FAC-6) |
| growth factor | from governor's risk tolerance (PRD-3) |
| patrol upkeep, policing spend | ECO-4, PIR-15 |
| static defence | current strength, maximum (MIL-5) |
| fuel price | fixed at world build (ECO-11) |
| pirate | flag, plus the goods-made set to restore on recapture (PIR-3) |
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
| pirate status | none, full pirate (PIR-11) |
| suspicion | per faction root, plus Authority (INF-14) |
| goal, action | current long-term goal and short-term action (CHR-2) |
| broke since | for running out of money (PIR-4) |

The pirate faction's shared opinion of each character (PIR-9) and the Authority's
opinion (FAC-14) are stored as two fields on the character.

### MOD-4 Contract `v1`

| Field | Notes |
|---|---|
| kind | fealty (governor to governor), service (commander to governor), merchant salary, escort, courier, scout, security, mercenary task, pirate hire |
| parties | employer, employee |
| pay | amount and schedule (per period, or on completion) |
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
| war ledger | planets and fleet power lost by each side since the war began, for suing for peace (DIP-3) |

When a root changes (defection, capture of the root's planet), relations carry over to
the new root *(proposed)*: the breakaway starts at peace with everyone except the pirates.

### MOD-6 Army `v1`

| Field | Notes |
|---|---|
| commissioner | governor ID (MIL-9) |
| members | commander IDs |
| intent | target planet (MIL-7) |
| action | muster, move, besiege, retreat, disband |
| location | as a character's, moving at the slowest member's speed |

### MOD-7 Siege and battle `v1`

| Record | Fields |
|---|---|
| **Battle** | location, sides (character IDs per side), start time, rounds fought, losses so far (MIL-13) |
| **Siege** | planet, attacking army, start time, remaining static defence, defenders present (MIL-10) |

### MOD-8 Fact `v1`

A fact is `(subject, value, observed time)`. Subjects in v1:

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
| kind | courier, scout, security, smuggling, legal goods, trade delivery |
| level | planet or faction (INF-10) |
| poster | governor ID (or pirate planet) |
| at | planet where it's posted |
| target | planet, sector or region |
| pay | grows the longer it sits (CHR-10) |
| requirements | minimum combat or trade power, faction membership |
| posted, expires | times |

### MOD-10 World event `v1`

`(kind, planets or region, good, efficiency or consumption change, start, end)` for
PRD-11.

### MOD-11 The scheduled-event queue `v1`

`(time, sequence number, kind, record ID, payload)`, ordered by time then sequence
(TIM-2).
