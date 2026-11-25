# Piracy and contraband (PIR)

The pirate faction, pirate planets, how commanders and the player become pirates or
smugglers, and how lawful space polices contraband.

## The pirate faction

### PIR-1 One collective pirate faction, an anarchy `v1`

All pirates belong to **one pirate faction**, hostile to everyone outside it. It's an
**anarchy**: pirates don't attack each other, but they don't coordinate either. There's
no hierarchy, no root and no couriers. It takes no part in diplomacy (DIP-7).

### PIR-2 Pirates are slow and live on the fringe `v1`

Full pirates **can't use the Lattice**: the Authority refuses them (WLD-12), so they have
no Lattice fuel and move everything through open space. That makes them weak near the
highways, where lawful forces arrive fast and no ally can reinforce a pirate planet along
the Lattice. **Pirate planets near the Lattice don't last.** On the fringe the
disadvantage fades and their black market gives them the edge, so the only
self-sustaining piracy is there.

*See:* WLD-21, CHK-4.

## Pirate planets

### PIR-3 How pirate planets work `v1`

- **A planet turns pirate through economic failure** (ECO-13), joining the pirate faction.
  *(proposed)* Only a planet with no vassals turns pirate; one with vassals secedes
  instead (ECO-13).
- **It produces only contraband** (PRD-1), with its whole labour on it, and **switches
  back to its old production when it's recaptured**.
- **All its life support comes from missions and raids,** so a starving pirate planet
  raids more.
- **It offers two kinds of mission,** commissioned locally to whoever is there: smuggling
  runs to offload contraband, and legal-goods runs to bring it food, water and oxygen.
- **It sells fuel** (ECO-11).
- **It has a low cost base** and pays no dues (ECO-14).
- **It doesn't redeem itself.** It returns to lawful space only by being captured
  (MIL-12).

### PIR-4 Commanders who go broke `v1`

A commander who runs out of money **travels to a pirate planet they know of, renounces
their allegiances and is hired** by it.

- **Running costs are suspended on the way.** They stay lawful until they renounce, so
  they can still ride the Lattice for most of the trip.
- **They pick from what they know** (INF-2). A commander who knows no pirate planet goes
  broke the ordinary way.
- `experimental` **Risk tolerance decides** between piracy (aggressive) and downsizing by
  selling ships (conservative), compared in headless runs against every broke commander
  turning pirate (EXP-9). Keep whichever produces a better pirate population.
- *(proposed, tuning pass part 4)* **Turning is rare, and a story beat.** It's weighed
  once a broke spell, on the first day past `char.broke_days`, not every day it stays
  broke. Only a character at least `pirate.turn_min_risk` bold is ever tempted, and then
  with chance `pirate.turn_p` × its risk tolerance; everyone else goes down the lawful
  bottom rung (CHR-16). Many characters never turn. *(Fixed: the daily roll made nearly
  every broke commander who knew a pirate planet turn within a couple of weeks.)*
- *(proposed, tuning pass part 4)* **Piracy is terminal.** There is no way back (PIR-7);
  a pirate's story ends when it dies (PIR-16).
- *(proposed, tuning pass part 4)* **Its debts stay behind** when it renounces: written off
  (ECO-13), so it starts its pirate life at zero rather than deeper than the lawful
  bottom rung.

### PIR-5 Pay and loot `v1`

- **Pirate planets pay a lower salary** than lawful planets pay their commanders.
- **Loot is sold to the pirate planet's market,** the only place a pirate can sell, at its
  prices. **The planet's prices steer what pirates raid:** if food is short there, food
  sells high, and pirates go after food freighters.
- Raids take **cargo only**. *(later)* Stealing credits, which needs the cash merchants
  carry to be modelled.

### PIR-6 The money loop `v1`

1. Lawful populations buy contraband from smugglers, untaxed (PRD-6).
2. Smugglers sell to and buy from the pirate planet.
3. The pirate planet pays stipends and buys loot.

The pirate economy is funded by real flows, not from nowhere.

### PIR-7 No redemption for ships `v1`

A pirate ship must be destroyed. Its commander escapes in a pod (CHR-15) and rebuilds,
as a pirate or through normal channels. Old opinions of them fade toward neutral over
time (REL-3). *(later)* A pardon.

### PIR-8 Pirates raid; they don't invade `v1`

Pirates attack ships and never capture planets. *(proposed)* If too few commanders go
broke to keep a healthy pirate population, a procedural top-up of named pirates is the
fallback; the headless numbers decide whether it's needed.

