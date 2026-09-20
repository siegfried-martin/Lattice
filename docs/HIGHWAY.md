# The Highway — how the road is built, authored and verified

*Written 2026-09-07 with ADR 0096, replacing `HIGHWAY_STRUCTURE_PLAN.md` and
`HIGHWAY_LATTICE_PLAN.md`. This is the document a session reads before touching the
road. The design intent is in `EXPLORATION_DESIGN.md`; the decision is ADR 0096; this
is the mechanism.*

## In one paragraph

A road is a centre-line of straight legs joined by circular arcs, with one or two
rectangular tubes swept along it. A highway is two carriageways side by side under one
roof with a glass median, traffic on the right; a ramp is one carriageway. The tube is
the only collision primitive: the ship is always in exactly one tube or in open space,
and a wall is passable only where the point straight through it is inside another
road's tube — which is exactly where the mesh leaves the wall out. Ramps are built by
one rule from where the data says they start and end. The mesh streams in around the
ship; the collision is analytic and holds everywhere. The road is verified by flying
it, in the gate, against its own rendered triangles.

Since 2026-09-20 the highway runs **inside the wormhole** (`docs/WORMHOLE_PROTOTYPE.md`,
a prototype under test): open space holds only each ramp's twin, ending at the
planet's mouth, and a ship crosses between the worlds partway along a ramp. The
section below, *The two worlds*, is that mechanism; everything else here is unchanged
by it.

## The files

| File | What it is |
|---|---|
| `data/routes.json` | The map: systems, which systems each highway joins, which ramps exist. Hot-reloaded. |
| `scripts/autoload/routes.gd` | Loads it, polls it, emits `reloaded`. |
| `scripts/lib/wormhole_layout.gd` | Lays the wormhole's highways out from the map, and places the planet mouths. |
| `scripts/lib/road_path.gd` | The centre-line: straights and arcs, analytic `closest`, `frame`, `at`. |
| `scripts/lib/tube.gd` | One carriageway's volume: `contains`, `local`, `world`, `sample` (the `CruiseLane`). |
| `scripts/lib/road.gd` | One road: its path, its tubes, its ribs. |
| `scripts/lib/road_collider.gd` | The collision for one hull. `hold()` keeps it inside its tube or bounces it off the outside. |
| `scripts/lib/road_probe.gd` | A scripted pilot for the gate, flying through the same collider. |
| `scripts/world/road_network.gd` | Builds the roads and ramps from the data, validates, streams the mesh, owns portals and gates. |
| `scripts/world/road_mesh.gd` | The visible structure, in chunks, with the one clip rule. |
| `scripts/world/road_berth.gd` | The dock on the roadway (ADR 0082), on tubes. |
| `scripts/world/wormhole_tunnel.gd` | The inside of the wormhole: the streak spindle that rides with the ship. Background layer. |
| `scripts/world/wormhole_mouth.gd` | The opening between the worlds, drawn at a ramp's crossing point in each. Background layer. |
| `scripts/world/system_map.gd` | Places the systems and corridors, owns both worlds' networks, hands the ship its tube each frame, crosses it between the worlds. |
| `tools/tests/road_suite.gd` | The flying half of the gate. |
| `tools/tests/road_report.gd` | `make roads`: build the map headless and list every problem. |

## The two worlds

Two `RoadNetwork`s in one frame, both children of the map and both resident all the
time: **the wormhole**, `WormholeLayout.ORIGIN` below the plane, holds every highway
and the ramps that join them; **open space** holds each ramp's twin and no highway.
`RoadNetwork.build_worlds` builds them in that order, and the ship's one collider
knows every tube of both, so which world the ship is in is a fact about the tube it is
in (`SystemMap._in_wormhole`), never a mode.

- **The layout** (`WormholeLayout.lay`). Each highway's nodes are its systems in
  order, at the map's bearings, each leg the world leg times `wormhole_scale` clamped
  to between `wormhole_exit_min_seconds` and `wormhole_exit_max_seconds` at
  `cruise_speed`. The road dead-ends `wormhole_end_run` past its first and last node.
  Each highway is in its own pocket, `POCKET_SPACING` from the next. Corners are
  rounded at the nodes with a radius a little over the highway's floor.
