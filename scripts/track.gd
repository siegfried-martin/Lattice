class_name Track
extends RefCounted
## Polyline path parameterised by arc length. Lateral offsets use right = tangent x up,
## so positive x is to the right of the direction of travel along increasing s.

var pts: PackedVector3Array
var tans: PackedVector3Array
var cum: PackedFloat64Array
var closed: bool
var length: float


func _init(points: PackedVector3Array, is_closed: bool) -> void:
	pts = points
	closed = is_closed
	var n := pts.size()
	var count := n + 1 if closed else n
	cum.resize(count)
	cum[0] = 0.0
	for i in range(1, count):
		cum[i] = cum[i - 1] + pts[i % n].distance_to(pts[i - 1])
	length = cum[count - 1]
	tans.resize(n)
	for i in n:
		var a: Vector3
		var b: Vector3
		if closed:
			a = pts[(i - 1 + n) % n]
			b = pts[(i + 1) % n]
		else:
			a = pts[maxi(i - 1, 0)]
			b = pts[mini(i + 1, n - 1)]
		tans[i] = (b - a).normalized()


func _seg(s: float) -> int:
	var lo := 0
	var hi := cum.size() - 1
	while hi - lo > 1:
		var mid := (lo + hi) >> 1
		if cum[mid] <= s:
			lo = mid
		else:
			hi = mid
	return lo


func pos(s: float) -> Vector3:
	var n := pts.size()
	if closed:
		s = fposmod(s, length)
	elif s <= 0.0:
		return pts[0] + tans[0] * s
	elif s >= length:
		return pts[n - 1] + tans[n - 1] * (s - length)
	var i := _seg(s)
	var f := (s - cum[i]) / maxf(cum[i + 1] - cum[i], 0.0001)
	return pts[i].lerp(pts[(i + 1) % n], f)


func tangent(s: float) -> Vector3:
	var n := pts.size()
	if closed:
		s = fposmod(s, length)
	elif s <= 0.0:
		return tans[0]
	elif s >= length:
		return tans[n - 1]
	var i := _seg(s)
	var f := (s - cum[i]) / maxf(cum[i + 1] - cum[i], 0.0001)
	return tans[i].lerp(tans[(i + 1) % n], f).normalized()


func right(s: float) -> Vector3:
	return tangent(s).cross(Vector3.UP).normalized()


func point(s: float, x: float, y: float = 0.0) -> Vector3:
	return pos(s) + right(s) * x + Vector3.UP * y


## Nearest arc-length parameter to p, refined locally from a nearby guess.
func project(p: Vector3, s_hint: float, iters := 4) -> float:
	var s := s_hint
	for i in iters:
		s += clampf((p - pos(s)).dot(tangent(s)), -200.0, 200.0)
		s = fposmod(s, length) if closed else clampf(s, 0.0, length)
	return s


## Nearest parameter to p by a coarse scan of the whole track, then local refinement.
func project_global(p: Vector3) -> float:
	var best := 0.0
	var best_d := INF
	var s := 0.0
	while s < length:
		var dd := pos(s).distance_squared_to(p)
		if dd < best_d:
			best_d = dd
			best = s
		s += 10.0
	return project(p, best)
