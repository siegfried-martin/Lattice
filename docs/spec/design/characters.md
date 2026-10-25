# Characters (CHR)

What a character is, what it owns, how characters are made, how careers work, what
happens when a character loses, and how the player fits in. Relationships and contracts
are in `relationships.md`.

## The model

### CHR-1 One kind of entity, many roles `v1`

There is one kind of entity, the **character**. Governor, steward, commander, merchant,
spy, scout, courier, escort, mercenary, smuggler, pirate and the player are **roles** a
character holds: a current contract or status (REL-8), which changes. A career is a path
through contracts, not a change of entity; the same character, with the same ships,
wealth, skills and relationships, carries across every step.

**Ambient ships are not characters** (TRD-15). They're spawned, never persisted, and
promoted to a character only when something lasting happens.

*Why:* it's the only model where the player's rulebook and the simulation's agree.

### CHR-2 What every character has `v1`

- **Ships and wealth,** personally owned (CHR-8).
- **A culture** (WLD-18).
- **Skills,** which accrue from the missions taken (CHR-10).
- **Relationships:** an opinion of other characters, and theirs of it (REL-2).
- **Reputation** (REL-7).
- **Two fleet power ratings,** combat and trade (CHR-12).
- **One personality trait, risk tolerance** (CHR-4).
- **Its own information** (INF-2), which is what it acts on.
- **Long-term goals that stay fixed, and short-term actions** that change with what it
  currently knows, where it is, and how far away things are. A governor's goals come
  from personality and position; a commander's from their governor's objective and their
  own ambition. Armies follow the same rule as a force (MIL-7).

  *(proposed)* **Goals and decisions are separate, and a decision never replans a
  goal.** A goal is what a character is trying to do: make this trade, take this
  mission, join this siege. It's planned when the character lands, is idle or is told
  something (an event), and it's the expensive part. A decision is a reaction on the
  way, such as running from a pirate while making the trade. It's taken on the
  open-space step (TIM-6), which finds the few ships close enough for something to
  happen, and it's cheap. A decision changes what the ship is doing (flee, divert);
  if it breaks the goal (cargo lost, a route now believed dangerous), it schedules the
  goal to be replanned through the event queue rather than replanning on the spot.
  *Why:* frequent decisions stay cheap and expensive goals stay rare, in the
  simulation and in the rendered game, where nearby ships decide every frame.

### CHR-3 No classes `v1`

There are no classes and no personality types dividing merchants from warriors. **The
ship decides which missions a character can do:** combat hardpoints against cargo
capacity. A mixed loadout takes both small deliveries and escort jobs because the
hardware allows it.

*(proposed, roles round)* A ship's profile also has **stealth** and **detection**
(INF-14), and whether it carries a Lattice Drive or armour (TRD-18).

### CHR-4 Risk tolerance is the one personality trait `v1`

Each character has one trait, **risk tolerance**, from conservative to aggressive. It is
the only personality input anywhere in the design. It governs:

- save-or-spend decisions (CHR-10);
- how a broke commander chooses between piracy and downsizing (PIR-4, experimental);
- a governor's growth factor (PRD-3);
- how concentrated a ruler's spending is (ECO-8);
- a governor's lean in a war vote (DIP-4, proposed).

It does **not** govern mission choice, which is fit and pay (CHR-10).

*(proposed, roles round)* **A planet's decisions read the trait of whoever makes them**
(FAC-16):

- **the governor:** how much goes to war and defence against everything else, and how
  that share is spent (commanders, security, policing, invasions); tribute rates and
  relief; objectives; war votes; swearing fealty and defecting;
- **the steward:** everything else, including growth (PRD-3), patronage, a root's
  merchant salaries, mission top-ups (MSN-8), and how concentrated that spending is.

*Why:* one trait players learn to read, reused everywhere, instead of a trait per
decision. A greed trait was considered and not needed.

## Where characters come from

### CHR-5 Random generation, no families `v1`

Characters are generated at random. There are no births, families or heirs. A **target
population** keeps the world stocked: new characters are generated when the count falls
below it. A dead character is gone, replaced by this flow.

### CHR-6 New characters start at the bottom `v1`

A new character is low-level and independent, with no standing, no relationships and
low reputation. *(proposed)* They own a basic first-tier hull rather than waiting to be
hired. Everything above is earned by climbing. There are no academies or other special
entry points.

*See:* OPN-12 (which hulls, which path a spawn takes, where it appears).

### CHR-7 The starting world is seeded `v1` `proposed`

