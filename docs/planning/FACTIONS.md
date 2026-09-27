# Factions and allegiance: design

*Written 2026-09-27 from a design session. This covers how political power is
organised in the simulation: who holds planets, who holds fleets, how factions form
and split, how cultures give regions their own hardware, how the Lattice is
governed, and how information and armies move. It
feeds roadmap step 2 (simulation architecture). The economy (resources, supply
chains, fuel pricing) is a separate document still to be written.*

*Items marked **(proposed)** came up in the session as suggestions and weren't
explicitly signed off. Everything else is decided. Numbers are tuning, not design.*

## Principles this serves

From `VISION.md`, restated because they govern everything below:

- The simulation is authoritative. Factions, characters and fleets act whether or not
  the player is near them.
- The player lives under the same rules as everyone else. No special information, no
  special access.
- Pressure is chosen. Danger is readable before the player commits to it.
- Difficulty is geography. Safe space is safe for reasons the world can explain.

## The Lattice is a given

The Lattice exists and nobody questions why. It's built infrastructure (roadway,
lamps, docking, the trucker's-highway feel), not a natural wormhole. Every planet
wants to be on it, because being cut off from interstellar travel and commerce is
ruinous. It sustains itself on that incentive.

There is no combat on the Lattice or on hop lanes.

Other (alien) factions don't necessarily use the Lattice. Their default transport is
larger-scale hop lanes: a variant of an existing primitive, not a new travel system.
One alien faction may be connected to the Lattice through an old alliance with
humans. Alien factions are otherwise out of scope for this document.

## Travel rules this design depends on

These came out of the same session. They belong in `WORLD_AND_TRAVEL.md` and should
be moved there.

- **Fuel.** Ships burn fuel on the Lattice. The only places to refuel are planets and
  space stations. Fuel is what forces ships off the safe network and into open space.
  Fuel prices will vary as part of the economy (deferred).
- **Running out of fuel** drops the ship out of the Lattice into open space at the
  mapped position. **(proposed)** A small reserve lets the ship limp rather than sit
  dead.
- **Bail out anywhere.** A ship can leave the Lattice into open space at any point, at
  its mapped position, with its current heading and speed.
- **Ramps give a boost.** Taking an exit ramp slingshots the ship toward the
  destination the ramp was built to serve (a planet or station). The direction is
  fixed by the ramp, not chosen by the player: builders pointed their infrastructure
  where traffic should go. Bailing out gets no boost. Knowing where a mid-route bail
  lands you near something useful is earned map knowledge.
- **Destinations nobody built a ramp for** (new outposts, pirate havens) are reached
  by bailing out or flying.

## Who holds power

### Planets and governors

Every planet is its own state with its own autonomy, run by a **governor**
(a politician). A planet has an economy, its own defences, and the commanders who
have sworn to it.

**Planetary defences** (missiles, torpedoes and the like) mean it takes a
sufficiently large force to take a planet. A typical planet can defend itself but
can't field enough force to take a defended neighbour alone.

### Commanders

**Commanders** are military characters who own fleets. They gain rank by serving a
more powerful lord and by growing their force. A commander has three possible
careers:

- **Sworn:** in fealty to a governor or a higher lord.
- **Mercenary:** no liege, working for money (see below).
- **Pirate:** renounced everyone (see below).

### Fealty and factions

Power is feudal, with governors and commanders in one structure:

- A governor gains power by getting the fealty of other governors (and their
  planets) or of commanders (and their fleets).
- Every character has at most one liege. The structure is a forest of fealty trees.
- **A faction is the root of a tree and everything under it.** It isn't stored as a
  separate thing. When a vassal breaks fealty and takes its own vassals with it, a new
  faction exists.
- The game **starts** with three seats of power: three powerful planets, far apart,
  each run by a politician at the root of a large tree. That is an initial condition,
  not an invariant. Factions can split, merge and new ones can form.

What fealty gives a vassal: when attacked, a planet can ask its faction for help, and
the faction can send forces along the Lattice.

### Where military power comes from

Force comes from commanders, and commanders go to whoever can pay and offer prestige.
So **wealth is the root of military power**:

- A typical planet can't support enough commanders to conquer a neighbour. A very
  rich or very lucky one can, and that's one way a new faction starts, or an existing
  one gets stronger.
- Starving a planet's trade starves its ability to keep commanders. Economic warfare
  is real warfare.
- A rich rival can lure commanders away from their lord.

### Conquest