- **The ramps** are built once, in the wormhole, by the ramp rule below, and each is
  placed again in open space by the rigid move that puts its far end at the planet's
  mouth (`WormholeLayout.space_mouth`, `RoadPath.transformed`): the same shape, so a
  point (t, u, v) in one is the image of the same point in the other. A system at the
  end of a highway gets an on-ramp and an off-ramp; one in the middle gets four.
- **The crossing** (`SystemMap._cross`). A ramp's run beside the road carries it:
  `wormhole_swap_metres` along an on-ramp from the planet's mouth the ship is moved to
  the same (t, u, v) in the wormhole's twin, rotated by the rigid transform between
  the two ramps' frames there, with its velocity, reticle, road axis and berth carried
  over and the camera moved by the same transform (`crossed`). An off-ramp crosses
  out that far from its end. Forward only, so nothing flaps between the worlds; the
  jump's metres are not fuel.
- **What each world draws** (`Road.draw_from`, `draw_to`): its own side of the ramp,
  to `wormhole_ramp_overlap` past the crossing. The collider ignores the range; the
  tube is whole in both, so a hull either side of the crossing is held by the same
  walls. Ribs, markings and the far mesh all respect it.
- **What each world shows.** Open space: the discs and corridors (sectors off), the
  planets and stars, the deep field, the ramps' twins. The wormhole: its network, lit
  by its own lamps, inside the tunnel, with the road's own tube regions as its
  playable space. The sky is the world's colour (`arena/background_color` or
  `wormhole_background_color`), repainted by the scene on a crossing. The sector
  layer is not ticked inside, and names the sector the ship arrives in on the way out.
- **The tunnel** (`WormholeTunnel`). A spindle of streaking light,
  `wormhole_tunnel_radius` around the road's centreline, closing to a throat
  `wormhole_throat_metres` ahead and the same behind, placed every frame by the map
  on the road the ship is on: on a ramp, the highway it joins, so the one tunnel
  holds both carriageways and the ramp beside them. The streaks are fixed in the
  wormhole (they slide back by what the ship travels) and flow toward it at
  `wormhole_streak_speed` on top; `wormhole_streak_density`, `_length` and `_color`
  shape them, and the fill is `wormhole_background_color`, the sky's, so the throat
  has no edge. Seen through the road's glass walls, which tint it.
- **The mouths** (`WormholeMouth`). One disc of `wormhole_mouth_radius` on the axis
  of every ramp at its crossing point, in each world. From space it is the wormhole's
  own dark with its streaks converging and a rim of their light; from inside it is
  open space's colour beyond the same rim. One-sided: the camera lags the ship, so
  right after a crossing the mouth stands between them, and each is shown only to a
  camera on the side of the ramp its world draws (entries in space and exits inside
  face the approach; the other two face back down the ramp).

## The section

