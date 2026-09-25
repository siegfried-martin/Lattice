class_name HighwayDrive
extends RefCounted
## Docked movement on the highway network. The ship cruises along either a road carriageway
## (road, d, u) or a link track (ramp or junction turn), and can step between lanes.
## Keeping to the right lane past an exit fork takes it; at HWY 2's ends the side you keep to
## picks the turn onto HWY 1.

const CRUISE_FRACTION := 0.8   # docked ships run at this fraction of their top speed
const ACCEL := 4.0
const LAT_SPEED := 8.0
const HOVER := 3.5
const CAM_BACK := 30.0
const CAM_UP := 10.0
const MERGE_GRACE := 250.0

var on_road := true
var road := 0
var d := 1
var u := 0.0
var link: Dictionary = {}
var s := 0.0
var lat := 0.0          # metres right of the current reference line
var lat_target := 0.0
var lat_vel := 0.0
var cam_lat := 0.0
var speed := 0.0
var cruise := 32.0
var odo := 0.0          # distance travelled while docked
var exit_gate: Dictionary = {}
var ship_xform := Transform3D()
var cam_xform := Transform3D()
var throttle_vis := 0.0
var _bob := 0.0
var _grace := 0.0


func dock_road(r: int, dir: int, at_u: float, lateral: float, entry_speed: float, max_speed: float) -> void:
	on_road = true
	road = r
	d = dir
	u = at_u
	link = {}
	_start(lateral, entry_speed, max_speed)
	lat_target = _lane_center(_lane_of(lat))


func dock_link(l: Dictionary, at_s: float, lateral: float, entry_speed: float, max_speed: float) -> void:
	on_road = false
	link = l
	s = at_s
	_start(lateral, entry_speed, max_speed)
	lat_target = 0.0


func _start(lateral: float, entry_speed: float, max_speed: float) -> void:
	lat = clampf(lateral, -lat_limit(), lat_limit())
	cam_lat = lat
	lat_vel = 0.0
	speed = maxf(entry_speed, 0.0)
	cruise = max_speed * CRUISE_FRACTION
	exit_gate = {}
	_grace = 0.0
	_update_xforms(0.0)


func lat_limit() -> float:
	return Galaxy.CARR_W * 0.5 - 3.0 if on_road else Galaxy.LANE_W * 0.5 - 1.5


func _lane_of(x: float) -> int:
	return clampi(roundi((x + Galaxy.RIGHT_LANE_LAT) / Galaxy.LANE_W), 0, Galaxy.LANES - 1)


func _lane_center(i: int) -> float:
	return -Galaxy.RIGHT_LANE_LAT + Galaxy.LANE_W * i


## Step one lane left (-1) or right (+1). Links are a single lane.
func change_lane(dir: int) -> void:
	if on_road:
		lat_target = _lane_center(clampi(_lane_of(lat_target) + dir, 0, Galaxy.LANES - 1))


## Carriageway (road, d) the ship is on or nearest to, for traffic, HUD and lights.
func ref_carriageway() -> Vector2i:
	if on_road:
		return Vector2i(road, d)
	var e := _link_ref()
	return Vector2i(e.road, e.d)


func ref_u() -> float:
	if on_road:
		return u
	var e := _link_ref()
	var tr: Track = link.track
	if is_same(e, link.from):
		return e.u + e.d * s
	return e.u - e.d * (tr.length - s)


func _link_ref() -> Dictionary:
	var tr: Track = link.track
	var from: Dictionary = link.from
	var to: Dictionary = link.to
	if not from.is_empty() and (to.is_empty() or s < tr.length * 0.5):
		return from
	return to


## Frame at distance t ahead along the ship's path (t may be negative):
## ref: the line the ship's lat is measured from; center: the median (tunnel centre) at road
## level; carr: the carriageway centre; fwd / right: travel frame.
func frame_at(t: float) -> Dictionary:
	if on_road:
		return _road_frame_ahead(road, d, u, t)
	return _link_frame(link, s + t)


static func road_frame(r: int, dir: int, uu: float, ref_lat := 0.0) -> Dictionary:
	var tr := Galaxy.road_track(r)
	var right := tr.right(uu) * dir
	var carr := tr.point(uu, dir * Galaxy.CARR_CENTER)
	return {"ref": carr + right * ref_lat, "center": tr.pos(uu), "carr": carr, "fwd": tr.tangent(uu) * dir, "right": right}


## Road frame, continuing into the turn at HWY 2's end that the current lane leads to.
func _road_frame_ahead(r: int, dir: int, from_u: float, t: float) -> Dictionary:
	var uu := from_u + dir * t
	var tr := Galaxy.road_track(r)
	var over := (uu - tr.length) if dir == 1 else -uu
	if over > 0.0 and t > 0.0:
		var l := _end_link(r, dir, lat)
		if not l.is_empty():
			var lf := _link_frame(l, over)
			lf.ref = (lf.ref as Vector3) - (lf.right as Vector3) * (l.from.lat as float)
			return lf
	return road_frame(r, dir, uu)


