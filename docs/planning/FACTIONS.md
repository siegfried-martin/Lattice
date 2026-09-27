# Factions and allegiance: design

*Written 2026-09-27 from a design session, and revised the same day with the human's
corrections to the first write-up (see "Revised after review" at the end). This covers
how political power is organised in the simulation: who holds planets, who holds
fleets, how factions form and split, how cultures give regions their own hardware, how
the Lattice is governed, and how information and armies move. The economy (resources,
supply chains, fuel pricing), recruitment and relationships are separate documents
still to be written.*

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
be moved there when this document is promoted.

- **The Lattice Drive.** Entering the Lattice takes a ship fitted with a **Lattice
  Drive**, the engine that burns Lattice fuel. (Earlier documents call it a
  "threader"; that name was a speech-to-text error.) The player starts without one.
- **Fuel.** Ships burn fuel on the Lattice. The only places to refuel are planets and
  space stations. Fuel is what forces ships off the safe network and into open space.
  Fuel prices will vary as part of the economy (deferred).
- **Running out of fuel** drops the ship out of the Lattice into open space at the
  mapped position. **(proposed)** A small reserve lets the ship limp rather than sit
  dead.
- **Ramps give a boost.** Leaving by an exit ramp sends the ship out through the
  gate's frames with a speed boost aimed at the destination the ramp was built to
  serve (a planet, station or other point of interest). The direction is fixed by the
  ramp, not chosen by the player: builders pointed their infrastructure where traffic
  should go. This is what makes exits the normal way off the network, and why NPCs
  use them.
- **Bail out anywhere (the player).** A ship can leave the Lattice into open space at
  any point, at its mapped position, with its current heading and speed, and no
  boost. It's the player's alternative to an exit, and knowing when it pays off is
  earned map knowledge. For example:
  - a station with no exit of its own is reached faster by bailing at the right point
    on the road than by taking the nearest exit;
  - a smuggler who knows the next exit has security scans and guards bails out before
    it and sneaks round.
- **NPCs never bail out**, except when they run out of fuel. They travel exit to exit.
- **Destinations nobody built a ramp for** (new outposts, pirate havens) are reached
  by bailing out or flying.

## Who holds power

### Planets and governors

Every planet is its own state with its own autonomy, run by a **governor** (a
politician). A planet has an economy, its own defences, and the commanders who have
sworn fealty to its governor.

**Planetary defences** (missiles, torpedoes and the like) mean it takes a
sufficiently large force to take a planet. A typical planet can defend itself but
can't field enough force to take a defended neighbour alone.

There is also a **standing set of politicians with no planet**. They are candidates
when a planet needs a new governor (see Conquest).

### Factions

**A faction is a hierarchy of governors.** A governor gains power by getting other
governors, and so their planets, to swear fealty to them.

- Every governor has at most one liege. The structure is a forest of fealty trees.
- **A faction is the root governor and everything under them.** It isn't stored as a
  separate thing. When a vassal governor breaks fealty and takes their own vassals
  with them, a new faction exists.
- Standing with a faction means standing with its root.
- The game **starts** with three seats of power: three powerful planets, far apart,
  each run by a politician at the root of a large tree. That is an initial condition,
  not an invariant. Factions can split, merge and new ones can form.

What fealty gives a vassal: when attacked, a planet can ask its faction for help, and
the faction can send forces along the Lattice.

### Commanders

**Commanders** are military characters who own fleets. They are not governors and
aren't nodes of the faction tree themselves; they attach to it by swearing fealty to a
governor.

- **Sworn:** in fealty to a governor. The governor pays them well, so every commander
  has a large incentive to swear.
- **Mercenary:** a commander with no fealty to a governor. To pay for ships they take
  contracts, from the Trade Authority or from governors, or turn to piracy.
- **Pirate:** renounced everyone (see Pirates).

Commanders want to grow in power and get more ships, and that growth is bounded by how
much money they can make. A commander's power is their standing fleet plus their
ability to get more ships. That ability comes from recruitment and relationships,
which are separate documents.

### Stewards: governors who take the field

A governor is the most powerful character in the structure, but on their own they don't
*do* anything. They decide and others act. So a governor can **appoint a steward** to run
the planet and **raise a fleet of their own**, leading it in person like a commander.

- Not every governor does this. The more skilled tacticians do, which fits a commander
  who rose to become a governor.
- It's what makes governor a role worth playing: the player, once installed on a
  planet, doesn't have to stop flying.
- A governor in the field is still the planet's governor. Their fealty, their vassals
  and their commanders are unchanged.

Open questions:

- What the steward controls while the governor is away (economy, defences, recruiting,
  calling for help), and what still waits for the governor.
- Whether stewards are drawn from the standing politicians, and whether a steward's
  competence and loyalty matter. A disloyal steward with a planet and an absent governor
  is an obvious opening for betrayal.
- What happens to the planet if the governor is captured or killed in the field
  (succession ties into the relationship and loyalty document).

### What governors want

Governors want military power: to defend themselves, to gain standing with their
liege, and to be able to launch invasions. So they recruit the most powerful
commanders they can.

