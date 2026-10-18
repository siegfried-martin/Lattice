# Lattice: project brief

*A code-free summary of the game and where it stands, for brainstorming outside the
repo. Current as of 2026-10-18. It condenses `VISION.md`, the prototype docs
(`WORLD_AND_TRAVEL.md`, `COMBAT.md`), `ROADMAP.md`, and the full game's specification in
`docs/spec/`, whose rules are cited here by ID (FAC-8 and so on).*

## The pitch

Lattice is a space sandbox set in a living simulation. Factions hold territory and
fight over it. An economy produces real goods and moves them on real ships. Named
characters run their own fleets whether or not the player is nearby. The player joins
in whatever role they like: trade moves prices, fighting moves borders, and their
reputation follows them.

Tying the map together is **the Lattice**, a network of wormhole highways that makes
a vast map reachable for any ship that can enter it.

The main inspiration is **Escape Velocity**, plus what it never had:
- real faction wars over territory that changes hands;
- persistent fleets;
- a working economy;
- skill-based real-time combat that doesn't ask the player to master vector thrust.

The audience is sandbox players (Starsector, Mount & Blade, X4). They enjoy building
up power in a world more than being tested by it. The risk to design against is
boredom at hour thirty, not frustration in hour one.

## Design pillars

- **You choose how to play.** Summoner, explorer, merchant, warfighter and casual are
  all first-class paths (see below). No path is gated behind combat skill.
- **Difficulty is where you go and what you bring.** There is no difficulty menu.
  Geography, ship, crew and money set how hard the game is. Safe space is really
  safe, and the player can see the danger gradient before entering it.
- **Pressure is chosen.** The player sees a cost before committing to it and can back
  out. There are no forced fights and no ambient dread.
- **The player makes the difference.** Hired help is a convenience. An engaged player
  at any station does better than the crew member they replace.
- **Progression is access and reputation**, not an XP bar. Standing with a faction
  opens its markets, equipment and trust.
- **The world is a simulation, and the screen is a window onto it.** Entities exist
  and act when unseen. Nothing important is invented just because the player looked.
- **Travel feel comes first.** Other systems are built to fit it.

### Playstyles

These are examples of how the systems combine, not classes.

| Playstyle | Looks like | The challenge |
|---|---|---|
| Summoner | Big ship, turrets, hired gunners. You drive slowly and they fight. | Money and crew quality |
| Explorer | Fast scout heading for the corners of the map, running from trouble | Distance, self-reliance, knowing when to leave |
| Merchant | Trade up and hire escorts (easy mode, earned) | Markets, routes, what you spend on protection |
| Warfighter | Gunship with a faction, pushing into enemy territory | As much as you choose to take on |
| Casual | Safe sector, small missions, the occasional missile flown into a pirate | Very little, and only when wanted |

## The signature: you fly the missile

Fire a missile and the view goes with it; you steer it into the target yourself,
around obstacles and enemy blockers. This sidesteps the classic space-combat problem
of 3D interception. Ships are slow and munitions are fast, so the skill is steering a
fast thing precisely, not wrestling a slow ship into position. It's a signature, not a
requirement: a summoner may never ride a missile.

## What exists today (playable prototype)

### Open space
- The map is a 4×4 grid of hexagonal sectors, wide and flat, forming one continuous
  volume.
- There's a real up and down: no roll, and a ceiling and floor you bounce off.
- Your current sector renders fully. Neighbouring sectors show only their planets,
  pushed back and hazed so they read as distant.
- Flight uses momentum and a mouse reticle the ship turns toward. There's a
  climb/dive limit.
- Hitting anything bounces you and cuts your speed: planets, stations, asteroids,
  other ships (which get knocked away).
- **Systems** are groups of one to four planets in a sector, mostly near a Lattice
  interchange. At least one sits off the network. About a quarter of planets have one
  to three moons.
- **Service stations** sit beside a few Lattice interchanges.
- **Landing:** flying close to a planet or station pauses the game on a landing menu,
  where you can refuel. It's the hook for markets, missions and repairs later.
