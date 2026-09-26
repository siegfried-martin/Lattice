extends Node
## Shared world layout used by both the open-space scene and the highway scene: hex sectors,
## star systems (planets, moons, hop lanes, asteroid clusters) and the highway network with
## its links and gates. Generated deterministically at startup.

const SQRT3 := 1.7320508075688772

# Sectors are pointy-top hexagonal prisms, much wider than they are tall.
const HEX_R := 10000.0
const HEX_APOTHEM := HEX_R * SQRT3 * 0.5
const SECTOR_HALF_H := 3000.0
const COLS := 4
const ROWS := 4
const ROW_NAMES := ["A", "B", "C", "D"]

# Highway network: a scaled-down replica of the map, shaped like a cent sign. HWY 1 is the C
# (open to the east, ending in exits); HWY 2 is the vertical stroke, ending in T-junctions on
# HWY 1 at both ends.
const HW_SCALE := 1.0 / 20.0
# The road's cross-section and the gate frames are sized for the freighter, and grow with it.
# Lengths along the road grow less, so HWY 2's junctions and interchange still fit on it.
const ROAD_SCALE := ShipMesh.FREIGHTER_SCALE
const LANE_W := 10.0 * ROAD_SCALE
const LANES := 3
const MEDIAN := 4.0 * ROAD_SCALE
const SHOULDER := 3.0 * ROAD_SCALE
const CARR_W := LANE_W * LANES
const CARR_CENTER := MEDIAN + CARR_W * 0.5
const RIGHT_LANE_LAT := CARR_W * 0.5 - LANE_W * 0.5
const ROAD_HALF_W := MEDIAN + CARR_W + SHOULDER
const RAMP_LEN := 260.0
const RAMP_SHIFT := 24.0 * ROAD_SCALE
const RAMP_HALF_W := LANE_W * 0.5 + 2.0 * ROAD_SCALE
const GATE_W := 30.0 * ROAD_SCALE
const GATE_H := 20.0 * ROAD_SCALE
const GATE_Y := GATE_H * 0.5
const HOP_GATE_W := 60.0 * ROAD_SCALE
const HOP_GATE_H := 40.0 * ROAD_SCALE
const INTERCHANGE_GAP := 60.0  # exit and entrance gates sit this far either side of an interchange
const TERMINAL_GATE_U := 20.0 * ROAD_SCALE  # dead-end gates sit this far in from the road end
const JUNCTION_GAP := 150.0    # HWY 2 stops this far short of HWY 1's centreline
const JUNCTION_SPAN := 170.0   # turn links fork / merge this far from the junction

const C_CENTER := Vector2(30000.0, 22500.0)
const C_RX := 21000.0
const C_RZ := 17000.0
const C_END_X := 57000.0

const ROAD_NAMES := ["HWY 1", "HWY 2"]
const SYSTEM_NAMES := ["Vesper", "Kharon", "Ossia", "Tethra", "Maru", "Calyx", "Ione"]
const ROMAN := ["I", "II", "III", "IV", "V"]
const PALETTES := [
	[Color(0.55, 0.35, 0.22), Color(0.78, 0.6, 0.4), Color(0.95, 0.9, 0.8), Color(1.0, 0.7, 0.4)],
	[Color(0.08, 0.2, 0.45), Color(0.15, 0.45, 0.3), Color(0.9, 0.95, 1.0), Color(0.4, 0.7, 1.0)],
	[Color(0.3, 0.12, 0.35), Color(0.65, 0.3, 0.55), Color(0.95, 0.75, 0.9), Color(0.9, 0.4, 1.0)],
	[Color(0.35, 0.38, 0.4), Color(0.6, 0.6, 0.58), Color(0.85, 0.85, 0.8), Color(0.6, 0.8, 0.9)],
	[Color(0.5, 0.15, 0.08), Color(0.85, 0.4, 0.15), Color(1.0, 0.8, 0.5), Color(1.0, 0.45, 0.2)],
	[Color(0.1, 0.3, 0.35), Color(0.3, 0.65, 0.6), Color(0.85, 1.0, 0.95), Color(0.4, 1.0, 0.85)],
]

