class_name HighwayLayout
extends RefCounted
## The road as a list of tubes: two carriageways and the ramps that leave and join
## them. Every tube is a chain of `HighwaySection` tiles; this decides where the
## frames go and which walls are open, and nothing else.
##
## **Build order step 2** (`docs/HIGHWAY_BUILD_ORDER.md`): a road with somewhere to
## go. It bends and climbs, it has a carriageway each way with a median between, and
## every junction has an on-ramp and an off-ramp on each side.
##
## How the pieces are made, all out of the one tile:
##
## - **A spine** of frames: straight junction stretches with bends between them. Each
##   frame is a heading and a pitch and never a roll, so the road keeps the shared
##   horizon (ADR 0045) however it winds.
## - **Two carriageways**, each offset from the spine to its own right — traffic on
##   the right, so the oncoming lane is on your left from either seat. The
##   southbound one is the spine walked backwards and turned round; same code.
## - **An off-ramp** is a taper and a gore. For a few tiles the carriageway's right
##   wall is open and a ramp tile beside it grows out of nothing, its left wall open
##   too; then both walls close and the ramp steps away, leaving a knife-edge nose
##   between them. Then it turns off into open space and ends in a mouth.
## - **An on-ramp** comes in from open space, turning until it touches the
##   carriageway, and from that moment the wall between them is gone: a run
##   alongside, then a taper to nothing into it. No gap and no gore on the way in —
##   a wall between two tubes is open for exactly as long as they touch.
##
## On-ramp before off-ramp at each junction, so the two ramps' tails point away from
## each other and can never cross.
##
## Pure: no scene tree, no tuning, no disk. Every number comes in through `build`.

var routes: Array[HighwayRoute] = []
## The spine, for tests and for anything that wants "the middle of the road".
var spine: Array[Transform3D] = []

var _p: Dictionary = {}


## The basis for a heading and a pitch, with no roll — ever.
static func pose(yaw: float, pitch: float) -> Basis:
	return Basis(Vector3.UP, yaw) * Basis(Vector3.RIGHT, pitch)


## Lay out the whole road. `p` carries every number, already read from tuning:
##
##     tile, width, height, median, junctions, bend_tiles, bend_deg, climb_deg,
##     end_tiles, ramp_width, taper_tiles, diverge_tiles, ramp_gap, tail_tiles,
##     tail_deg, gap_tiles
func build(p: Dictionary) -> void:
	_p = p
	routes.clear()
	var junction_tiles := junction_length_tiles()
	var yaws := PackedFloat32Array()
	var pitches := PackedFloat32Array()
	var zones: Array[int] = []
	_lay_spine(junction_tiles, yaws, pitches, zones)

	var tiles := spine.size() - 1
	var offset := float(p["median"]) * 0.5 + float(p["width"]) * 0.5
	for direction in 2:
		var frames: Array[Transform3D] = []
		var headings := PackedFloat32Array()
		for i in spine.size():
			var j := i if direction == 0 else spine.size() - 1 - i
			var yaw := yaws[j] if direction == 0 else yaws[j] + PI
			var basis := pose(yaw, pitches[j] if direction == 0 else -pitches[j])
			frames.append(Transform3D(basis, spine[j].origin + basis.x * offset))
			headings.append(yaw)
		var name := "northbound" if direction == 0 else "southbound"
		var road := HighwayRoute.new()
		road.name = name
		road.kind = HighwayRoute.Kind.CARRIAGEWAY
		var carriageway_id := routes.size()
		routes.append(road)
		# The same junctions, met in this carriageway's own order.
		var starts: Array[int] = []
		for zone in zones:
			starts.append(zone if direction == 0 else tiles - (zone + junction_tiles))
		starts.sort()
		var open_right := {}
		var number := 0
		for zone in starts:
			number += 1
			_lay_junction(carriageway_id, frames, headings, zone, number, open_right)
		for i in tiles:
			var opening := HighwaySection.Wall.RIGHT if open_right.has(i) else 0
			road.sections.append(_tile(frames[i], frames[i + 1],
				float(p["width"]) * 0.5, float(p["width"]) * 0.5, opening, i, name,
				carriageway_id))
		road.finish()
	# The carriageways are filled in after the ramps were appended, so distances down
	# them only exist now; the ramps were told which tile they join at, and turn that
	# into a distance here.
	for route in routes:
		if route.kind != HighwayRoute.Kind.CARRIAGEWAY:
			route.joins_at = routes[route.carriageway].distance_to_section(
				int(route.joins_at))
			route.finish()