- **Hop lanes** are one-way high-speed lanes between planets, open to any ship. They
  loop around a system, so moving within one is cheap.
- **Asteroids** come in drifting, tumbling clusters and belts. Hitting one hard
  damages the hull, and they block weapon fire, so a rock field is cover.

### Ships
- **Fighter:** about 8 m long and about 2.5× the freighter's speed. It can't enter the
  Lattice.
- **Freighter:** about 50 m long, deliberately big, slow and heavy. It carries a
  **Lattice Drive**, the equipment that opens the Lattice. Getting one is meant to be the
  moment the universe opens up.
- The intended feel:
  - Crossing sectors by engine is inconvenient in a fighter and painful in a
    freighter.
  - A freighter on the Lattice covers the map faster than a fighter can in open space.
  - Docked cruising leaves time to talk to other ships or read the map.

### The Lattice
- **Its own place:** a 1/20-scale replica of the map, directionally accurate, so
  going east on it takes you east.
  - Only its **gates** exist in open space, each at the true position of its
    counterpart.
  - Position and heading carry through a gate, so entering and leaving are continuous.
- **The tunnel** is an industrial box section: steel plating, glowing seams and warm
  median lamps.
  - It has two carriageways of three lanes each, with a median you can see across but
    not cross.
  - Traffic keeps right.
- **Two ways to travel:**
  - **Free flight** uses the normal controls inside the tunnel.
  - **Docked** cruises on rails at 80% of top speed; you step between lanes one at a
    time.
- **Lattice fuel:** the Lattice Drive burns fuel per kilometre on the Lattice (a
  freighter tank goes about 7 km). Running dry drops you into open space where you
  are; refuel at planets and stations.
- **Getting on and off:** interchanges beside systems group the exit and the entrance
  together. When docked, keeping to the right lane past a fork takes the ramp.
- **Network shape:** a cent sign (¢).
  - HWY 1 is the C, with exits at its open ends.
  - HWY 2 is the vertical stroke.
- **Junctions use junction gates**, blue gates that exist only inside the Lattice.
  - Taking a ramp or lane into one brings you out of its paired gate on the other
    highway, merging from the right.
  - This came from two problems: a right-hand-traffic highway can't turn left across
    oncoming traffic, and in a world drawn as a tunnel around your path, real
    interchanges would show roads through each other's walls.
- **Navigation is on the HUD**, with no overhead signs (they blocked the view).
  - A **subway-line strip** shows the current road: the last stop passed, your
    position, and the next few exits and junctions with distances.
  - **Lane guidance** appears near forks.
  - A full network map would be a separate screen, not yet built.

### Gates, by colour
- **Green:** Lattice entrance, with lead-up frames to line you up.
- **Amber:** Lattice exit.
- **Blue:** junction, inside the Lattice only.
- **Violet:** hop lane.

### Combat (first prototype)
- **Stations:** the player holds one job at a time (pilot, turret gunner or missile).
  While you're away from the helm, the ship holds its course.
- **Weapons:**
  - **Auto-cannon**, from the freighter's turret or the fighter's nose. It also shoots
    down missiles.
  - **Blocker:** the freighter turret fires a canister that bursts into a pellet cloud
    and catches missiles. The gaps in the cloud are worth aiming for.
  - **Laser (fighter):** short range and high damage, with a heat lockout.
  - **Missile:** you fly it with the mouse to steer, W to boost (limited), S to slow
    and turn tighter, and A/D to dodge. It has a fuse, and left click detonates it
    early for reduced blast damage. Detonating is a way to salvage a near miss.
- **Test enemies:** a target drone, a hostile freighter (missile and blocker user) and
  a hostile fighter. They're deliberately beatable, with wandering aim, late reactions
  and poor missile leading.
- **Sensors:** ships exist whether or not they're drawn.
  - Only ships within sensor range (4 km) are rendered, on radar and targetable.
  - Ships in active combat persist beyond it.