func _link_frame(l: Dictionary, ss: float) -> Dictionary:
	var tr: Track = l.track
	var from: Dictionary = l.from
	var to: Dictionary = l.to
	if ss < 0.0 and not from.is_empty():
		return road_frame(from.road, from.d, from.u + from.d * ss, from.lat)
	if ss > tr.length and not to.is_empty():
		return road_frame(to.road, to.d, to.u + to.d * (ss - tr.length), to.lat)
	var p := tr.pos(ss)
	var right := tr.right(ss)
	var center := p + right * Galaxy.link_mid_off(l, ss)
	return {"ref": p, "center": center, "carr": center + right * Galaxy.CARR_CENTER, "fwd": tr.tangent(ss), "right": right}


## The turn link taken at the end of carriageway (r, dir) from lateral position x.
func _end_link(r: int, dir: int, x: float) -> Dictionary:
	var side := -1 if x < -2.0 else 1
	for l in Galaxy.links:
		if l.kind == "turn" and l.from.road == r and l.from.d == dir and l.from.get("at_end", false) and int(l.from.side) == side:
			return l
	return {}


func update(delta: float) -> void:
	var prev_speed := speed
	speed = move_toward(speed, cruise, ACCEL * delta)
	var target_throttle := 0.35 + clampf((speed - prev_speed) / maxf(delta, 0.0001) / ACCEL, 0.0, 1.0) * 0.6
	throttle_vis = lerpf(throttle_vis, target_throttle, 1.0 - exp(-4.0 * delta))

	var lim := lat_limit()
	lat_target = clampf(lat_target, -lim, lim)
	var prev_lat := lat
	lat = move_toward(lat, lerpf(lat, lat_target, 1.0 - exp(-2.5 * delta)), LAT_SPEED * delta)
	lat_vel = (lat - prev_lat) / maxf(delta, 0.0001)

	var step := speed * delta
	odo += step
	if on_road:
		var u_prev := u
		u += d * step
		_grace = maxf(0.0, _grace - step)
		_advance_road(u_prev)
	else:
		s += step
		var tr: Track = link.track
		if s >= tr.length:
			var to: Dictionary = link.to
			if to.is_empty():
				exit_gate = link.gate
			else:
				var over := s - tr.length
				var to_lat: float = to.lat
				on_road = true
				road = to.road
				d = to.d
				u = to.u + d * over
				link = {}
				_shift_lateral(to_lat)
				# Ease toward the middle lane so the next exit isn't taken by accident.
				lat_target = _lane_center(1)
				_grace = MERGE_GRACE
	_update_xforms(delta)


func _advance_road(u_prev: float) -> void:
	var te := Galaxy.terminal_exit(road, d)
	if not te.is_empty() and (te.terminal.u - u_prev) * d > 0.0 and (te.terminal.u - u) * d <= 0.0:
		exit_gate = te
		return
	var tr := Galaxy.road_track(road)
	var over := (u - tr.length) if d == 1 else -u
	if over > 0.0:
		var l := _end_link(road, d, lat)
		if not l.is_empty():
			_enter_link(l, over)
			return
	if _grace > 0.0:
		return
	for l in Galaxy.links:
		var from: Dictionary = l.from
		if from.is_empty() or from.road != road or from.d != d or from.get("at_end", false):
			continue
		var side: float = from.side
		if (side > 0.0 and lat < Galaxy.RIGHT_LANE_LAT - Galaxy.LANE_W * 0.5) or (side < 0.0 and lat > -Galaxy.RIGHT_LANE_LAT + Galaxy.LANE_W * 0.5):
			continue
		if (from.u - u_prev) * d > 0.0 and (from.u - u) * d <= 0.0:
			_enter_link(l, (u - from.u) * d)
			return


func _enter_link(l: Dictionary, at_s: float) -> void:
	var from_lat: float = l.from.lat
	on_road = false
	link = l
	s = at_s
	_shift_lateral(-from_lat)
	lat_target = 0.0


func _shift_lateral(k: float) -> void:
	lat += k
	lat_target += k
	cam_lat += k


func _update_xforms(delta: float) -> void:
	_bob += delta
	var f := frame_at(0.0)
	var fwd: Vector3 = f.fwd
	var right: Vector3 = f.right
	var ref: Vector3 = f.ref
	var p: Vector3 = ref + right * lat + Vector3.UP * (HOVER + sin(_bob * 2.1) * 0.15)
	var yaw := -atan2(lat_vel, maxf(speed, 10.0)) * 0.9
	ship_xform = Transform3D(Basis(Vector3.UP, yaw) * Basis.looking_at(fwd, Vector3.UP), p)

	cam_lat = lerpf(cam_lat, lat, 1.0 - exp(-3.0 * delta)) if delta > 0.0 else lat
	var fb := frame_at(-CAM_BACK)
	var cam_p: Vector3 = (fb.ref as Vector3) + (fb.right as Vector3) * cam_lat + Vector3.UP * CAM_UP
	var fa := frame_at(45.0)
	var look: Vector3 = (fa.ref as Vector3) + (fa.right as Vector3) * (cam_lat * 0.6) + Vector3.UP * 2.5
	cam_xform = Transform3D(Basis.looking_at(look - cam_p, Vector3.UP), cam_p)