var sectors := {}          # Vector2i -> {coord, name, center, bodies, gates}
var roads: Array = []      # {id, name, track}
var junctions: Array = []  # {u (on HWY 1), v_s (HWY 2 end), v_in (dir arriving), v_out (dir leaving)}
var links: Array = []      # ramps and junction turns
var gates: Array = []      # highway entrances / exits and hop gates
var events := {}           # Vector2i(road, d) -> [{u, text, kind}] in travel order
var boundaries: Array = [] # {road, u, a, b}: road crosses from sector a to b going +u
var systems: Array = []
var bodies: Array = []     # planets and moons
var hops: Array = []
var clusters: Array = []   # asteroid clusters
var start := {}            # {gate, pos}


func _init() -> void:
	for row in ROWS:
		for col in COLS:
			var c := Vector2i(col, row)
			sectors[c] = {"coord": c, "name": sector_name(c), "center": sector_center(c), "bodies": [], "gates": []}
	var rng := RandomNumberGenerator.new()
	rng.seed = 20260923
	_build_roads()
	_build_boundaries()
	_build_junctions()
	_build_systems(rng)
	_build_events()
	_build_hops()
	_build_clusters(rng)
	print("Galaxy: HWY 1 %.0f m, HWY 2 %.0f m, %d links, %d gates, %d systems, %d bodies, %d hops, %d clusters" % [
		roads[0].track.length, roads[1].track.length, links.size(), gates.size(), systems.size(), bodies.size(), hops.size(), clusters.size()])


# --- Hex math -----------------------------------------------------------------------------

func sector_name(c: Vector2i) -> String:
	return "%s%d" % [ROW_NAMES[c.y], c.x + 1] if is_valid(c) else "--"


func sector_center(c: Vector2i) -> Vector3:
	return Vector3(SQRT3 * HEX_R * (c.x + 0.5 * (c.y & 1)), 0.0, 1.5 * HEX_R * c.y)


func is_valid(c: Vector2i) -> bool:
	return c.x >= 0 and c.x < COLS and c.y >= 0 and c.y < ROWS


func hex_of(p: Vector3) -> Vector2i:
	var q := (SQRT3 / 3.0 * p.x - p.z / 3.0) / HEX_R
	var r := (2.0 / 3.0 * p.z) / HEX_R
	var x := q
	var z := r
	var y := -x - z
	var rx := roundf(x)
	var ry := roundf(y)
	var rz := roundf(z)
	var dx := absf(rx - x)
	var dy := absf(ry - y)
	var dz := absf(rz - z)
	if dx > dy and dx > dz:
		rx = -ry - rz
	elif dy > dz:
		ry = -rx - rz
	else:
		rz = -rx - ry
	var aq := int(rx)
	var ar := int(rz)
	return Vector2i(aq + (ar - (ar & 1)) / 2, ar)


func hex_distance(a: Vector2i, b: Vector2i) -> int:
	var aq := a.x - (a.y - (a.y & 1)) / 2
	var bq := b.x - (b.y - (b.y & 1)) / 2
	var dq := aq - bq
	var dr := a.y - b.y
	return (absi(dq) + absi(dr) + absi(dq + dr)) / 2


## Horizontal signed distance from p to the hex of sector c (negative inside).
func hex_sdf(p: Vector3, c: Vector2i) -> float:
	var cc := sector_center(c)
	var d := Vector2(p.x - cc.x, p.z - cc.z)
	var m := 0.0
	for k in 3:
		var a := deg_to_rad(60.0 * k)
		m = maxf(m, absf(d.dot(Vector2(cos(a), sin(a)))))
	return m - HEX_APOTHEM


func compass(dir: Vector3) -> String:
	var a := fposmod(rad_to_deg(atan2(dir.x, -dir.z)), 360.0)
	return ["NORTH", "NORTHEAST", "EAST", "SOUTHEAST", "SOUTH", "SOUTHWEST", "WEST", "NORTHWEST"][int(roundf(a / 45.0)) % 8]


