# Roles, progression and missions

*Planning draft, 2026-10-22. **Not authoritative:** the rules are in `docs/spec/`. Everything
here is `proposed` unless tagged otherwise. It answers OPN-42 and names the rules it
changes in section 9. Spec read for this draft: README, OPEN, and `characters`,
`relationships`, `trade`, `factions`, `military`, `economy`, `information`, `piracy`.
Not read: `world`, `production`, `diplomacy`, `DECISIONS.md`, `sim/`. Check those before
formalising.*

## Summary

1. **Paths are tendencies, not ladders.** Three common paths (merchant, spy, warrior) are
   descriptions of what the ship and the market tend to produce. One fitness function
   decides who gets picked for a post.
2. **A governor commands a fleet** (admiral; grand admiral with vassals). A **steward**,
   an NPC role, runs the planet's economy.
3. **Missions are one primitive:** a posted offer from an issuer, with a payout fixed at
   posting, funded from the issuer's budget. Merchant, warrior and spy missions are
   different issuers and different targets on that primitive.
4. **Spies** use a per-character stealth rating against a detection stat that planets,
   sectors and ships all have. Exposure accumulates in a per-trip suspicion counter.
5. **Deliberate exceptions** to simulation fidelity are allowed, if each is named, tunable
   and measured (P3).

## 1. Principles

**P1. No path has to lead anywhere.** A merchant who runs dangerous routes with his own
fleet may drift to commander or governor. Nothing names that route. *Why:* the pillar
"you choose how to play" (BRIEF) says no path is gated behind another.

**P2. One mission primitive** (section 4). No separate type system per path.

**P3. Fidelity exceptions are allowed and logged.** If a mission mechanism costs its
issuer more than it returns, the surcharge may be treated as free money (created by the
issuer, outside ECO-1), the way Bannerlord's caravan escorts pay above what a merchant
could afford. Each such exception must be (a) a named tunable, (b) tracked as its own
flow in the metrics, (c) listed in `DECISIONS.md` as a deliberate exception.
Costs to issuers are tracked first, so the exception is chosen with numbers in hand.

**P4. Nothing that affects the simulation is invented.** (Restates the BRIEF principle.)
A mission target that exists only for the mission (a spawned pirate, a bugged package)
affects nothing: it follows TRD-15 (non-persisted, killing it never touches the economy,
promoted to a character only if something lasting happens). Anything on a faction's war
objective board is real.

## 2. Paths and the fitness function

The three paths describe tendencies:

| Path | Typical first jobs | Typical ship | Tends toward |
|---|---|---|---|
| Merchant | courier, single trips, Authority board missions | cargo hull; with or without Lattice Drive | rich merchant; steward; commander or governor if he builds a fleet |
| Warrior | scout, bounties, escort | armed hull | escort, commander, governor |
| Spy | covert delivery, smuggling, tracking | small, fast, low-profile hull | spy for factions; smuggler; steward |

**One fitness function** decides every pick. A chooser scores a candidate on data that
already exists, with weights per post:

| Post | Chooser | Leans on |
|---|---|---|
| Governor (a vassal) | the liege or capturing governor (FAC-8, FAC-9) | combat power, reputation, opinion |
| Steward | the governor | trade power, reputation, opinion |
| Commander | the governor | combat power, affordability, opinion |

- Inputs: CHR-12's two power ratings, REL-7 reputation, the chooser's opinion (REL-2),
  and for commanders affordability (MIL-2 stipends). Weights are Phase A tunables.
  Opinion must carry real weight, and money should have diminishing returns, or the
  richest merchant wins every post.
- **Governor and steward are single seats:** pick the best fit. **Commanders are an open
  market:** a governor hires **zero to many**; a candidate is hired when their fitness
  clears a bar that rises as the budget tightens. Zero is a valid state, and the
  governor's own fleet is the baseline.
- Risk tolerance (CHR-4) is **not** in the function.

*Answers:* OPN-15 (how a governor weighs hiring), OPN-40 (what reputation is worth to an
employer: it is a weighted input to every pick), part of FAC-9.

## 3. Governor as admiral, steward, succession

### 3.1 The governor commands a fleet

A governor keeps and sails the ships they owned before taking the planet (CHR-8: promotion
never confiscates). A governor with a fleet is a commander (MIL-1), so there is no new
role. **Titles are derived, not roles:** *admiral* is a governor with a fleet; *grand
admiral* is one with vassals.

