class_name WormholeLayout
extends RefCounted
## The wormhole highway's geometry, derived from the map (`docs/WORMHOLE_PROTOTYPE.md`).
## Pure: no scene tree, no disk. Tuning is read for the keys named below.
##
## Each highway in `data/routes.json` is a list of systems and nothing else: there is
## no road between them in open space any more. Inside the wormhole each highway is a
## compressed road whose nodes are its systems in order, at the map's bearings, each
## leg the world leg times `wormhole_scale` clamped to between
## `wormhole_exit_min_seconds` and `wormhole_exit_max_seconds` of travel at
## `cruise_speed`. It dead-ends `wormhole_end_run` past its first and last node: the
## last exit is the final off-ramp. Each highway sits in its own pocket of the
## wormhole, far from the others and from the map, so nothing is ever seen across.
##
## Ramps exist in both worlds. They are built once in the wormhole, against the
## carriageway they join (`RoadNetwork._wormhole_ramp`), and their twins are placed in
## open space by the rigid move that puts a ramp's end at the planet's mouth
## (`space_mouth`). A system at the end of a highway gets two ramps, an on-ramp onto
## the carriageway leaving it and an off-ramp from the one arriving; a system in the
## middle gets four.

## Where the wormhole is, in the map's frame, and how far apart its pockets are.
## Infrastructure, not feel: nothing is seen across it. Below the map, at the same
## order of distance as the map's own far corner, so the precision is the map's.
const ORIGIN := Vector3(0.0, -200000.0, 0.0)
const POCKET_SPACING := 200000.0


## The route data for the wormhole's `RoadNetwork.build`, generated from the map's:
## `{highways: [{name, systems, closed, points, radii}], planet_ramps: [{highway,
## system, sides, kinds, at}], legs: [{highway, from, to, world, metres, seconds}]}`.
## Points are arrays of floats, the shape the JSON has, so one build path serves both.
static func lay(data: Dictionary, positions: Dictionary) -> Dictionary:
	var speed := Tuning.num("exploration/cruise_speed")
	var scale := Tuning.num("exploration/wormhole_scale")
	var shortest := Tuning.num("exploration/wormhole_exit_min_seconds") * speed
	var longest := maxf(Tuning.num("exploration/wormhole_exit_max_seconds") * speed, shortest)
	var end_run := Tuning.num("exploration/wormhole_end_run")
	# A bend a highway may ask of the ship, with a little in hand (`RoadNetwork._validate`).
	var radius := 1.1 * speed / (clampf(Tuning.num("exploration/road_turn_share"), 0.05, 1.0)
		* deg_to_rad(Tuning.num("exploration/cruise_turn_rate_deg_per_sec")))
	var highways: Array = []
	var ramps: Array = []
	var legs: Array = []
	var nodes_of := {}
	var pocket := 0
	for h: Dictionary in data.get("highways", []):
		var name := String(h.get("name", ""))
		var systems: Array = h.get("systems", [])
		if systems.size() < 2:
			continue
		var origin := ORIGIN + Vector3(POCKET_SPACING * pocket, 0.0, 0.0)
		pocket += 1
		var nodes: Array[Vector3] = [origin]
		var bearings: Array[Vector3] = []
		for i in systems.size() - 1:
			var a: Vector3 = positions.get(String(systems[i]), Vector3.ZERO)
			var b: Vector3 = positions.get(String(systems[i + 1]), Vector3.ZERO)
			var bearing := _level(b - a)
			var world := Vector2(b.x - a.x, b.z - a.z).length()
			var metres := clampf(world * scale, shortest, longest)
			bearings.append(bearing)
			nodes.append(nodes[i] + bearing * metres)
			legs.append({"highway": name, "from": String(systems[i]), "to": String(systems[i + 1]),
				"world": world, "metres": metres, "seconds": metres / maxf(speed, 0.01)})
		var points: Array = []
		var radii: Array = []
		points.append(_triple(nodes[0] - bearings[0] * end_run))
		radii.append(0.0)
		for n in nodes:
			points.append(_triple(n))
			radii.append(radius)
		points.append(_triple(nodes[nodes.size() - 1] + bearings[bearings.size() - 1] * end_run))
		radii.append(0.0)
		var made := {"name": name, "systems": systems.duplicate(), "closed": false,
			"points": points, "radii": radii}
		highways.append(made)
		var by_system := {}
		for i in systems.size():
			by_system[String(systems[i])] = nodes[i]
		nodes_of[name] = by_system
	for r: Dictionary in data.get("planet_ramps", []):
		var highway := String(r.get("highway", ""))
		var system := String(r.get("system", ""))
		if not nodes_of.has(highway) or not (nodes_of[highway] as Dictionary).has(system):
			# Left for the network to report as it does today.
			ramps.append(r.duplicate(true))
			continue
		var order: Array = []
		for h: Dictionary in highways:
			if String(h["name"]) == highway:
				order = h["systems"]
		var index := order.find(system)
		var at: Vector3 = (nodes_of[highway] as Dictionary)[system]
		for side: String in r.get("sides", ["R", "L"]):
			for kind: String in r.get("kinds", ["exit", "entry"]):
				if not wanted(index, order.size(), side, kind):
					continue
				ramps.append({"highway": highway, "system": system, "sides": [side],
					"kinds": [kind], "at": _triple(at)})
	return {"highways": highways, "planet_ramps": ramps, "legs": legs}