# --- Roads --------------------------------------------------------------------------------

func _build_roads() -> void:
	var top_z := C_CENTER.y - C_RZ
	var bot_z := C_CENTER.y + C_RZ
	var step := 30.0 / HW_SCALE
	var pts: Array = []
	_sample_line(pts, Vector2(C_END_X, top_z), Vector2(C_CENTER.x, top_z), step, true)
	var n := 240
	for i in range(1, n + 1):
		var phi := PI * i / n
		pts.append(Vector2(C_CENTER.x - C_RX * sin(phi), C_CENTER.y - C_RZ * cos(phi)))
	_sample_line(pts, Vector2(C_CENTER.x, bot_z), Vector2(C_END_X, bot_z), step, false)
	var v_pts: Array = []
	var gap := JUNCTION_GAP / HW_SCALE
	_sample_line(v_pts, Vector2(C_CENTER.x, top_z + gap), Vector2(C_CENTER.x, bot_z - gap), step, true)
	roads = [
		{"id": 0, "name": ROAD_NAMES[0], "track": Track.new(_to_hw(pts), false)},
		{"id": 1, "name": ROAD_NAMES[1], "track": Track.new(_to_hw(v_pts), false)},
	]
	var C: Track = roads[0].track
	var V: Track = roads[1].track
	junctions = [
		{"u": C.project_global(_to_hw([Vector2(C_CENTER.x, top_z)])[0]), "v_s": 0.0, "v_in": -1, "v_out": 1},
		{"u": C.project_global(_to_hw([Vector2(C_CENTER.x, bot_z)])[0]), "v_s": V.length, "v_in": 1, "v_out": -1},
	]


func _sample_line(pts: Array, a: Vector2, b: Vector2, step: float, include_start: bool) -> void:
	var n := maxi(1, int(ceil(a.distance_to(b) / step)))
	for i in range(0 if include_start else 1, n + 1):
		pts.append(a.lerp(b, float(i) / n))


func _to_hw(pts: Array) -> PackedVector3Array:
	var out := PackedVector3Array()
	for p in pts:
		out.append(Vector3(p.x * HW_SCALE, 0.0, p.y * HW_SCALE))
	return out


func road_track(road: int) -> Track:
	return roads[road].track


## Point on carriageway (road, d): lat is metres right of the carriageway centre.
func carr_point(road: int, d: int, u: float, lat: float, y := 0.0) -> Vector3:
	return road_track(road).point(u, d * (CARR_CENTER + lat), y)


func carr_fwd(road: int, d: int, u: float) -> Vector3:
	return road_track(road).tangent(u) * d


func carr_right(road: int, d: int, u: float) -> Vector3:
	return road_track(road).right(u) * d


func world_of_hw(p: Vector3) -> Vector3:
	return Vector3(p.x / HW_SCALE, 0.0, p.z / HW_SCALE)


## Where carriageway (road, d) leads: the sector at its far end.
func carr_destination(road: int, d: int) -> String:
	var t := road_track(road)
	var u := t.length if d == 1 else 0.0
	var end := t.pos(u)
	if road == 1:
		# HWY 2 stops short of HWY 1; name the sector of the junction it leads to.
		end += t.tangent(u) * d * JUNCTION_GAP
	return sector_name(hex_of(world_of_hw(end)))


func _build_boundaries() -> void:
	for road in roads:
		var t: Track = road.track
		var cur := hex_of(world_of_hw(t.pos(0.0)))
		var u := 0.0
		while u < t.length:
			var c := hex_of(world_of_hw(t.pos(u)))
			if c != cur:
				boundaries.append({"road": road.id, "u": u, "a": cur, "b": c})
				cur = c
			u += 5.0


# --- Links and gates ----------------------------------------------------------------------

static func _smooth(t: float) -> float:
	return t * t * (3.0 - 2.0 * t)