Taking a planet is **lucrative**: the conquered planet swears fealty to the
conqueror, adding its wealth and gates to their tree. That's the incentive behind all
conquest. In practice, only lords who already have vassals can muster a force large
enough to take a planet.

### Holding the tree together (proposed)

Without opposing forces the system either fragments or consolidates into one empire.
Candidate forces:

- **Together:** the liege protects and pays; breaking fealty costs reputation and
  invites punishment.
- **Apart:** ambition, grievance, a liege who fails to send help, a better offer from
  a rival.
- **Fracture pressure grows with size,** and with distance from the liege measured in
  *network* distance. Vassals near their liege on the Lattice stay loyal; off-network
  frontier worlds drift.
- **Culture** (decided, see Cultures): planets are more loyal when more of their own
  culture shares the faction.

## Cultures

**Cultures** are separate from factions. A faction is political and its territory
moves. A culture is regional and **static**: it stays where it is whoever holds the
planets.

- At the start of the game, cultures and factions tend to line up, but not exactly.
  A large faction may contain several cultures.
- **Culture decides what ships and weapons are available** at a planet. Each culture
  has its own flavour of hardware.
- **Availability spreads by distance.** Any market has a chance of stocking another
  culture's ships and weapons, falling off with distance from that culture's region,
  out to a maximum of about two or three sectors (hexes) away. Sector distance follows
  the Lattice closely enough in most cases to be a fine approximation. Planets near a
  cultural boundary carry hardware from both sides.
- **Availability is geographic, not political.** Owning a planet doesn't spread its
  culture's gear to the rest of your faction's markets.
- **Conquest crosses cultures.** A faction that takes planets from another culture
  gains access to that culture's ships and weapons through those planets' markets.
  This is a second conquest incentive alongside wealth.
- **Every character has a culture:** governors, commanders and the player. One system
  for everyone.
- **Culture eases recruitment.** Planets acquire commanders of their own culture more
  easily, and commanders recruit combat ships of their own culture more easily. The
  player is subject to the same effects. Recruitment itself is a separate document.
- **Culture carries loyalty.** A planet is less likely to defect when more planets of
  its own culture are in the same faction.

Consequences:

- **Diversity trades gear for cohesion.** A multicultural faction has the broadest
  arsenal and the weakest loyalty. A monocultural one is cohesive but limited to one
  culture's hardware. Expanding across a cultural border is a real decision, and it
  works as an anti-snowball force alongside size.
- **Faction lines drift away from culture lines over time,** and those mismatches are
  where defections are most likely.
- **The player's catalogue has two layers:** culture decides what exists at a planet,
  and standing decides whether it's sold to you.
- **A faction's fleets take on its cultural makeup.** Commanders and ships come most
  easily from the cultures a faction holds, so a faction that conquers across a border
  gradually fields a mixed fleet.
- **(proposed)** A commander's culture affects loyalty the same way a planet's does.
- **(proposed)** Lattice infrastructure style (junction gates versus continuous
  interchanges, ramp strength) belongs to culture rather than faction, since culture
  is what stays tied to a region. This replaces the earlier idea of
  faction-specific highway technology.

## The Trade Authority

A neutral faction that runs the Lattice.

- It holds **no planets.**
- It collects a **flat monthly due from every planet** for use of the network.
- It spends that income on **maintaining the network,** and uses the surplus to **hire
  mercenaries** to hunt specific pirates and keep stretches of the network clear of
  pirate influence.
- It takes no side in wars between factions. It cares about dues and pirates.

Because dues are flat per planet, the Authority's income only drops when planets go
pirate. It's close to static: the one fixed point in a moving world.

## Mercenaries

Mercenaries work like Bannerlord's: commanders with no liege and no territory, leading
smaller fleets, less prestigious than sworn commanders, working for money.

- **Anyone can hire them:** the Trade Authority, planets, factions.
- **Contracts are specific tasks:** hunt this pirate, keep this stretch clear, guard
  this depot.
- Mercenary work is the Authority's visible presence in the world: its enforcement is
  the mercenary fleets you see.

## Pirates

A pirate is anyone (a character, a fleet or a planet) that has **renounced allegiance
to both the Trade Authority and every faction.** Pirates are enemies of everyone.

- **Pirates are barred from the Lattice.** The only way on is the spy mechanic:
  freighters posing as neutral traders.
- **Pirates operate a black market** that pays better than the legal one, if you
  aren't caught.
- **Pirate planets near the Lattice don't last.** They're lucrative targets with no
  diplomatic cost to attacking them, and no ally can send reinforcements along the
  Lattice to defend them. Factions and planets take them.
