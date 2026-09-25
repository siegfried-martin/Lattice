# Lattice: vision

*Written 2026-09-24, at the rebuild. This is the intent every other document serves.
Where a later decision conflicts with it, talk it through rather than quietly picking
one.*

## The game in a paragraph

Lattice is a space sandbox set in a living simulation. Factions hold territory and
fight over it, an economy produces and moves real goods on real ships, and named
characters with their own fleets go about their business whether or not the player is
anywhere near them. The player joins that world in whatever role they like and the
world responds to it: trade moves prices, fighting moves borders, and a reputation
follows you around. Tying the map together is **the Lattice**, a network of
wormhole highways that shrinks the universe down for any ship that can enter it.

The lineage is Escape Velocity with the parts it never had: real faction wars over
territory that changes hands, persistent fleets, a working economy, and combat that is
skill-based in real time without asking the player to master vector thrust.

## The core idea: you choose how to play

There is no single intended way to play. The design supports several playstyles as
first-class paths, and the player moves between them as they like.

| Playstyle | What it looks like | Where the challenge comes from |
|---|---|---|
| **Summoner** | A big ship with turrets and hired gunners. You drive slowly and they fight. | Money and crew quality. Your attention stays low. |
| **Explorer** | A fast scout heading for the corners of the map, running from trouble. | Distance, self-reliance, knowing when to leave. |
| **Merchant** | Trade your way up and hire escorts. You've earned the right to play on easy mode. | Markets, routes, and what you spend on protection. |
| **Warfighter** | A gunship with a faction, pushing deeper into enemy territory to take it. | As much as you choose to take on, by going further in. |
| **Casual** | A safe sector, low-paid missions, and the occasional missile flown into a pirate. | Very little, and only when you want it. |

These are examples of how the systems combine, not classes to choose. What follows
from them:

- **Difficulty is where you go and what you bring**, never a menu. Geography,
  your ship, your crew and your money together set how hard the game is. Safe space
  is really safe and dangerous space is really dangerous, with a gradient the player
  can read before they enter it.
- **Every path works on its own.** A merchant who never fires a shot and an explorer
  who never fights should still progress and grow powerful in their own terms.
  No path is gated behind combat skill.
- **Pressure is something the player chooses.** They can see a cost before committing
  to it and can back out of it. Being forced into a fight, ambient dread, or two jobs
  demanding attention at once all go against this.
- **The player makes the difference.** Hired help is convenience; an engaged player at
  any station does better than the crew member they replace.
- **Progression is access and reputation**, not an XP bar. Standing with a faction
  opens up its markets, its equipment and its trust.
- **The audience is the sandbox player** (Starsector, Mount & Blade, X4). They enjoy
  accumulation and becoming powerful in a world, rather than being tested by it. The
  risk to design against is boredom at hour thirty, not frustration in hour one.

## The signature: you fly the munitions

One way to fight is novel to this game: **you fly the missile.** Fire it, the view
goes with it, and you guide it into the target yourself, around the obstacles and
blockers in its way. This is how the game avoids the classic space-combat problem
of 3D interception: munitions are fast and ships are slow, so the skill is in
steering a fast thing precisely rather than in wrestling a slow ship into position.

The concept carries forward from the earlier build; how it was implemented does not.
It will be prototyped fresh against this world's scale and feel. It is a signature,
not a requirement. A summoner may never ride a missile, and a casual player might
fly one now and then for the fun of it.

## The world is a simulation, and what you see is a window onto it

Simulation state is authoritative. What is on screen is the part of that state near
the player, made visible:

- Factions, characters, fleets, stations and markets exist and act whether or not
  they are rendered. A fleet you saw last week is still out there somewhere.
- When the player gets close, the simulation hands its entities to the rendered world.
  When the player leaves, it takes them back. Nothing important is invented only
  because the player happened to look.
- The world responds to the player through the same simulation that runs everything
  else, not through scripted reactions.

## The foundation: travel that feels right

Getting the speed and scale of space to feel right took several attempts. The
prototype this repo was rebuilt from is the first that does it. Open space is big
and slow enough to feel vast, crossing a sector on your own engine is a real
undertaking, and the Lattice makes the whole map reachable without making it small.
`docs/WORLD_AND_TRAVEL.md` records that design. Other systems are built to fit it,
not the other way round.

## How the game gets built

- **One prototype per feel question.** Feel is found by playing, not by writing specs.
  Each system that has to feel right gets a playable prototype first, and the design
  doc records what worked.
- **Numbers are tuning, not design.** Speeds, sizes and rates change freely. The
  design is the relationships and the feel they produce.
- **Feel verdicts belong to the human.** The builder makes the instrument and shows
  it working (screenshots, scripted tours); the human plays it and says what's right.
- **Build for depth, tune for accessibility.** A deep system can always be made
  gentler later; a shallow one can't be made deep.