- The governor still sets objectives (MIL-2) and now may take them. It also joins armies
  it commissions (MIL-9) and takes missions its liege offers (3.4).
- Planetary defences and patrols (MIL-5) stay the planet's. The governor's fleet is
  personal property.
- **The fleet dies with its commander** (MIL-13: the commander is the last ship lost, then
  the pod rolls, CHR-15). A governor whose pod fails dies; the planet is succeeded (3.3).
- `open` **How the governor's fleet is funded.** Default: the governor pays itself a
  stipend from the planet budget under MIL-2, like any commander, and keeps its own
  bounties and spoils. One rule, no special case.

### 3.2 The steward

An **NPC-only role in v1** (`later` for the player: time stops at a planet, so there is
nothing for a player steward to do). Appointed by the governor under a contract (REL-8);
either side can end it. Chosen by the fitness function (section 2).

- **Steward decides the economy.** The steward's risk tolerance drives the budget lines
  that are not military: growth (PRD-3), patronage, merchant salaries at a root, mission
  top-ups (section 5.3), and how concentrated spending is (ECO-8).
- **Governor decides the military and politics:** the defence share and stipends
  (MIL-2, MIL-5), policing (PIR-15), tribute rates and relief (FAC-6, FAC-15), objectives,
  war votes (DIP-4), swearing fealty or defecting. The governor's trait drives these.
- `open` The exact line between the two lists. The default above is a first cut.
- While the governor sails, the steward runs the planet. No "governor can't leave
  during a siege" rule: leaving is the governor's risk-tolerance call.
- A steward is a natural destination for NPC merchants (information and trade power) and
  spies (information). It has to be worth taking: salary from the planet budget (an
  operating cost, ECO-4), and the faction's information (INF-4).

### 3.3 Succession

- **Governor dies:** the steward runs the budget in the interim, with no new objectives
  posted. The liege (or the governor who holds the planet in the tree) selects the
  successor by the fitness function. The steward is a candidate.
- **Root dies:** the **steward of the root's planet takes over** as the new governor, and
  so as the root (FAC-2). The root appoints it, so it goes to someone it trusts. The
  vassals' opinion of a new root starts from their opinion of that person, so a
  succession is a natural defection risk (FAC-11), with no extra rule.
- **Fallback** if the seat is empty: the vassal with the highest opinion of the old root.
- NPC-only in v1 (a player cannot be a steward).

### 3.4 Missions between governors

A liege may offer a vassal missions beyond a fleet loan (MIL-3), using the same board and
the same rules: every request has a deadline and a default, no answer is a refusal, the
fleet's arrival is the answer (INF-16). Refusal is a deed (REL-10); make its size
proportional to the ask so a liege cannot spam requests to push a vassal away.

## 4. The mission primitive

A **mission** is an offer posted by an **issuer** (a governor, a root, the Trade
Authority, a merchant). It carries:

- a **type and target spec** (cargo and route, a fleet power and sector, a named target,
  a place to scout);
- a **payout fixed at posting**, visible to the player up front;
- a **deadline**, and a **gate** (standing, reputation, loadout) so the right characters
  can take it;
- the **issuer's funds**: the payout comes from the issuer's budget (ECO-8).

Rules shared by all missions:

- **NPCs and the player use the same primitive.** NPCs pick by fit and pay (CHR-10), and
  resolve unrendered work with the MIL-13 auto-resolve. The player sees the same offer.
- **Unfilled missions pay more the longer they sit, and expire** (CHR-10).
- **Failure ends the contract**, with no payout and a bad deed with the issuer
  (REL-3). It does not touch reputation (REL-7). Non-rush missions have loose deadlines.
- **Completion is a good deed** (REL-3 already lists it). Do a lot of work for one
  issuer and its opinion rises, which unlocks **higher tiers**: better-paying and
  riskier missions are gated on opinion (REL-5) and reputation (CHR-10). Missions also
  grow skills (CHR-10). This is the progression: access and standing, not XP.
- **Issuer cost is tracked per issuer** (P3). Metrics: spend, completions, what the
  issuer got back.
- **Issuers see the same stale information everyone does** (INF-2): they decide on what
  they believe.

### 4.1 Where missions appear

Faction-level missions travel at courier speed (INF-11). The Trade Authority has no
couriers, so its offers appear at a planet from that planet's **Authority data store**
(section 5.1), as old as the store.

## 5. Merchant path

### 5.1 The Trade Authority board `v1`

