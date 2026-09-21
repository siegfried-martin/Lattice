class_name HighwaySection
extends RefCounted
## One tile of road: the four-walled space between two frames.
##
## The tile is the unit the human asked for — *"a tileset of square tubes that can
## connect and branch"*. A tile's two ends are never walls, because an end is how two
## tiles connect; its four sides are walls unless declared **open**, and an open wall
## is simply a quad that is not made. That one declaration is every opening the road
## has: the seam between tiles, the side of a carriageway where a ramp peels away, the
## side of the ramp that meets it.
##
## Each end is a rectangle placed by a frame, and the walls join corner to corner. So
## a tile bends when its two frames face different ways, climbs when one is higher,
## and tapers when one rectangle is narrower — which is how a ramp opens out of the
## road from nothing. No special tile for any of those; one tile, different frames.
##
## Square rather than the lozenge in `EXPLORATION_DESIGN.md`: the brief's
## one-sentence requirement says square tubes and says to judge against it first.
## Flagged in `docs/HIGHWAY_BUILD_ORDER.md` rather than settled here.
##
## Pure: no scene tree, no tuning, no disk. Geometry is in the road's own frame, so
## the whole road moves with one node under the floating origin (ADR 0020).

## Bit flags, so a tile can open more than one wall. Appended to, never renumbered.
enum Wall { LEFT = 1, RIGHT = 2, FLOOR = 4, CEILING = 8 }
const ALL_WALLS: int = Wall.LEFT | Wall.RIGHT | Wall.FLOOR | Wall.CEILING

## Centre of each end face, with -z along the road, x to the right, y up — the
## ship's own convention, so a tile's frame and a ship's basis mean the same thing.
var start: Transform3D = Transform3D.IDENTITY
var end: Transform3D = Transform3D.IDENTITY
var start_half_width: float = 0.0
var end_half_width: float = 0.0
var half_height: float = 0.0
## Walls this tile does NOT have, as `Wall` flags. Zero is a closed tube.
var open: int = 0
## Where it sits in its tube, and which tube that is.
var index: int = 0
var tube: String = ""
## Which route in the shell this tile belongs to. An index rather than a reference,
## so a route and its tiles do not hold each other alive.
var route_id: int = -1

var _walls: Array[HighwayQuad] = []
var _box: AABB


static func make(from: Transform3D, to: Transform3D, from_half_width: float,
		to_half_width: float, tall: float, opening: int, at_index: int,
		in_tube: String) -> HighwaySection:
	var section := HighwaySection.new()
	section.start = from
	section.end = to
	section.start_half_width = from_half_width
	section.end_half_width = to_half_width
	section.half_height = tall * 0.5
	section.open = opening
	section.index = at_index
	section.tube = in_tube
	section._build()
	return section


## A straight box. What step 1 was, and still what most of a road is.
static func straight(from: Transform3D, along: float, across: float, tall: float,
		opening: int, at_index: int) -> HighwaySection:
	var to := Transform3D(from.basis, from.origin - from.basis.z * along)
	return make(from, to, across * 0.5, across * 0.5, tall, opening, at_index, "")


func _build() -> void:
	_walls.clear()
	var s := _corners(start, start_half_width)
	var e := _corners(end, end_half_width)
	var right := (start.basis.x + end.basis.x).normalized()
	var up := (start.basis.y + end.basis.y).normalized()
	# Corners are [left-bottom, left-top, right-top, right-bottom]. Every wall lists
	# its two START corners first and its END corners after, in matching order, so
	# `ribs()` can find the start edge without knowing which wall it is looking at.
	for face: Array in [
			[Wall.LEFT, "left", s[0], s[1], e[1], e[0], right],
			[Wall.RIGHT, "right", s[3], s[2], e[2], e[3], -right],
			[Wall.FLOOR, "floor", s[0], s[3], e[3], e[0], up],
			[Wall.CEILING, "ceiling", s[1], s[2], e[2], e[1], -up]]:
		if open & int(face[0]):
			continue
		var quad := HighwayQuad.make(face[2], face[3], face[4], face[5], face[6],
			String(face[1]), tube)
		if not quad.is_empty():
			_walls.append(quad)

	_box = AABB(s[0], Vector3.ZERO)
	for corner: Vector3 in s + e:
		_box = _box.expand(corner)


func _corners(frame: Transform3D, half_width: float) -> Array[Vector3]:
	return [frame * Vector3(-half_width, -half_height, 0.0),
		frame * Vector3(-half_width, half_height, 0.0),
		frame * Vector3(half_width, half_height, 0.0),
		frame * Vector3(half_width, -half_height, 0.0)]


## Every wall this tile actually has. The road is drawn from this and collided
## against this; there is no third thing.
func walls() -> Array[HighwayQuad]:
	return _walls


## A band across each wall at the tile's start, lying just inside the surface.
##
## Not decoration: a kilometre of smooth tube reads as a still image, and the tile
## boundary is the one feature a road has of its own. Built from `walls()`, so an
## open wall has no rib either and the road cannot grow a stripe across a gap.
func ribs(width: float, inset: float) -> Array[HighwayQuad]:
	var list: Array[HighwayQuad] = []
	var share := clampf(width / maxf(length(), 0.001), 0.0, 1.0)
	var s := _corners(start, start_half_width)
	var e := _corners(end, end_half_width)
	var right := (start.basis.x + end.basis.x).normalized()
	var up := (start.basis.y + end.basis.y).normalized()
	for face: Array in [
			[Wall.LEFT, s[0], s[1], e[1], e[0], right],
			[Wall.RIGHT, s[3], s[2], e[2], e[3], -right],
			[Wall.FLOOR, s[0], s[3], e[3], e[0], up],
			[Wall.CEILING, s[1], s[2], e[2], e[1], -up]]:
		if open & int(face[0]):
			continue
		var a: Vector3 = face[1]
		var b: Vector3 = face[2]
		var shift: Vector3 = Vector3(face[5]) * inset
		var rib := HighwayQuad.make(a + shift, b + shift,
			b.lerp(face[3], share) + shift, a.lerp(face[4], share) + shift,
			face[5], "rib", tube)
		if not rib.is_empty():
			list.append(rib)
	return list


func length() -> float:
	return start.origin.distance_to(end.origin)


## How far through the tile a point is, 0 at the start face and 1 at the end, or -1
## when it is not between the two. Measured against each end's own facing, so a tile
## in a bend is judged by both of its ends rather than by one of them.
func t_of(point: Vector3) -> float:
	var into := (point - start.origin).dot(-start.basis.z)
	var left := (end.origin - point).dot(-end.basis.z)
	if into < 0.0 or left < 0.0 or into + left <= 0.0:
		return -1.0
	return into / (into + left)


## The frame partway through: where the road is here and which way it faces.
func frame_at(t: float) -> Transform3D:
	return Transform3D(start.basis.slerp(end.basis, t),
		start.origin.lerp(end.origin, t))


func half_width_at(t: float) -> float:
	return lerpf(start_half_width, end_half_width, t)


## Is the point inside this tile's cross-section? What "on the road" means, for the
## cruise drive, the camera and the HUD — never for collision, which is the walls'.
func contains(point: Vector3) -> bool:
	var t := t_of(point)
	if t < 0.0:
		return false
	var local := frame_at(t).affine_inverse() * point
	return absf(local.x) <= half_width_at(t) + 0.001 \
		and absf(local.y) <= half_height + 0.001


func aabb() -> AABB:
	return _box
