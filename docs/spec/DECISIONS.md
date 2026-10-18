# Decisions

A dated log of every change to the spec, newest first, and the ideas that were rejected so
they aren't reintroduced. One line per decision; the rule holds the detail.

## Log

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
