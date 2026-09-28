# Economy: design

*Written 2026-09-28 from a design session that followed `FACTIONS.md`, and revised the
same day with the human's answers to a review (see "Revised after review"). This covers where
wealth comes from, how goods are produced and moved, how the supply chain heals itself,
how fuel, munitions and ships fit in, and how contraband works. Recruitment and
relationships are still separate documents to come.*

*Items marked **(proposed)** came up in the session as suggestions and weren't
explicitly signed off. Items marked **(experimental)** are decided as worth trying, with
the verdict left to play. Everything else is decided. Numbers are tuning, not design,
and the money model in particular is expected to be adjusted if the simulation doesn't
bear it out.*

## Principles this serves

From `VISION.md` and `FACTIONS.md`:

- The simulation is authoritative. Goods exist on real ships, and markets move because
  real cargo arrives or doesn't.
- Wealth is the root of military power. Governors pay commanders, commanders buy
  ships, so the economy decides who can fight.
- Economic warfare is real warfare. Killing a planet's merchants has to hurt it through
  the value of the trade it loses, not through an artificial penalty.
- The player lives under the same rules as everyone else, and every playstyle works on
  its own. A merchant who never fires a shot must still be able to grow powerful.
- The player never has to memorise the supply chain. They see its effects: prices,
  shortages, and missions asking for specific goods from specific places.

## Money

### Where money comes from

**Population creates demand, and trade turns demand into money.**

- Every planet's population has purchasing power, scaled by its size, productivity and
  happiness. This is the only place new money enters the world.
- That purchasing power becomes real money only when goods are delivered and bought:
  food, oxygen, luxuries and the rest. A population with nothing to buy generates
  nothing.
- **The governor taxes transactions at the planet's market,** sales and purchases alike.
  A planet's income is its tax rate times the trade volume through its market.

This is what makes blockades work without special rules. A planet whose merchants are
killed loses three ways at once: its exports pile up and their price collapses, the
inputs its industry needs stop arriving, and the goods its population wants stop
arriving, so happiness and productivity fall. How badly it's hurt depends on its place
in the supply chain, which is the point.

Rejected alternatives:

- **Money from population alone** (a scale factor on population and happiness). A
  blockaded planet would keep printing income, so trade wouldn't matter.
- **Money from trade taxes alone.** Taxes only move money around; nothing would explain
  where it enters the world.

### Where money leaves: war

The economy needs sinks, or money piles up over a long run. Destroying goods isn't
enough on its own: the money that paid for them has already gone to a seller and keeps
circulating. So the economy has real money sinks, and they are based in **combat and
conquest**.

This sets up a cycle between peace and war:

- **In peace,** ships, weapons and munitions accumulate and get cheaper, which makes
  going to war more attractive.
- **War consumes** money, weapons and ships. Treasuries drain and military goods get
  expensive, until peace looks better.

How war removes money (rather than only goods) is still to be settled, for example the
cost of building and rearming ships and munitions leaving the economy rather than
passing to someone. The headless prototype checks the result: the money supply stays in
a band over long runs.

Rejected: **a closed money loop** (population purchasing power paid as wages from
production, so money only circulates). It looks elegant, but money risks bunching up in
one part of the economy, and most games avoid it, probably for that reason.

### Merchants

- **Most of a planet's wealth is generated through merchants.** Independent merchants
  balance goods across the map by following price. Planets operate at a much larger
  scale than any single merchant.
- **(proposed)** Planets may also make **trade deals** with each other for large
  standing flows. The supply chain already provides the incentive, so this is only
  worth adding if the simulation shows planets need it.
- **Market depth scales with population (proposed).** A big delivery swamps a small
  colony's market and barely moves a capital's. This is what lets player trade matter
  at the right scale.

### The merchant playstyle can grow

Hiring other ships is expensive. It's open to successful, wealthy merchants, to
commanders sworn to a governor, and occasionally to a very successful mercenary.
Recruitment covers how.


Bannerlord's caravan play caps early because a bigger caravan is a slower one, so the
best caravan is the smallest one that bandits leave alone. Lattice shouldn't have that
cap. A player who commands one or more large freighters can start to move markets
themselves, and that is intended. Size costs speed in open space, but the Lattice is
the fast network either way.

