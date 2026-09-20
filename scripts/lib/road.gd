class_name Road
extends RefCounted
## A built structure along one `RoadPath` carrying one or two carriageways (`Tube`s).
## A highway has two carriageways under one roof with a glass median; a ramp has one.
## Rendering and rib layout are per road; collision is per tube. Pure apart from the
## tuning it is sized from.

var name: String = ""
## "highway" or "ramp".
var kind: String = "highway"
var path: RoadPath
var tubes: Array[Tube] = []
var carriageway_w: float = 240.0
var carriageway_h: float = 150.0
## Centre-to-centre distance of the two carriageways.
var separation: float = 240.0
var rib_spacing: float = 400.0
var rib_thickness: float = 60.0
var rib_protrusion: float = 24.0
## t of the first rib. Lets a ramp continue the mainline's beat rather than restart it,
## because the beat is the road's strongest speed cue.
var rib_phase: float = 0.0
## The rib positions, cached per gear setting and junction count (see `rib_positions`).
var _ribs: PackedFloat32Array = PackedFloat32Array()
var _ribs_key: String = ""
## Half extents of the whole section.
var half_width: float = 0.0
var half_height: float = 0.0
## Open ends into space: `{t, label}`.
var mouths: Array[Dictionary] = []
var bounds: AABB


static func make(road_name: String, road_kind: String, road_path: RoadPath,
		lanes: int, inset: float) -> Road:
	var r := Road.new()
	r.name = road_name
	r.kind = road_kind
	r.path = road_path
	r.carriageway_w = Tuning.num("exploration/lane_width") - 2.0 * inset
	r.carriageway_h = Tuning.num("exploration/lane_height") - 2.0 * inset
	r.separation = Tuning.num("exploration/deck_separation")
	r.rib_spacing = Tuning.num("exploration/structure_module_length")
	r.rib_thickness = Tuning.num("exploration/structure_rib_thickness")
	r.rib_protrusion = Tuning.num("exploration/structure_rib_protrusion")
	if road_path.closed:
		# Keep the beat continuous across the seam of a loop.
		r.rib_spacing = road_path.length / maxf(1.0, roundf(road_path.length / r.rib_spacing))
	var hw := r.carriageway_w * 0.5
	var hh := r.carriageway_h * 0.5
	if lanes == 2:
		# Traffic on the right: the +1 carriageway sits to the path's right, the -1 one
		# to its left, which is ITS right as its own traffic travels (ADR 0077).
		for side: int in [1, -1]:
			var t := Tube.new()
			t.name = road_name + (" R" if side == 1 else " L")
			t.road = r
			t.path = road_path
			t.u0 = side * r.separation * 0.5
			t.hw = hw
			t.hh = hh
			t.direction = side
			t.route_name = road_name
			t.compute_bounds()
			r.tubes.append(t)
		r.half_width = r.separation * 0.5 + hw
	else:
		var t := Tube.new()
		t.name = road_name
		t.road = r
		t.path = road_path
		t.hw = hw
		t.hh = hh
		t.direction = 1
		t.compute_bounds()
		r.tubes.append(t)
		r.half_width = hw
	r.half_height = hh
	r.bounds = road_path.aabb(0.0)
	return r


## THE HIGHWAY GEAR (`docs/SECTOR_PROTOTYPE.md`, prototype 2). On a highway, between
## junctions, the lane pushes the ship through the world at `highway_gear` times the
## felt speed, and the structure is spaced to match, so the ribs pass at the felt
## rate while the world outside passes faster. The gear is 1 at every junction and at
## a road's open ends, and reaches full `highway_gear_shift_metres` away from them, so
## nothing built around a ramp changes. A ramp is always in first.
func gear_at(t: float) -> float:
	if kind == "ramp":
		return 1.0
	var gear := Tuning.num("exploration/highway_gear")
	if gear <= 1.0:
		return 1.0
	var shift := maxf(Tuning.num("exploration/highway_gear_shift_metres"), 1.0)
	var nearest := INF if path.closed else minf(t, path.length - t)
	for tube in tubes:
		for j in tube.junctions:
			var d := absf(t - float(j["t"]))
			if path.closed:
				d = minf(d, path.length - d)
			nearest = minf(nearest, d)
	if nearest == INF:
		return gear
	return 1.0 + (gear - 1.0) * clampf(nearest / shift, 0.0, 1.0)


## Is the gear doing anything on this road? When not, the ribs are the regular beat
## `rib_spacing` apart and the margin is arithmetic.
func geared() -> bool:
	return kind != "ramp" and Tuning.num("exploration/highway_gear") > 1.0


## Rib collar t-positions along the path. `rib_spacing` apart in road units: where the
## gear is in, the world spacing is the beat times the gear there, so the beat is what
## a ship at the felt speed passes.
func rib_positions() -> PackedFloat32Array:
	var key := "%s/%s/%d" % [Tuning.num("exploration/highway_gear"),
		Tuning.num("exploration/highway_gear_shift_metres"), _junction_count()]
	if key == _ribs_key:
		return _ribs
	var out := PackedFloat32Array()
	var t := fposmod(rib_phase, rib_spacing)
	var end := path.length - (0.0 if path.closed else rib_thickness * 0.5)
	var stretch := geared()
	while t <= end:
		if path.closed or t >= rib_thickness * 0.5:
			out.append(t)
		t += rib_spacing * (gear_at(t) if stretch else 1.0)
	_ribs = out
	_ribs_key = key
	return out


## How far the built structure stands out from the glass at t: the rib's protrusion
## inside a collar, nothing between them. The outside collision reads this.
func rib_margin_at(t: float) -> float:
	if not geared():
		var local := fposmod(t - rib_phase, rib_spacing)
		if local < rib_thickness * 0.5 or local > rib_spacing - rib_thickness * 0.5:
			return rib_protrusion
		return 0.0
	var ribs := rib_positions()
	if ribs.is_empty():
		return 0.0
	var i := ribs.bsearch(t)
	var half := rib_thickness * 0.5
	for k: int in [i - 1, i]:
		if k >= 0 and k < ribs.size() and absf(ribs[k] - t) <= half:
			return rib_protrusion
	if path.closed and (absf(ribs[0] + path.length - t) <= half
			or absf(ribs[ribs.size() - 1] - path.length - t) <= half):
		return rib_protrusion
	return 0.0


func _junction_count() -> int:
	var n := 0
	for tube in tubes:
		n += tube.junctions.size()
	return n
