# Military (MIL)

Commanders and what they cost, how planets defend themselves, how armies form and move,
how invasions and sieges resolve, and how combat away from the player is decided.

## Commanders

### MIL-1 Commanders `v1`

A **commander** is a character with a fleet (CHR-1). Commanders aren't nodes of the
faction tree; they attach to it by a contract with a governor (REL-11). Commanders want
to grow in power and get more ships, bounded by how much money they can make.

### MIL-2 The stipend `v1`

A sworn commander is paid a **stipend** by their governor. It's compensation the
commander spends on ships they own (CHR-8) and on munitions. The governor controls how
much a commander can spend; the commander decides how. **Governors set fleets' overall
objectives** (attack, defend, join an army); **commanders make the tactical decisions**
in service of them. A war costs a governor the stipends it pays out. A commander who
serves well keeps or grows their stipend.

### MIL-3 Serving the liege `v1`

When a liege raises an army from its vassals, **the liege pays the stipends of the
vassals' commanders while they serve**. So the vassal isn't paying for its liege's war;
its cost is a home left less defended. The commanders want to go, because campaigns are
how a commander wins a planet (FAC-9).

### MIL-4 What a fleet costs `v1`

- **A low running cost** (CHR-9), including fuel (ECO-11).
- **Munitions** are a set refill cost after a battle, paid on landing at any friendly or
  neutral planet, which has as many as the fleet needs. A money sink in v1 (ECO-6).
- **Ships trickle in:** planets produce them slowly, almost magically, for recruitment,
  like Mount & Blade's units, drawn from the planet's culture (WLD-18). Buying one is a
  money sink in v1.
- **Equipment** is sold by outfitters for money, outside the economy, in v1.

*(later)* Munitions, ships and equipment come into the supply chain (PRD-13); the
player's missiles are finite in a magazine and restock depends on a planet's stock.

### MIL-5 Planetary defence `v1`

A planet has **static defences** (guns, missiles, torpedoes that fire at any opposing
ship in range) and a **security force**.

- **Static defences** *(proposed)* scale with what the planet spends on security.
- *(proposed, round 3)* **Defence is bought in proportion to income,** not in fixed
  amounts, so a rich planet's spending is on the scale of its wealth. Money beyond a
  reserve makes a governor value defence more: in war, deep treasuries drain into fleets
  and guns. Ships destroyed and planets lost are where that money finally leaves.
- **Routine patrols are procedurally generated**, not characters. The planet's patrol
  upkeep (ECO-4) is the budget line that sets how strong they are.
- **Planets may post paid security missions** for the player and NPCs.
- A typical planet can defend itself but can't field enough force to take a defended
  neighbour alone.

*Why:* patrolling is boring for the player and not a sensible career for an NPC.

*See:* CHK-6 (defence numbers are a first-class lever).

### MIL-6 Hiring commanders `v1` `open`

A governor choosing a commander weighs what they can afford, the candidate's fleet power
and their opinion of the candidate. How these combine, how often a governor decides and
how many candidates it considers are open (OPN-15).

## Armies

### MIL-7 Intent and action `v1`

Forces gather into **armies** before a major operation, as in Bannerlord.

- **Intent** is the strategic objective ("invade planet X"). It's set when the army
  forms, telegraphed (a scout who sees the army learns it) and hard to overturn.
- **Action** is what the army is doing now in service of it: moving, fleeing a larger
  force, refuelling, attacking a vulnerable target on the way. Actions change freely.

*Consequences:* an army's position goes stale fast but its intent stays valid (INF-5).
The muster is the earliest warning, for whoever can see deep into enemy territory. The
army's exit gate is a strong clue to its target. The player can shape an army's actions
without changing its intent: harassing it, hunting its scouts, baiting a detour.

- *(proposed)* Aborting (after losing a commander, or seeing an overwhelming defence) is
  cheap. Retargeting costs time, cohesion and the commander's reputation.
- *(proposed)* An army pursues a side target only if the delay is small relative to its
  intent.

### MIL-8 How armies move `v1`

Along the Lattice, off at the nearest exit to the target that will let them out
(WLD-13), then through open space. Armies never bail out.

### MIL-9 Invasions `v1`

An invasion is an army **commissioned by a governor**, made of the fleets of the
commanders sworn to that governor and of the commanders of its vassals (serving under
MIL-3). In practice only governors with vassals can muster enough. Invasions happen only
against factions at war (DIP-6) and pirate planets. Whether and where to invade is a
budget decision on believed value against full cost, aftermath included (ECO-8).

*(open)* How the target is picked and when an invasion launches (OPN-17).

### MIL-10 Sieges `v1`

An attacked planet fights with its **static defences and its defence fleet** (the
commanders sworn to its governor who are present, and its patrols). **Destroying both is
the attacker's victory, and it takes time.** While the siege runs, the faction sends
defenders if it has been told (MIL-11).