### Prices travel physically

There is no instant communication, and prices are information. The player's knowledge
of a market is what they've seen or been told, with a time on it: the same belief-state
model planets and characters use in `FACTIONS.md`. Trading becomes route knowledge
rather than a spreadsheet, and the Lattice matters twice over, because a ship's speed is
really two speeds depending on whether it's on the network.

## Population

A planet's population has a **size** and a **happiness**.

- **Size** sets how much it consumes (life support in particular) and how much labour
  its industry has.
- **Happiness** raises growth and productivity.
- More population means more production, more trade and more tax for the governor.
- **Life support** shortages cause decline, first unhappiness and then population loss.
  Never a sudden die-off. **(proposed)**
- **Luxury goods** raise happiness. Populations always want them.

## Production

- **Resources are static.** What a region can mine or grow is fixed at world generation
  and stays with the region, like culture. Who profits from it changes as planets change
  hands, which is what makes conquest's economic incentive legible.
- **Facilities are mutable.** Refineries, factories and shipyards are what a planet has
  built, and they can change (see Role switching).
- **Output is labour times facilities times inputs.** It goes into the planet's market
  and is sold to whoever comes to buy it.

### The shape of the supply chain (proposed)

The design is meant to create trading opportunities without becoming fragile.
Opportunity comes from **breadth and spatial separation**, not from chain length. Every
tier added multiplies fragility; every good added at an existing tier adds opportunity
without it.

- Chains are at most three tiers deep: **raw → refined → finished.**
- Wide rather than deep: many goods at each tier, and finished goods taking two or three
  inputs each.
- **Hard dependencies for life support, soft for production.** A population without
  oxygen declines. A refinery without an optional input runs slower rather than
  stopping.
- **Fuel cells** are the model soft dependency: an input to many refining processes
  that speeds them up. Without fuel cells a refinery runs slower on local planetary
  power. This creates demand across the map without making fuel cells a god resource
  or a single point of failure.
- **Substitution** (aluminum in place of steel, at higher cost) is used sparingly, only
  where it tells a story. Every alternate recipe adds evaluation work for the simulation
  and balance work for the designer.

## Goods

Examples from the session, to show the shape. The first-pass list is still to be
written (see Open questions).

### Life support

Food, water, oxygen and medical supplies. Every populated planet needs them.