`lane_width` × `lane_height` per carriageway, `deck_separation` between the two
carriageway centres, so a highway is `deck_separation + lane_width` across. The floor
is a slab of `structure_floor_thickness`; the walls, roof and median are glass; corner
beams of `structure_beam_size` and a collar every `structure_module_length` of
`structure_rib_thickness`, standing out by `structure_rib_protrusion`. The collars are
instances rather than part of the chunk mesh, because they move (the gear's
treadmill, below). All of it sits
*outside* the flyable box (ADR 0078's unit-section rule survives): the space the ship
flies is exactly `lane_width × lane_height`, less its own hull.

Markings — five lines on each carriageway's floor — are the lane paint. The ridden
carriageway's are brighter and a ramp's are darker.

Every quad is wound **clockwise from the side its normal faces**, which is Godot's
front face. The materials are double-sided, and Godot flips a back face's normal to
face the viewer, so a quad wound the other way is lit from the wrong side: the roadway
seen from above was being lit by the star underneath it. `RoadSuite` checks the
winding of a built chunk.

## The lights

Between systems it is dark on purpose (STATUS.md, the star), so the light out there
is carried. `RoadLamps` puts a lamp bar on every rib, above the glass roof of each
carriageway, as an instanced mesh in the chunk — the string of lights down the road —
and keeps a pool of real omni lights (at most `RoadLamps.MAX_LIGHTS`) that the map
re-places every frame onto the bars within `track_light_reach` of the ship, along the
tube it rides or the nearest one. The ship's `Headlight` is a spot on the nose with
two visible lamps, toggled with `L`. Everything you would nudge is under `;;; Lights`
in `tuning.cfg`, and the HUD's `lights` row says what is live.

## The gear (a prototype, `highway_gear`)

`docs/SECTOR_PROTOTYPE.md`, prototype 2. `Road.gear()` is `highway_gear` on a highway
and 1 on a ramp. What reads it:

- `Mothership._fly_cruise` multiplies only the velocity's component along the road
  axis by the gear it is applying, and slews the road axis that much faster so it
  keeps up with a bend. Steering, the lane's push, the collider and the felt speed
  are as they were. The applied gear chases the lane's target: up over
  `highway_upshift_seconds` after a merge, down over `highway_downshift_seconds`.
- **The slow zone** (`Tube._gear_here`). Around every junction on a carriageway,
  and each open end, the lane's gear eases to `junction_gear` over
  `junction_slow_seconds` of world travel each side, so a ramp, a merge or a mouth
  is met at a few times the felt speed rather than at the full gear: the world
  slows on the approach to a place with ramps and winds back up on the way out,
  while the road itself keeps passing at the felt speed. The exit downshift takes
  the last step to 1 for the exit actually taken. A cluster of junctions (an
  interchange) is a slow stretch rather than a crawl.
- **Something beside the road.** Between systems there is nothing near enough to
  read the geared speed against, so in the open world the deep field's dust is
  scattered just past every road's own space along the whole map
  (`SystemMap.relayout`, `RoadNetwork.spine_all`), world-fixed, and it is what
  whizzes.
- **The exit downshift** (`Tube._lined_up_for_exit`, `CruiseLane.downshift`). In
  gear, an exit's opening passes in a fraction of a second, so an exit is only
  takeable out of gear. The lane's target drops to 1 when the ship is on the right
  of the lane with an exit's opening within `exit_downshift_seconds` ahead at the
  speed it is making through the world, or has taken the exit from the strip.
  Drift back left and it shifts back up. A missed exit costs one hop, as before.
- The chase camera is handed the gear's share of each frame's displacement and
  moves by it rigidly, so its lag is against the felt motion only. On the road the
  boom is `road_boom_scale` times longer.
- **The treadmill** (`Road.slip`, `RoadRibs`). Spacing the ribs by the gear only
  made one pass every few seconds; each still swept past at world speed, and so
  did the lane paint, so nothing near the ship moved at the felt speed. Now the
  ridden road's ribs slide along with the ship by the gear's surplus
  (`SystemMap` calls `Road.roll` with the ship's `gear_moved`), so a collar passes
  at the felt speed while the world outside passes faster. The collars and the
  lamp bars on them are instances placed every frame from `rib_positions()`
  rather than part of the chunk mesh, hidden where they would stand inside a
  neighbouring tube; `rib_margin_at` reads the same slip, so the collision moves
  with them. The lane paint's inner three lines are dashed
  (`marking_dash_metres`, `marking_gap_metres`) and the dashes scroll with the
  slip in the marking shader.
- The berth's rail runs at the lane's speed times the road's gear, so a berth on a
  highway is slower than driving it by `berth_speed_fraction` and not by the gear
  as well. Rebinding to a ramp drops it to the ramp's speed at once.
- The HUD's `gear` row and the `leg` row's by-road estimate.

A ramp also has its own speed limit, `ramp_speed`: the lane's ceiling on a ramp tube.
The validator floors a ramp's bends at that speed, which is what lets a ramp bend at a
couple of hundred metres. At `highway_gear` 1 the road is exactly what it was. The
gate's probe (`RoadProbe`) applies the gear the way the ship does, downshift included,
so the laps are flown in gear and the steered exits are taken out of it.

## Authoring the map

`data/routes.json`, metres in the map's frame, Y up, the combat plane at y = 0:

```json
{
  "systems": { "SYSTEM A": [0, 0, 0], "SYSTEM B": [11500, 0, 0] },
  "highways": [
    { "name": "A-377B", "systems": ["SYSTEM A", "SYSTEM B", "SYSTEM C"],
      "mouth_height": 160, "mouth_along": 600 }
  ],
  "planet_ramps": [ { "highway": "A-377B", "system": "SYSTEM A" } ],
  "ramps": [
    { "name": "X1", "from": { "highway": "A-377B", "side": "R", "near": [4500, 360, 120] },
      "to": { "highway": "K-112", "side": "R", "at": "SYSTEM B", "offset": 5700 },
      "points": [[8000, 330, 1200], [11612, 320, 2435]], "radii": [1200, 1200] },
    { "name": "X3", "mirror_of": "X1", "about": "SYSTEM B" }
  ]
}
```

- A **highway** is the systems it joins, in order. Its geometry is laid out inside the
  wormhole from that order (*The two worlds*, above); in open space the systems listed
  are what its corridors join and what its entry mouths are labelled toward. (The
  network's `build` still takes waypoints with a corner radius each, 0 at an open
  end, and `closed: true` for a loop; the layout generates them.)
- **`planet_ramps`** declares that a system on a highway gets ramps: an exit and an
  entry on both carriageways where they make sense, shaped by the rule below.
  `mouth_height` and `mouth_along` on the highway move that highway's mouths in open
  space, which is how K-112's ramps at B clear A-377B's.
- A **ramp** in `ramps` is an interchange, kept for when interchanges come back to the
  wormhole and not built now: `from` and `to` name a highway, a side
  (`R` is the carriageway to the right of the path's direction, `L` the other), and
  where on it — `near` a point, or `at` a system plus an `offset` in travel metres.
  Either end may instead be `{"mouth": "SYSTEM X"}`. `points` are the waypoints
  BETWEEN the standard exit head and the standard merge tail, with a radius each.
- **`mirror_of` + `about`** turns a ramp half a turn about a system's centre, which
  maps a ramp between two forward carriageways onto one between the reverse ones.

Save the file and the map relays out under the ship. `make roads` builds it headless
and prints every problem: a bend tighter than `road_turn_share` of the ship's turn rate
allows, a highway pitched past `road_pitch_max_deg`, a ramp leg within 15 m of a tube
it is not meant to join, two highways touching, a corner whose arc does not fit its
legs. Fix the one it names and run it again; a map with problems still builds and
still flies, so the report is the loop and the gate is the stop.

## The ramp rule

Every ramp's two ends are built the same way, from the `exploration/ramp_*` keys:

**The head sits beside the carriageway.** An exit's box runs `ramp_head_offset` to
the driver's right of the carriageway's centre, half the lane width by default, so it
straddles the wall: the opening is full depth from its first metre, a ship on the wall
is at the ramp's centre when it crosses, and the ramp's axis is the road's until the
peel bends away. Coincident, the opening began as a sliver narrower than a hull and
the ship was held on the wall until it widened, then shoved onto the ramp's centre:
an invisible wall and a jerk, from the seat. Entries still end coincident, inside the
carriageway.

- **Exit head**: from the carriageway's centre at `from`, level for `ramp_exit_lead`,
  then a bend of `ramp_exit_radius` onto a leg diverging right at `ramp_exit_angle_deg`
  for `ramp_exit_length`. The ramp's tube nests inside the carriageway for the lead
  (it is `RAMP_INSET` smaller) and leaves through the wall on the diverging leg.
- **Merge tail** of an interchange ramp: `ramp_merge_drop` below the carriageway's
  centre, climbing at `ramp_merge_pitch_deg` through the floor, then level inside it
  for `ramp_merge_lead`, ending at `to`. Bends of `ramp_merge_radius`.
- **Planet ramps** (`RoadNetwork._wormhole_ramp`) are the exit head, a bend of
  `ramp_bend_radius`, then a straight run of `wormhole_swap_metres` plus
  `wormhole_ramp_overlap` beside the carriageway, on which the crossing sits; an
  entry is the same shape reversed, converging in through the wall onto a lead that
  runs on the carriageway's own centre-line, so the ramp's end is wholly inside the
  host (a lead beside the centre, as an exit's head is, left half its end cap in the
  void). Level throughout, about a kilometre long. An exit ends `wormhole_node_gap` short of the system's node and an
  entry begins that far past it, so a node's ramps take the gap plus a ramp's length
  each side and two nodes cannot be closer than the sum: `make roads` prints each
  leg's seconds. Where the ramp's far end sits in open space is the twin's business:
  `ramp_mouth_along_offset` short of (exit) or past (entry) the system's centre on
  the highway's bearing through it, `ramp_mouth_side_offset` to the driver's right,
  `ramp_mouth_height` above the plane (ADR 0097: low beside the planet). A ramp may
  use `ramp_turn_share` of the ship's turn rate, more than a highway's
  `road_turn_share`, so its bends are tighter; `make roads` floors each road at its
  own share.

Where a ramp's tube overlaps its host's, the host's wall, roof or floor is open — and
only there.

## Collision

`RoadCollider.hold(prev, next, velocity, basis, half)` once per frame, after the ship
has flown:

- **Inside a tube**: the hull's oriented extents shrink the section; each axis is
  clamped unless the point straight through that wall is inside a neighbour tube —
  and not a SEALED one: a ramp and its host's other carriageway (`Tube.sealed`) clip
  each other's structure but never open a surface between them, because the ramp's
  tail wall is the median. A
  clamped axis reflects the velocity component through it by
  `structure_bounce_restitution` and reports how square the hit was, which the ship
  charges to the throttle once per contact (never below
  `structure_bounce_throttle_floor`). If the held point is no longer inside the tube —
  through an open wall, or past an open end — the hull is in whichever tube contains
  it, or in open space.
- **Outside every tube**: the hull bounces off the exterior of any road it has run into,
  and enters through an open end if that is how it arrived.

The map reads `ship.road.tube` each frame: in a tube with a drive that can run, cruise
is on; crossed into another tube, that is the road now; out through a mouth, cruise
winds down. Nothing chooses a road; the geometry says where the hull is.

## Streaming

`RoadMesh.plan` cuts each road into chunks of `CHUNK_RINGS` rings. `RoadNetwork.stream`
builds the chunks within `road_detail_radius` of the ship on worker threads, nearest
first, and frees the ones beyond 1.5× that. A coarse unclipped version of the whole
road (`build_far`) is visible whenever none of that road's chunks is loaded. Building
a chunk is cheap on a straight and a second or so through a junction, because the
clip subdivides walls to `MIN_CLIP` metres where another tube passes through them.

## Verification

`make check` runs `RoadSuite`: both worlds' networks build with no problems; every
tube contains its own centre-line; a probe flies every carriageway end to end, every
ramp from host to destination or from its planet mouth to where the wormhole takes
over, dives at every wall at every junction edge and bend, and flies drunk on every
tube — and a ray from each step's start to its end must never cross a rendered
triangle, the probe must never be stopped, and there must be structure within 2.5 km
on all four sides while inside a tube. The whole mesh is built for it. The exploration
scene's test then drives the real ship up an on-ramp under its own power and checks
the crossing: the same (t, u, v), speed and heading in the twin, the camera carried
along, and the berth still bound down an off-ramp on the way back out.

`make roads` is the fast loop while authoring. `make shot` with `ROAD_SHOT_SPOT=Exit`
(or `Merge`, `Bend 1`, `Mouth`, `Arrive`, `Spawn`, `Planet`) renders a frame from the seat at
that spot, and `K` in the game drops the ship at the next spot. When the suite reports
a pass-through, `tools/tests/repro_drunk.tscn` flies that one tube in a minute and
names the road and triangle a step crossed (see its header).

## What is deliberately not here

- No physics bodies on the road. The collider is analytic because one rule serves the
  mesh and the collision; nothing forbids collision shapes if that property is kept
  (ADR 0096).
- No curved mesh art yet. The structure is generated from the section; real art
  replaces the generator's strips in place.
- No traffic. Steps 9 and 10 of `EXPLORATION_POC_IMPLEMENTATION.md`.
