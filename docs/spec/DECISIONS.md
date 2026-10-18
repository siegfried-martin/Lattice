# Decisions

A dated log of every change to the spec, newest first, and the ideas that were rejected so
they aren't reintroduced. One line per decision; the rule holds the detail.

## Log

### 2026-10-18 · MS-5 part 1, war

- **Correction:** escorts come with risk in MS-5, not MS-6 as MS-3's entry said; MS-5's
  scope already lists them.
- **Diplomacy:**
  - weekly war vote and suing with first-guess weights;
  - no declarations in the warm-up;
  - relations between roots, forgotten by a new root and inherited by a captured root's
    heir;
  - declarations take effect at once until DIP-5 is built (SYS-12).
- **Combat:** damage carries between rounds; zero-power ships go first; the strongest
  fleet's risk tolerance sets the retreat point; pods land at the nearest planet not at
  war with the pilot (SYS-14).
- **Defence:**
  - static defence is a budget option bought in units of monthly security spend;
  - defence and invasions are worth more at war and on the border;
  - static defence rebuilds daily when not besieged (MIL-5, SYS-5).
- **Invasions:**
  - a one-unit budget option on P(win) = A² ÷ (A² + D²);
  - vassals answer by opinion;
  - armies move as one around enemy gates;
  - static defence is only damaged once the defending ships are gone;
  - relief goes at P(win) of at least a third, or the call goes up the tree;
  - keep or grant scores protection by travel days (SYS-13, FAC-8, FAC-9).
- **Capture of a root:** its most populous vassal becomes the root, with the wars.
- **Goods ledger:** capture losses and destroyed cargo are recorded. A merchant carries
  one good at a time.

### 2026-10-18 · MS-4, politics

- **Opinion** fades lazily (the same as a weekly pass) and keeps the last
  `rel.recent_deeds` deeds per relationship (SYS-10).
- **Tribute** is paid at the vassal's monthly payment on its income since the last one,
  cascading; the liege's tools are rules: raise on the loyal when in the red, lower for
  the wavering, each a deed, with `deed.tribute_lowered` new (SYS-11, FAC-12).
- **Defection** goes to the rival root that thinks best of the defector within
  `fealty.threat_days`, else independence; a "not set" time is now `kNever`, not −1
  (SYS-11).
- **Budget:** commanders and patronage as unit options valued in money per month, with
  the `value.*` heuristics; risk eases diminishing returns as
  `budget.diminishing ^ (1 − risk ÷ 2)`; the existing commanders are funded first
  (SYS-5, SYS-9). Every value is a first guess for Phase A.
- **Commanders** serve at their governor's planet in peacetime; starting commanders are
  one gunship each, up to `gen.commanders_per_income` × income (GEN-9).
- **Dues** at each planet's monthly payment; the Authority keeps its surplus until it
  can hire mercenaries (SYS-20).

### 2026-10-18 · MS-3, characters, movement, information and trade

- **Order:** fealty trees (GEN-8) and governors (GEN-9) are built in MS-3, ahead of MS-4,
  because faction merchants, sharing and couriers need them. Escorts move to MS-6 (no
  risk before pirates); sightings to MS-5.
- **Ships:** every culture has four classes (courier, scout, freighter, gunship) at three
  tiers, from per-class stats scaled per tier and varied per culture, as placeholders for
  the content round (OPN-35).
- **Movement:** journeys are routes of legs; open space is straight legs with known
  times, interpolated by the kinematic step, which hashes only ships something could
  happen to (TIM-5, TIM-6). Fuel is per Lattice transit, at the last landing's price.
- **Facts** are per-market price records; members are governors and holders of standing
  contracts; members share on landing at a faction planet and on meeting in open space
  (INF-4, SYS-15).
- **Merchants:** slice-by-slice leg sizing, cargo aboard sold first, docking time, a
  faction merchant heads home when away, idle merchants explore stale markets (SYS-7).
  Every root offers a fixed salary until the budget model exists (SYS-9).
