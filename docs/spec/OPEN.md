# Open questions (OPN)

Everything undecided, with the rules it blocks and how it gets resolved: **design** (a
planning round or a review), **tuning** (Phase A decides), or **play** (only playing can
tell). When one is settled, write the answer into the rule, delete the entry here
(leaving its number retired), and log it in `DECISIONS.md`.

The planning agent should start a round from the **design** entries.

## Design

| ID | Question | Blocks | Working default |
|---|---|---|---|
| OPN-1 | What triggers a war declaration? | DIP-4, SYS-12 | Weighted vote by tree height, from idle commanders, rich targets, believed strength, peace prosperity; against current wars and losses; scaled by risk |
| OPN-2 | Ceasefire tribute: how much, how long, who receives it | DIP-2 | To the winner's root, scaled by how badly the war went, ~3 months |
| OPN-4 | How sieges play in the rendered game | MIL-10 | — |
| OPN-5 | How many culture regions and starting factions at v1 scale, and the initial tree shapes | GEN-1, GEN-8, FAC-2 | 5–6 regions; seats grown outward over the Lattice |
| OPN-6 | How the Lattice network is generated | GEN-4 | Spanning tree plus neighbourhood-graph loops, junctions at meeting points |
| OPN-7 | Does tribute cascade? | FAC-6, SYS-11 | Yes (EXP-12 tests it) |
| OPN-8 | How capture drops prosperity and how it recovers | FAC-10, SYS-13 | Capture cuts happiness, stocks and a little population; recovery through ordinary economics and patronage |
| OPN-10 | Does governing skill exist? | FAC-9 | No: reputation and opinion choose governors |
| OPN-11 | How reputation is computed | REL-7 | Grows with missions completed and fleet power |
| OPN-12 | Generation details: which starter hulls, which path a new character takes, where it appears | CHR-6, SYS-18 | Armed or unarmed first-tier hull at random, at a populous lawful planet |
| OPN-13 | The player's fleet power formula | CHR-12 | — |
| OPN-15 | How a governor weighs affordability, fleet power and opinion when hiring; how often; how many candidates | MIL-6, SYS-9 | SYS-9's weighted score |
| OPN-16 | Escort pay timing and whether refusing a call to arms costs opinion | REL-9, REL-10 | Paid at trip end; refusal is a bad deed |
| OPN-17 | When an invasion launches and how its target is picked | MIL-9, SYS-13 | A budget option on believed value × chance of winning |
| OPN-21 | Does Lattice access need neutral or positive Authority standing? | WLD-12 | Neutral |
| OPN-22 | Do junction gates have only the Authority's key? | WLD-12 | Yes |
| OPN-24 | Escort ratio, leash distance, and whether an independent's escort job has a standing gate | TRD-8, TRD-11 | No gate |
| OPN-25 | How the player finds intercept targets and what they start with | TRD-17 | — |
| OPN-26 | The size of the area manifested around the player, and the handoff | ARC-13 | Current sector plus neighbours |
| OPN-28 | Do lawful ships get Mk tiers? How does weight affect a ship (speed, acceleration, turn rate, fitting budget)? | MIL-16, ECO-12 | — |
| OPN-29 | An information market: player-only, NPC too, or never | INF-15 | Deferred |
| OPN-30 | Do planets need trade deals or loans? | ECO-9 | Only if the simulation shows the need |
| OPN-31 | Do neutral planets sell armies passage? | WLD-13 | — |
| OPN-34 | Does war between the largest factions need special treatment? | DIP-1 | No; it happens when the simulation produces it |
| OPN-39 | Should a governor's budget read net income (receipts less what it pays merchants for imports) rather than gross receipts? A big importer's budget now overstates what it can afford | SYS-5 | Not yet: found in round 3, after security was sized to income |
| OPN-38 | Should output per head fall a little as a population grows (limited land and resources), so a shrunken planet recovers and a crowded one imports? | PRD-5, PRD-7 | Not yet: held in reserve if the poverty floor and the other forces aren't enough |
| OPN-37 | What happens to a failing root with vassals: nothing, its vassals break away, or it turns pirate and frees them? | ECO-13 | Nothing (it stays as it is) |
| OPN-36 | How the player hires crew (gunners for the summoner playstyle) and what crew quality means, now academy worlds are dropped | VISION playstyles | — |
| OPN-35 | World content: how much ship and weapon variety, and what gives each culture an identity players get excited about; *(round 3)* the eight luxuries' names, placeholders for now | WLD-17, PRD-1 | A creative discussion of its own |

## Tuning

| ID | Question | Blocks |
|---|---|---|
| OPN-3 | Siege length and static defence strength | MIL-10, SYS-13 |
| OPN-9 | Deed sizes and how fast opinion fades | REL-3, SYS-10 |
| OPN-14 | Pod details: upgrade tiers, how hard a winner can hunt one, the out-of-range death chance | CHR-15 |
| OPN-18 | The spread rule's form, and how pressure grows on a negative treasury | ECO-8, SYS-5 |
| OPN-19 | Dues tier amounts against the advantage of holding a gate | ECO-10 |
| OPN-23 | Calibrating auto-resolved combat against rendered combat | MIL-13 (`later`) |
| OPN-32 | Danger throttling of osmosis and its floor | TRD-14 (`later`) |
| OPN-33 | Does contraband move by osmosis? | TRD-13 |

## Play

| ID | Question | Blocks |
|---|---|---|
| OPN-20 | How big the real map gets beyond v1 | WLD-3 |
| OPN-27 | Does local opinion (distributed standing) confuse players? | REL-6 |