## Tiles of straight road a junction needs: an on-ramp's converge and taper, a gap,
## an off-ramp's taper and diverge, and one tile of plain road at each end.
func junction_length_tiles() -> int:
	return 2 * (int(_p["taper_tiles"]) + int(_p["diverge_tiles"])) \
		+ int(_p["gap_tiles"]) + 2


func _lay_spine(junction_tiles: int, yaws: PackedFloat32Array,
		pitches: PackedFloat32Array, zones: Array[int]) -> void:
	spine.clear()
	var plan: Array[Vector2] = []   # (turn, climb) in radians, per tile
	for _i in int(_p["end_tiles"]):
		plan.append(Vector2.ZERO)
	var junctions := maxi(int(_p["junctions"]), 1)
	for k in junctions:
		zones.append(plan.size())
		for _i in junction_tiles:
			plan.append(Vector2.ZERO)
		if k == junctions - 1:
			break
		# Bends alternate left and right, and humps alternate up and down, so the road
		# winds and rolls without ever drifting off in one direction.
		var turn := deg_to_rad(float(_p["bend_deg"])) * (1.0 if k % 2 == 0 else -1.0)
		var climb := deg_to_rad(float(_p["climb_deg"])) * (1.0 if k % 4 < 2 else -1.0)
		var n := int(_p["bend_tiles"])
		for i in n:
			var hump := climb if i < n / 2 else (-climb if i >= n - n / 2 else 0.0)
			plan.append(Vector2(turn, hump))
	for _i in int(_p["end_tiles"]):
		plan.append(Vector2.ZERO)

	var yaw := 0.0
	var pitch := 0.0
	var at := Vector3.ZERO
	var tile := float(_p["tile"])
	spine.append(Transform3D(pose(yaw, pitch), at))
	yaws.append(yaw)
	pitches.append(pitch)
	for step in plan:
		var next_yaw := yaw + step.x
		var next_pitch := pitch + step.y
		# Advanced along the MID heading, so a tile's chord runs between the two
		# frames it joins rather than off one of them.
		at += pose((yaw + next_yaw) * 0.5, (pitch + next_pitch) * 0.5) \
			* Vector3.FORWARD * tile
		yaw = next_yaw
		pitch = next_pitch
		spine.append(Transform3D(pose(yaw, pitch), at))
		yaws.append(yaw)
		pitches.append(pitch)


## One junction on one carriageway, starting at tile `zone`: an on-ramp, then an
## off-ramp. Marks the carriageway tiles whose right wall the ramps open.
func _lay_junction(carriageway_id: int, frames: Array[Transform3D],
		headings: PackedFloat32Array, zone: int, number: int,
		open_right: Dictionary) -> void:
	var taper := int(_p["taper_tiles"])
	var diverge := int(_p["diverge_tiles"])
	var road_half := float(_p["width"]) * 0.5
	var ramp_half := float(_p["ramp_width"]) * 0.5
	var gap := float(_p["ramp_gap"])
	var road_name := routes[carriageway_id].name

	# --- on-ramp: in from space, touching the road, open, tapering into it -------
	# No gap on the way in. The rule is that the wall between two tubes is open for
	# exactly as long as they touch (the human, 2026-09-21: "I should be able to get
	# on as long as they are touching"), and an entrance that closed on the road
	# across a gap had a stretch of wall where the two looked joined and were not.
	# So the tail turns in until it touches, and from that frame to the end of the
	# taper — `diverge` tiles alongside, then `taper` tiles narrowing — there is no
	# wall between them at all. The exit keeps its gore: there the walls close at
	# exactly the point the tubes stop touching, which is the same rule.
	var join := zone + 1
	var on_frames: Array[Transform3D] = []
	var on_halves := PackedFloat32Array()
	for k in diverge + 1:
		on_frames.append(_beside(frames[join + k], road_half + ramp_half))
		on_halves.append(ramp_half)
	for k in range(1, taper + 1):
		var half := ramp_half * (1.0 - float(k) / float(taper))
		on_frames.append(_beside(frames[join + diverge + k], road_half + half))
		on_halves.append(half)
	for k in diverge + taper:
		open_right[join + k] = true
	# The tail ends ON the first converge frame, so that frame is shared rather than
	# repeated — a repeated frame is a tile of zero length, and a wall of zero area.
	var tail := _tail(on_frames[0], headings[join], -1.0)
	var on := _ramp_route("%s on-ramp %d" % [road_name, number],
		HighwayRoute.Kind.ON_RAMP, carriageway_id, join)
	var all_frames: Array[Transform3D] = tail.duplicate()
	all_frames.append_array(on_frames.slice(1))
	var all_halves := PackedFloat32Array()
	for _i in tail.size():
		all_halves.append(ramp_half)
	all_halves.append_array(on_halves.slice(1))
	var first_touch := tail.size() - 1
	_fill_ramp(on, all_frames, all_halves, first_touch, first_touch + diverge + taper)

	# --- off-ramp: growing out of the road, a gore, then away into space ------------
	var leave := join + diverge + taper + int(_p["gap_tiles"])
	var off_frames: Array[Transform3D] = []
	var off_halves := PackedFloat32Array()
	for k in taper + 1:
		var half := ramp_half * float(k) / float(taper)
		off_frames.append(_beside(frames[leave + k], road_half + half))
		off_halves.append(half)
	for k in range(1, diverge + 1):
		var edge := gap * float(k) / float(diverge)
		off_frames.append(_beside(frames[leave + taper + k],
			road_half + edge + ramp_half))
		off_halves.append(ramp_half)
	for k in taper:
		open_right[leave + k] = true
	var away := _tail(off_frames[off_frames.size() - 1],
		headings[leave + taper + diverge], 1.0)
	off_frames.append_array(away.slice(1))
	for _i in away.size() - 1:
		off_halves.append(ramp_half)
	var off := _ramp_route("%s off-ramp %d" % [road_name, number],
		HighwayRoute.Kind.OFF_RAMP, carriageway_id, leave)
	_fill_ramp(off, off_frames, off_halves, 0, taper)


