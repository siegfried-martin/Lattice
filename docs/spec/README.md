# Lattice specification

The source of truth for the full game's design (`design/`) and for the simulation that
runs it (`sim/`). Everything here is current. History lives in `DECISIONS.md` and in git.

*Created 2026-10-18 by consolidating the planning drafts `FACTIONS.md`, `ECONOMY.md`,
`TRADE.md` and `CHARACTERS.md` (git tag `planning-drafts-1`) and the review rounds that
followed them.*

## How the docs fit together

| Where | What it holds | Who writes it |
|---|---|---|
| `docs/VISION.md` | The intent everything serves | The human |
| `docs/spec/` | The game's rules and the simulation spec. Authoritative. | The implementation agent, from settled decisions |
| `docs/planning/` | Ideation drafts for the next round. Not authoritative. | The planning agent |
| Other files directly in `docs/` | The current prototype (travel, combat) and the brief | Kept current with the code |

**The process.**
1. The planning agent reads this index and `OPEN.md`, then writes drafts in
   `docs/planning/`, naming the rule IDs it would change ("replaces FAC-7").
2. The human reviews the drafts with the implementation agent.
3. The implementation agent writes the settled decisions into the spec, adds a line to
   `DECISIONS.md`, and deletes the drafts.

## Conventions

**Rule IDs.** Every rule is a heading of the form `### PRE-n Title` followed by its
tags. IDs are stable: a rule keeps its number when it's edited, and a removed rule's
number is retired (noted in `DECISIONS.md`), never reused. New rules take the next
number in their file. Cite rules by ID everywhere: in other rules, in the sim spec, in
code comments, in tests and in commit messages.

**Tags,** in backticks after the title so they can be searched for (`` grep -rn '`later`' docs/spec ``):

| Tag | Meaning |
|---|---|
| `v1` | In the first playable simulation |
| `later` | Decided as wanted, not in v1 |
| `proposed` | Suggested and not yet signed off by the human. Treat as the working default. |
| `experimental` | Decided as worth trying; the simulation or play gives the verdict |
| `open` | Undecided. The rule states the options; `OPEN.md` tracks it. |
| `rendered` | Applies only to the rendered game near the player, not to the headless simulation |

A part of a rule can carry its own tag inline, *(proposed)*. A rule with no `proposed`,
`experimental` or `open` tag is decided.

**Each rule** states what is true, then optionally *Why:* (one or two sentences) and
*See:* (related IDs). Rules don't restate each other; they cite.

**Design against sim.** `design/` says what the game does and why, at the level a
designer argues about. `sim/` says how v1 implements it: formulas, cadences, data and
tunables, citing design IDs. A formula lives only in `sim/`. Numbers are tuning, not
design: every number in `design/` is illustrative, and the starting values live in
`sim/tunables.md`.

## Index

### Shared files

| File | Holds |
|---|---|
| `GLOSSARY.md` | One definition per term |
| `OPEN.md` | Open questions (`OPN-n`), each naming the rules it blocks |
| `DECISIONS.md` | Dated log of every change to the spec, and rejected ideas (`REJ-n`) not to reintroduce |

### Design (`design/`)

| File | Prefix | Covers | Key terms |
|---|---|---|---|
| `world.md` | WLD | Map, sectors and systems, the Lattice and its travel rules, gates and their two keys, landing, the clock, cultures and regions, aliens | sector, system, group, gate holder, Lattice Drive, bail out, ramp boost, culture region, fringe |
| `characters.md` | CHR | The one character model, attributes, the risk trait, generation, ownership, careers and paths, fleet power, losing and escape pods, the player, the fitness function | role, risk tolerance, skill, path, fleet power, preset variant, escape pod, autosave, fitness |
| `relationships.md` | REL | Money and relationships as the only two systems, opinion and deeds, standing, reputation, contracts | opinion, deed, loyalty, favor, standing, reputation, contract |
| `factions.md` | FAC | Governors and fealty trees, the fealty deal, tribute, patronage, keep-or-grant, choosing governors, stewards and succession, defection, the Trade Authority | governor, liege, vassal, root, tribute, patronage, keep or grant, steward, succession, defection, Trade Authority |
| `diplomacy.md` | DIP | War, peace and ceasefire between factions | war, peace, ceasefire, suing for peace, war vote |
| `military.md` | MIL | Commanders and stipends, running costs, planetary defence, armies, invasions, sieges, off-screen combat | stipend, running cost, security force, army, intent, action, siege, relief, loan, objective, bounty, spoils, admiral |
| `economy.md` | ECO | Where money enters and leaves, the state trader, taxes, operating costs, budget decisions, money within a faction, Authority dues, fuel, planets turning pirate | purchasing power, state trader, transaction tax, sink, budget model, dues, fuel price |
| `production.md` | PRD | v1 goods, population, production and labour, prices, efficiency events, the full supply chain (later) | life support, luxury, contraband, reserve, efficiency, labour share, happiness, productivity |
| `trade.md` | TRD | Merchants and their trip shapes, escorts, osmosis, ambient ships, the risk number, trade missions for the player | faction merchant, independent merchant, trip shape, escort, osmosis, ambient, risk |
| `piracy.md` | PIR | The pirate faction, pirate planets, commanders turning pirate, loot, the pirate relation, smuggling, contraband policing | pirate faction, anarchy, pirate relation, smuggler, full pirate, policing |
| `information.md` | INF | Facts and beliefs, sharing, couriers, scouts, mission travel, spies, suspicion | fact, belief, sharing, courier, scout, sighting, spy, suspicion, stealth, detection, exposure |
| `missions.md` | MSN | Missions as the market for wants, the one mission primitive, success and failure, free money, spawned targets, the Authority's board, top-ups, and each path's missions | mission, issuer, tier, spawned target, free money, Authority board |

### Simulation (`sim/`)

| File | Prefix | Covers |
|---|---|---|
| `README.md` | SIM | What the simulation is for, the v1 scope, the goals (stability, then ablation) |
| `architecture.md` | ARC | C++ core, Godot binding, determinism, config, saves, metrics, toolchain |
| `model.md` | MOD | Every record the simulation stores, and its fields |
| `time.md` | TIM | The clock, the event queue, decision cadences, movement |
| `systems.md` | SYS | Each system's state, step and formulas, citing design rules |
| `worldgen.md` | GEN | World generation, development and v1 sizes |
| `tunables.md` | TUN | Every config key, its meaning and starting value |
| `checks.md` | CHK | Phase A: what "stable and working as intended" means, as measurable checks |
| `experiments.md` | EXP | Phase B: ablation experiments that test whether each mechanic earns its cost |
| `milestones.md` | MS | Build order |

## Lookup tips

- **A rule by ID:** `grep -rn '### FAC-7' docs/spec`. **Everything that cites it:**
  `grep -rn 'FAC-7\b' docs/spec`.
- **Everything deferred:** `` grep -rn '`later`' docs/spec/design ``. **Everything not signed
  off:** search for `` `proposed` `` and `` `open` ``.
- **A term:** `GLOSSARY.md` first, then the key-terms column above.
