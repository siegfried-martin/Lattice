# Sectors, a highway gear, and a bigger map — a prototype plan

*Written 2026-09-19 from the human's direction. This is a test, not a decision. Each
piece is built behind a flag so the current game is the control, and nothing here
gets an ADR until it has been flown and kept. Expect to change any of it.*

## What we are trying to find out

Three things about the exploration layer have not settled after several passes:

1. **The world reads small in the distance**, even though travel times feel about
   right. One compression curve on the far layer helped; it was not enough.
2. **The highway cannot be balanced with one number.** Lower travel time, greater
   distance between systems and a calmer felt speed pull on the same key,
   `cruise_speed`. Each pass has traded one for another.
3. **The road looks like more structure than a system could build.** Part of this is
   scale (a 300 m planet is an asteroid), part is how much structure is visible.

The three prototypes below each aim at one of these. They also fit together, and
prototype 3 only makes sense with the first two in place.

## Prototype 1 — Hex sectors

### The idea

The playable world is a tiling of hexagonal sectors with a single outer border. The
discs, corridors, funnels and apertures go away; inside the border, space is open. A
sector may hold a star and planets or may be empty. The map, later, would mark the
ones with bodies.

A sector is also what drives how far things are drawn, and at what size. A body is
drawn according to which sector it is in, relative to the player's:

| Body is in | Drawn as |
|---|---|
| the player's sector | near perspective; the thing you fly around |
| a neighbouring sector | visible, on a steep distance curve: it grows as you head for it |
| two sectors away | a point of light, present but plainly far |
| further | not drawn |

The point is legibility, not a render budget. The size a body is drawn at tells the
player which tier it is in, and therefore whether it is somewhere they can go now,
next, or later. Distance alone cannot say that; the sector can.

### The crossing is an event

A body never changes sector, so the only moment the tiers change is when the player
crosses a hex edge. Rather than snap, every body tweens to its new size over about a
second while the HUD shows the sector's name. The re-read of the world and the sign
arrive together, like a state line on a road trip. The alternative, blending tiers as
the player nears an edge, was set aside because it muddies the picture at the point
where the player is choosing a direction. It can be tried if the event version feels
wrong.

The road's far representation (the line of lights between systems) follows the same
tiering, so it visibly leads into the next sector.

### Knobs, all in `tuning.cfg` under a new `;;; Sectors` group

- `sectors_enabled` — the flag. Off is today's discs and corridors.
- `sector_radius` — centre to vertex. Starts at 20 km, near the A to B spacing.
- `sector_next_power`, `sector_far_power` — the far layer's compression for the
  next tier and the one past it. The existing `far_compress_power` is the home value.
- `sector_far_rings` — how many rings out are drawn at all. Start at 2.
- `sector_crossing_seconds` — the tween. Start at 1.0.
- `sector_banner_seconds` — how long the name stays on the HUD.
- `sector_border_margin` — how far past the farthest system or highway point the
  one border reaches.

### What to fly, and what we are watching

- Fly from A to B on the road and off it. Does the world read as large? Is the
  crossing an event or an interruption?
- Look at C from A. Does "a point of light two sectors away" read as far, or as
  broken?
- Turn toward a neighbouring sector's planet from open space. Does it grow the way a
  destination should?
- Does the open world, with no corridor, make off-road travel feel possible but slow,
  which is what the road has to beat?

### What this does not decide

Whether sectors carry gameplay (ownership, hazards, traffic), what the map shows, or
whether the tiers should be three. It also does not touch the combat arena.

## Prototype 2 — The highway gear

### The idea

On the road, the lane carries the ship along the tube axis at `cruise_speed`. A
gear multiplies that push and nothing else:

- **Forward motion through the world** is `cruise_speed × highway_gear` between
  systems. Near ramps and mouths the gear is 1, so everything built there is
  unchanged.
- **Everything the player steers against stays at the felt speed.** Lateral response,
  the steering cone and the collider run at `cruise_speed`. The ribs, lamps and
  collars are spaced in road units and stretched by the gear in the world, so they
  pass at the felt rate.
- **The world outside passes at the geared rate.** Planets and the far layer go by
  quickly; the road itself does not feel faster.

Seen from outside, ships on the road move at the geared speed and the structure is
sparser. This is the "system whizzing past while you move at a moderate pace" picture
without a second scene, a constructed backdrop, or a seam at the portal. Every bullet
of ADR 0057 still holds as written, and a ship that leaves the lane mid-route is
simply where it is.

This is the same-scene answer to the compressed highway. The separate-scene versions
(a hyperspace road, or a second scaled copy of the map behind a window) were set
aside for now because they give up the road being in the same space as everything
else, which is the part of the highway that has worked. If the gear does not deliver,
they are still on the table, and ADR 0057 would be superseded with the evidence.

### Knobs

- `highway_gear` — start at 1.0, which is today. Try 3 and 5.
- `highway_gear_shift_metres` — over how many metres past a ramp the gear ramps from
  1 to full. Start at 2000.

The felt-speed knob is still `cruise_speed`. Lower it while raising the gear to see
where calm and quick meet.

### What we are watching

- Does a felt speed around 200 with a gear of 5 read as calm while the system
  passes? Or does the outside moving faster than the inside read as wrong?
- Does the stretched structure look built rather than thin?
- Do bends still feel right? The lane carries the heading, so the road's bends should
  cost nothing, but the road validator's turn floors are written against
  `cruise_speed` and may need the geared speed near mid-route bends. Check with
  `make roads` before flying.

