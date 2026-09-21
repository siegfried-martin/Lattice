class_name HighwayQuad
extends RefCounted
## One wall of the highway: four corners, drawn as two triangles and collided as
## the same two triangles.
##
## **This is the road's one rule for where a wall is.** A quad is drawn and a quad
## is bounced off; a wall that is open is a quad that was never made. There is no
## second description of the road to keep in step with this one — see
## `docs/HIGHWAY_BUILD_ORDER.md`.
##
## Four arbitrary corners rather than a rectangle, because a tile that bends, climbs
## or tapers into a ramp has walls that are trapezoids and a floor that can narrow to
## a point. Collision is against the triangles themselves, so it is exact against
## whatever shape is drawn, and a triangle of zero area is dropped from both at once.
##
## Pure: no scene tree, no tuning, no disk.

## Twice the area below which a triangle is a line and is neither drawn nor hit.
## Infrastructure, not feel.
const DEGENERATE: float = 0.0001

## What the HUD calls it: "left", "ceiling", and so on.
var wall_name: String = "wall"
## Which tube it belongs to, so a wall that continues across a tile seam is one wall
## to the bounce and not a fresh one at every seam.
var tube: String = ""

## Every triangle, three vertices each, wound so each face points into the tube.
var _tris: PackedVector3Array = PackedVector3Array()
## One inward unit normal per triangle.
var _normals: PackedVector3Array = PackedVector3Array()


## `a b c d` in order round the quad; `inward` is any direction pointing into the
## tube, used only to decide which way each triangle faces.
static func make(a: Vector3, b: Vector3, c: Vector3, d: Vector3, inward: Vector3,
		called: String, in_tube: String) -> HighwayQuad:
	var quad := HighwayQuad.new()
	quad.wall_name = called
	quad.tube = in_tube
	for tri: PackedVector3Array in [PackedVector3Array([a, b, c]),
			PackedVector3Array([a, c, d])]:
		var n := (tri[1] - tri[0]).cross(tri[2] - tri[0])
		if n.length() <= DEGENERATE:
			continue
		if n.dot(inward) < 0.0:
			tri = PackedVector3Array([tri[0], tri[2], tri[1]])
			n = -n
		quad._tris.append_array(tri)
		quad._normals.append(n.normalized())
	return quad


## The triangles, in the order the mesh emits them. The gate compares this against
## what the `ArrayMesh` actually holds.
func triangles() -> PackedVector3Array:
	return _tris


func normals() -> PackedVector3Array:
	return _normals


func is_empty() -> bool:
	return _tris.is_empty()


## Where a sphere of `radius` centred at `point` meets this wall, or null.
##
## Exact against the triangles, with no margin past their edges: an invisible strip
## of collision beyond where the mesh stops is precisely the bug the first road had,
## and it is why a branch opening is safe to cut here.
##
## Over a face, the push is always **into** the tube, even from behind the plane — a
## ship that got part-way through is put back on the inside. Off a face it is a push
## away from the nearest edge, which is what lets a ship clip the lip of an exit or
## the nose of a gore and slide off it instead of catching. A ship further outside
## than its own radius is left alone: the wall is a surface, not a magnet.
func contact(point: Vector3, radius: float) -> HighwayContact:
	var best: HighwayContact = null
	for i in _normals.size():
		var found := _triangle_contact(_tris[i * 3], _tris[i * 3 + 1],
			_tris[i * 3 + 2], _normals[i], point, radius)
		if found != null and (best == null or found.depth > best.depth):
			best = found
	return best


## How far this point is from the wall: signed over a face (negative is outside
## it), an ordinary distance to the nearest edge otherwise. What the HUD reads.
func distance(point: Vector3) -> float:
	var best := INF
	for i in _normals.size():
		var a := _tris[i * 3]
		var b := _tris[i * 3 + 1]
		var c := _tris[i * 3 + 2]
		var n := _normals[i]
		var d := (point - a).dot(n)
		var here := absf(d)
		var signed := d
		if not _over_face(point - n * d, a, b, c, n):
			here = point.distance_to(_closest_on_edges(point, a, b, c))
			signed = here
		if here < absf(best) or best == INF:
			best = signed
	return best


func _triangle_contact(a: Vector3, b: Vector3, c: Vector3, n: Vector3,
		point: Vector3, radius: float) -> HighwayContact:
	var d := (point - a).dot(n)
	if _over_face(point - n * d, a, b, c, n):
		if absf(d) >= radius:
			return null
		# Signed: a point that has passed through the plane reports a depth greater
		# than the radius and is pushed all the way back in.
		return HighwayContact.make(n, radius - d, wall_name, tube)
	var closest := _closest_on_edges(point, a, b, c)
	var away := point - closest
	var gap := away.length()
	if gap >= radius:
		return null
	if gap <= 0.000001:
		return HighwayContact.make(n, radius, wall_name, tube)
	return HighwayContact.make(away / gap, radius - gap, wall_name, tube)


static func _over_face(q: Vector3, a: Vector3, b: Vector3, c: Vector3,
		n: Vector3) -> bool:
	return (b - a).cross(q - a).dot(n) >= 0.0 \
		and (c - b).cross(q - b).dot(n) >= 0.0 \
		and (a - c).cross(q - c).dot(n) >= 0.0


static func _closest_on_edges(p: Vector3, a: Vector3, b: Vector3,
		c: Vector3) -> Vector3:
	var best := _closest_on_segment(p, a, b)
	for candidate: Vector3 in [_closest_on_segment(p, b, c),
			_closest_on_segment(p, c, a)]:
		if p.distance_squared_to(candidate) < p.distance_squared_to(best):
			best = candidate
	return best


static func _closest_on_segment(p: Vector3, a: Vector3, b: Vector3) -> Vector3:
	var ab := b - a
	var span := ab.length_squared()
	if span <= 0.0000001:
		return a
	return a + ab * clampf((p - a).dot(ab) / span, 0.0, 1.0)
