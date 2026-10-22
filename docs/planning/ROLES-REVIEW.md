# Review of ROLES.md

*Implementation agent, 2026-10-22. A review of `ROLES.md` against the spec, `DECISIONS.md`
and the simulation as built (main, 301eb31). The draft hadn't read `DECISIONS.md` or
`sim/`; this fills that gap. Not authoritative.*

## 1. What it changes in the simulation as built

Ordered by how much built code each part moves.

### 1.1 Governors with fleets (3.1) are the biggest change

How governors work in the sim today:

- **They're inert.** A governor never wakes or moves (`chars/characters.cpp:229` skips
  anyone who governs).
- **Their ships don't defend.** A planet's defence is its service-contract commanders
  only (`war/armies.cpp:33`, `ready()`).
- **Generated governors have no ships** (`make_governor`). A commander promoted at
  capture keeps its ships (CHR-8), but they do nothing.
- **Governors never die.** Capture deposes a governor; nothing kills one.

Letting governors sail pulls these in during the **same** round, so none of them can be
deferred:

- **Succession (3.3).** A governor who sails can die (MIL-13, CHR-15), which leaves
  `planet.governor` pointing at a dead character. There's no code for that today.
- **Someone to run the planet while the governor is away,** which is the steward (3.2).
- **Whose beliefs the planet decides on.** The governor's facts are the planet's
  knowledge (`info::governor_facts`): threat assessment, war votes and the budget all read
  them. Away from home, does the governor keep them, or does the steward's copy drive
  the planet?
- **Who receives messages.** INF-16 sends loan and relief requests to a governor. If that
  governor is out with the fleet, either the steward answers or the request waits.

A smaller first step would put a governor's own ships in its home defence pool, with the
governor staying home. That gives "the governor has a fleet" without any of the above.

### 1.2 The steward and splitting the trait (3.2, CHR-4)