## A frame beside a carriageway frame, `across` metres to its right, facing the same
## way. Ramp tiles next to the road are built from these, so where a ramp runs beside
## the carriageway their seams line up exactly and the shared edge is one edge.
func _beside(frame: Transform3D, across: float) -> Transform3D:
	return Transform3D(frame.basis, frame.origin + frame.basis.x * across)


## The part of a ramp that turns off into open space. `sense` +1 builds forward from
## `from`, turning right, away from the road; -1 builds backward from it, so that
## driven forward it arrives from the right and straightens up. Returns the frames in
## travel order, `from` included.
func _tail(from: Transform3D, heading: float, sense: float) -> Array[Transform3D]:
	var tile := float(_p["tile"])
	var bend := deg_to_rad(float(_p["tail_deg"]))
	var out: Array[Transform3D] = [from]
	var at := from.origin
	var yaw := heading
	for _i in int(_p["tail_tiles"]):
		var next := yaw - bend * sense
		var step := pose((yaw + next) * 0.5, 0.0) * Vector3.FORWARD * tile
		at += step * sense
		yaw = next
		var frame := Transform3D(pose(yaw, 0.0), at)
		if sense > 0.0:
			out.append(frame)
		else:
			out.push_front(frame)
	return out


func _ramp_route(called: String, kind: HighwayRoute.Kind, carriageway_id: int,
		joins_tile: int) -> HighwayRoute:
	var route := HighwayRoute.new()
	route.name = called
	route.kind = kind
	route.carriageway = carriageway_id
	# A tile index for now; `build` turns it into a distance once the carriageway
	# has its tiles.
	route.joins_at = float(joins_tile)
	routes.append(route)
	return route


## Tiles between consecutive frames. Tiles `open_from` to `open_to` have their left
## wall open: that is the stretch that runs alongside the carriageway with nothing
## between them.
func _fill_ramp(route: HighwayRoute, frames: Array[Transform3D],
		halves: PackedFloat32Array, open_from: int, open_to: int) -> void:
	var id := routes.find(route)
	for i in frames.size() - 1:
		var opening := HighwaySection.Wall.LEFT if i >= open_from and i < open_to else 0
		route.sections.append(_tile(frames[i], frames[i + 1], halves[i],
			halves[i + 1], opening, i, route.name, id))


func _tile(from: Transform3D, to: Transform3D, from_half: float, to_half: float,
		opening: int, at: int, called: String, route_id: int) -> HighwaySection:
	var section := HighwaySection.make(from, to, from_half, to_half,
		float(_p["height"]), opening, at, called)
	section.route_id = route_id
	return section


## The tightest bend on the road, as a radius. EXPLORATION_DESIGN.md invariant 1: a
## road may not bend tighter than its carriageways are offset from the spine, or the
## inner one folds through itself.
func tightest_radius() -> float:
	var bend := deg_to_rad(float(_p["bend_deg"]))
	return INF if bend <= 0.0 else float(_p["tile"]) / bend
