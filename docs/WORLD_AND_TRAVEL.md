# World and travel: design of record

*Written 2026-09-24 from the travel prototype this repo was rebuilt from. This
describes how the world is laid out and how moving through it works and feels.
Numbers are listed at the end as current tuning; they are not the design.*

## Open space

### Sectors

The map is a grid of **sectors**, each a hexagonal prism much wider than it is tall.
The prisms tile the map with no gaps, so open space is one continuous volume the
player can fly anywhere in.

- There is a real **up and down**. Every ship shares one horizon, nothing rolls, and
  the top and bottom of a sector are limits you bounce off.
- Leaving the edge of the map bounces you back, as the top and bottom do.
- Sector walls show only as a faint grid when you are close, so the boundary can be
  read without cluttering the view.

### What is rendered

- **Your current sector** renders as it is.
- **Neighbouring sectors** show only their planets and moons. These are pushed further
  away along your line of sight by a logarithmic factor and hazed (dimmed and
  cooled), so they read as distant. The push is 1 at that sector's border and grows
  with how far you are from it, so a planet is in its true place by the time you
  arrive.
- **Sectors two or more away** render nothing.
- Asteroid clusters and gates near you still show when they belong to the
  neighbouring sector, so a border never makes something beside you vanish.
- The world uses a **floating origin**: it is re-centred on the current sector as you
  cross borders, so positions stay precise anywhere on the map.

### Flying

- **Thrust with momentum.** W accelerates toward top speed, S brakes or reverses.
  With no input the ship keeps its speed. Sideways drift is damped, so turns
  stay controllable.
- **The mouse moves a reticle, and the ship turns toward it** at its own turn rate.
  The reticle can lead the ship only so far.
- **There is a climb/dive limit.** Near it the camera stops following fully, so you
  can see the ship pitched toward the limit. A pitch gauge on the HUD shows how close
  you are.
- **Hitting anything bounces you back a little and cuts your speed**: walls,
  planets, asteroids, map limits. Your thrust is untouched, so you accelerate back up.
- There are no speed-streak effects in open space. You sense motion from the things
  around you: asteroids, planets and other ships.

### Ship classes

The prototype has two, to establish the relationship:

- **Fighter:** fast and agile, about 2.5 times the freighter's speed. It can't
  enter the Lattice.
- **Freighter:** slow, heavy and big, about 50 m long against the fighter's 8 m. It
  has a **threader**, the equipment that lets a ship enter the Lattice.

The Lattice is sized around the freighter: its lanes, tunnel and gate frames scale with
the freighter's model (`Galaxy.ROAD_SCALE`, tied to `ShipMesh.FREIGHTER_SCALE`), so a
freighter fills one lane and fits a gate frame whatever size it is.

The intended feel: crossing between sectors in a fighter is inconvenient, and in a
freighter it is painful. But a freighter in the Lattice covers the map far faster
than a fighter can in open space. There is time on the Lattice, docked, to talk to
other ships or look at the map. Getting a threader is the moment the universe
opens up.

### Systems

A **system** is a group of planets in one sector: one to four planets, some with
moons. Most systems sit a few kilometres from a Lattice interchange; at least one is
off the network, reachable by hop lane or by a long flight.

### Hop lanes

**One-way lanes between planets**, usable by any ship. Enter through the violet
gate beside one planet and you are carried to the next at high speed, then released
near it at a normal flying speed.

- In a system with three or more planets they form a loop. With two there is a lane
  each way. A few lanes connect neighbouring systems.
- Each lane keeps to the right of the line between its planets, so the two
  directions never overlap.
- Faint frames mark each lane in open space so it can be followed by eye.
- Hop lanes are what make a system a place with a local objective: getting from one
  planet to the next doesn't cost a long flight.

### Asteroids

Asteroids come in **clusters**, not an even scatter: belts around some planets, and
clumps elsewhere, dense in the middle and thinning out. Each rock slowly orbits the
centre of its cluster and tumbles, on average slower than a freighter. They are
solid; hitting one bounces you like any other obstacle.

## The Lattice

### What it is