### Where military power comes from

Force comes from commanders, and commanders go to whoever can pay. So **wealth is the
root of military power**:

- A typical planet can't support enough commanders to conquer a neighbour. A very
  rich or very lucky one can, and that's one way a new faction starts, or an existing
  one gets stronger.
- Starving a planet's trade starves its ability to keep commanders. Economic warfare
  is real warfare.
- A rich rival can lure commanders away from their governor.

### Invasions and conquest

An **invasion force** is commissioned by a governor. It is made of:

- the fleets of the commanders sworn to that governor, and
- the fleets of the planets whose governors are sworn to them.

When the force takes a planet, **the commissioning governor installs a new governor**,
who starts with fealty to them. The new governor can be one of their commanders or one
of the standing politicians with no planet, so **commanders can become politicians**:
this is how a military character, the player included, rises to hold a planet. Who
gets chosen is decided by a relationship and loyalty mechanic (separate document).

Taking a planet is **lucrative**: the conquered planet joins the conqueror's tree,
adding its wealth and gates. That's the incentive behind all conquest. In practice,
only governors who already have vassals can muster a force large enough.

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

A **limited faction with a fixed ruleset** that runs the Lattice. It isn't a fealty
tree and it doesn't play politics.

- It holds **no planets.**
- It collects a **flat monthly due from every planet** for use of the network.
- It spends that income on **maintaining the network,** and uses the surplus to **hire
  mercenaries** to hunt specific pirates and keep stretches of the network clear of
  pirate influence. Those contracts are also a main source of **player missions**.
- It takes no side in wars between factions. It cares about dues and pirates.

Because dues are flat per planet, the Authority's income only drops when planets go
pirate. It's close to static: the one fixed point in a moving world.

## Mercenaries

Mercenaries work like Bannerlord's: commanders with no fealty and no territory,
leading smaller fleets, less prestigious than sworn commanders, working for money.

- **Anyone can hire them:** the Trade Authority and governors.
- **Contracts are specific tasks:** hunt this pirate, keep this stretch clear, guard
  this depot.
- Mercenary work is the Authority's visible presence in the world: its enforcement is
  the mercenary fleets you see.
- A mercenary without contracts can't pay for ships. Swearing fealty or turning pirate
  are the ways out.

## Pirates

A pirate is anyone (a character, a fleet or a planet) that has **renounced allegiance
to both the Trade Authority and every faction.** Pirates are enemies of everyone.

- **Pirates don't use the Lattice.** The Authority refuses them, so they have no
  access to Lattice fuel. The only exception is a pirate disguised as a neutral
  merchant (the spy mechanic), who can buy fuel and ride the network while the
  disguise holds.
- **Pirates are slow.** Without the Lattice, the best logistics network there is,
  they move everything through open space. That makes them weak near the highways.
- **Pirates run a black market** that pays better than the legal one, if you aren't
  caught. Their economy is stronger than the legal one.
- **Pirate planets near the Lattice don't last.** They're lucrative targets with no
  diplomatic cost to attacking them, lawful forces reach them quickly, and no ally
  can send reinforcements along the Lattice to defend them.
- **On the fringe, far from the Lattice, the pirates' disadvantage fades or
  disappears**, and their black-market economy gives them the edge. The only
  self-sustaining piracy is there.

This produces the danger gradient without authoring it. The "civilised" region near
the Lattice rarely has pirates, who are dangerous to most playstyles. The fringe is
pirate-heavy, less lawful and more dangerous.

## Gates: two keys

Entry and exit at every gate is controlled by **both** the planet that holds it and
the Trade Authority. A ship passes only if both allow it.

- **The Trade Authority** decides who may be on the network at all. It lets on
  anyone whose standing with it is at least neutral. **(open)** Whether the bar is
  neutral or positive is a progression tuning choice. Known pirates are refused at
  every on-ramp.
- **The faction** controls the entrances and exits held by its planets. A ship needs
  neutral or better standing with the faction (its root governor) to use them.
- Planets hold gates, which is why they pay the Authority's dues. A planet that won't
  or can't pay becomes a pirate planet. **(proposed)** Its gates are shut using the
  Authority's key.

Consequences:

- **Enemy forces can't exit in hostile territory.** Invasion forces never bail out
  (see Armies), so they leave the Lattice at the nearest exit to their target that
  will let them out, and cross the rest in open space. That crossing is slow and
  visible, which gives the defender time to react: a home-field advantage.
- **Fronts advance gate by gate.** Taking a planet flips its gates and gives the
  attacker a new exit one step deeper.
- **A pirate-held interchange is dead in both directions:** pirates can't board, and
  nobody else can exit there.
- **(proposed)** Neutral planets decide whether to let a passing army out, possibly
  for a price. Passage rights are a diplomatic bargaining chip.
- A refused player always has a legible reason: the faction doesn't want them, or the
  Authority doesn't.
- A player refused at an exit can still bail out before it and fly in, without the
  boost.

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
the nearest exit that will let them out, against the defender's detection, scout
return, courier run and reinforcement travel.