static func _hermite(p0: Vector3, t0: Vector3, p1: Vector3, t1: Vector3, n := 40) -> PackedVector3Array:
	var m := p0.distance_to(p1)
	var pts := PackedVector3Array()
	for i in n + 1:
		var t := float(i) / n
		var t2 := t * t
		var t3 := t2 * t
		pts.append((2 * t3 - 3 * t2 + 1) * p0 + (t3 - 2 * t2 + t) * m * t0 + (-2 * t3 + 3 * t2) * p1 + (t3 - t2) * m * t1)
	return pts


## Links carry the player between carriageways or to / from a gate.
## from / to: {road, d, u, lat} or {} when the link starts / ends at a gate.
## mid_offs: per point, lateral offset from the link to the median (the tunnel centre).
func _add_link(kind: String, pts: PackedVector3Array, from: Dictionary, to: Dictionary, mid_offs: PackedFloat32Array, label: String) -> Dictionary:
	var l := {"id": links.size(), "kind": kind, "track": Track.new(pts, false), "from": from, "to": to,
		"mid_offs": mid_offs, "label": label, "gate": {}}
	links.append(l)
	return l


func link_mid_off(l: Dictionary, s: float) -> float:
	var offs: PackedFloat32Array = l.mid_offs
	var tr: Track = l.track
	var f := clampf(s / tr.length, 0.0, 1.0) * (offs.size() - 1)
	var i := mini(int(f), offs.size() - 2)
	return lerpf(offs[i], offs[i + 1], f - i)


func _add_gate(kind: String, hw: Transform3D, extra: Dictionary) -> Dictionary:
	var o := hw.origin
	var w := world_of_hw(o)
	var g := {"id": gates.size(), "kind": kind, "hw": hw, "world": Transform3D(hw.basis, Vector3(w.x, o.y, w.z))}
	g.merge(extra)
	_register_gate(g)
	return g


func _register_gate(g: Dictionary) -> void:
	g.id = gates.size()
	g.sector = hex_of(g.world.origin)
	g.sector_name = sector_name(g.sector)
	gates.append(g)
	if is_valid(g.sector):
		sectors[g.sector].gates.append(g)


func _gate_xform(p: Vector3, fwd: Vector3) -> Transform3D:
	fwd.y = 0.0
	return Transform3D(Basis.looking_at(fwd.normalized(), Vector3.UP), p + Vector3.UP * GATE_Y)


func _build_junctions() -> void:
	for j in junctions:
		# From HWY 2 (its carriageway ends here) onto HWY 1 either way: keep left / right.
		for dc in [1, -1]:
			var vin: int = j.v_in
			var right_v := carr_right(1, vin, j.v_s)
			var side := signf(carr_fwd(0, dc, j.u).dot(right_v))
			var from_lat := side * RIGHT_LANE_LAT
			var p0 := carr_point(1, vin, j.v_s, from_lat)
			var u_m: float = j.u + dc * JUNCTION_SPAN
			var to_lat := signf((p0 - carr_point(0, dc, u_m, 0.0)).dot(carr_right(0, dc, u_m))) * RIGHT_LANE_LAT
			var p1 := carr_point(0, dc, u_m, to_lat)
			var pts := _hermite(p0, carr_fwd(1, vin, j.v_s), p1, carr_fwd(0, dc, u_m))
			var offs := _lerp_offs(pts.size(), -(CARR_CENTER + from_lat), -(CARR_CENTER + to_lat))
			var label := "%s %s > %s" % [ROAD_NAMES[0], compass(carr_fwd(0, dc, u_m)), carr_destination(0, dc)]
			var l := _add_link("turn", pts, {"road": 1, "d": vin, "u": j.v_s, "lat": from_lat, "at_end": true, "side": side},
				{"road": 0, "d": dc, "u": u_m, "lat": to_lat}, offs, label)
		# From HWY 1 onto HWY 2 (its carriageway starts here).
		for dc in [1, -1]:
			var vout: int = j.v_out
			var u_f: float = j.u - dc * JUNCTION_SPAN
			var p1c := carr_point(1, vout, j.v_s, 0.0)
			var side := signf((p1c - carr_point(0, dc, u_f, 0.0)).dot(carr_right(0, dc, u_f)))
			var from_lat := side * RIGHT_LANE_LAT
			var p0 := carr_point(0, dc, u_f, from_lat)
			var to_lat := signf((p0 - p1c).dot(carr_right(1, vout, j.v_s))) * RIGHT_LANE_LAT
			var p1 := carr_point(1, vout, j.v_s, to_lat)
			var pts := _hermite(p0, carr_fwd(0, dc, u_f), p1, carr_fwd(1, vout, j.v_s))
			var offs := _lerp_offs(pts.size(), -(CARR_CENTER + from_lat), -(CARR_CENTER + to_lat))
			var label := "%s %s > %s" % [ROAD_NAMES[1], compass(carr_fwd(1, vout, j.v_s)), carr_destination(1, vout)]
			_add_link("turn", pts, {"road": 0, "d": dc, "u": u_f, "lat": from_lat, "side": side},
				{"road": 1, "d": vout, "u": j.v_s, "lat": to_lat}, offs, label)