The Authority maintains the Lattice and is a neutral party (FAC-13). It also fields a
**mission computer** at each planet.

- **The data:** each planet holds a store of market facts (INF-2 form). **Only merchants
  under an Authority contract share into it**, on landing, keeping the newest facts
  (INF-4's rule, with the Authority as a second sharing group beside a faction).
  Independents and faction merchants not under contract cannot read it.
- **The offer:** to the player it reads "deliver N units of X to planet Y for $Z."
  The payout is the **believed spread minus the Authority's cut**.
- **The risk the fee buys:** the payout is fixed at the quote. By delivery the real
  price may have moved, because other traders react to the same data. The Authority
  eats the drift, up or down, as a wash. The player carries transit risk (pirates, fuel,
  time).
- **Cargo is issuer-owned:** the Authority buys at the origin and sells at the
  destination (real market trades, so goods move on real ships and the transaction tax
  applies, ECO-3). The player is paid the fixed fee. Walking off with the cargo is a bad
  deed, and the loss is not worth tracking.
- **The cut is the Authority's income,** alongside dues (ECO-10). It funds the network
  and the mercenary contracts that reduce sector risk (FAC-14). The cut circulates, so it
  belongs in the "circulates" column of ECO-5 until the metrics say otherwise.
- **Rush missions:** a shorter deadline with a smaller cut, which reads as a bonus. They
  favour fast ships and buffer starving planets until bulk merchants arrive. Tie to
  ECO-13 so a chronic recipient still fails on schedule.
- **NPC merchants under an Authority contract** get the pooled data as their information
  (the Authority version of TRD-3). The board is a player-facing view; NPCs have no
  separate mission object.

### 5.2 Ship classes

Three merchant profiles; the sim needs only the first two attributes (a Lattice Drive,
and armour) at first:

| Class | Typical use |
|---|---|
| Lattice Drive, unarmoured | safe trade on or near the Lattice |
| No drive, unarmoured | short trades away from the Lattice |
| No drive, armoured | runs through contested space |

The Lattice Drive costs cargo space or speed. Speed matters for a merchant because a
faster ship reacts to market information sooner and cuts escort overhead. Heavy cargo
is still the main income, so merchants are the slowest hulls. *Culture variation is
`later`.* `open` How weight affects a ship: OPN-28.

### 5.3 Governor and root delivery missions `proposed`

A governor or root with several planets may post delivery missions, as a **top-up to a
merchant's margin** on a route. Trigger: a planet in its tree has stock below reserve
and **cannot bid its price up enough to pull supply**, because it is poor, starved or
blockaded (MIL-17). The mission is funded from the issuer's budget (the steward's lines,
3.2), and the issuer owns the cargo, bought from its own planet (ECO-2), so the money
circulates inside the tree.

- This **reverses TRD-2 and REJ-26 only for that case.** Everywhere else, price is the
  motivator. A mission is a funded margin boost on a route, so merchants respond to it
  as to any margin, and NPCs need no mission object.
- It is a more targeted supplement to osmosis (TRD-13) and gives the budget a concrete
  line.

### 5.4 War margins

Faction merchants and osmosis stop at a war line (TRD-6), so prices drift apart until a
gap opens. A long war creates its own trade. The issuer posts a delivery from enemy space
when its believed price there is well below its own, or to supply a planet cut off by a
blockade. **Payout = believed spread, plus a risk premium for the detection exposure,
minus the cut.** The issuer's willingness is capped by the value of what it lacks, not by
the spread, so it cannot overpay for a cheap good it does not need.

## 6. Warrior path

Missions by issuer. "Real" targets are sim entities; "spawned" targets follow P4.