- *(proposed)* The siege resolves in rounds of the combat formula (MIL-13); how long the
  planet holds out is its defensive strength against the attacker's damage per round.
- *(proposed)* A planet whose commanders are away serving their liege is more exposed.
  That trade-off needs no extra rule.

*(open)* How a siege plays in the rendered game (OPN-4). A player arriving mid-siege
finds it in progress (MIL-13).

### MIL-11 Calls for help and relief `v1`

A planet that learns of a threat (from its scouts, INF-9) sends couriers up the fealty
tree asking for help (INF-8). The liege sends forces back along the Lattice. **Which
vassal is helped first, when several call at once, is the liege's opinion of each
(REL-2).** A relief force that arrives fights the besiegers.

The timing race is the core of defence: the invader's open-space crawl from the nearest
allowed exit, against the defender's detection, scout return, courier run and
reinforcement travel (INF-12).

### MIL-12 Capture `v1`

When the attacker wins a siege, the planet joins the attacking governor's tree by keep
or grant (FAC-8), gets a new governor (FAC-9), loses prosperity (FAC-10), and its gates
flip with it (WLD-13). *(proposed, round 3)* **Holding it is a choice:** the army's
commissioner weighs garrisoning the prize (the stipends, and its own home left exposed)
against the believed risk of losing it, and otherwise sends the army home. Border
planets are expected to change hands often in a war, a few times, not dozens. A pirate planet that's captured returns to its old production
(PIR-3).

### MIL-17 Gate attacks and blockades `v1` `proposed`

An army at war can attack an enemy's **gate** (a group's interchange) instead of a
planet. It's a cheaper objective than an invasion: it cuts the enemy's trade and
reinforcement without taking territory.

- **Defence:** the gate has static defences of its own, a share (`blockade.defence_share`)
  of what its holder's security spending buys (MIL-5), so it's a little easier to take
  than the planet. Any force garrisoned at the gate fights too. The attack resolves in
  siege rounds like a planet's (MIL-10).
- **Under attack, the gate is still the holder's:** its faction can get on and off there.
  The holder knows at once and calls for help as for a siege (MIL-11). Relief comes
  along the Lattice, so reinforcing a gate is fast.
- **Captured, it's a blockade:** a discrete state, not a gradual one, so everyone can tell
  whether they may use the gate. A blockaded gate is shut to everyone except the
  captor's faction, whose armies can use it as a forward exit (WLD-13). The holder's
  faction loses its way on and off the Lattice there.
- **Holding it:** a blockade has no static defences. The captor may leave a garrison for
  a while; a blockaded gate with nobody holding it is retaken by any force of the
  holder's faction that arrives. A garrisoned one is retaken by beating the garrison.
- **It ends** when the gate is retaken, when the two factions are no longer at war, or
  when the holder planet falls to the captor's faction (then it's the captor's gate
  anyway).
- **News:** a blockade is a fact (INF-2) that travels like any other. Merchants and fleets
  plan routes around blockaded gates they know of. One that doesn't know finds out on
  arriving at the gate along the Lattice, and replans from there.
- **Through traffic** on the road keeps moving; only getting on and off at the gate is
  shut. *(later)* Cutting a road.
- Pirates can't attack gates (PIR-2).

*Why:* wars should cost the enemy's economy without every war changing who owns the map.
Discrete states make a blockade easy to track and to see.

## Combat away from the player

### MIL-13 Off-screen combat is auto-resolved `v1`

Combat away from the player is resolved from **fleet power plus randomness**, with a
simple formula in v1 (SYS-14). It produces losses, cargo destroyed or seized, and
retreat. **Battles have duration,** so a player who arrives mid-fight finds it in
progress with the losses so far applied. **In the simulation a fleet's commander is
always the last ship destroyed;** their pod then rolls for escape (CHR-15).

*(later)* Calibrate auto-resolve against rendered combat in the headless harness, so the
player's presence doesn't bias outcomes (OPN-23).

### MIL-14 A fleet without its commander `v1` `rendered`

In the rendered game, if a commander's ship goes down first, the rest of the fleet fights
on mindlessly. It's either destroyed, or wins and lingers around the battle until it
despawns when the player leaves range.

### MIL-15 Raids `v1`

A raid attacks merchants or other ships for cargo. A raider never recovers everything it
destroys or seizes, so every raid is a small money sink (ECO-5). Pirates sell loot only
to a pirate planet (PIR-5).

### MIL-16 Mk variants `later`

Military contraband flowing into pirate production gives pirate fleets a chance of **Mk
variants**: the same ship with a more dangerous loadout, as in Escape Velocity ("that's
a Mk 4, stay away"). Illegal weapons are the pirate playstyle's gear reward. *(open)*
Whether lawful ships get Mk tiers through legal upgrades (OPN-28).