- **Pirate planets far from the Lattice survive,** because they're not worth the trip.
  The only self-sustaining piracy is on the fringe. The danger gradient emerges from
  this rather than being authored.
- **(proposed)** Pirate raiding range is bounded by fuel. Pirates can't refuel on the
  network except through the black market or disguise, so pirate pressure fades with
  distance from their havens.

## Gates: two keys

Entry and exit at every gate is controlled by **both** the planet that holds it and
the Trade Authority. A ship passes only if both allow it.

- **The planet** decides who may exit into (and enter from) its space, based on its
  stance toward the traveller's faction.
- **The Authority** decides who may be on the network at all. Known pirates are
  refused at every on-ramp. **(proposed)** A planet that stops paying dues has its
  gates shut, using the same key.

Consequences:

- **Enemy forces can't exit in hostile territory.** They exit at the last gate that
  will let them out and cross the rest in open space. That crossing is slow and
  visible, which gives defenders a home-field advantage.
- **Fronts advance gate by gate.** Taking a planet flips its gates and gives the
  attacker a new exit one step deeper.
- **A pirate-held interchange is dead in both directions:** pirates can't board, and
  nobody else can exit there.
- **(proposed)** Neutral planets decide whether to let a passing army out, possibly
  for a price. Passage rights are a diplomatic bargaining chip.
- A refused player always has a legible reason: the planet doesn't want them, or the
  Authority doesn't.

## Information travels physically

There is no instant communication. News moves at the speed of the ships carrying it.
This applies to the player exactly as it does to NPCs.

### Scouts

A planet sends out **scouts** to find enemy fleets and bring the information home. A
scout is a vessel of its planet, and so of its faction, and is treated as an enemy by
anyone at war with that faction. Scouting is open-space work.

### Couriers

When a planet learns of a threat, it sends **messengers** along the Lattice to other
planets in its faction, asking for help. The faction sends forces back along the
Lattice. Because the Lattice is much faster than open space, a well-connected planet
can get defenders in place before an invasion arrives. A planet with slow scouts or
poor information may not.

That timing race is the core tension of defence: the invader's open-space crawl from
their last usable exit, against the defender's detection, scout return, courier run
and reinforcement travel.

### Spies

Planets employ **agents who pose as neutral traders.** Neutral standing lets them ride
the Lattice deep into enemy territory, which is their advantage over scouts. If
they're discovered, they're killed or captured.

Scouts catch armies in transit at the border. Spies catch them forming at home.

**(proposed)** Detection mechanism, shared by spies, smugglers and disguised pirates:

- A suspicion meter per agent, per faction (and for the Authority).
- Raised by behaviour: lingering near fleets or musters, visiting military systems
  without a trade reason, cargo that doesn't fit the route, repeat visits.
- Lowered by genuine trading. The cover has to be real, so good spies are good
  traders.
- Inspection and counter-intelligence intensity scales with how paranoid or at war a
  planet is.
- The player can see their own meter, so the risk is legible and they can back off.

### Belief state (proposed)

Each planet and named character works from what it knows, not the truth: a set of
sightings, each with a position, a time and what was seen. Decisions are made on
possibly stale beliefs. A sighting happens when something comes within sensor range
of an observer (the existing 4 km rule is the detection primitive); a report arrives
when a courier or scout delivers it.

The player's map uses the same model: it shows what they've seen or been told.

A market for information (buying and selling intelligence) is **deferred.** For now,
planets want information and hand out scouting missions to get it. Revisit if the
world doesn't feel dynamic enough.

## Armies: intent and action

Forces gather into **armies** before a major operation, as in Bannerlord. Two layers
govern what an army does:

- **Intent** is the strategic objective, such as "invade planet X." It is set when the
  army forms, it's telegraphed (a scout who sees the army learns its intent), and it's
  hard to overturn. Because intent is sticky, information stays good by the time a
  scout gets it home.
- **Action** is what the army is doing right now in service of its intent: moving,
  fleeing a larger force, refuelling, attacking a vulnerable target along the way.
  Actions change freely. Intent doesn't.

Consequences:

- Information decays at two rates. An army's position goes stale quickly, but its
  intent stays valid for a long time.
- The muster is visible before the army launches: the earliest warning, available to
  whoever can see deep into enemy territory.
- An army's choice of exit gate is itself a strong clue to its target.
- **(proposed)** Aborting (turning back after losing a commander, or seeing an
  overwhelming defence) is cheap. Retargeting costs something real: time, cohesion,
  the commander's reputation.