## The pirate relation

### PIR-16 Raid until destroyed `v1` `proposed`

*(tuning pass part 4)* **A pirate's work is raiding.** A pirate takes a mission if one is
offered, sells what's in its hold, and otherwise goes out raiding on its own from the
pirate planet it's at, choosing targets as a pirate planet's dispatch does (lawful
planets within `pirate.range_days`, weighted by population). One away from the fringe,
landed by a pod say, heads back to the nearest pirate planet it knows first. A salaried
pirate is still sent out by its planet (PIR-3); a pirate the planet can't afford to keep
raids anyway.

- **The cycle is raid until destroyed, then again until dead.** Raiding draws patrols
  and stronger lawful forces, so pirates lose fights often. Each loss is the ordinary
  pod roll (CHR-15, `pod.death_p`), with no extra danger for pirates: they die young
  because they lose a lot, like bandits in Bannerlord.
- **The bottom rung for a pirate is a fighter.** A pirate with no ship and too little for
  one gets a free tier-1 scout, uninsured (CHR-16), so it can raid straight away. A
  pirate rebuilds and upgrades with gunships (SYS-8).
- **A pirate can still get rich.** Loot sold at the pirate market builds savings, savings
  buy a bigger fleet, and a bigger fleet wins more often and lasts longer.
- Pirates are a flow, not a stock: they turn, raid and die, and the population top-up
  (SYS-18) replaces them with new lawful characters.

*Why:* a pirate with no salary used to sit at its haven forever, in debt, with no work it
was allowed to take. By year 5 nearly a third of all characters were idle pirates
(CHK-17b, CHK-23).

### PIR-9 One pirate relation `v1`

The pirate faction keeps **one shared opinion of each character**: the exception to
standing being a root's opinion (REL-5). It does two jobs at once:

- **neutral or better, and pirates don't attack you;**
- **the higher it is, the more lawful inspections flag you** through suspicion (INF-14).
  *(proposed, roles round)* It adds to each exposure (INF-14).

Its resting point toward outsiders is **hostile**, and it **decays slowly** toward it, so a
former smuggler drifts back to being attacked by pirates and unsuspected by the law. One
pirate mission never buys permanent safety.

*Why:* protection from pirates always comes with suspicion, so there's no free pass.

### PIR-10 The player's way in `v1`

1. A pirate **hails the player** with a smuggling or cargo mission. Declining means that
   pirate attacks, as it would have anyway.
2. **Accepting sets the pirate relation to neutral,** so pirates don't attack.
3. **Failing the mission or running out of time** makes the player a pirate enemy again.
4. **Succeeding raises the pirate relation.**

### PIR-11 Full pirates `v1` `proposed`

A character becomes a **full pirate** by an unprovoked attack on a non-pirate ship (not
counting the enemies of their own faction in a war, DIP-6), or by taking a pirate
commander contract. Only full status carries no redemption (PIR-7) and refusal from the
Lattice (WLD-12). In v1, full status is known to everyone at once rather than spreading
as information.

### PIR-12 Smugglers are the disguised pirates `v1`

A character with a good pirate relation who has never become a full pirate is a
**smuggler**. They can use the Lattice; suspicion is their risk of being caught. This is
the "pirate disguised as a neutral merchant" of the drafts. *(proposed, roles round)*
A smuggler's risk is the same per-trip suspicion as a spy's (INF-14).

### PIR-13 NPC smugglers `v1`

Independent merchants with a high risk tolerance take smuggling and legal-goods runs
under the same mechanic: pirates offer runs on contact, with the same relation and the
same decay. The hail is just how the offer reaches the player.

*Why:* if only the player smuggles, the money loop runs on one person.

## Contraband in lawful space

### PIR-14 One list for lawful space `v1`

A single list of illegal goods applies to all lawful space, enforced by the Authority on
the Lattice and by planets at their markets, through suspicion (INF-14). Lawful planets
never want illegal goods for their own militaries.

### PIR-15 Policing is a governor's choice `v1`

Policing is a planetary expense, paid through the security force (MIL-5). It cuts
contraband inflow and costs the treasury. *(proposed)* What it buys is **inspection
intensity**, the suspicion system's dial (INF-14). *(proposed, roles round)* That dial
is the planet's detection stat. Policing is the governor's decision (CHR-4). A governor can keep the population
happy with legal luxuries, run a police state, or live with low productivity; governors
land in different places, so smuggling routes shift as governors change.

*See:* CHK-10.