static func _lerp_offs(n: int, a: float, b: float) -> PackedFloat32Array:
	var offs := PackedFloat32Array()
	for i in n:
		offs.append(lerpf(a, b, _smooth(float(i) / (n - 1))))
	return offs


## Exit and entrance for both directions, grouped around u_i so each exit has an entrance
## close by in open space.
func _add_interchange(road: int, u_i: float, sys: Dictionary) -> void:
	var t := road_track(road)
	var n := 28
	for d in [1, -1]:
		var fork: float = u_i - d * (INTERCHANGE_GAP + RAMP_LEN)
		var pts := PackedVector3Array()
		var offs := PackedFloat32Array()
		for i in n + 1:
			var k := float(i) / n
			var lat_m := CARR_CENTER + RIGHT_LANE_LAT + RAMP_SHIFT * _smooth(k)
			pts.append(t.point(fork + d * k * RAMP_LEN, d * lat_m))
			offs.append(-lat_m)
		var off := _add_link("off", pts, {"road": road, "d": d, "u": fork, "lat": RIGHT_LANE_LAT, "side": 1.0}, {}, offs,
			"EXIT %s (%s)" % [sys.name, "?"])
		var tr: Track = off.track
		off.gate = _add_gate("off", _gate_xform(tr.pos(tr.length), tr.tangent(tr.length)), {"link": off, "system": sys.name})
		off.label = "EXIT %s (%s)" % [sys.name, off.gate.sector_name]
		off.gate.label = "EXIT  %s" % sys.name

		var gate_u: float = u_i + d * INTERCHANGE_GAP
		pts = PackedVector3Array()
		offs = PackedFloat32Array()
		for i in n + 1:
			var k := float(i) / n
			var lat_m := CARR_CENTER + RIGHT_LANE_LAT + RAMP_SHIFT * _smooth(1.0 - k)
			pts.append(t.point(gate_u + d * k * RAMP_LEN, d * lat_m))
			offs.append(-lat_m)
		var dest := "%s %s > %s" % [ROAD_NAMES[road], compass(carr_fwd(road, d, gate_u)), carr_destination(road, d)]
		var on := _add_link("on", pts, {}, {"road": road, "d": d, "u": gate_u + d * RAMP_LEN, "lat": RIGHT_LANE_LAT}, offs, dest)
		on.gate = _add_gate("on", _gate_xform(pts[0], on.track.tangent(0.0)), {"link": on, "label": dest, "system": sys.name})


## HWY 1 ends: an exit spanning the arriving carriageway and an entrance for the other.
func _add_terminal(at_start: bool, sys: Dictionary) -> void:
	var L := road_track(0).length
	var u: float = TERMINAL_GATE_U if at_start else L - TERMINAL_GATE_U
	var d_in := -1 if at_start else 1
	var d_out := -d_in
	var ex := _add_gate("off", _gate_xform(carr_point(0, d_in, u, 0.0), carr_fwd(0, d_in, u)),
		{"terminal": {"road": 0, "d": d_in, "u": u}, "system": sys.name, "label": "END  -  EXIT %s" % sys.name})
	ex.wide = true
	var dest := "%s %s > %s" % [ROAD_NAMES[0], compass(carr_fwd(0, d_out, u)), carr_destination(0, d_out)]
	var en := _add_gate("on", _gate_xform(carr_point(0, d_out, u, 0.0), carr_fwd(0, d_out, u)),
		{"terminal": {"road": 0, "d": d_out, "u": u}, "system": sys.name, "label": dest})
	en.wide = true