- **(proposed)** Opportunistic actions get a detour budget: an army pursues a side
  target only if the delay is small relative to its intent. Otherwise armies wander.
- The player can shape an army's actions without changing its intent: harassing it,
  raiding its fuel stop, or baiting a detour to buy time for the defender.

## Prisoners

Any named character, including agents and the player, can be taken prisoner, with a
small chance of death. Prisoners can be ransomed, exchanged or escape.

- A captured lord leaves vassals leaderless, which is an opening for defection.
- For the player, capture is harsh but recoverable (lost time, cargo, possibly the
  ship). It's the main answer to "what happens when you lose," and it keeps losing as
  progress rather than a reload.

## The player in this structure

The player follows every rule above. They **start as a neutral trader.** From there
the structure offers roles rather than classes:

- **Trader:** neutral hauling, which is also cover.
- **Spy or smuggler:** trading while gathering information or moving black-market goods.
- **Scout:** open-space intelligence missions for planets.
- **Courier:** carrying dispatches on the Lattice.
- **Mercenary:** contracts for the Authority, planets or factions.
- **Sworn commander:** fealty to a governor, fighting for a faction.
- **Pirate:** renouncing everyone.

## Simulation layers (proposed)

1. **Strategic:** intent, fealty, diplomacy, conquest decisions. Changes rarely. Fully
   headless.
2. **Operational:** army and fleet actions on the travel graph (Lattice roads with
   gate permissions as an edge filter, hop lanes, open-space sectors). Headless.
3. **Tactical:** combat, rendered only near the player.

The existing sensor-range handoff is the boundary between layers 2 and 3. The
simulation hands a fleet to the rendered world mid-action and takes it back
afterwards. Intent never leaves the simulation.

## What the headless prototype should check (proposed)

For roadmap step 3, run the world with nothing rendered, many times faster than real
time, and measure:

- The number of independent factions stays in a healthy band: never collapsing to one
  and never fragmenting into dust.
- Pirate planets near the Lattice get retaken in reasonable time; fringe pirate havens
  persist without growing indefinitely. If pirates take and hold a system on a major
  route, conquest incentives are mistuned.
- The Trade Authority's income stays close to steady.
- Defence works sometimes and fails sometimes: invasions against well-connected
  planets usually meet reinforcements, and invasions against poorly informed ones
  sometimes don't.
- Culturally mixed factions fracture more often than monocultural ones, but not so
  often that crossing a cultural border is never worth it.
- Planetary defence numbers are a first-class lever. Too low and everyone conquers
  everyone; too high and the map freezes.

## Superseded in this session

Recorded so they aren't reintroduced:

- **The Lattice as a natural wormhole.** Rejected: it breaks the built, trucker's-
  highway feel.
- **A committee of the three powers running the Lattice, with proxy wars and cores
  that never fall.** Replaced by emergent fealty trees and the neutral Authority.
- **A Trade Authority with planets and its own large army.** Replaced by an Authority
  with no planets whose enforcement is hired mercenaries.
- **Three fixed factions.** Replaced by three initial seats of power in an emergent
  structure.
- **Faction-specific ships, weapons and highway tech.** Flavour now comes from
  cultures, which are static and regional.

## Open questions

- Can the player rise to be the root of a faction, or do they stay below that tier?
- The economy: resources, supply chains, what planets produce and need, fuel pricing.
  Planets' wealth drives everything above, so this is the next document.
- The information exchange: whether intelligence becomes a traded good.
- Whether direct war between the largest factions needs any special treatment, or just
  happens when the simulation produces it.
- How alien factions and their hop-lane networks interact with the fealty structure.
  Alien races are presumably cultures of their own.
- **World scale and content, which have to be settled together:**
  - How many cultures, and how large their regions are.
  - How many systems in total, and how many habitable planets.
  - How many human seats of power the game starts with.
  - How many alien civilisations, and whether they use the feudal system. Probably
    not.
  - How much ship and weapon variety is realistic to build, and what gives each
    culture an identity the player gets excited about (for example, a culture known
    for its armour). This is a creative question that needs its own discussion.
- Recruitment: how planets acquire commanders and how commanders recruit ships.
  Separate document.

## For the implementation agent

- Add this as `docs/FACTIONS.md`.
- Move the fuel, bail-out and ramp-boost rules into `WORLD_AND_TRAVEL.md`.
- Fold a summary into `BRIEF.md`, and update its open questions (several simulation
  questions are now answered here).
- Point roadmap step 2 at this document.
- Recruitment and the economy will be their own documents. Leave hooks, not designs.
- This is design only. Don't build it yet.