## Prototype 3 — A bigger map with a matching gear

With sectors drawing the distance and the gear decoupling travel time from it, the
map can be spread out. Triple the system spacing in `data/routes.json` with a gear of
3 for the same travel time, and see whether systems now read as far apart and the
road as proportionate.

This is the piece that only makes sense after 1 and 2 have been flown.

## What is built (2026-09-19)

Prototypes 1 and 2, **on by default** since the first flight: this is what is being
tested, so the flags default to the test. `sectors_enabled` off and `highway_gear` 1
are the control.

- **Sectors**: `HexGrid` (the tiling, pure), `HexRegion` (the border, one region in
  the field), `SectorLayer` (the ship's cell, the tiers, the crossing tween and the
  sign). `SystemMap.relayout` hides the discs and corridors and lays the border
  when `sectors_enabled` is on; `_compress_the_distance` scales planets and stars by
  tier and hands the far shaders the same picture as global uniforms, so the road's
  far mesh and markings tier with the bodies. The sign is a label in the exploration
  scene; the `sector` HUD row says which cell you are in and how many crossings.
- **The gear**: a property of the road, `highway_gear` on a highway and 1 on a ramp,
  blended on the ship over `highway_gear_shift_seconds`. `docs/HIGHWAY.md` has the
  mechanism. The `gear` HUD row shows the applied gear, the felt and world speeds.

After the first flight (2026-09-19), three changes from the human's notes:

- The gear used to shift by distance, to 1 at every junction and back over 2 km, so
  on a map with a junction every few km the world only sped up between them and the
  shifting itself was what you noticed. Now the gear is the road's and the ship
  shifts in time when it changes tube. Entering or leaving at the geared speed is
  not possible and is not meant to be; the shift is the cost of a ramp.
- Ramps have a speed limit, `ramp_speed`, and their bends are floored at it. With
  that, the planet ramps are 0.9 to 1.2 km (were 2.4 to 3.1) and the interchange
  ramps 10 to 11 km (were 17), on `ramp_bend_radius` 220 at 60°, leads of 150 m, a
  peel of 400 m at 20°, and mouths 300 m above the highway. Still not the 5× asked
  for on the interchange; its shape is authored in `data/routes.json` and can be
  pulled in further once the shorter planet ramps have been flown.
- The chase camera's lag was against the geared motion, so the boom stretched in
  the gear. It now carries the gear's displacement rigidly and lags the felt motion.

After the second flight (2026-09-19), from the human's notes: the highway felt fast
and exits were hard to take; the merge upshift was near instant; the far camera of
the stretched boom had read right.

- The cause of the hard exits: the highway is in gear through the junction, so at
  1250 m/s a 400 m opening passes in a third of a second. Now the ship shifts down
  when it is lined up for an exit (on the right, opening within
  `exit_downshift_seconds` ahead at world speed) or has taken it from the strip.
- Two shift times: `highway_upshift_seconds` 6 and `highway_downshift_seconds` 2.5.
- Felt `cruise_speed` 120 with `highway_gear` 10, so the world goes by about as fast
  as before while the inside is the lazy drive asked for. `ramp_speed` 80.
- `road_boom_scale` 4 puts the camera further back on the road.

After the third flight (2026-09-19): the human could not see the outside moving
faster than the inside, and was right. Spacing the ribs by the gear made one pass
every few seconds, but each rib, lamp and floor line was a fixed thing in the
world and swept past at world speed; nothing near the ship moved at the felt
speed, which is also why the speed read as high. Now the road is a treadmill: the
ridden road's ribs and lamp bars slide along with the ship by the gear's surplus,
and the lane paint's inner lines are dashed and scroll the same way, so what is
near the ship passes at the felt speed and the planets and far road pass at the
geared one. `docs/HIGHWAY.md` has the mechanism. Also: the camera change was
not asked for and `road_boom_scale` is back to 1; the exit opening is longer
(`ramp_exit_length` 800 at 12°) so there is more of it to steer into once out of
gear.

Known edges, left for the flying to judge:

- Fuel burns on world metres, so a leg in the gear costs the same fuel as before
  while taking less time. If the gear stays, decide whether fuel is per road metre.
- The gate's probe flies at the lane's speed without the gear.
- The deep field is scattered near the roads and rejected inside playable space, so
  with one border it is mostly below the floor.
- The border has no mesh yet. The HUD's `bounds` row and the speed clamp work at it,
  but nothing reddens on the way out, since the red faces belong to the hidden discs.
  Worth a mesh if the open world stays.
- Exits pitch up to 45° in their S-bend, since the rise is the same over a shorter
  run. `ramp_bend_deg` and `ramp_mouth_height` are the knobs if that reads wrong.

## Order, and how each one ends

Build 1 and 2 behind their flags, flags off, so `make check` passes as today and the
road suite is unchanged. Turn them on in the F2 panel and fly. Then 3, as an edit to
the map data.

Each prototype ends one of three ways: it is kept and gets an ADR with what was
flown; it is changed and flown again; or it is removed and the flag with it. The
write-up of what was seen goes in `STATUS.md` either way.

## Parked

- Whether the road's *near* structure should also thin out (Freelancer's rings rather
  than a continuous tube) to address the material question directly. The tube is the
  core requirement and stays; this is about how much of it is drawn.
- The fiction of who built the road, which is a cheaper answer to "no system could
  build this" than any geometry.
- What a sector boundary means for the boundary field's out-of-bounds rules. With one
  outer border, the existing rules apply there and nowhere else.
