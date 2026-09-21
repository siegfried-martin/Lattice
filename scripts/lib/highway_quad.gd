class_name HighwayQuad
extends RefCounted
## One wall of the highway: a flat rectangle with an inward normal.
##
## **This is the road's one rule for where a wall is.** A quad is drawn and a quad
## is bounced off; a wall that is open is a quad that was never made. There is no
## second description of the road to keep in step with this one, which is the whole
## reason the shape is expressed this way — see `docs/HIGHWAY_BUILD_ORDER.md`.
##
## Pure: no scene tree, no tuning, no disk. The mesh built from `triangles()` and
## the collision answered by `contact()` come from the same four corners, and the
## gate asserts the drawn triangles and the collided quads are the same set.

## Centre of the rectangle, in the road's own frame.
var center: Vector3 = Vector3.ZERO
## The two unit axes of the rectangle's face, and its half-extents along them.
## `u` runs along the road; `v` runs across the wall.
var u: Vector3 = Vector3.RIGHT
var v: Vector3 = Vector3.UP
var half_u: float = 0.0
var half_v: float = 0.0
## Unit, and pointing **into** the tube — the direction this wall pushes.
var normal: Vector3 = Vector3.UP
## What the HUD calls it: "left", "ceiling", and so on.
var wall_name: String = "wall"


static func make(at: Vector3, along: Vector3, across: Vector3,
		half_along: float, half_across: float, inward: Vector3,
		called: String) -> HighwayQuad:
	var quad := HighwayQuad.new()
	quad.center = at
	quad.u = along.normalized()
	quad.v = across.normalized()
	quad.half_u = half_along
	quad.half_v = half_across
	quad.normal = inward.normalized()
	quad.wall_name = called
	return quad


## The four corners, wound so the face is front-facing seen from inside the tube.
func corners() -> Array[Vector3]:
	var du := u * half_u
	var dv := v * half_v
	# Winding chosen so that (b - a) x (c - a) points along `normal`. Which of the
	# two orders that is depends on the handedness of (u, v, normal), so it is
	# decided here rather than assumed.
	var wound_forward := u.cross(v).dot(normal) > 0.0
	if wound_forward:
		return [center - du - dv, center + du - dv, center + du + dv, center - du + dv]
	return [center - du - dv, center - du + dv, center + du + dv, center + du - dv]


## Two triangles, in the order the mesh emits them. The gate compares this against
## what the `ArrayMesh` actually holds.
func triangles() -> PackedVector3Array:
	var c := corners()
	return PackedVector3Array([c[0], c[1], c[2], c[0], c[2], c[3]])


## How far this point is from the wall. Signed while the point is over the face —
## negative means it is outside the wall — and an ordinary positive distance to the
## nearest edge when it is not. What the HUD's clearance row reads.
func distance(point: Vector3) -> float:
	var rel := point - center
	var du := rel.dot(u)
	var dv := rel.dot(v)
	if absf(du) <= half_u and absf(dv) <= half_v:
		return rel.dot(normal)
	var closest := center + u * clampf(du, -half_u, half_u) \
		+ v * clampf(dv, -half_v, half_v)
	return point.distance_to(closest)


## Where a sphere of `radius` centred at `point` meets this wall, or null.
##
## Exact against the rectangle, with no margin past its edges: an invisible strip
## of collision beyond where the mesh stops is precisely the bug the first road had,
## and it is the reason a branch opening is safe to cut here.
##
## Over the face, the push is always **into** the tube, even from behind the plane —
## a ship that got through is put back on the inside rather than shoved further out.
## Off the end of the face it is a push away from the edge, which is what lets a ship
## clip the lip of an opening and slide off it instead of catching.
func contact(point: Vector3, radius: float) -> HighwayContact:
	var rel := point - center
	var du := rel.dot(u)
	var dv := rel.dot(v)
	var closest := center + u * clampf(du, -half_u, half_u) \
		+ v * clampf(dv, -half_v, half_v)
	var away := point - closest
	var distance := away.length()
	if distance >= radius:
		return null

	var over_face := absf(du) <= half_u and absf(dv) <= half_v
	if over_face:
		# Signed, so a point that has passed through the wall reports a depth
		# greater than the radius and is pushed all the way back in.
		return HighwayContact.make(normal, radius - rel.dot(normal), wall_name)
	if distance <= 0.000001:
		return HighwayContact.make(normal, radius, wall_name)
	return HighwayContact.make(away / distance, radius - distance, wall_name)