## Whether a ramp makes sense: the R carriageway travels the systems in order and the
## L one back, so the first system has no exit from R and no entry onto L, and the
## last has no entry onto R and no exit from L. A ramp onto a dead end is a trap.
static func wanted(index: int, count: int, side: String, kind: String) -> bool:
	var first := index == 0
	var last := index == count - 1
	if side == "R":
		return not (first and kind == "exit") and not (last and kind == "entry")
	return not (first and kind == "entry") and not (last and kind == "exit")


## The highway's direction of travel (in system order) where it passes a system: the
## bearing of the leg at an end, the bisector of the two legs in the middle. Level.
static func bearing_at(data: Dictionary, positions: Dictionary, highway: String,
		system: String) -> Vector3:
	for h: Dictionary in data.get("highways", []):
		if String(h.get("name", "")) != highway:
			continue
		var systems: Array = h.get("systems", [])
		var i := systems.find(system)
		if i < 0 or systems.size() < 2:
			return Vector3.FORWARD
		var here: Vector3 = positions.get(system, Vector3.ZERO)
		var out := Vector3.ZERO
		if i > 0:
			out += _level(here - positions.get(String(systems[i - 1]), Vector3.ZERO))
		if i < systems.size() - 1:
			out += _level(positions.get(String(systems[i + 1]), Vector3.ZERO) - here)
		return _level(out) if out.length_squared() > 1e-6 else Vector3.FORWARD
	return Vector3.FORWARD


## Where a ramp meets open space beside a planet, and which way its traffic travels
## there: `{pos, fwd}`. The same rule the road used when it ran through the system:
## the mouth sits `ramp_mouth_side_offset` to the driver's right of the highway's
## line through the system's centre, `ramp_mouth_along_offset` short of the centre
## for an exit or past it for an entry, `ramp_mouth_height` above the plane, with a
## highway's own `mouth_along` and `mouth_height` overriding the two keys. `direction`
## is the carriageway's, +1 with the system order and -1 against it.
static func space_mouth(data: Dictionary, positions: Dictionary, highway: String,
		system: String, direction: int, kind: String) -> Dictionary:
	var along := Tuning.num("exploration/ramp_mouth_along_offset")
	var height := Tuning.num("exploration/ramp_mouth_height")
	for h: Dictionary in data.get("highways", []):
		if String(h.get("name", "")) == highway:
			along = float(h.get("mouth_along", along))
			height = float(h.get("mouth_height", height))
	var fwd := bearing_at(data, positions, highway, system) * float(direction)
	var right := fwd.cross(Vector3.UP)
	var centre: Vector3 = positions.get(system, Vector3.ZERO)
	var pos := centre + fwd * (-along if kind == "exit" else along) \
		+ right * Tuning.num("exploration/ramp_mouth_side_offset") + Vector3.UP * height
	return {"pos": pos, "fwd": fwd}


static func _level(v: Vector3) -> Vector3:
	var flat := Vector3(v.x, 0.0, v.z)
	return flat.normalized() if flat.length_squared() > 1e-9 else Vector3.FORWARD


static func _triple(v: Vector3) -> Array:
	return [v.x, v.y, v.z]