- **Targeting** is Escape Velocity style:
  - Tab cycles through ships in range.
  - R picks the nearest enemy, or the nearest ship outside combat. Pressing R again
    flips between the two nearest.
  - The target gets brackets in view and an info panel: name, class, stance, range,
    speed, closing speed and hull.
- **HUD:**
  - speed meter;
  - hull, missile, blocker and heat bars;
  - a heading-up radar;
  - the target panel;
  - a "MISSILE INBOUND" warning.
- **Collisions:** ships don't pass through each other.
  - In open space they push apart.
  - On the Lattice, traffic follows the car ahead, yields and changes lanes.

### Playtest verdicts so far
- The travel prototype was the first attempt at the speed and scale of space that felt
  right. The whole game was rebuilt around it.
- Flying a missile into a target felt hard but rewarding: "really accomplished."
- The missile fuse (unspecified, but picked up from the design) works well.

## The full game (from the spec)

The design of the full game is settled enough to build its simulation. In brief:

- **One kind of character.** Governor, commander, merchant, scout, courier, escort,
  smuggler, spy, pirate and the player are roles, held as contracts that either side can
  end without penalty (CHR-1, REL-8). Everyone owns their own ships (CHR-8). New
  characters start at the bottom and climb: scout → escort → commander → governor, or
  courier → merchant → rich merchant (CHR-11). There's one personality trait, risk
  tolerance (CHR-4).
- **Losing:** no capture, ever. A destroyed ship's pilot rides a missile-fast escape pod
  to safety; if the player's pod is shot down they reload from the autosave made on
  landing (CHR-14, CHR-15, CHR-18).
- **Two systems: money and relationships.** Opinion between characters is moved by deeds
  and fades over time. A vassal's opinion of its liege is loyalty, the liege's of the
  vassal is favor, and standing with a faction is its root's opinion of you (REL-1 to
  REL-5).
- **Factions are fealty trees of governors,** one governor per planet. The liege gives
  protection, patronage, intelligence and trade; the vassal pays tribute and serves in
  its wars. A conquering governor keeps a planet (installing a new governor under
  themselves) or grants it down the tree. Vassals defect after sustained low opinion when
  there's somewhere better to go (FAC-5 to FAC-11).
- **Diplomacy:** war, peace and ceasefire between neighbouring factions. A losing or
  war-weary faction sues for peace and pays tribute; the winner holds out for what it
  hoped to take. Money piled up in peace votes for war. A faction is at war a little more
  than half the time, in wars of one to three game months (DIP-1 to DIP-4).
- **War:** armies with telegraphed intent cross open space from the nearest gate that
  lets them out; sieges grind down a planet's defences while couriers race to bring help
  (MIL-7 to MIL-11). A conqueror holds its prize only when it fears losing it more than
  it needs its army home (MIL-12). Planets buy defence as a share of income, more at war
  and more again from a deep treasury (MIL-5). Armies can also capture an enemy's gate: a blockade, shut to all
  but the captor, which cuts the planets it serves off from the Lattice until it's
  retaken or peace lifts it (MIL-17). Combat away from the player is auto-resolved
  (MIL-13).
- **Economy:** population buying goods is the only source of new money; every planet is
  a state trader that taxes other traders; operating costs are the baseline sink (ECO-1
  to ECO-5). The v1 goods are food, water, oxygen, eight luxuries (six regional, two
  from the fringe; placeholder names) and one contraband (PRD-1). Each planet makes three
  goods at high, medium and low efficiency, partly by region; a luxury takes more labour
  than food, and fringe worlds grow more of their own; prices come from stock against a
  reserve; droughts and festivals keep them moving (PRD-7 to PRD-11). *Planned (round 4):*
  making goods costs money, more at low efficiency than high, so efficient planets
  export, dear producers make a good only when it's scarce, and prices rest on what
  goods cost to make; contraband is cheap to make. Happiness is a
  signed scale that settles toward what a population gets: each luxury with diminishing
  returns, hunger pulling it down. Productivity rises and falls with it toward a ceiling
  and a poverty floor, so poor planets stay poor without collapsing; an unhappy planet
  pays more for luxuries (PRD-4, PRD-5). Fuel is a fixed local price in v1 (ECO-11).