**Oxygen** is shipped as solid oxygen tablets (a stable oxide of some kind; the chemistry
doesn't need to be real) because shipping gas is inefficient. Making tablets takes an
oxygen-rich planet, a rare catalyst mined elsewhere and, optionally, fuel cells. So a
near-universal need has a physical reason to be traded and a reason to be made in few
places.

### Luxury goods

Raise population happiness. Their own supply chains, like everything else.

### Industrial and military

Illustrative chains:

- **Laser guns:** aluminum and laser packs.
- **Railguns:** steel and electrodes.
- **Turrets:** steel, lubricant and magnets.
- **Ships:** titanium, turrets, guns, power cores, shield generators.

"Guns" at this scale is a **commodity**: a trade good that moves through the supply
chain and goes into building ships. See the two scales below.

### Fuel

- **Lattice fuel** is sold by planets and made through its own supply chain of raw
  materials and refining. The Trade Authority doesn't sell it.
- **Fuel producers come at every scale,** and there are a lot of them:
  - some planets produce none and have to import;
  - some produce a little and import to keep prices reasonable;
  - some are modest exporters;
  - a few are huge exporters, and high-value targets for capture.

  With that many producers, fuel is almost always available somewhere nearby. What
  varies is the price.
- **Fuel cells** are a separate product used in production (see above). They were
  called "land fuel" in the session.

### Contraband

See Contraband below.

## Two scales: commodities and the catalogue

Military goods exist at two scales, and only one of them is in the supply chain.

- **Commodity scale.** Guns, turrets, missiles and hulls in general are trade goods.
  They're produced, hauled and consumed, and they go into building ships.
- **Catalogue scale.** The specific equipment a player fits (a blue laser, a red laser,
  a super railgun) is too specific to be part of the supply chain. Markets stock it by
  **random chance, weighted by distance from the culture that makes it**, as
  `FACTIONS.md` describes. The worlds that make lots of guns carry the deepest gun
  catalogue.
- **(proposed)** The stocking chance could also be weighted by local supply of the
  parent commodity, so a blockaded gun world's catalogue thins out too. Add this only if
  catalogues feel static in play.

## Specialist worlds

Every culture has one or more planets that are expert at making **ships**, one or more
expert at **guns**, and one or more expert at **missiles**, possibly more. These are
where that culture's hardware comes from, and they're prizes in war.

## Munitions

Munitions are limited and have their own supply chain.

- **War consumes munitions.** Demand spikes at the front, prices rise there, and
  merchants run munitions toward the fighting. A front line can be visibly well supplied
  or starved, and supplying one is a merchant playstyle of its own.
- **Player restock depends on stock.** A planet near the front may be out of missiles.
- The player's missiles are finite, held in a magazine, as they always were meant to be.
  The prototype's unlimited missiles are a testing convenience.

## Ships

Ships are part of the supply chain. Shipyards consume commodities (titanium, turrets,
guns, power cores, shield generators) to build hulls, and commanders buy hulls with the
money their governors pay them. That closes the loop from `FACTIONS.md`: production →
trade → tax → governor → commander pay → ships.

### Capacity costs weight; contents don't

- **Fuel tank size and missile magazine size are fitting choices,** made when the ship
  is outfitted, and cost weight like any other fitting. A ship built for long runs on
  the Lattice gives up something else, such as big guns.
- **Filling them costs nothing.** A full tank weighs the same as an empty one, and
  fuel and munitions never compete with cargo space. Buying a missile must never mean
  dropping cargo.

## Redundancy and role switching

### Redundancy comes from geography

At world generation, every important good has **at least two or three producers per
culture region** (proposed numbers). The guarantee is per region, not per faction,
because factions move and regions don't. A faction can still lose all its steel through
war. That's intended: it's the economic cost of losing territory.

### Recovery comes from price: role switching

When a good stays scarce and expensive long enough, a planet with a suitable resource
**converts** to producing it. Role switching does two jobs:

1. **Recovery.** A faction that loses its steel worlds gradually gets steel again.
2. **Self-correction.** World generation won't place exactly the right number of
   producers in every region. Prices move goods around, but in a complex supply chain
   that isn't enough on its own. If a region was generated with too few gun producers,
   gun prices climb until switching to guns becomes attractive, and the economy heals
   the generator's mistake. World generation only has to be roughly right.

Rules:

- **Both directions.** Planets leave overproduced roles as well as enter scarce ones,
  or gluts never clear.
- **Driven by price, not scripted.** The same signal merchants follow.
- **Native producers stay better.** A converted planet runs below one producing from
  its natural resource, so original producers stay valuable and the map doesn't
  homogenise.
- **Damped, so it doesn't oscillate.** If gun prices spike and five planets switch at
  once, the result is a glut and a switch back (the cobweb cycle), made worse because
  planets act on stale prices. Candidate dampers **(proposed)**:
  - the price signal must hold for a while before a planet commits;
  - switching is slow and costs something;
  - a conversion in progress counts against the signal other planets see.
- **Speed:** start with a placeholder of a few in-game months and tune from the
  simulation.

## Gates serve places

The Lattice will be re-engineered a little so that **each exit and entrance is dedicated
to one place**: a planet, a station, a moon or another point of interest. It sits either
right beside that place or a short hop away using the local travel mechanic (hop lanes).
The exit's speed boost (`FACTIONS.md`) is aimed at the place it serves.

This answers who holds a gate: **the place it serves.** So it's clear which planet pays
the gate-holding tier of dues below.

## Trade Authority dues

Two tiers, replacing the flat monthly due in `FACTIONS.md`:

- **Network access:** a planet pays for its own ships to travel the Lattice.
- **Gate holding:** a planet with a Lattice entrance also pays for the right to control
  entry and exit there.

Being on the highway brings large trade advantages and has a real cost. **It's binary:**
a planet that holds gates pays the higher tier or becomes a pirate planet. There is no
lawful middle state of an unpaid, closed interchange, because the player couldn't tell a
poor planet's closed exit from a pirate one.

The Authority's income stays close to steady; it only drops when planets go pirate.

**Fringe worlds** off the network are at a disadvantage. It's softened by giving them
natural resources in high demand, legal or black market, or by making them pirate
systems.

## Contraband

### One list for lawful space

There is a single list of illegal goods for all lawful space. It's enforced by the
Trade Authority on the Lattice and by planets at their markets, using the suspicion
system from `FACTIONS.md` (shared by spies, smugglers and disguised pirates).

A lawful planet never has a demand for illegal goods for its own military; if it
wanted them, they wouldn't be illegal. Contraband comes in two kinds:

### Military contraband is pirate-internal

Pirates produce illegal weapons and use them on their own ships. Lawful markets don't
want them.

- Contraband can be raw or refined. **Plutonium** is a raw resource used to make
  illegal weapons, so the resource itself is illegal.
- In the simulation, military contraband flowing into pirate production gives pirate
  fleets a chance of **Mk variants**: the same ship with a more dangerous loadout, as in
  Escape Velocity (Mk 1, 2, 3; missile variants carrying more missiles). This gives
  ships variety without rules for how NPCs fit their own ships, and it's readable to
  the player: "that's a Mk 4, stay away."
- For the player, illegal weapons are the pirate playstyle's gear reward, reached
  through pirate trade and standing.

### Luxury contraband is the smuggling economy

Lawful populations want illegal luxuries because they raise happiness far more than
legal ones. Governors ban them. That gap between demand and legality is why the black
market exists.

- Legal goods can be combined into illegal ones at pirate facilities. For example,
  oxide catalyst and medical supplies are both legal, but a pirate planet can combine
  them into a drug that makes populations much happier. The right merchant or mission
  pays a lot for it.
- The pirate economy is stronger than the legal one because pirates have facilities
  lawful space doesn't, and their trade is untaxed.

### The downside for lawful planets

Contraband raises happiness but **lowers productivity by more than that happiness would
have raised it.**

- **The population wants it,** because it cares only about the most happiness for the
  least money, and contraband is the cheapest happiness there is.
- **The governor hates it,** because the reason to have a happy population is
  productivity, and contraband buys happiness at the cost of productivity.
- **An unhappy population pays more for contraband** (demand rises as happiness falls).

So a governor has three choices: keep the population happy with legal luxury goods, run
a police state to keep contraband out, or live with low productivity and the lower
income that comes with it.

On top of that, money spent on contraband isn't spent on legal goods, so it never passes
through the taxed market, and it leaves lawful space for the pirates who made it.
Contraband shrinks the tax base and funds the enemy.

**(proposed)** Policing is a governor's choice. Enforcement spending buys inspection
intensity (the same dial as the suspicion system's inspection intensity), which cuts
contraband inflow and costs the treasury. Governors weigh a happy population against a
healthy tax base and the cost of policing, and land in different places, so smuggling
routes shift as governors change.

**Tuning target:** contraband is net positive for a population and net negative for its
governor. That tension keeps smugglers in business.

## Crew: academy worlds (experimental)

Some planets have an **academy**, a facility that trains crew of that planet's culture
at a quality set by the academy. The player goes to an academy world to hire good
gunners. This replaces the idea of pilots as a supply chain with transported people.
It's in the design to be tried; play will decide whether it stays.

## What a headless prototype should check

- **The merchant-kill test.** Run a deep trade network with an indestructible pirate
  faction that kills one planet's merchants. The planet's wealth should fall, with the
  size of the effect depending on its place in the supply chain, and the damage should
  spread to its faction without collapsing the map.
- **Bad seeds heal.** Generate deliberately broken worlds (too few gun producers, a
  region with no fuel refinery). Role switching should converge to a healthy economy
  without oscillating.
- **The money supply stays in a band.** No runaway inflation or deflation over long
  runs.
- **Front lines are supplied.** Munitions flow toward fighting, and blockaded fronts run
  short.
- **Contraband tension holds.** Some governors police hard and some don't, and
  smuggling stays profitable without taking over.
- **Fringe worlds survive.** Off-network planets aren't simply poorer forever.

## Decided in this session that affects other documents

- **Human highway technology is the same across all human factions and cultures.** The
  idea of faction- or culture-specific highway tech is dropped.
- **Alien factions.** Each is a unique species with some unique technology and none or
  some human technology. They don't follow the feudal system: they act like a hive mind
  with a fixed hierarchy, can't split, and don't have competing internal priorities.
  They can ally with factions and with the player. They exist mainly to add danger and
  playstyle variety for advanced players. Their details wait until human factions,
  cultures and the economy are done.
- **Hop lanes are open to any ship, pirates included.** They exist so a slow freighter
  isn't punished for visiting planets far from a system's exit. Combat abuse is
  possible and not a concern for now.
- **The player starts in a small ship with no Lattice Drive.** The prototype's ship
  switching exists only to test the highway.
- **Trade Authority dues become two tiers** (see above), replacing the flat due.

## Superseded in this session

Recorded so they aren't reintroduced:

- **Fuel and munitions competing with cargo space.** Rejected: it makes buying
  consumables feel bad. Capacity costs weight; contents don't.
- **Dependence as the downside of luxury contraband.** Rejected as not sensible.
- **Lost tax base as contraband's only downside.** Too weak: the happiness contraband
  brings would raise productivity and win back the tax. Contraband now lowers
  productivity by more than its happiness raises it.
- **A closed money loop.** Rejected: money risks bunching up in one part of the economy.
  Sinks are based in combat and conquest instead.
- **Goods destruction counted as a money sink.** Destroying goods doesn't remove the
  money that paid for them.
- **Dropping gate dues without going pirate.** Rejected: it creates a closed exit the
  player can't explain.
- **Separate legality lists for the Authority and each faction.** Replaced by one list
  for lawful space.
- **Catalogue availability through the supply chain.** Specific equipment stays random
  chance weighted by culture distance; only commodities are in the supply chain.
- **A supply chain for pilots.** Replaced by academy worlds.

## Open questions

- **The first-pass goods list:** life support, luxury, raw, refined, military
  commodities, fuel and contraband, with which resources and planet types produce
  each. A rough target is about 4 life support, 3 luxury, 8 raw, 6 refined, a handful
  of military commodities, and a few contraband goods, to be adjusted by the
  simulation.
- Whether planets make trade deals, or independent merchants are enough.
- Whether lawful ships also have Mk tiers through legal quality upgrades, and if so how
  they stay visually distinct from pirate variants.
- How weight affects a ship: speed, acceleration, turn rate, or a fitting budget.
- Role-switching speed and which dampers are needed.
- How much of a planet's trade runs on its own merchant fleet versus independent
  merchants.
- Tier amounts for Authority dues, and how they compare with the trade advantage of
  holding a gate.
- How exactly war removes money from the economy (see Where money leaves).
- **World scale.** The target is hundreds of sectors and upwards of 100 habitable
  planets, which the economic and political systems need. The prototype's 4×4 map is
  far too small to hold one culture region's supply chain.
- Alien economies, deferred with the rest of alien design.

## Revised after review

The human answered a review on 2026-09-28:

- **Money sinks** are based in combat and conquest, giving a peace/war cycle. A closed
  money loop was rejected.
- **Contraband** lowers productivity by more than its happiness raises it, so it's
  wanted by populations and hated by governors.
- **Fuel** has many producers at every scale, from importers to huge exporters.
- **Finite missiles** were always the plan; the prototype's are unlimited for testing.
- **Hiring ships** is expensive and open to wealthy merchants, sworn commanders and
  occasionally mercenaries.
- **World scale** target: hundreds of sectors, over 100 habitable planets.
- **Gates serve places:** each exit and entrance is dedicated to a planet, station, moon
  or point of interest, which holds it.

## For the implementation agent

Not to be acted on while this document is in `docs/planning/`. When it's promoted:

- Add it as `docs/ECONOMY.md`, and add a row for it to `docs/planning/README.md` now.
- Update `FACTIONS.md`: two-tier dues, the alien stance, uniform human highway tech,
  and a pointer here for fuel pricing and contraband.
- Add fuel, tank and magazine capacity, and the no-cargo-competition rule to
  `WORLD_AND_TRAVEL.md` alongside the travel rules moving from `FACTIONS.md`.
- Fold a summary into `BRIEF.md` and update its open questions.
- This is design only. Don't build it yet.