- **It reverses REJ-8** ("Stewards running planets for governors in the field. Characters
  rise organically instead") and the FAC-9 sentence "There are no stewards". The draft
  flags this, and it needs to be a conscious reversal logged in `DECISIONS.md`.
- **The budget is one optimizer.** It values every line (defence, stipends, patronage,
  growth, salaries) as units with diminishing returns, and the governor's risk sets both
  the curve and the credit limit (`politics/budget.cpp:111-112`). Splitting by trait
  means one of two things:
  - **(a)** each unit reads its own decider's risk. This is cheap, and the competition
    between lines stays in one budget.
  - **(b)** two budgets, with a split decided first. That needs a new rule for the split.

  I'd do (a).
- **The growth factor** (PRD-3, set in `make_governor`) moves to the steward.
- **A new salary line** (ECO-4) and a new contract kind. Both change the save layout.

### 1.3 The fitness function (section 2) is cheap and fits what's built

The sim already has two scorers:

| Scorer | Where | Weighs |
|---|---|---|
| Hiring a commander | `politics/budget.cpp` `candidates` | power, opinion, culture |
| Choosing a governor | `war/armies.cpp:325` | reputation, opinion, a bonus for the capturing commander |

Unifying them is a small change, but the draft drops three things the sim uses:

- the **culture weight** in hiring;
- the **capture bonus**, which FAC-9 requires;
- the **candidate pool**. Today it's only the keeper's own sworn commanders; the draft
  says "existing characters".

"Zero to many above a rising bar" (MIL-6) is already how hiring behaves. A commander is a
unit competing for budget, so a poor planet hires none. I'd record that as the answer to
OPN-15 rather than build a separate bar.

### 1.4 Spies and suspicion (section 7) rewrite MS-7

What's built: spies hold **salaried** Spy contracts (`war/spies.cpp`, `spy.salary`).
Suspicion is **per character, per faction, fading weekly**, and smugglers share it
(`pirates/suspicion.cpp`). There's no stealth rating or detection stat.

The draft changes all of this:

- spies paid per job;
- a per-trip counter;
- a stealth field on characters and a detection stat on planets, sectors and ships.

That rewrites SYS-17 and INF-13/14 and needs a save version bump. It doesn't touch
war, fealty or the economy, so it can be its own round. EXP-4 (is it worth having spies
at all?) hasn't been run yet; running it first would say whether a rewrite is worth it.

### 1.5 The mission primitive (section 4) is mostly built already

Missions today:

- An issuer posts at a planet.
- Pay grows while a mission waits (`missions::pay_now`) and comes from the poster's
  treasury or the Authority's.
- Completing one raises skill and reputation and is a `JobDone` deed
  (`chars/missions.cpp:251-263`).
- Missions expire.

What the draft adds:

- a bad deed when a mission is failed;
- tiers gated on opinion;
- spend tracked per issuer.

All three are small. The war and fealty round already planned to post objectives as
`Command` missions, which matches "faction missions are the objective board".

### 1.6 The Authority board and data store (5.1) are a large trade change

- **For the player it's just a view.** For NPCs it means a new contract kind and the
  Authority trading with its treasury. It also means a second information-sharing group,
  and contract merchants earning a fixed fee instead of the spread.
- **That touches the code that decides merchant trips,** about 43% of the run time
  (`merchant::best_trip`).
- **It lands on round 4's open problem.** 23% of characters are idle and 15% are pirates,
  because merchants are being squeezed. The board might fix that, or worsen it for
  independents, which is the draft's own check.
- **I'd make it its own trade round,** proposed as the response to the squeeze, not mixed
  into war and fealty.

### 1.7 Governor delivery top-ups (5.3) overlap with tribute relief

Both answer a poor planet that can't afford what it needs:

- **FAC-15 tribute relief** was designed last round and isn't built yet;
- **the life-support price levers** were discussed but not decided.

Adding top-ups as well breaks your "one area per round, targets first" rule. I'd hold
5.3 until relief has been measured.

### 1.8 Spawned bounties lowering sector risk (section 6) breaks P4

The draft says killing a spawned pirate "lowers the sector's risk number". But risk is
truth computed from real raids, hostile ship-days and patrol days (`war/risk.hpp`).

- If spawned kills lower it, an invented target changes merchant hiring and escort
  demand, which P4 forbids.
- The real pirates are still there, so belief and truth drift apart.

Two honest options:

- **(a)** count the time spent hunting as patrol days, the way Authority clearing
  contracts already work (`authority.cpp`);
- **(b)** spawned bounties change nothing in the sim.

I'd do (a).

### 1.9 Root succession and capture are different cases

3.3 handles a root who **dies**. When a root's planet is **captured**, the sim already
makes the most populous vassal the new root (MOD-5, proposed). The draft doesn't say
whether the steward route replaces that. A captured planet's steward goes with its
planet, so I'd keep MOD-5 for capture and use the steward route only for death.

## 2. Conflicts with the spec

| Draft | Conflicts with | Note |
|---|---|---|
| Steward (3.2) | REJ-8, FAC-9 | Has to be a logged reversal |
| Mission failure is a bad deed (4) | REL-8: ending a contract carries no relationship penalty | A character about to fail just quits first. Either quitting a mission counts as failing it (narrowing REL-8 for one-job contracts), or failing carries no deed |
| Payout fixed at posting, and unfilled missions pay more (4) | Each other | I read it as fixed **when taken**, which is what the sim does |
| P3 free money | ECO-1, REJ-16, REJ-17 | ECO-1 says population is the only source of new money. Starting wealth is already a tracked exception (`ledger.money_spawned`), so there's precedent. It still needs explicit sign-off |
| Steward is NPC-only (3.2) | CHR-17: every role is open to everyone | Defensible, because time stops at a planet. CHR-17 needs the exception written in |
| Governor's own fleet joins missions (3.4) | MIL-3: loans move the employer, never the ships (REJ-12) | A governor has no service contract to lend, so 3.4's "missions between governors" is the route. The draft implies this but should say it |
| Top-ups (5.3) | TRD-2, REJ-26 | Flagged by the draft |
| TRD-6 row (9) | Nothing | Independents can already cross a war line, and spies already do in the sim. This only clarifies |

## 3. What can wait

These don't touch the sim, or can be added later without changing what's built:

- the start and the Lattice Drive (8);
- merchant ship classes beyond "has a drive" and "has armour" (5.2);
- the player-facing board UI;
- war-margin deliveries (5.4);
- the spy mission menu beyond the basic exposure;
- copying data, sabotage and meeting leaders;
- promoting escaped targets to rivals;
- culture variants.

## 4. Questions

1. **Stewards:** do you want to reverse REJ-8? The original reason was that characters
   rise organically. The draft's reason is that governors can sail.
2. **Sailing governors:** in the next build round, or later? If they sail, succession,
   the steward, absent-governor messages and whose beliefs drive the planet all come
   together (1.1). My suggestion: next round, a governor's ships defend its home and it
   stays put; sailing is its own round.
3. **The trait split:** is it fine for each budget line to read the risk of whoever
   decides it, inside one budget (1.2a)? And which lines are the governor's? The draft's
   first cut is defence, stipends, policing, tribute, relief and objectives.
4. **Fitness inputs:** should it keep the culture weight in hiring and FAC-9's capture
   bonus? And who is in the pool for a governor's seat: the keeper's commanders (as
   built) or anyone with standing?
5. **Mission failure versus REL-8:** does quitting a one-job contract count as failing it?
6. **Spawned bounties:** count the hunt as patrol days (1.8a), or have them change
   nothing?
7. **Per-trip suspicion:** the counter resets on landing at any planet not hostile to
   the character, neutral ones included. A spy can then reset by hopping through a
   neutral system next to the target. Is that intended, or should only a friendly
   planet reset it?
8. **P3 free money:** accept the principle now (named, tracked, logged), or wait until a
   mission is shown to lose money?
9. **Order of rounds:** my proposal is in section 5.

## 5. Proposed order

1. **War and fealty (already designed), plus the cheap parts of this draft:**
   - the shared fitness function (with your answers to Q4);
   - the mission rules: failure deed, tiers, spend per issuer;
   - objectives as missions;
   - governors' ships joining home defence;
   - OPN-15 answered as built.

   Stewards and sailing are left out, so MIL-2 and MIL-3 can be built as written.
2. **Governors as admirals:** sailing, the steward, succession, the trait split.
3. **The Authority board and contract merchants,** as the answer to the merchant
   squeeze.
4. **Spies:** EXP-4 first, then the stealth and detection rewrite if spies earn their
   place.
5. **Top-ups (5.3),** only if tribute relief doesn't fix poor planets.

## 6. The human's answers (2026-10-22)

- **Missions are the market for wants.** Missions plus characters free to choose among
  them work like a free market. An entity says what it wants, and whoever finds it worth
  doing does it. Much of the design was already heading this way: objectives, loans,
  scouting, couriers, clearing contracts, the Authority board and top-ups. Price stays the
  motivator for ordinary trade (TRD-2). Missions carry wants that a price can't express.
- **Failing a mission is a bad deed** with its issuer. Abandoning a one-job contract
  counts as failing it, which narrows REL-8 for missions. Standing contracts can still be
  ended without a penalty.
- Q1: **Stewards are allowed** (REJ-8 is reversed). Stewarding is also a way up for
  merchants and spies.
- Q2: **Sailing governors come now.** Balancing a model they'd overturn wastes the time.
- Q3: **One budget.** The governor splits war and defence from everything else, and
  decides how the war and defence share is spent. The steward decides everything else.
- Q4: Cultures exist in a minimal form. A character's culture is its birth region. It
  gives that region's ships a small spread in their stats, and governors give it a small
  hiring preference (`hire.w_culture`). It's cheap and already built, so it stays as an
  input. *(The implementation agent's answer; not yet confirmed.)*
- Q5: See the first point.
- Q6: Pending. "Patrol time" needed explaining.
- Q7: **The per-trip reset on any non-hostile planet is fine for now.**
- Q8: **Free money (P3) is accepted in principle.** Try without it first, but for a small
  issue, use free money rather than reworking the model.