- **Scouting earlier means reacting earlier.** The further out an invasion is seen,
  the more time the defender has.
- **Killing scouts buys surprise.** An invasion force that destroys the enemy's scouts
  can get closer before the alarm is raised.

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

Forces gather into **armies** before a major operation, as in Bannerlord. An invasion
force is an army commissioned by a governor (see Invasions and conquest). Two layers
govern what an army does:

- **Intent** is the strategic objective, such as "invade planet X." It is set when the
  army forms, it's telegraphed (a scout who sees the army learns its intent), and it's
  hard to overturn. Because intent is sticky, information stays good by the time a
  scout gets it home.
- **Action** is what the army is doing right now in service of its intent: moving,
  fleeing a larger force, refuelling, attacking a vulnerable target along the way.
  Actions change freely. Intent doesn't.

Movement follows the travel rules: along the Lattice, off at the nearest exit to the
target that will let them out, then through open space. Armies never bail out.

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
  raiding its fuel stop, hunting its scouts, or baiting a detour to buy time for the
  defender.

## Prisoners

Any named character, including agents and the player, can be taken prisoner, with a
small chance of death. Prisoners can be ransomed, exchanged or escape.

- A captured lord leaves vassals leaderless, which is an opening for defection.
- For the player, capture is harsh but recoverable (lost time, cargo, possibly the
  ship). It's the main answer to "what happens when you lose," and it keeps losing as
  progress rather than a reload.

## The player in this structure

The player follows every rule above. They **start as a neutral trader**, with neutral
standing with the Trade Authority and a ship **without a Lattice Drive**. Getting one
is the moment the universe opens up. From there the structure offers roles rather than
classes:

- **Trader:** neutral hauling, which is also cover.
- **Spy or smuggler:** trading while gathering information or moving black-market goods.
- **Scout:** open-space intelligence missions for planets.
- **Courier:** carrying dispatches on the Lattice.
- **Mercenary:** contracts for the Authority or governors.
- **Sworn commander:** fealty to a governor, fighting for a faction.
- **Governor:** installed on a conquered planet by the governor they serve. They
  can appoint a steward and keep flying with a fleet of their own.
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

## What a headless prototype should check (proposed)

When the simulation is built, run the world with nothing rendered, many times faster
than real time, and measure:

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
- **"Threader."** A speech-to-text error. The equipment is the Lattice Drive.
- **Pirate raiding range bounded by Lattice fuel.** Pirates don't use Lattice fuel at
  all (except in disguise). Their limit is that they travel slowly through open space.

## Open questions

- How far the player can rise: governor is possible (see Invasions and conquest), but
  can the player become the root of a faction by breaking fealty with their planet?
- Whether Lattice access needs neutral or positive standing with the Trade Authority.
  A progression choice.
- The economy: resources, supply chains, what planets produce and need, fuel pricing.
  Planets' wealth drives everything above, so this is the next document.
- Relationships and loyalty: how governors choose commanders and successors, and how
  loyalty is kept or lost. Separate document.
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
  - The current prototype map (4×4 sectors, 6 systems) is far too small for any of
    this: culture spread of 2–3 sectors would cover it all, and there is no fringe.
    The fixed ¢-shaped network will also need to become a generated one.
- Who holds an interchange when a system has several planets, and whether junction
  gates (which no planet holds) have only the Authority's key.
- Recruitment: how planets acquire commanders and how commanders recruit ships.
  Separate document.

## Revised after review

The first write-up of this session got several things wrong. The human corrected them
on 2026-09-27:

- **Bail-out and the ramp boost.** The boost through an exit's frames is what makes
  exits the normal way off; bailing out is a player option for shortcuts and
  smuggling. NPCs never bail out except when out of fuel, so invasions still exit at
  the nearest allowed gate and crawl.
- **Pirates and fuel.** Pirates don't use Lattice fuel except in disguise. Their
  weakness near the Lattice is slow travel; on the fringe it fades and their black
  market gives them the edge.
- **What a faction is.** A hierarchy of governors. Commanders swear fealty to
  governors; mercenaries are commanders without it. The Authority is a limited faction
  with a fixed ruleset. Invasions are commissioned by a governor, who installs the
  conquered planet's new governor, possibly a commander.
- **Stewards (added).** A governor can appoint a steward and lead a fleet of their own,
  so the most powerful role is also a playable one.
- **Player start and access.** The player starts neutral with the Authority, with a
  ship without a Lattice Drive. Faction standing controls a faction's entrances and
  exits; Authority standing controls the network.

## For the implementation agent

Not to be acted on while this document is in `docs/planning/`. When it's promoted:

- Add it as `docs/FACTIONS.md`.
- Move the travel rules (Lattice Drive, fuel, ramp boost, bail-out) into
  `WORLD_AND_TRAVEL.md`.
- Fold a summary into `BRIEF.md`, and update its open questions.
- Recruitment, relationships and the economy will be their own documents. Leave hooks,
  not designs.
- This is design only. Don't build it yet.