func terminal_exit(road: int, d: int) -> Dictionary:
	for g in gates:
		if g.kind == "off" and g.has("terminal") and g.terminal.road == road and g.terminal.d == d:
			return g
	return {}


func _build_events() -> void:
	for road in roads:
		for d in [1, -1]:
			var list: Array = []
			for l in links:
				if l.from.is_empty() or l.from.road != road.id or l.from.d != d:
					continue
				if l.from.get("at_end", false):
					continue
				list.append({"u": l.from.u, "text": l.label, "kind": l.kind, "side": l.from.side})
			for j in junctions:
				if road.id == 1 and d == j.v_in:
					list.append({"u": j.v_s, "text": "JCT %s:  < %s  |  %s >" % [ROAD_NAMES[0], _turn_label(j, -1), _turn_label(j, 1)], "kind": "jct_end", "side": 0.0})
			var te := terminal_exit(road.id, d)
			if not te.is_empty():
				list.append({"u": te.terminal.u, "text": te.label, "kind": "end", "side": 0.0})
			list.sort_custom(func(a, b): return a.u * d < b.u * d)
			events[Vector2i(road.id, d)] = list


## Label of the junction turn taken from HWY 2 by keeping to side (-1 left, +1 right).
func _turn_label(j: Dictionary, side: int) -> String:
	for l in links:
		if l.kind == "turn" and l.from.road == 1 and l.from.d == j.v_in and int(l.from.side) == side:
			return l.label.get_slice(" >", 0)
	return "?"


func upcoming(road: int, d: int, u: float, count: int) -> Array:
	var out: Array = []
	for e in events.get(Vector2i(road, d), []):
		var dist: float = (e.u - u) * d
		if dist >= 0.0:
			out.append({"dist": dist, "text": e.text, "kind": e.kind})
			if out.size() >= count:
				break
	return out


# --- Systems ------------------------------------------------------------------------------

func _build_systems(rng: RandomNumberGenerator) -> void:
	var C := road_track(0)
	var V := road_track(1)
	var ang_pt := func(phi_deg: float) -> Vector3:
		var phi := deg_to_rad(phi_deg)
		return _to_hw([Vector2(C_CENTER.x - C_RX * sin(phi), C_CENTER.y - C_RZ * cos(phi))])[0]
	var sites := [
		{"road": 0, "terminal": true, "u": TERMINAL_GATE_U},
		{"road": 0, "u": C.project_global(ang_pt.call(50.0))},
		{"road": 1, "u": V.length * 0.5},
		{"road": 0, "u": C.project_global(ang_pt.call(130.0))},
		{"road": 0, "terminal": true, "u": C.length - TERMINAL_GATE_U},
	]
	for site in sites:
		var t := road_track(site.road)
		var p := world_of_hw(t.pos(site.u))
		var out := Vector3(1, 0, 0)
		if site.road == 0:
			out = (p - Vector3(C_CENTER.x, 0, C_CENTER.y)).normalized()
		var sys := _new_system(p + out * 4500.0)
		if site.get("terminal", false):
			_add_terminal(site.u < C.length * 0.5, sys)
		else:
			_add_interchange(site.road, site.u, sys)
	# One system away from the highway, reached by hop lane.
	_new_system(sector_center(Vector2i(0, 0)) + Vector3(1500, 0, -1000))
	for sys in systems:
		_build_system_bodies(sys, rng)
	# Start in front of an entrance near the system in the middle of the map.
	for g in gates:
		if g.kind == "on" and g.get("system", "") == systems[2].name:
			start = {"gate": g, "pos": g.world.origin + g.world.basis.z * 700.0}
			break