- **Trade:** merchants are characters. A faction root pays merchants a salary for its
  information network, in return for trips that start and end at home; independents run
  single trips. Merchants hire escorts by weighing route risk against cargo value. A slow
  osmosis keeps markets from becoming deserts (TRD-3 to TRD-13).
- **Piracy:** one anarchic pirate faction on the fringe, unable to use the Lattice. Broke
  commanders turn pirate; loot sells only at pirate planets, which make contraband. A
  planet may fail any week, likelier the unhappier and deeper in debt it is; with no
  vassals it turns pirate, with vassals it secedes and is attacked by its old faction
  (ECO-13). One
  pirate relation both shields you from pirates and raises lawful suspicion, so smuggling
  is a real risk (PIR-1 to PIR-13).
- **Information moves at the speed of ships.** Every character has its own dated facts;
  members of a faction share on contact; couriers carry news, orders and calls for help
  down the fealty tree; scouts and spies watch the enemy (INF-1 to INF-14).
- **The simulation** is a C++ library with a thin Godot binding, deterministic and fast
  enough to run many worlds in parallel. First it gets tuned until the world is stable;
  then experiments switch mechanics off for one faction to see whether each earns its
  cost (`docs/spec/sim/`).

Still open, among others: what triggers a war, how sieges play when rendered, how the
Lattice network is generated, and how the player hires crew (`docs/spec/OPEN.md`).

## Decisions worth remembering

- **Feel is found by playing.** There's one prototype per feel question, and the human
  gives the feel verdicts.
- **Numbers are tuning, not design.** The design is the relationships between them.
- **The Lattice is sized around the freighter.** Lanes and gates scale with its model.
- **Human highway technology is the same everywhere.** An earlier idea had each
  faction building its Lattice differently (junction gates in one, continuous
  interchanges in another). It's dropped; variety between regions comes from culture's
  ships and weapons instead.
- **Rendering follows sensors.** The simulation is authoritative, and the screen shows
  only what's near.

## Open questions

### Combat
- Should the fighter carry missiles?
- What should a missile blast affect: the player, other missiles, blockers? Should the
  fuse running out detonate it?
- Losing the ship: what does death or disablement mean?
- Should the radar show height, gates or bodies?
- Should the target feed the weapons (missile reticle, lead indicator)?
- Should sensor range vary by ship class?

### World
- A full-network map screen.
- Authored places versus generated ones.
- How big the real map gets.

### The full game
- The design's open questions are tracked in `docs/spec/OPEN.md`, by ID.

### Playstyles
- What each one's loop looks like end to end.
- How crew hiring and crew quality work (OPN-36).

## Roadmap

- **Done:** the travel foundation, a first combat prototype, and four rounds of design
  (factions, economy, trade, characters) formalised as the spec.
- **Now:** building the headless simulation in C++, milestone by milestone, then tuning
  it for stability and testing each mechanic's worth. The skeleton and toolchain are
  done, and every v1 system is built (world generation, the economy, characters,
  movement, information, trade, politics, war, piracy, suspicion and spies); next is
  tuning at full scale.
- **Next:** connecting the simulation to the rendered game.

## Glossary

| Term | Meaning |
|---|---|
| **Lattice** | The wormhole highway network, and the game's name |
| **Lattice Drive** | Ship equipment needed to enter the Lattice |
| **Sector** | A hexagonal cell of the open-space map |
| **System** | A group of planets within a sector |
| **Hop lane** | One-way fast lane between planets, usable by any ship |
| **Interchange** | Where the Lattice's exits and entrances sit, beside a system |
| **Junction gate** | Paired blue gates that move you between highways |
| **Docked** | Riding the Lattice on rails at cruise speed |
| **Station** | The player's current job aboard: pilot, gunner or missile |
| **Blocker** | A pellet-cloud canister that catches missiles |
| **Sensor range** | The radius within which ships are rendered and targetable (4 km) |
