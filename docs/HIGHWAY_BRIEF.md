# The highway — a clean-room brief

*Written 2026-09-20. This branch starts from the last commit before any road existed
(`bf97b40`, exploration step 5). Two earlier highways were built and set aside; this
brief is what the next attempt builds from, and it deliberately says what, not how.*

## The one-sentence requirement

In the human's own words:

> A tileset of square tubes that can connect and branch and allow a ship to move
> through with bounce collision on the walls.

Judge every design choice against that sentence first.

## The rules the road must obey

- `docs/EXPLORATION_DESIGN.md`, **Locked Decisions**: one-way carriageways side by
  side with traffic on the right, a wide flat section, a lane boundary that is soft,
  portals as the only way on and off, a cruise drive that works only on the road, the
  camera held to the road's direction, roads that curve and climb, a lane that is
  visually open. That document cites ADR 0077 for the side-by-side carriageways; that
  ADR belonged to a discarded road and is not on this branch. The text stands on its
  own.
- `decisions/0057-the-highway-is-a-place.md`: no camera cut, no scene load, no
  non-interactive transit, the surrounding space stays rendered, floating origin and
  the LOD/collision rule apply inside the tube.
- `decisions/0060-a-portal-opens-for-a-cruise-drive.md`: a portal's colour is the
  whole of "may I use this", and entry is on contact.
- `CLAUDE.md`, all of it: feel values in `tuning.cfg`, code-first scenes, the gate,
  no interdiction, the speed hierarchy by hull class.

## The picture to build toward: the highway as its own place

The concept art is `docs/reference/wormhole_highway_concept_art.jpeg`: a road with
rails and signs, two directions of traffic, inside a tunnel of streaking light whose
throat is some way ahead. The human's note: the throat in the art is too far off. The
earlier in-world road's art is `docs/reference/highway_concept.jpeg`, kept for the
structure's look.

What we are trying to find out is whether a highway that is its own place, a road
inside a wormhole reached and left by ramps that exist in both worlds, gives what an
in-world road could not without fighting physics: a casually slow drive inside, a
sense of great speed outside, short hops between exits, no big acceleration on
entering, and ramps that can be as gradual as we like. And what it costs: the road is
not visible from open space, and you cannot leave it mid-route.

- **Two worlds in one scene.** Open space stays what it is: systems, planets, the open
  world inside one border. A second place holds the wormhole. Both are resident all
  the time, so there is nothing to load at the crossing. The ship is moved from one
  to the other; the camera follows without a cut.
- **Only the ramps exist in both.** In open space the highways do not show at all;
  each system has its on and off ramps, ending at a wormhole mouth a short way from
  the planet. In the wormhole, the same ramps join the highway.
- **A ramp is one shape placed twice.** Each ramp is built once, against the
  carriageway it joins, and its twin is placed in open space so that its mouth is at
  the planet's mouth. The ship crosses between worlds partway along the ramp,
  carrying its position in the ramp, its felt speed and its heading relative to the
  ramp's axis. Same tube, same place in it; only the backdrop changes. No fade, no
  cut.
- **The wormhole highway follows the map.** Its nodes are the highway's systems in
  order, at the map's bearings, each leg a tuned fraction of the world leg, clamped
  to a tuned range of seconds at cruise. It dead-ends a short run past the first and
  last systems: the last exit is the final off-ramp.
- **Both directions in one tunnel**, two carriageways and the median, as the art
  shows. What that buys is the other side's traffic: someone being pulled over by two
  gunships with red lights while the channel announces why, seen through the median.
  Traffic is not in this prototype; the tunnel is shaped for it.
- **The throat opens ahead.** Inside, the tunnel is a streak cylinder that rides with
  the ship along the carriageway's axis, closing to a throat a tuned distance ahead,
  closer than the art. On an on-ramp in space the wormhole's mouth is drawn at the
  crossing point ahead of you, and on an off-ramp inside, space opens at the crossing
  point.
- **Speeds.** Ramps and the highway close to each other, so entering is a small change
  rather than a launch. The streaks carry the sense of speed outside; the road inside
  is the slow part.
- **Interchanges are deferred.** A system on two highways means taking one's off-ramp
  and the other's on-ramp. Same topology, one fewer mechanism for the first attempt.

Everything above is a test, not a decision: nothing gets an ADR until it has been
flown and kept.

## What the last two attempts taught, and nothing more

Both earlier highways are on other branches for reference only (`main` has the first;
`feat/highway-tubes` has the second and a working wormhole). Do not read their code
to build from it. Two lessons are worth carrying:

- **The mesh and the collider must share one rule for where a wall is open.** The
  first road computed collision as path arithmetic with declared exceptions and drew
  a mesh decoupled from it; the human found missing glass and wrong collision at ramp
  edges within seconds of every flight.
- **A road is verified by flying it**, in the gate, against its own rendered surfaces.
  A check that only reads the data is not a check.

And one rule from `CLAUDE.md` that the first attempt broke: a bug-fix session is not
an ADR. Thirteen ADRs about the first road's mechanism were deleted because their
forbids had boxed the road in.

## Deferred, for after the first flight

- Interchanges as junctions inside the wormhole.
- Traffic, and the channel.
- Signs for the next exit, and the map inset.
- Fuel: a wormhole leg is short, so charging per metre flown makes the highway nearly
  free. Charging each hop by its world distance is the obvious answer.
- What happens past the last exit: an open end, a sealed one, or a turnaround.