func _new_system(center: Vector3) -> Dictionary:
	var sys := {"name": SYSTEM_NAMES[systems.size()], "center": center, "sector": hex_of(center), "planets": []}
	systems.append(sys)
	return sys


func _build_system_bodies(sys: Dictionary, rng: RandomNumberGenerator) -> void:
	var n_planets: int = [1, 2, 2, 3, 3, 4][rng.randi() % 6]
	for k in n_planets:
		# Every system gets its first planet: if the gates crowd it out, move further out.
		for attempt in 150 if k > 0 else 300:
			var radius := rng.randf_range(450.0, 1400.0)
			var reach := 900.0 if attempt < 150 else 3000.0
			var dist := rng.randf_range(0.0, reach) if k == 0 else rng.randf_range(1500.0, 4500.0)
			var ang := rng.randf() * TAU
			var p: Vector3 = sys.center + Vector3(cos(ang) * dist, rng.randf_range(-700.0, 700.0), sin(ang) * dist)
			if not _body_ok(p, radius, sys.sector, 1500.0, 2500.0):
				continue
			var pl := _add_body("planet", "%s %s" % [sys.name, ROMAN[k]], p, radius, sys, rng)
			sys.planets.append(pl)
			break
	for pl in sys.planets:
		if rng.randf() > 0.45:
			continue
		var n_moons := 2 if rng.randf() < 0.3 else 1
		for m in n_moons:
			for attempt in 60:
				var mr := rng.randf_range(120.0, 340.0)
				var ang := rng.randf() * TAU
				var dist: float = pl.radius * rng.randf_range(2.0, 3.0) + 400.0
				var p: Vector3 = pl.world + Vector3(cos(ang) * dist, rng.randf_range(-0.3, 0.3) * pl.radius, sin(ang) * dist)
				if _body_ok(p, mr, sys.sector, 400.0, 1200.0):
					_add_body("moon", "%s-%s" % [pl.name, ["a", "b"][m]], p, mr, sys, rng)
					break


func _add_body(kind: String, name: String, p: Vector3, radius: float, sys: Dictionary, rng: RandomNumberGenerator) -> Dictionary:
	var b := {
		"kind": kind, "name": name, "world": p, "radius": radius, "sector": sys.sector, "system": sys.name,
		"palette": PALETTES[3] if kind == "moon" and rng.randf() < 0.6 else PALETTES[rng.randi() % PALETTES.size()],
		"banding": 1.0 if kind == "planet" and rng.randf() < 0.4 else 0.0,
		"seed": rng.randf() * 100.0,
		"ringed": kind == "planet" and rng.randf() < 0.3,
		"ring_tilt": Vector3(rng.randf_range(-0.5, 0.5), rng.randf() * TAU, rng.randf_range(-0.3, 0.3)),
	}
	bodies.append(b)
	sectors[sys.sector].bodies.append(b)
	return b


func _body_ok(p: Vector3, radius: float, sector: Vector2i, body_gap: float, gate_gap: float) -> bool:
	if hex_of(p) != sector or hex_sdf(p, sector) > -(radius + 800.0):
		return false
	for g in gates:
		if p.distance_to(g.world.origin) < radius + gate_gap:
			return false
	for b in bodies:
		if p.distance_to(b.world) < radius + b.radius + body_gap:
			return false
	return true


# --- Hop lanes ----------------------------------------------------------------------------

func _build_hops() -> void:
	for sys in systems:
		var ps: Array = sys.planets
		if ps.size() == 2:
			_add_hop(ps[0], ps[1])
			_add_hop(ps[1], ps[0])
		elif ps.size() >= 3:
			for i in ps.size():
				_add_hop(ps[i], ps[(i + 1) % ps.size()])
	# Occasionally to a nearby system: link the off-highway system to its nearest neighbour.
	var lone: Dictionary = systems[-1]
	var best := {}
	for sys in systems:
		if sys != lone and not sys.planets.is_empty() and (best.is_empty() or
				(sys.center as Vector3).distance_to(lone.center) < (best.center as Vector3).distance_to(lone.center)):
			best = sys
	if not best.is_empty() and not lone.planets.is_empty():
		_add_hop(best.planets[0], lone.planets[0])
		_add_hop(lone.planets[0], best.planets[0])


