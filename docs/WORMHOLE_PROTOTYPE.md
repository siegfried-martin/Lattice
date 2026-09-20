# The wormhole highway — a prototype plan

*Written 2026-09-20 from the human's direction, after the gear prototype
(`docs/SECTOR_PROTOTYPE.md`) got close enough to see what it would be when polished.
That effort is parked, its code left in place behind `highway_gear` 1, and this is the
comparison. It is a test, not a decision: nothing here gets an ADR until it has been
flown and kept. Expect to change any of it.*

The concept art is `temp/wormhole_highway_concept_art.jpeg`: a road with rails and
signs, two directions of traffic, inside a tunnel of streaking light whose throat is
some way ahead. The human's note: the throat in the art is too far off.

## What we are trying to find out

Whether a highway that is its own place — a road inside a wormhole, reached and left
by ramps that exist in both worlds — gives what the in-world gear could not without
fighting physics: a casually slow drive inside, a sense of great speed outside, short
hops between exits, no big acceleration on entering, and ramps that can be as gradual
as we like. And what it costs: the road is no longer visible from open space, and you
cannot leave it mid-route.

## The picture

- **Two worlds in one scene.** Open space stays what it is: systems, planets,
  sectors, the open world inside one border. A second root holds the wormhole. Both
  are resident all the time, so there is nothing to load at the crossing. The ship is
  moved from one root to the other; the camera follows without a cut.
- **Only the ramps exist in both.** In open space, the highways no longer show at
  all; each system has its on and off ramps, ending at a wormhole mouth a short way
  from the planet. In the wormhole, the same ramps join the highway.
- **A ramp is one shape placed twice.** Each ramp is built once, in the wormhole,
  against the carriageway it joins, and its twin is placed in open space by the rigid
  move that puts its mouth at the planet's mouth. The ship crosses between worlds at
  a tuned fraction along the ramp, `wormhole_swap_at`, carrying its position in the
  ramp, its felt speed and its heading relative to the ramp's axis. Same tube, same
  place in it; only the backdrop changes. No fade, no cut.
- **The wormhole highway follows the map.** Its nodes are the highway's systems in
  order, at the map's bearings, with each leg's length the world leg times
  `wormhole_scale`, clamped between `wormhole_exit_min_seconds` and
  `wormhole_exit_max_seconds` of travel at `wormhole_speed`. So it roughly fits the
  sectors and where the ramps are, and exits are 5 to 20 seconds apart. It dead-ends a
  short run past the first and last systems: the last exit is the final off-ramp, and
  past it the road ends at a buffer.
- **Both directions in one tunnel**, two carriageways and the median, as now and as
  the art shows. What that buys is the other side's traffic: someone being pulled
  over by two gunships with red lights while the channel announces why, seen through
  the median. Traffic is not in this prototype; the tunnel is shaped for it.
- **The throat opens ahead.** Inside, the tunnel is a streak cylinder that rides with
  the ship along the carriageway's axis, closing to a throat `wormhole_throat_metres`
  ahead, closer than the art. On an on-ramp in space the wormhole's mouth is drawn at
  the crossing point ahead of you, and on an off-ramp inside, space opens at the
  crossing point.
- **Speeds.** Ramps at `ramp_speed`, the highway at `wormhole_speed`, close to each
  other, so entering is a small change rather than a launch. The streaks carry the
  sense of speed outside; the road inside is the slow part.
- **Interchanges are deferred.** B is on both highways, so changing highway means
  taking B's off-ramp and its other on-ramp. Same topology, one fewer mechanism for
  the first attempt.

## Data

`data/routes.json` keeps `systems` and each highway's `systems` order, and its
`mouth_height` / `mouth_along` overrides. The highways' world `points` and `radii` go:
there is no world road to author. Interchange ramps stay in the file, unbuilt, until
interchanges come back.

## Keys, all under a new `;;; The wormhole` group unless noted

- `wormhole_speed` — the lane's ceiling on the wormhole highway. Start 100.
- `wormhole_scale` — world leg length to wormhole leg length. Start 0.02.
- `wormhole_exit_min_seconds`, `wormhole_exit_max_seconds` — the leg clamp. 5, 20.
- `wormhole_end_run` — metres past the end systems to the buffer. 400.
- `wormhole_swap_at` — where along a ramp, from the mouth, the crossing is. 0.5.
- `wormhole_tunnel_radius`, `wormhole_throat_metres` — the streak cylinder. 400, 500.
- `wormhole_streak_speed`, `wormhole_streak_density`, `wormhole_streak_color`,
  `wormhole_background_color`.
- The ramp keys stay and open up: `ramp_bend_radius` 400, `ramp_bend_deg` 35, leads
  300, since nothing in space has to line up with them any more.
- Parked, not removed: `highway_gear` 1, `junction_*`, `exit_downshift_seconds`.

## The crossing, frame by frame

1. The ship is riding a ramp tube. The map it is in checks the ship's `t` in the
   ramp each frame, measured from the mouth end.
2. On an on-ramp in space, when `t` passes `swap_at × length`: read `(t, u, v)`, the
   felt speed, the nose relative to the ramp's travel frame, and the camera's
   transform relative to the ship. Find the twin ramp in the wormhole.
3. Reparent the ship to the wormhole root at `twin.world(t, u, v)`, rotate it by the
   rigid transform between the two ramps' frames at `t`, set the collider's tube to the
   twin, sample the twin's lane, apply the same rigid transform to the camera. Hide
   and disable the space root, show and enable the wormhole root, switch the
   `WorldEnvironment`'s environment.
4. The reverse on an off-ramp inside, when `t` from the mouth end drops below
   `swap_at × length`.
