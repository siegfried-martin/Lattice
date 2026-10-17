# Characters: design

*Written 2026-10-17 from a design session that followed `FACTIONS.md`, `ECONOMY.md` and
`TRADE.md`. This covers what a character is, how characters are employed and promoted,
what they own, what happens when they lose, how merchants hire protection, and how
information and missions travel through the fealty tree. It supersedes parts of the three
earlier documents; each of those notes the change where it applies (see "What this
changes elsewhere").*

*Items marked **(proposed)** came up in the session and weren't explicitly signed off.
Items marked **(open)** are unresolved. Everything else is decided. Numbers are tuning,
not design.*

*Revised later on 2026-10-17 after review: characters are generated at random at the
bottom rung, with no births or families; the bottom rungs are scouting (warrior path) and
couriering (merchant path); fleets have little or no running cost; escorts are hired for
one mission, and merchant missions come in two shapes.*

## Principles this serves

From `VISION.md`, and the reason the model looks the way it does:

- **The player lives under the same rules as everyone else.** Earlier drafts kept
  running into places where the player's rulebook and the simulation's rulebook
  disagreed: a governor who owns the fleet is right for a pure simulation and wrong for
  a player who has spent hours building and modding a ship. One model for both removes
  the contradiction.
- **Progression is access and reputation,** and the player's ships are theirs to build
  and personalise.
- **Every character acts on its own incentive.** Nobody waits idle for the world to
  hand them a job.
- **Idle costs nothing but time.** Fleets have little or no running cost. A recurring
  burn rate would bankrupt any commander between missions and turn every lull into a
  death spiral, so there isn't one.

## One character, many roles

There is one kind of entity: the **character**. Governor, steward, commander, merchant,
spy, scout, courier, escort pilot, mercenary, pirate and the player are **roles** a
character holds, not types. A role is a current contract or status, and it changes.

- **Career is a path through contracts,** not a change of entity. An escort pilot becomes
  a commander, who takes a planet and becomes a governor. The same character, with the
  same ships, wealth, skills and relationships, carries across every step.
- **Named merchants are characters** who happen to hold the merchant role. Their ships,
  wealth and risk calculation are the same as anyone's.
- **Ambient traders are not characters** (`TRADE.md`, ambient trade). They are spawned,
  unpersisted, and promoted to a character only when something lasting happens.

What a character has, whatever the role:

- **Ships and wealth,** personally owned (see Ownership).
- **Skills,** which accrue from the missions they take.
- **Relationships and standing** with governors, factions and other characters.
- **A fleet power rating,** in two parts (see Fleet power).
- **One personality trait: risk tolerance,** which governs spending (see Careers).
- **A belief state** (`FACTIONS.md`, Belief state), which is what they act on.
- **Goals and actions** as `FACTIONS.md` describes them: fixed long-term goals, short-term
  actions that follow what they currently know.

## Where characters come from

**Characters are generated at random. There are no births, families or heirs.** Games
like Mount & Blade model family formation; it isn't interesting here and doesn't fit.

- **A target population keeps the world stocked.** New characters are generated when
  the number present falls below a target, so deaths are replaced without the count
  drifting. The target and the refill rate are tuning.
- **New characters enter at the bottom.** Low-level, independent, with no standing and no
  relationships, owning a basic first-tier hull (see The ladder). Everything above is
  earned by climbing, not generated.
- **The starting world is seeded with characters already in the higher roles,** since
  planets have governors from the first tick. **(proposed)** After that, only the bottom
  rung is generated.
- **A death is replaced by this flow, not by an heir.** The dead character is gone. The
  escape pod makes that rare (see Losing).
- **Ambient traders** are a separate thing, as above: unpersisted, and promoted only when
  something lasting happens.

## Fealty and employment are contracts

**There is no caste line between a sworn commander and a mercenary.** Both are a
character with a contract with a governor. They differ only in what the contract says
and how long it has run.

- **Fealty is a standing contract** that persists until something ends it: the
  commander quits, loses their fleet, takes a better offer, or the governor no longer
  needs them.
- **A mercenary is a character with no standing contract,** working specific tasks for
  whoever pays (the Authority, governors, merchants).
- **Hiring an escort is the same mechanism as hiring a fleet commander,** at a different
  scale. An escort isn't a special slot waiting in a pool. They're a character whose
  current contract is "protect this convoy".
- **The player is in the same system.** Swearing fealty is signing a contract. Leaving
  is ending one.

**How long a contract runs depends on the job.** Fealty is standing, as above. A
mercenary task, an escort job and a courier or scout mission are each for one job: the
contract starts with it and ends with it. Nobody is on retainer.

Why: it's the only definition that works for the player and the simulation at once, and
it makes promotion fall out of success. A commander who takes a planet for their liege
is installed as its governor, which is their pay for the job. The liege gains a vassal,
which is theirs. Both are paid in what they wanted.

## Ownership

**Every character owns their ships and their wealth, always.**

- Governors and factions **don't own and lend out fleet hardware.** That was considered
  and rejected. It's what a real government does, and it's the right model for a pure
  simulation, but it breaks the player fantasy: building a ship, modding it, capturing
  another, mixing ships from different factions. It would also mean that being promoted
  to commander or governor confiscated the thing the player was proudest of.
- **A stipend is compensation,** and the commander spends it to buy ships and grow a
  fleet they own. It isn't a budget for governor-owned assets. This matches `ECONOMY.md`,
  where recruited ships are bought with stipend money; what changes is that the commander
  keeps title.
- **Running a fleet has little or no ongoing cost** (assumed, and important). A
  commander between missions pays nothing for waiting. Without that, a recurring burn rate
  would bankrupt commanders who can't find work, and a quiet month would be the easiest
  way to fall out of the game. What remains are one-off costs: buying ships and
  equipment, and the set munitions refill after a battle (`ECONOMY.md`). The recurring
  crew-wage and fuel bill an earlier draft attached to the stipend is dropped.
- **War cost to the governor is still the stipends paid.** The budget model in
  `ECONOMY.md` is unchanged.

The cost of this choice: a governor has no hold over a commander's hardware, so the
relationship has to carry the loyalty. It does, through hiring (below).

## Losing

**Capture is removed.** Prisoners, ransom, exchange and captivity-escape are gone from
the design. They produced clunky situations (a captive dragged along with the captor's
army, a lone ship suddenly carrying a whole fleet's cargo) and they came from a
historical setting Lattice isn't in.

What replaces it is the **escape pod**.

- When a ship is destroyed, its pilot launches a pod. The pod flies like a missile:
  guided, fast, no warhead, and **no fuse or cooldown** racing against it. It reuses the
  missile flight model that already exists.
- **In rendered range, the pod is a real object** and can be shot down. That is the
  chance of death, and for the player it is a skill moment: you ride your pod out of the
  wreckage and try to reach the nearest friendly or neutral planet. A winner who wants to
  deny the escape can hunt it.
- **Pods can be upgraded** (faster, more armoured) as ship equipment.
- **Out of range, it's a low-percentage dice roll.** There's no point simulating
  something nobody can see.
- **An escaped character is safe at the nearest friendly or neutral planet** and carries
  on.
- Fights stay frequent and exciting. Deaths stay rare. That is the point: characters
  persist, so the player can meet the same ones over time.

Consequences of losing a fleet:

- **Any contract that needed the fleet ends automatically.** This isn't disgrace or
  demotion. The job (command a fleet) no longer exists.
- **Standing, reputation, relationships and skills stay.** Only the ships and cargo lost
  in the fight are gone.
- **Rehiring is the market, not a rule.** There's no protection for established
  commanders. A governor choosing their next fleet commander weighs what they can afford,
  the candidate's fleet power rating, and their relationship with the candidate. So:
  - a commander who saved money rather than spending it all on one campaign can buy ships
    fast and is hired again quickly, by the same governor or a neighbour who remembers
    them;
  - a commander who spent everything on one gamble and lost has nothing to rebuild with
    and starts smaller.
  - Saving against spending is a real strategic choice and the game doesn't referee it.

The "private contractor Adams, who used to be fleet commander Adams" story comes from
this for free. A character's history is visible in what they fly and who hires them.

## Careers and specialisation

**There are no classes and no personality types dividing merchants from warriors.**

- **The ship decides which missions a character can do.** Combat hardpoints against
  cargo capacity follows the equipment-slot principle already in the design. A mixed
  loadout takes small deliveries and escort jobs both, because the hardware allows it,
  not because the character is a "hybrid type".
- **Skills accrue from the missions taken.** A character who takes escort jobs gets
  better at them, which reinforces the path.
- **A character picks what to take by fit and pay.** Loadout fit, skill and payout, an
  optimisation with no personality in it.
- **Unfilled missions pay more the longer they sit,** so characters drift toward
  unpopular work without a scheduler. Missions may also expire if never taken.
- **When a character has saved enough, they spend it:** upgrade the ship they have, or
  buy another and grow a fleet. The choice weighs skills, which missions have been
  paying, and the one personality trait, **risk tolerance** (conservative against
  aggressive spending). Risk tolerance governs only save-or-spend decisions, not mission
  choice. It is what explains why one commander is rehired quickly and another isn't.
- **Reputation and fleet power unlock better missions,** which feeds income and skill,
  and the loop closes.

### The ladder

Two career paths, each starting with a job anyone can take on day one.

- **Warrior path:** scout, then escort, then fleet commander, then governor.
- **Merchant path:** courier, then merchant.

Tier one:

- **Scouting is the first warship job,** done in a basic armed hull. **Couriering is the
  first merchant job,** done in a fast unarmed one. Both are open early, to the player
  and to NPCs alike.
- **Choosing between them is the ordinary fit-and-pay choice.** The hull decides what is
  possible; pay and skill decide what is taken.
- **Planet-level courier and scout missions are open to anyone,** so a new character
  always has something to do. Faction-level ones need membership **(proposed)**, as
  described under Information and missions.
- **The work isn't make-work.** It is what carries information through the fealty tree
  (see below).

Above tier one:

- **Escorting merchants is the second rung of the warrior path.** It needs more combat
  power, and for a faction's convoys, good standing with that faction (see Merchants and
  protection).
- **Commander and governor follow,** as described under Fealty and employment.
- **Merchant is the step after courier:** a character with a trade hull running trade
  missions. **(open)** Where that path goes after merchant.

The paths aren't walls. There are no classes, so a courier who buys a gunship starts
taking scout missions. The ladder describes where the gates fall, not a track.

### Fleet power

A character has **two ratings, not one:** combat power and trade (cargo) power.

- **Both derive from data that already exists:** ship class, tier and loadout. They
  aren't authored per ship.
- **NPCs read a static value per preset ship variant.** For example (numbers
  illustrative): a baseline Valkyrie combat ship reads combat 4, its heavy-weapons
  variant 4.5, a Dragon one tier up 6. A cargo ship of that tier, an Argonaut, reads
  low combat and high trade. A mixed loadout sits between.
- **The player's rating is computed live** from their actual piece-by-piece build, so
  it moves as they mod the ship ("the Dragon's base is six, mine is eight because of
  what I put on it"). It is also a progression number the player can watch rise. **(open)**
  The formula; it isn't decided.
- **Same number space, so the same gates read either.** An escort contract can require a
  minimum combat power, a bulk freight contract a minimum trade power, and a governor
  hiring a commander looks at both.

**NPCs only fly preset variants** (a Valkyrie, a Valkyrie heavy-weapons variant, a
Valkyrie armoured variant, drawn from their culture's catalogue). Piece-by-piece
customisation is a player-only option. That keeps a couple of thousand NPCs cheap to
generate and reason about, and keeps the deep modding fantasy where it's actually felt.

## Merchants and protection

This replaces the idea that a planet runs a trade operation of its own. See "What this
changes elsewhere" for what that does to `TRADE.md`.

### Why a government doesn't hire traders

Price is the one motivator for trade. A governor who wants a good just lets its price
rise, and merchants come. A planet matchmaker that lines up convoys with escorts was
considered and dropped: it adds waiting (nobody can act until the planet has posted
something), it doesn't fit characters having careers, and it duplicates what price
already does.

### Merchants are independent characters

A merchant's goal is a **trade mission.** That fits the information economy better than
drifting between markets. Missions come in two shapes:

- **Faction missions are a triangle of three legs.** Deliver a good to a planet, go to a
  second planet and pick something up, then bring it back to the first planet or to a
  third. It keeps the merchant inside the faction's territory and moving goods both ways.
- **Independent missions are single trips:** carry a good from A to B and sell it there.
  They are open to everyone, or to a lower tier only. **(open)** Which.

### The faction pays merchants to stay home

Trade moves the whole faction's economy, so the **faction (its root governor) pays
merchants a stipend** to operate within its territory.

- **A merchant stipend is not a commander stipend.** It is paid by the faction as a
  whole, for being present and staying in bounds, with **no tasking.** A commander's is
  paid by a governor and comes with orders and long-term objectives (attack, defend, join
  a fleet).
- **Merchants may trade with factions not at war with their own,** but must return
  promptly.
- **They stay targetable.** Because they are in their faction's territory, an enemy
  faction can raid them. That is where the economic-warfare targets come from.

### Merchants hire their own protection

- **Only successful merchants hire escorts.** Poorer merchants take the risk.
- **The escort is a private contract** between two characters. No planet or governor is
  involved.
- **An escort is hired for one mission, not on retainer.** The contract starts with the
  mission and ends with it. Nobody pays an escort to sit idle, and nobody sits idle on a
  retainer. This is what keeps the no-running-costs assumption honest (see Ownership).
- **A faction's triangle is escorted as a whole.** The escort job covers all three legs
  and is open to characters in good standing with that faction.
- **An independent merchant's single-trip escort job has no standing gate.** **(proposed)**
- **It is the second rung of the warrior path,** after scouting. Escorting merchants
  gives a character income, skill and reputation on the way to a fleet command.

### How the decision is made

The map carries a **risk number per sector.** A merchant already plans a route to find
its way; it sums the risk of the open-space sectors it will cross (the Lattice is safe
road) and weighs that against the cargo's value and the price of an escort. A cheap load
through safe space never bothers. A rich load through pirate-adjacent space does, even
for a merchant who isn't wealthy; they only need enough cash to make the offer.

- **The cost is trivial:** it's a lookup summed along a route that exists anyway, over a
  map of about 150 sectors for the first version.
- **The trigger is risk against cost, not a flat wealth threshold.** Wealth only decides
  whether the merchant can act. A merchant who grows wealthier carries more valuable
  loads, so their risk rises, and that's what pushes them to shop for a better escort.
  **(proposed)** The comparison runs again for every mission, so hiring, hiring a
  stronger escort and going without are one decision, made per mission.
- **Per-sector risk is a belief, not a fact.** Each character holds its own copy, as
  deviations from the truth, and updates it by the same belief-state rules as everything
  else (`FACTIONS.md`): by visiting the sector in open space, by what a faction shares.
  Bad information makes bad decisions, and that is wanted:
  - joining a faction gets you its shared intelligence, a reason to swear that doesn't
    depend on standing alone;
  - killing a faction's scouts degrades its merchants' decisions;
  - a merchant with stale data can skip an escort it needed or buy one it didn't.
  It's the same mechanism as every other belief, not a second information system.

### How the offer reaches an escort

- **The merchant broadcasts the offer over comms,** to anyone within sensor range, along
  the whole route, highway included. It is the same in-range comms the game already
  plans for hails, with a new message type: a contract offer for money. The player can
  accept or decline.
- **The offer goes to NPCs in range as well,** who accept by the optimisation above.
- **Warrior characters don't sit at planets waiting.** They travel with their bands and
  take what they come across, including an escort job if it fits.
- **A merchant who loses an escort mid-mission keeps trading** and posts a new offer for
  the legs that remain. Stopping to wait costs more than risk. It is the same mechanism
  as the first hire, triggered by loss. A convoy alone in dangerous territory is more
  likely to be picked up by a passing escort than one in a quiet core system, which is
  emergent, not authored.
- **A new escort is needed at the start of each mission,** since the contract ends with
  the mission. Mid-mission, the reason is a destroyed escort. An escort who walks away
  mid-mission breaks the contract and loses standing for it. **(proposed)**

## Information and missions travel through the fealty tree

This extends "Information travels physically" in `FACTIONS.md`. Nothing is instant, and
the one exception is zero distance.

### Contact sharing

When two characters with a **common interest** are in contact, they share what they
know. A character who lands on a planet of their own faction gets that planet's
information. A character who meets another member of the faction exchanges beliefs.
That's the sighting primitive at zero range, not a new rule.

- **It gives members a reason to land on their own faction's planets.**
- **Traitors and spies benefit too:** a spy sworn to a faction gets its information
  when they land, without hiring their own scouts.

### Couriers and scouts

These are two different jobs, with different ships and characters:

| | Courier | Scout |
|---|---|---|
| **Where** | Inside the faction, on the Lattice | Outside the faction, mostly open space |
| **Ship** | Fast, no combat capability | Some combat capability |
| **Risk and pay** | Low | Higher, longer range |

They are also the bottom rung of the two career paths (see The ladder): scouting for the
warrior path, couriering for the merchant path.

A faction with more money runs more couriers more often, and its planets' information
goes stale more slowly. A rich faction has better information, which lets it trade and
defend better.

### Who has an incentive

- **A planet scouts out of self-preservation,** and only if it expects an attack. A
  fringe or border planet scouts the enemy's territory and sends couriers home when it
  finds troops near. Planet scouts stay as `FACTIONS.md` describes them.
- **A faction scouts for strategy and trade:** enemy troop movements for the whole war,
  prices in neutral territory. No single planet would pay for that alone, because the
  payoff goes to everyone.
- **This gives four mission types:** planet-level and faction-level couriers, and
  planet-level and faction-level scouts. **(proposed)** Faction-level missions are open
  only to characters who have joined the faction. Not set in stone.
- **A scout is a character generating sightings,** like anyone. Anyone near enough to
  see what a faction scout sees gets the sighting too. It is not a private channel.

### Missions travel at courier speed

Faction-level missions don't appear everywhere at once. The mission **travels with the
couriers.** A governor who wants a region scouted sends a courier to the relevant
planet, which both delivers the order to the planet's mission computer and brings back
that planet's current information. The next courier run is when a mission shows up there.

- **Delivery follows the fealty tree, hop by hop.** The root governor's couriers go to
  their direct vassals. Each governor who has vassals runs their own courier network to
  the planets below them. Information flows back up the same way.
- **A planet many fealty hops from the root learns and is commissioned more slowly.**
  That's the same distance that drives fracture pressure, so loyalty and information lag
  have one cause.
- **Missions spread out through the world** instead of piling up at the capital, so a
  player in the fringe finds jobs from whichever governor is nearest.
- **The player can read the hierarchy from the traffic** (a missions board that says one
  lord wants his top general), which is worldbuilding for free.
- **Couriers do three jobs:** carry information, carry orders and missions, and carry
  the help-requests `FACTIONS.md` already describes.
- **Investing in couriers matters twice:** a faction with poor courier coverage learns of
  danger late and also commissions a response late.

### What stays deferred

- **A market for information** is still deferred (`FACTIONS.md`). Maybe a thing the player
  can do and NPCs can't, or maybe never.
- **Spies** are as `FACTIONS.md` describes them.

## What this changes elsewhere

Recorded here and noted in the other documents so a reader of one isn't misled.

**`FACTIONS.md`**
- *Commanders* (sworn / mercenary / pirate): now contract status, not three classes.
- *The stipend:* now compensation plus operating allowance for a commander who owns
  their ships, not a budget for governor-owned assets.
- *Mercenaries:* the same character, no standing contract.
- *Prisoners:* removed. Replaced by the escape pod.
- *Scouts and couriers:* a planet's scouts stay. Faction-level scouting, contact
  sharing, mission travel and the courier/scout split are added. Both are also the first
  rung of the two career paths.
- *The player's roles:* same system as everyone.

**`ECONOMY.md`**
- Stipends still pay for ships and munitions, which are still money sinks. The ships
  belong to the commander.
- Fleets have little or no ongoing cost, so no recurring bill is charged to a character
  between missions. The munitions refill stays as a set one-off after a battle. Planet
  operating costs and patrol upkeep are unaffected. Merchant fleet operating costs go
  with the open question on planet merchant fleets.
- Hiring ships is open to any character who can afford to, not only to wealthy merchants,
  sworn commanders and "occasionally" mercenaries.

**`TRADE.md`**
- *Escorts:* hired by merchants as private contracts, from characters, for one mission
  each. The governor's procedural escort budget line is superseded.
- *Merchant missions:* a faction merchant runs a three-leg triangle, escorted as a whole
  by a character in good standing with the faction; an independent merchant runs single
  trips.
- *Planet merchant fleets:* **(open)** see below.
- *Missions for the player:* escort jobs arrive as comms offers from convoys in range,
  not as a planet's posted mission.

## Open questions

- **The fate of planet merchant fleets.** `TRADE.md` has a planet send persistent fleets
  of merchant hulls on missions, with the governor paying operating and escort costs. If
  all trade is done by independent merchants paid a faction stipend, that layer has no
  motivator of its own. This document doesn't decide it. Options: drop the layer and let
  merchants and osmosis carry the economy, or keep it as the faction-stipended merchants'
  form (a merchant's convoy being "the planet's fleet" in name only). This changes what
  economic warfare has to target and should be settled before `TRADE.md` is promoted.
- **Hiring detail.** How a governor weighs affordability, fleet power and relationship
  into one choice; how often it decides; how many candidates it considers.
- **Merchant stipend size** and how it is split across a faction.
- **Steward as contract.** Whether a steward is a role held under the same model (almost
  certainly) and what a disloyal one costs (`FACTIONS.md` open questions).
- **Pod details:** upgrade tiers, how hard a winner can hunt one, and the dice-roll
  percentage out of range.
- **The player's fleet power formula.**
- **Generation details.** The target population, how fast it refills, and where a new
  character appears. Tuning, not design.
- **Starter hull.** A new character owns a basic first-tier hull rather than waiting to be
  hired **(proposed)**. Which hulls, and which path a spawn takes, are open.
- **Where the merchant path goes** after merchant.
- **Who can take independent merchants' missions:** everyone, or a lower tier only.
- **Fuel as a running cost.** Fuel is sold by planets (`ECONOMY.md`). A per-trip charge
  has to stay small enough not to act as a burn rate.
- **Recruitment and relationships** remain separate documents. This one leaves hooks:
  contracts, relationships and standing are the inputs they will define.

## What a headless prototype should check (proposed)

- Careers happen: a meaningful share of scouts become escorts, some of those become
  commanders, and a few become governors. A meaningful share of couriers become merchants.
- The population holds near its target, and new characters find tier-one work straight
  away.
- No commander goes bankrupt waiting for missions: a character with no work loses time,
  not money.
- Escort contracts end with their missions, and a faction's triangle is escorted across
  all three legs.
- Commanders who lose a fleet are rehired in a plausible time; reckless ones recover
  slower than conservative ones.
- Deaths per fight stay low while fights stay frequent.
- Merchants hire escorts when route risk and cargo value justify it, and not when they
  don't. Hiring doesn't herd every merchant onto the same few routes.
- No character, merchant or mercenary, sits idle for long.
- Faction-level missions reach fringe planets late but reach them, and the hierarchy
  shows in the traffic.
- Stale mission pay pulls characters toward unpopular work without starving popular work.

## For the implementation agent

Not to be acted on while this document is in `docs/planning/`. When it's promoted:

- Add it as `docs/CHARACTERS.md`.
- Fold a summary into `BRIEF.md` and update its open questions.
- Reconcile `FACTIONS.md`, `ECONOMY.md` and `TRADE.md` against it before promoting any
  of them.
- Recruitment and relationships remain their own documents. Leave hooks, not designs.
- This is design only. Don't build it yet.