func _add_hop(a: Dictionary, b: Dictionary) -> void:
	var dir: Vector3 = (b.world - a.world).normalized()
	# Keep to the right of the line between the planets, so the lanes each way don't overlap.
	var side := dir.cross(Vector3.UP).normalized() * 250.0
	var entry: Vector3 = a.world + dir * (a.radius + 1600.0) + side
	var exit: Vector3 = b.world - dir * (b.radius + 1300.0) + side
	if not _segment_clear(entry, exit):
		entry.y += 2000.0 * (1.0 if entry.y < 0.0 else -1.0)
		exit.y += 2000.0 * (1.0 if exit.y < 0.0 else -1.0)
		if not _segment_clear(entry, exit):
			return
	dir = (exit - entry).normalized()
	var basis := Basis.looking_at(dir, Vector3.UP)
	var hop := {"id": hops.size(), "from": a, "to": b, "entry": entry, "exit": exit, "dir": dir, "length": entry.distance_to(exit)}
	hops.append(hop)
	var gi := {"kind": "hop_in", "world": Transform3D(basis, entry), "hop": hop, "label": "HOP > %s" % b.name}
	var go := {"kind": "hop_out", "world": Transform3D(basis, exit), "hop": hop, "label": "HOP from %s" % a.name}
	_register_gate(gi)
	_register_gate(go)


func _segment_clear(a: Vector3, b: Vector3) -> bool:
	for body in bodies:
		var p: Vector3 = body.world
		var ab := b - a
		var t := clampf((p - a).dot(ab) / ab.length_squared(), 0.0, 1.0)
		if p.distance_to(a + ab * t) < body.radius + 400.0:
			return false
	return absf(a.y) < SECTOR_HALF_H - 200.0 and absf(b.y) < SECTOR_HALF_H - 200.0


# --- Asteroids ----------------------------------------------------------------------------

func _build_clusters(rng: RandomNumberGenerator) -> void:
	for sys in systems:
		if sys.planets.is_empty() or rng.randf() > 0.6:
			continue
		var big: Dictionary = sys.planets[0]
		for pl in sys.planets:
			if pl.radius > big.radius:
				big = pl
		var r: float = big.radius * 4.0 + 700.0
		clusters.append({"kind": "belt", "center": big.world, "radius": r, "sigma": 260.0, "count": 360,
			"seed": rng.randi(), "sector": big.sector, "extent": r + 800.0})
	# A clump beside the start so asteroids are in view straight away.
	var g: Dictionary = start.gate
	var sc: Vector3 = start.pos + g.world.basis.x * 1900.0 - g.world.basis.z * 300.0
	_add_clump(sc, 550.0, 240, rng)
	var tries := 0
	while clusters.size() < 12 and tries < 400:
		tries += 1
		var c := Vector2i(rng.randi_range(0, COLS - 1), rng.randi_range(0, ROWS - 1))
		var ang := rng.randf() * TAU
		var p := sector_center(c) + Vector3(cos(ang), 0, sin(ang)) * rng.randf_range(0.0, HEX_APOTHEM * 0.6) + Vector3.UP * rng.randf_range(-900.0, 900.0)
		var sigma := rng.randf_range(600.0, 1400.0)
		var ok := true
		for b in bodies:
			if p.distance_to(b.world) < b.radius + sigma * 3.0:
				ok = false
		for gg in gates:
			if p.distance_to(gg.world.origin) < sigma * 2.5:
				ok = false
		for cl in clusters:
			if p.distance_to(cl.center) < sigma * 3.0 + cl.extent:
				ok = false
		if ok:
			_add_clump(p, sigma, rng.randi_range(140, 280), rng)


func _add_clump(p: Vector3, sigma: float, count: int, rng: RandomNumberGenerator) -> void:
	clusters.append({"kind": "clump", "center": p, "radius": 0.0, "sigma": sigma, "count": count,
		"seed": rng.randi(), "sector": hex_of(p), "extent": sigma * 3.0})