| Mission | Issuer | Target |
|---|---|---|
| Kill N pirates in a sector that has gained power | Authority (neutral and fringe space); faction root (inside its territory) | spawned |
| Kill a named pirate | Authority or root, by territory | spawned |
| Escort a merchant | the merchant (TRD-10) | needs an armed ship; a Lattice merchant needs a ship with a drive; an armoured merchant needs a minimum combat power |
| Scout a neutral or friendly planet | Authority (neutral); any governor (friendly) | real facts |
| Scout a dangerous pirate sector | faction root or Authority | real facts |
| Scout an enemy sector | any governor, usually border and fringe | real facts |
| Kill a named enemy ship | faction root | **real** |
| Destroy a caravan | faction root | **real** (this is TRD-17's raid) |
| Escort a fleet | any governor | real (MIL-3 loan, from the player side) |
| Destroy a fleet | governor with vassals, via the root | **real** |
| Capture a planet | governor with vassals, via the root | **real**; blocked on OPN-4 |

- **Spawned bounties** carry a power rating and a sector, not a ship record. NPCs take
  them by fit and pay and resolve them with MIL-13. Completing one lowers the sector's
  risk number (TRD-16), which feeds merchant hiring (TRD-9). Kill-count bounties pay by
  the sector's risk and disappear when it is cleared, so farming is self-limiting.
  A bounty completes on **destroying the ship**; a pod that escapes is the pilot
  surviving, with no extra for the pilot (CHR-15). Optional: promote an escaped target
  to a character with a grudge (TRD-15), which produces recurring rivals for free.
- **Faction missions are the MIL-2 objective board seen from the player's side.** The
  same bounty and spoils rules pay the player (MIL-18, MIL-19). Do not author them
  separately.
- **Authority and root split by territory:** the Authority posts bounties where no
  faction holds the sector. A faction posts inside its own. No double payment for the
  same risk.
- **Scouting is information-driven.** A governor posts a scout mission when its belief
  about a nearby threat is stale and the threat is large. Pay scales with the same
  product. A scout reports what is visible from outside, and takes combat risk
  (INF-9).
- **Scouting an enemy fleet commander or governor** is a **spy** mission (section 7).

## 7. Spy path

**A scout reports what it can see from outside. A spy works under cover.** A scout risks
combat; a spy risks detection. The spy is the smuggler turned good: the same small,
fast, low-profile hull (CHR-3), a different customer. Spy/pirate overlap is not
decided here (`open`).

### 7.1 Missions

| Mission | Notes |
|---|---|
| Land on an enemy planet | the basic exposure |
| Track an enemy ship | find it and stay within sensor range |
| Covert delivery | a normal merchant mission the enemy faction offers, but the package is "bugged": no real effect, the risk is being detected. The spy-path levelling job, like the spawned named pirate for the warrior |
| Smuggle | contraband for a pirate planet (PIR-3), or goods to or from a blockaded or cut-off planet (5.4) |
| Scout an enemy fleet commander or governor | the hardest and highest paying |
| Copy a planet's data | `proposed`, optional: land, copy the planet's information store, deliver it to the issuer |
| Sabotage a ship; meet an enemy leader | `later`: no political effect in v1; sabotage is safe only on a mission-only target |

- Issuers: any governor or root (the same staleness-times-threat trigger as scouting),
  the Authority, or pirate planets for smuggling (PIR-3).
- **Pay is per job** (REL-8). Freelance is fine in v1. Completion raises standing with
  the issuer, which unlocks higher-tier jobs (section 4).
- **NPCs take these missions too.** Spying is a valid tool for factions. The sim needs
  NPC independents and spy-contract characters able to cross a war line (TRD-6 stops
  only faction merchants and osmosis).

### 7.2 Stealth, detection and suspicion

- **Stealth** is a character rating. It grows by a very small amount on each successful
  exposure, with a cap and diminishing returns. `open` Whether the ship also modifies it
  (a covert hull); the default is yes, a small modifier.
- **Detection** is a stat on **planets** (set by policing spend, PIR-15), **sectors**
  (derived from patrols found by the encounter check, TIM-6, not stored) and
  **characters** (a ship attribute, relevant when ships meet; later, culture variation).
- **An exposure** is a landing at an enemy or inspecting planet, or an encounter in an
  enemy sector. It adds to the character's **suspicion counter** by how far the place's
  detection beats the character's stealth. Modifiers: a high pirate relation (PIR-9),
  cargo that does not fit the route, lingering. The formula lives in `sim/`.
- **Suspicion is one number per character, per trip.** It resets when the character
  lands at a planet that is not hostile to them. Landing at the planet being
  infiltrated is the exposure, so it does not reset.
- **Crossing the threshold:**
  - **In enemy territory:** the mission fails and the character is **flagged hostile**
    to the detecting faction until they land at a non-hostile planet. There is no
    capture (CHR-14). A flagged ship that is caught is a combat loss, and the pod rolls.
  - **In lawful space** (inspection of a smuggler): the contraband is **confiscated** and
    it is a bad deed (INF-14, proposed).
- **The player sees their own counter,** so the risk is legible and they can back off.
  No second persistent suspicion stat is planned; it is the upgrade path if Phase A
  shows spies never fail.
- **Roll frequency:** exposures at planet landings and encounter-check hits in enemy
  sectors, not every sector entered, so failure does not compound unrealistically.

## 8. The start (open)

The player starts as a neutral trader in a shuttle with no Lattice Drive (CHR-17). The
drive is a large money and weight investment, and the character must progress toward it.

`open` How the start reaches the drive. Constraints from the discussion: both the
merchant and the warrior paths should reach it by their own work; the warrior's escort
rung needs a drive for Lattice merchants; the merchant's choice between a drive and a
no-drive ship is a real fitting decision. Related: OPN-12 (starter hulls), OPN-21 (does
Lattice access need neutral Authority standing).

## 9. Rule changes

| Rule | Change |
|---|---|
| CHR-3 | The ship decides which missions a character can do: add stealth and detection to a ship's profile |
| CHR-4 | Budget lines that are not military read the **steward's** trait; war votes and defence read the governor's |
| CHR-8 | Clarify: a governor's fleet is the character's personal property, not the planet's |
| CHR-11 | **Rewrite as descriptive** (section 2), not a ladder. Add the shared fitness function |
| CHR-17 | Add: steward is NPC-only in v1 (`later` for players) |
| FAC-9 | **Replaces "no stewards and no pool of standing politicians."** Fitness-function selection, steward role, succession (3.3). Check REJ-8 and REJ-9 in `DECISIONS.md` before formalising |
| FAC-14, ECO-10 | The Authority also earns a mission cut; its mercenary contracts are the source of independent bounties |
| MIL-1, MIL-2, MIL-3 | A governor with a fleet is a commander; the governor takes its own and its liege's objectives; the loan generalises to missions between governors |
| MIL-6, OPN-15 | Commanders are an open market, zero to many, hired above a fitness bar |
| MIL-9 | The commissioning governor's own fleet joins the army |
| TRD-2, REJ-26 | Narrow reversal: funded top-ups for poor, blockaded or cut-off destinations (5.3) |
| TRD-3, INF-4 | The Authority is a second information-sharing group, with a per-planet data store |
| TRD-6 | Independents and spy-contract NPCs may cross a war line |
| TRD-17, OPN-25 | Spy missions give the player intercept targets (tracking), partly answering OPN-25 |
| INF-10 | Add spy missions to the four mission types |
| INF-13 | Spies take per-job missions; planets no longer "employ" agents |
| INF-14 | **Replaced** by the stealth, detection and per-trip suspicion counter (7.2) |
| PIR-9, PIR-12 | The pirate relation becomes a modifier on exposure |
| PIR-15 | Policing sets the planet's detection stat (the dial survives) |
| REL-3, REL-7, OPN-40 | Failing a mission is a bad deed; completing is a good one; reputation is a fitness input and is not hit by failure |
| OPN-42 | **Answered by this draft** |

## 10. Checks to add (Phase A and B)

- **Authority solvency:** cut vs drift loss, within a band, not exactly balanced.
- **Mission spend per issuer,** and what each got back. If an issuer loses on a mission
  type consistently, decide whether to treat it as P3 free money.
- **Mission share of goods moved** stays under a ceiling, so price signals still work
  (TRD-2 is narrowed, not abandoned).
- **The merchant squeeze:** does the Authority data store help independents less than
  members, and does it worsen the squeeze Phase A already shows?
- **Cross-path promotion rate:** if every governor is a warrior and every steward a
  merchant, the fitness weights are too narrow.
- **Commander hiring** does not run away in a long war (CHK-8).
- **Spy failure rate** inside a band: not zero, not constant, falls with stealth.
- **Spawned-bounty farming:** pay falls as the sector's risk falls.
- **Chronic subsidy:** governor-funded top-ups do not keep an ECO-13 failure alive
  indefinitely.
- **Phase B ablations:** no spies for one faction (EXP-4 already), no stewards (does the
  steward's trait change outcomes?), Authority board off (does osmosis alone hold prices?).

## 11. Open questions

- The start and the Lattice Drive (section 8).
- Where the budget splits between governor and steward (3.2).
- How a governor's fleet is funded (3.1).
- Whether the ship modifies stealth, and by how much (7.2).
- The spy/pirate overlap: a faction's spy running contraband for a pirate planet taints
  its own record. Proposed hold: faction-contract runs do not move the pirate relation,
  until the overlap is decided.
- How capture-planet missions play (OPN-4).
- `later`: a player steward; sabotage and meeting a leader; culture variants of merchant
  and covert hulls; a character detection stat varying by culture.