- **Missions:** courier missions are a share of the liege-to-vassal runs; roots post
  trade scouting as planet-level missions open to anyone *(proposed)*; missions are round
  trips paid on return; reputation is missions completed (SYS-8, REL-7).
- **Spending:** a ship of the class that has paid (freighter or gunship), added or
  replacing the cheapest one tier up; broke characters sell ships (SYS-8, PIR-4).
- **Found, not acted on:** merchants can't fix the starting values' food shortage, and the
  happiness loop is bistable (MS-3's note).

### 2026-10-18 · MS-2, world generation and the economy

Choices made while building, all *(proposed)* where they go beyond a rule:

- **Scale:** the planet count includes moons, every body a settled market (GEN-1).
  Distances are in sector widths, and reference travel speeds follow the prototype's
  scale, a week to cross a sector in open space and half a day on the Lattice (TIM-4).
- **Generation:** at most one system per sector, placed by smoothed random density with
  regions taking turns so each has enough planets (GEN-3); large systems split into
  inner and outer groups (GEN-3); fringe chosen toward edges and region borders (GEN-3);
  junctions where three or more roads meet an interchange (GEN-4); the producer repair
  never creates a new shortfall (GEN-6); seats of power are chosen now, for fuel prices
  (GEN-7, GEN-8); fringe fuel × `fuel.fringe_mult` (GEN-7).
- **Economy:** production runs before buying; a short budget scales life-support
  purchases down evenly; growth's excess is capped at 1; the variety threshold is a share
  of a luxury's full use, correcting SYS-1's formula, which made variety impossible
  (SYS-1). A tier below low is low × low ÷ medium (SYS-2). Events don't stack (SYS-4).
- **Osmosis** stops when the sale no longer covers the purchase, so it can never create
  money (SYS-6, ECO-1).
- **Saves:** each record lists its fields once; loading checks every index (ARC-9).
- **Found, not acted on:** with the starting values the world can't feed itself
  (MS-2's note). Tuning waits for merchants and Phase A.

### 2026-10-18 · MS-1, the simulation skeleton

- **Build:** the Godot binding is built with godot-cpp's CMake support, not SCons, so
  one build system covers the core, tests, runner and binding (ARC-11, ARC-12).
- **Determinism:** the RNG distributions are written in the core, and FMA contraction
  is off, so builds at different optimisation levels agree (ARC-6).
- **Event order:** events at the same time run by phase (scheduled, cadence, kinematic),
  then sequence number; `advance(dt)` covers `[now, now + dt)`; cadences are recurring
  events in the one queue (TIM-2, MOD-11).
- **Saves:** magic, version, payload and hash; config stored by key; a build reads only
  its own version (ARC-9).
- **Binding:** `make gdext` registers the extension itself, working around a Godot 4.7
  crash on the first discovery of a godot-cpp extension (ARC-4).

### 2026-10-18 · The spec is created

The planning drafts `FACTIONS.md` (2026-09-27), `ECONOMY.md` (2026-09-28), `TRADE.md`
(2026-10-12) and `CHARACTERS.md` (2026-10-17) were consolidated into `docs/spec/` and
removed from `docs/planning/`. They're preserved at the git tag `planning-drafts-1`. The
PR that made this change carries a coverage table mapping every draft section to its
rule.

Decided in the review rounds of 2026-10-17 and 2026-10-18 and recorded here for the
first time:

- **Process:** the planning folder is for ideation; the spec is the source of truth; drafts
  are deleted once formalised (README).
- **Characters:** stewards removed (REJ-8); standing politicians removed (REJ-9); the
  governor who keeps a conquered planet chooses its governor, preferring the capturing
  commander (FAC-9); pod death means reload and the game autosaves on landing (CHR-18);
  running costs are low, not zero (CHR-9); most couriers are automated (INF-8); the
  merchant path ends at rich merchant (CHR-11); patrols are procedurally generated, with
  optional paid security missions (MIL-5).
- **Fealty:** the fealty deal (FAC-5); tribute (FAC-6); patronage as investment (FAC-7);
  keep or grant (FAC-8); money and relationships as the only two systems, loyalty and
  favor as opinion read from each side, deeds and fading (REL-1 to REL-4); defection
  from sustained low opinion plus an alternative (FAC-11).
- **Merchants:** only the root pays faction merchants, who get the information network
  and run three trip shapes (TRD-3); independents run single trips (TRD-4); planet
  merchant fleets dropped (REJ-25).
- **Information:** every character has its own facts; sharing keeps the newest; for now
  only within a faction (INF-2, INF-4).
- **Contracts:** either side can end any contract with no penalty (REL-8).
- **How far the player can rise:** a player governor who leaves fealty becomes a root,
  by the ordinary rules (CHR-17). This closes the drafts' open question.
- **Standing** is the governor's or root's opinion (REL-5); local opinion is experimental
  (REL-6). Independent single trips are open to all but pirates; faction missions need
  membership (TRD-5, INF-10).
- **Piracy:** one collective pirate faction, an anarchy (PIR-1); broke commanders turn
  pirate, with the risk-tolerance split experimental (PIR-4); loot sells only to pirate
  planets, whose prices steer raids (PIR-5); no redemption for ships (PIR-7); pirates
  raid, don't invade (PIR-8); pirate planets make only contraband and live on missions
  and raids (PIR-3); the pirate relation as shield and suspicion (PIR-9, PIR-10);
  smugglers are the disguised pirates, in v1 (PIR-12).
- **Fuel** is a fixed local price in v1, not a good (ECO-11).
- **Production v1:** three life-support goods, three luxuries, one contraband (PRD-1);
  growth from the scarcest life support (PRD-2); three goods per planet at high, medium
  and low efficiency, drawn partly regionally (PRD-7); labour shifts toward what pays
  (PRD-9); price from stock as a smooth curve shown in bands (PRD-10); efficiency events
  (PRD-11); the governor's growth factor from risk tolerance (PRD-3).
- **Diplomacy:** war, peace and ceasefire, with suing for peace (DIP-1 to DIP-3).
- **Sieges** in the simulation (MIL-10); a simple combat formula for now (MIL-13).
- **Simulation:** C++20 core with a thin godot-cpp binding (ARC-1, ARC-4); world
  generation in C++ (ARC-10); all milestones built before judging, small worlds for
  building, verdicts at v1 scale (SIM-2); the goals of stability then ablation (SIM-2);
  spies in v1, built last (INF-13, MS-7); academies and aliens out of v1 (SIM-3).

Proposed in the same rounds and not yet signed off (tagged `proposed`): the war vote
(DIP-4), declarations by courier (DIP-5), ceasefire terms (DIP-2), siege detail
(MIL-10), the tribute cascade (FAC-6), prosperity loss on capture (FAC-10), escort pay at
trip end (REL-9), refusing a call as a deed (REL-10), reputation (REL-7), full pirate
status (PIR-11), the pirate top-up (PIR-8), and every `proposed` formula in `sim/`.

### 2026-09-27 to 2026-10-17 · The planning drafts

Four rounds of drafts and reviews, merged as PRs #24–#34. Their content is in the spec;
their text is at the tag `planning-drafts-1`.

## Rejected

Ideas considered and dropped. Don't reintroduce one without a new reason.

| ID | Rejected | Why | Replaced by |
|---|---|---|---|
| REJ-1 | The Lattice as a natural wormhole | Breaks the built, trucker's-highway feel | WLD-1 |
| REJ-2 | A committee of three powers running the Lattice, with proxy wars and cores that never fall | Static | Emergent fealty trees and the Authority (FAC-2, FAC-13) |
| REJ-3 | A Trade Authority with planets and a large army | Makes it a player in wars | Hired mercenaries (FAC-14) |
| REJ-4 | Three fixed factions | Static | Seats of power in an emergent structure (FAC-2) |
| REJ-5 | Faction- or culture-specific Lattice technology | Variety belongs to hardware | WLD-20 |
| REJ-6 | "Threader" | A speech-to-text error | The Lattice Drive (WLD-8) |
| REJ-7 | Pirate range bounded by Lattice fuel | Pirates don't use the Lattice | Slow open-space travel (PIR-2) |
| REJ-8 | Stewards running planets for governors in the field | Characters rise organically instead | FAC-9 |
| REJ-9 | A pool of standing politicians without planets | No job once governors are chosen from existing characters | FAC-9 |
| REJ-10 | Prisoners, ransom, exchange, captivity | Clunky, and from a historical setting Lattice isn't in. Never in the game. | Escape pods (CHR-15) |
| REJ-11 | A closed money loop (wages from production) | Money bunches up in one part of the economy | ECO-1, ECO-5 |
| REJ-12 | Governors owning and lending fleets | Breaks the player's ownership of their ships | CHR-8 |
| REJ-13 | No fleet running cost at all | Money piles up and fleet size isn't tied to income | Low running costs (CHR-9) |
| REJ-14 | Favor as a currency or ledger, alongside loyalty and money | Three systems where two do the job | REL-1 |
| REJ-15 | Reputation or opinion penalties for ending contracts | Players mustn't feel locked in | REL-8 |
| REJ-16 | Money from population alone | A blockaded planet would keep printing income | ECO-1 |
| REJ-17 | Money from trade taxes alone | Doesn't explain where money enters | ECO-1 |
| REJ-18 | Private producers and shop owners | Accounts without decisions anyone sees | ECO-2 |
| REJ-19 | Dropping gate dues without going pirate | A closed exit the player can't explain | ECO-10 |
| REJ-20 | Resetting a new pirate planet's productivity | A special case | Low cost base (ECO-14) |
| REJ-21 | Goods destruction as a money sink; sinks only in combat and conquest | Money keeps circulating, and piles up in peace | ECO-5 |
| REJ-22 | Dependence, or lost tax base alone, as contraband's downside | Not sensible; too weak | The productivity penalty (PRD-6) |
| REJ-23 | Discrete price levels internally | Every merchant sees the same jump and herds | Smooth curve shown as bands (PRD-10) |
| REJ-24 | Agent auction pricing (BazaarBot style) | Hard to tune and read; beliefs already give emergence | PRD-10 |
| REJ-25 | Planet merchant fleets | No motivator once merchants are characters paid by price | TRD-1, TRD-3 |
| REJ-26 | The planet as matchmaker for convoys and escorts | Adds waiting, duplicates price | TRD-2, TRD-8 |
| REJ-27 | Rust with gdext for the simulation | 0.x with breaking changes, few shipped games, weaker Windows story | C++ (ARC-1) |
| REJ-28 | Fuel and munitions competing with cargo space | Makes buying consumables feel bad | ECO-12 |
| REJ-29 | Separate legality lists for the Authority and each faction | Unreadable | PIR-14 |
| REJ-30 | Catalogue equipment through the supply chain | Too specific | PRD-14 |
| REJ-31 | A supply chain of pilots | Replaced by academy worlds, which were then dropped too (REJ-39) | CHR-6 |
| REJ-32 | Governors hiring procedurally generated escorts for planet convoys | Planet convoys are gone; escorts are characters | TRD-8 |
| REJ-33 | The invading commander automatically becoming governor | The keeper chooses, preferring them | FAC-9 |
| REJ-34 | A fracture-pressure rule by distance or size | Emerges from deeds | REL-4 |
| REJ-35 | Pirates as individuals, enemies of everyone including each other | Simpler as one anarchic faction | PIR-1 |
| REJ-36 | Purely random production draws | Every good ends up close to everywhere | PRD-7 |
| REJ-37 | Trading standing for money | Money is never traded for opinion | REL-1 |
| REJ-38 | A greed trait | Risk tolerance covers it | CHR-4 |
| REJ-39 | Academy worlds training crew of their culture | Not needed; new characters start at low level | CHR-6; crew hiring stays open (OPN-36) |

## Retired IDs

None yet.