Planets have governors from the first tick, so the starting world is generated with
characters already in the higher roles. After that, only the bottom rung is generated.

*See:* GEN-9.

## Ownership and costs

### CHR-8 Every character owns their ships and wealth `v1`

Governors and factions **don't own or lend out fleet hardware**. A commander's stipend
(MIL-2) is compensation they spend on ships they own. Promotion never confiscates
anything. *(proposed, roles round)* A governor's own fleet is its personal property, not
the planet's; the planet's static defences and patrols are the planet's (MIL-20).

*Why:* governor-owned fleets suit a pure simulation but break the player fantasy of
building, modding and capturing ships (REJ-12). The cost is that a governor has no hold
over a commander's hardware, so the relationship carries loyalty (REL-3).

### CHR-9 Running costs `v1`

Fleets have a **running cost**, low enough that NPCs don't go bankrupt easily
between jobs. Fuel is part of it (ECO-11). The other costs are one-offs: buying ships
and equipment, and the set munitions refill after a battle (MIL-4). The running cost's
job is to tie fleet size to income, not to punish waiting.

*(proposed, tuning pass part 2)* **Keeping a fleet is the larger spend.** Every ship
has to be maintained, crewed and refuelled. Crews aren't tracked person by person: each
class has an average crew, scaled by tier. Upkeep is sized so that a sworn commander's
stipend covers its fleet with a margin at peace, and its profit comes from war. Munitions
stay a small spend.

*Why:* with purses, bounties and spoils, war money moved from treasuries to commanders
and stayed there. A cheap fleet gave them nothing to spend it on.

*See:* REJ-13 (no running cost at all), CHK-18.

## Careers

### CHR-10 Choosing work and spending money `v1`

- **A character picks a mission by fit and pay:** loadout fit, skill and payout. No
  personality.
- **Skills accrue from the missions taken,** which reinforces a path.
- **Unfilled missions pay more the longer they sit,** so characters drift toward
  unpopular work without a scheduler. Missions may expire.
- **When a character has saved enough, they spend it:** upgrade their ship or buy
  another. The choice weighs skills, which missions have been paying, and risk tolerance
  (CHR-4).
- **Reputation and fleet power unlock better missions,** which feeds income and skill.
- *(proposed, roles round)* **Every mission follows one primitive** (MSN-2): an
  issuer, a target, a payout fixed when taken, a deadline and a gate. Failing one,
  abandoning it included, is a bad deed with the issuer (MSN-3).
- *(proposed, war and fealty round)* **A governor's war objectives are missions too**
  (MIL-2): defend here, join this siege. Their pay is what a commander expects from
  kills and spoils (MIL-18, MIL-19), less what it expects to lose. Expected loss is
  value, not personality, so risk tolerance still doesn't govern the choice (CHR-4).

### CHR-11 Paths, not a ladder `v1` `proposed`

*(proposed, roles round; replaces "the ladder")* **Paths describe what a ship and the
market tend to produce. They aren't ladders:** no path has to lead anywhere, and none is
gated behind another. A merchant who runs dangerous routes with its own fleet may drift
into commanding or governing; nothing names that route.

| Path | Typical first jobs | Typical ship | Tends toward |
|---|---|---|---|
| Merchant | courier runs, single trips, Authority board missions (MSN-7) | a cargo hull, with or without a Lattice Drive (TRD-18) | rich merchant; steward; commander or governor with a fleet of its own |
| Warrior | scouting, bounties, escorts (MSN-10) | an armed hull | escort, commander, governor |
| Spy | covert deliveries, smuggling, tracking (MSN-11) | a small, fast, low-profile hull | spy for factions, smuggler, steward |

Every job comes from the same mission primitive (MSN-2), and every post is filled by the
same fitness function (CHR-20). Planet-level courier and scout missions are open to
anyone; faction-level ones need membership (INF-10). A courier who buys a gunship starts
taking scout missions.

### CHR-12 Fleet power: combat and trade `v1`

A character has **two ratings: combat power and trade (cargo) power**, both derived from
data that already exists (ship class, tier and loadout), not authored per ship.

- **NPCs read a static value per preset ship variant.** Illustratively, a baseline
  Valkyrie reads combat 4, its heavy-weapons variant 4.5, a Dragon one tier up 6; an
  Argonaut cargo ship reads low combat and high trade.
- **The player's rating is computed live** from their actual build, so it moves as they
  mod the ship, and is a progression number they can watch. *(open)* The formula (OPN-13).
- **One number space,** so the same gates read either: an escort contract can require a
  minimum combat power, a freight contract a minimum trade power, and a governor hiring a
  commander looks at both.