The Lattice is a **wormhole highway network** that exists as its **own place**: a
scaled-down replica of the map (currently 1/20). It is directionally accurate. Each
road runs the same way as the sectors it connects, so heading east on the Lattice
takes you east across the map.

You can't see the Lattice from open space. Only its **gates** exist in both places.
A gate sits in open space at the true position of its counterpart in the Lattice.
Your position and heading relative to the gate carry across, so going in or out is
continuous.

### Inside

- **A box-section tunnel** that opens up ahead of you into a throat and closes
  behind you. The throat is fairly close ahead; everything outside the tunnel is
  hidden.
- **Industrial, not colourful.** Steel plating with thin seams that glow faintly
  from the wormhole outside. Warm lamps set into the median every few dozen metres
  give most of the light, with a little haze down the tunnel.
- **Two carriageways side by side**, traffic on the right, three lanes each, and a
  median barrier. You can see the opposing traffic across the median but can't
  reach it.
- **Signs and HUD:** current sector, road and heading, upcoming exits and junctions
  with distances.

### Two ways to travel

- **Free flight:** the same controls as open space, inside the tunnel. You're held to
  your own side of the median, inside the walls, and under a ceiling. The ceiling is
  kept below the tunnel roof, so the camera always has room above the ship.
- **Docked:** press **C** to dock to the road. The ship eases onto it and cruises at a
  fixed fraction of top speed. A / D step one lane at a time, and the mouse becomes
  a free cursor (for future points of interest). C again undocks.

### The network's shape

A cent sign (¢):

- **HWY 1** is the C, open to the east. Both open ends finish in an **exit gate**,
  with an entrance beside it for the other direction.
- **HWY 2** is the vertical stroke, ending in a T-junction on HWY 1 at the top and
  bottom. There you keep left or right to turn east or west. From HWY 1 you keep
  toward HWY 2's side to turn onto it.

### Getting on and off

- **Interchanges sit beside systems.** At each one, the exit and the entrance for
  each direction are grouped together, so wherever you get off, you can get back on
  nearby.
- **Docked:** keep to the right lane past an exit fork to take it. After merging, the
  ship eases toward the middle lane so the next exit isn't taken by accident.
- **Free flight:** fly through an exit gate.
- **Entering from open space:** fly through an entrance gate. It has lead-up frames
  on the approach side to line you up.

## Gates in open space

All gates are rectangular frames with a surface that swirls faintly:

- **Green:** Lattice entrance (needs a threader)
- **Amber:** Lattice exit (you come out of these)
- **Violet:** hop lane

Entrances have a line of lead-up frames on the approach side. Exits have a few
trailing frames.

## Traffic

The prototype's traffic is cosmetic. In open space, ships warp in and out, emerge
from exits, and fly into entrances. On the Lattice, docked freighters run in both
directions. Under the vision (`VISION.md`) traffic becomes simulation fleets,
rendered when they're near the player. That is the next big design piece.

Open-space traffic already follows that shape at a small scale. It spawns around
the player out to beyond sensor range (4 km, see `COMBAT.md`), is drawn only inside
it, and is handed back once it's well outside. Each traffic ship has a name and
class so it can be targeted.

## Current tuning

Snapshot at the rebuild. All of this is expected to move.

| | Value |
|---|---|
| Sector | hex, 10 km circumradius, 6 km tall; 4×4 map |
| Fighter / freighter top speed | 100 / 40 m/s |
| Docked speed | 80% of the ship's top speed |
| Lattice scale | 1/20 of the map |
| Freighter / fighter length | about 50 / 8 m |
| Lattice lane | 25 m (3 per carriageway, 10 m median) |
| Lattice tunnel | 360 m wide, 80 m tall; free-flight ceiling 60 m |
| Gate frame | Lattice 75 × 50 m, hop lane 150 × 100 m |
| Hop lane | up to 600 m/s |
| Neighbour planet push-out | 1 + 2.4 × ln(1 + distance past border / 1.5 km) |
| Asteroid drift | about 1–16 m/s |

## Known rough edges

- The tunnel bends sharply through junction turns.
- The backs of road signs are plain dark panels.
- Free flight through a junction follows whichever road is nearer, which can put you
  on the other road's far side.
- Traffic and system placement are generated once from fixed seeds. There are no
  authored places yet.