5. A berth engaged across the crossing rebinds to the twin at the same distance along
   it; an exit taken from the strip survives the crossing.

## What each world draws of a ramp

Open space draws a ramp from its mouth to a little past the crossing, where the
wormhole mouth swallows it; the wormhole draws it from a little before the crossing to
its merge. The tube is whole in both for the collider, so a ship a hull's width either
side of the crossing is held by the same walls. `Road` gets a drawn range that the
chunk builder respects.

## HUD, strip, spots

The strip and the road rows read whichever map the ship is in. The debug HUD says
which world. `K` drops on spots in both worlds; `J` jumps between systems in space.

## The gate

- `make roads` builds and reports both networks: the space ramps and the wormhole
  highways.
- The road suite flies both: laps of each wormhole carriageway end to end, every ramp
  through its junction, the dives and the drunk pilot as now.
- A crossing test in the exploration scene: drive the ship up an on-ramp under its
  own power, check the swap happens at the fraction with `(t, u, v)` preserved, ride
  to the merge, take an exit from the strip, and land back in space on the off-ramp
  near the destination, riding.
- ADR 0057's bullets about no scene, the road visible from space and leaving the lane
  anywhere are suspended for this test, on purpose. If the wormhole is kept, an ADR
  supersedes 0057 with what was flown as the evidence.

## Build order

1. Data and the space side: highways gone from open space, free-standing ramps per
   system and direction ending at a wormhole mouth, `make roads` clean.
2. `WormholeMap`: the compressed highways with dead ends and buffers, the ramps built
   against them, their twins handed to the space map, the berth, gates and portals it
   already knows how to own. Built whole at start; nothing streams.
3. The scene: two roots, the environment switch, the crossing both ways, camera
   continuity.
4. The tunnel: the streak cylinder and its throat, the mouth at a crossing point.
5. HUD, strip, spots, the crossing test, docs, STATUS.

Each step lands gate-clean.

## What is built — 2026-09-20

Steps 1 to 3 of the build order, and the gate. The plan above is kept as written;
where the build differs, this section says so.

- **Two networks in one frame, not two roots.** There is no floating-origin recentre
  yet, so the wormhole is a second `RoadNetwork` below the map
  (`WormholeLayout.ORIGIN`, one pocket per highway `POCKET_SPACING` apart) and the
  crossing is a teleport within the map's frame plus a visibility switch
  (`SystemMap._apply_world`). The ship's one collider knows every tube of both, so the
  world is derived from the tube the ship is in; a debug drop onto either world's road
  puts the map there.
- **`cruise_speed` is the highway's speed.** There is no `wormhole_speed`: with no
  highway anywhere but the wormhole, the key that always was the highway's lane
  ceiling is that. `highway_gear` is 1.
- **The crossing is in metres, not a fraction.** `wormhole_swap_metres` from the
  planet's end of the ramp, and `wormhole_ramp_overlap` drawn past it in each world,
  because a ramp's length now depends on the run those two make.
- **Planet ramps are compact and level**, not the S-bend: exit head, run, crossing;
  the entry the same reversed, in through the wall onto a lead on the carriageway's
  centre-line. The space side of a ramp is a straight stub of `wormhole_swap_metres`
  from the wormhole's mouth to the planet's. The S-bend and `ramp_bend_deg` are
  gone; `ramp_exit_lead` 120, `ramp_exit_length` 520 at 14°, `ramp_bend_radius` 600
  for the bend onto the run.
- **Found by the gate on the way**: the berth's felt speed was read back from the
  ship's displacement, closing pull included, and fed the next frame's budget, so a
  berth taken forty metres off the rail wound the ship up to twice cruise. The gear's
  wide bound had hidden it. The felt speed is the rail budget now.
- **The leg floor is the ramp footprint.** A node's exit and entry each take
  `wormhole_node_gap` plus a ramp's length (about 1.15 km) of carriageway, so two
  nodes cannot be closer than about 2.3 km, 19 s at 120 m/s. The keys start at 20 to
  30 s rather than the 5 to 20 in the plan; shorter needs smaller ramps, and
  `make roads` prints every leg's seconds so the trade is visible. Legs: A–B 20 s,
  B–C 30 s, D–B 20 s, B–E 24.6 s.
- **Two ramps at an end system, four in the middle.** An on-ramp toward a dead end
  and an off-ramp from a road's first metres would be traps, so they are not built.
- **The dead end is an open mouth into the void.** Past the buffer a ship that missed
  the final off-ramp leaves the tube into the wormhole's own boundary field (the
  road's tube regions), reddening as anywhere outside playable space. A sealed end or
  a turnaround is a decision for after the flight.
- **The berth rebinds across the crossing in both directions**, and a berth carried
  down an off-ramp's stub is released at the planet's mouth as any ramp's end
  releases it.
- **Not built yet: step 4, the tunnel.** Inside, the wormhole is black beyond the
  road's own lamps; there is no streak cylinder, no throat, and no mouth drawn at the
  crossing point. The HUD's `world` row says which world the ship is in.

To fly it: `make fly`; the on-ramp is straight ahead at spawn. `K` drops on spots in
both worlds (`Mouth` and `Arrive` in space, `Exit`, `Merge` and `Bend` inside);
`make shot` with `ROAD_SHOT_SPOT=` any of those renders the seat there.

## Deferred

- Interchanges as junctions inside the wormhole.
- Traffic, and the channel: the pulled-over scene is the reason the tunnel has two
  sides.
- Signs for the next exit, and the map inset.
- Fuel: a wormhole leg is short, so charging per metre flown would make the highway
  nearly free. Charging each hop by its world distance is the obvious answer and is a
  decision for after the flight.