### CHR-13 NPCs fly preset variants `v1`

NPCs only fly preset variants drawn from their culture's catalogue (a Valkyrie, its
heavy-weapons variant, its armoured variant). Piece-by-piece customisation is
player-only.

*Why:* it keeps a couple of thousand NPCs cheap to generate and reason about, and keeps
the modding fantasy where it's felt.

## Losing

### CHR-14 No capture, ever `v1`

Prisoners, ransom, exchange and captivity are not in the game, in any version.

*See:* REJ-10.

### CHR-15 Escape pods `v1`

When a ship is destroyed, its pilot launches an **escape pod** that flies like a missile:
guided and fast, with no warhead and no fuse. It reuses the missile flight model.

- **In rendered range** the pod is a real object and can be shot down, which is the
  chance of death. For the player it's a skill moment: ride the pod to the nearest
  friendly or neutral planet.
- **Out of range** it's a low-percentage roll for death.
- **Pods can be upgraded** (faster, more armoured) as equipment.
- An escaped character is safe at the nearest friendly or neutral planet and carries on.

Fights stay frequent and deaths rare, so the player meets the same characters over time.
Escape odds are tuned organically (pod speed, personalities) rather than by special
rules.

*See:* OPN-14 (pod details).

### CHR-16 Losing a fleet `v1`

- **Any contract that needed the fleet ends automatically.** No disgrace or demotion:
  the job no longer exists.
- **Standing, reputation, relationships and skills stay.** Only ships and cargo lost in
  the fight are gone.
- **Rehiring is the market.** A governor choosing a commander weighs affordability, fleet
  power and their relationship with the candidate (CHR-20). A commander who saved can
  rebuild fast and is rehired quickly; one who gambled everything starts smaller. The
  game doesn't referee saving against spending.

## The player

### CHR-17 The player is one more character `v1`

The player follows every rule here, with no special information or access. They
**start as a neutral trader**: neutral standing with the Trade Authority and a small ship
**without a Lattice Drive**. Getting one is the moment the universe opens up. The roles
open to them are the roles open to everyone: trader, courier, scout, smuggler, spy,
escort, mercenary, sworn commander, governor, pirate. A player governor who leaves fealty
becomes the root of their own faction, by the same rule as any governor (FAC-2, FAC-11).

*(proposed, roles round)* **The one exception is the steward,** which is NPC-only in v1
(FAC-16). Time stops while the player is at a planet, so a player steward would have
nothing to do. A player steward is `later`.

`open` How the start reaches the Lattice Drive (OPN-43). Both the merchant and the
warrior paths should reach it by their own work. The warrior's escort jobs need a drive
for Lattice merchants, and a merchant's choice between a drive and a ship without one is
a real fitting decision (TRD-18).

### CHR-18 Death and saving `v1`

If the player's pod is shot down, they die and reload. The penalty of death is the
reload; pod upgrades and careful play lower the risk. The game **autosaves when the
player lands** at a planet, as in Escape Velocity.

### CHR-19 Named characters and static casting `proposed`

Named merchants are characters like any other. Replacing procedurally generated
characters with hand-made static ones, so the player meets authored personalities, is an
idea to try; it may be too hard to manage.

## Choosing for posts

### CHR-20 One fitness function for every post `v1` `proposed`

*(roles round)* Whoever fills a post scores the candidates with **one fitness function**,
on data that already exists, with weights for each post:

| Post | Chooser | Leans on |
|---|---|---|
| Governor | the liege, the capturing keeper (FAC-8, FAC-9) or the succession (FAC-17) | combat power, reputation, opinion |
| Steward | the governor | trade power, reputation, opinion |
| Commander | the governor | combat power, affordability, opinion |

- **The inputs** are the two power ratings (CHR-12), reputation (REL-7), the chooser's
  opinion of the candidate (REL-2), shared culture (WLD-18), and for commanders, what the
  stipend costs (MIL-2). The weights are Phase A tuning.
- **Opinion carries real weight, and power and reputation have diminishing returns,** or
  the biggest fleet or the richest merchant wins every post.
- **Governor and steward are single seats:** the best fit takes it. **Commanders are an
  open market:** a governor hires zero to many, each when its value clears its cost in
  the budget (MIL-6). Zero is a valid state.
- **Risk tolerance isn't an input** (CHR-4).

*Answers:* OPN-15 (how a governor weighs hiring) and OPN-40 (what reputation is worth to
an employer: it weighs in every pick).

