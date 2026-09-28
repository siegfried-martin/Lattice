# Lattice: project brief

*A code-free summary of the game and where it stands, for brainstorming outside the
repo. Current as of 2026-09-27. The detailed design docs are `VISION.md`,
`WORLD_AND_TRAVEL.md`, `COMBAT.md` and `ROADMAP.md`; this brief condenses them.*

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

## Decisions worth remembering

- **Feel is found by playing.** There's one prototype per feel question, and the human
  gives the feel verdicts.
- **Numbers are tuning, not design.** The design is the relationships between them.
- **The Lattice is sized around the freighter.** Lanes and gates scale with its model.
- **Faction highway technology as a differentiator.** One faction's Lattice might use
  junction gates and another's continuous interchanges (trumpets, flyovers), so
  networks feel different.
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

### Simulation (later, once more of the game exists)
- How factions, characters, fleets, stations and markets exist unrendered.
- How fleets move on the graph of Lattice roads, hop lanes and open space.
- How the economy moves goods on actual ships.
- How entities are handed to the rendered world and back.
- How to keep one faction from snowballing.

### Playstyles
- What each one's loop looks like end to end.
- How crew hiring and crew quality work.
- What missions look like.
- How reputation opens access.

## Roadmap

The earlier roadmap, which went straight to simulation architecture, has been withdrawn
because the game needs many more of its elements before there is anything to simulate.

- **Done:** the travel foundation and a first combat prototype.
- **Now:** collecting design idea documents in `docs/planning/` and analysing them into
  a concrete path.

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
