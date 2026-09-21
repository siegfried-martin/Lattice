class_name HighwaySection
extends RefCounted
## One tile of road: a square-section tube with four walls and two open ends.
##
## The tile is the unit the human asked for — *"a tileset of square tubes that can
## connect and branch"*. Its ends are never walls, because an end is how two tiles
## connect; its four sides are walls unless declared **open**, and an open wall is
## simply a quad that is not made. Branching (build order step 3) is that one
## declaration and nothing else.
##
## Square rather than the lozenge in `EXPLORATION_DESIGN.md`: the brief's
## one-sentence requirement says square tubes and says to judge against it first,
## and flat walls are what let two tiles meet at a branch. Flagged in
## `docs/HIGHWAY_BUILD_ORDER.md` rather than settled here.
##
## Pure: no scene tree, no tuning, no disk. Geometry is in the road's own frame, so
## the whole road moves with one node under the floating origin (ADR 0020).

## Bit flags, so a tile can open more than one wall. Appended to, never renumbered.
enum Wall { LEFT = 1, RIGHT = 2, FLOOR = 4, CEILING = 8 }
const ALL_WALLS: int = Wall.LEFT | Wall.RIGHT | Wall.FLOOR | Wall.CEILING

## Centre of the entry face, with -z along the road, x to the right, y up — the
## ship's own convention, so a tile's frame and a ship's basis mean the same thing.
var entry: Transform3D = Transform3D.IDENTITY
var length: float = 0.0
var half_width: float = 0.0
var half_height: float = 0.0
## Walls this tile does NOT have, as `Wall` flags. Zero is a closed tube.
var open: int = 0
## Where it sits in the road, for the HUD and for failure messages.
var index: int = 0


static func make(at: Transform3D, along: float, across: float, tall: float,
		opening: int, at_index: int) -> HighwaySection:
	var section := HighwaySection.new()
	section.entry = at
	section.length = along
	section.half_width = across * 0.5
	section.half_height = tall * 0.5
	section.open = opening
	section.index = at_index
	return section


## The way the road runs through this tile.
func forward() -> Vector3:
	return -entry.basis.z


## The frame the next tile starts at. Straight, for now: build order step 2 turns
## this into an advance plus a yaw and a pitch, and nothing else has to change,
## because every other function here already works off `entry` alone.
func exit() -> Transform3D:
	return Transform3D(entry.basis, entry.origin + forward() * length)


func center() -> Vector3:
	return entry.origin + forward() * (length * 0.5)


## How far along the tile a point is, from its entry face. Outside 0..length means
## the point is in some other tile, or off the end of the road.
func along(point: Vector3) -> float:
	return (point - entry.origin).dot(forward())


## Every wall this tile actually has. The road is drawn from this and collided
## against this; there is no third thing.
func walls() -> Array[HighwayQuad]:
	var list: Array[HighwayQuad] = []
	for face: Dictionary in _faces():
		if open & int(face["flag"]):
			continue
		list.append(HighwayQuad.make(center() + Vector3(face["offset"]),
			forward(), Vector3(face["across"]),
			length * 0.5, float(face["half_across"]),
			Vector3(face["inward"]), String(face["name"])))
	return list


## A band across each wall at the tile's entry face, lying just inside the surface.
##
## Not decoration: a kilometre of smooth tube at 15 m/s reads as a still image, and
## the tile boundary is the one feature the road has of its own. Built from the same
## face list as `walls()`, so an open wall has no rib either and the road cannot
## grow a stripe across a gap.
func ribs(width: float, inset: float) -> Array[HighwayQuad]:
	var list: Array[HighwayQuad] = []
	var half := minf(width, length) * 0.5
	for face: Dictionary in _faces():
		if open & int(face["flag"]):
			continue
		var inward := Vector3(face["inward"])
		list.append(HighwayQuad.make(
			entry.origin + forward() * half + Vector3(face["offset"]) + inward * inset,
			forward(), Vector3(face["across"]),
			half, float(face["half_across"]),
			inward, String(face["name"])))
	return list


## The four sides, each as the offset from the tile's centre to the wall's centre,
## the axis across it, and the direction it pushes. One list, read by both `walls()`
## and `ribs()`, so "which walls does this tile have" is answered in one place.
func _faces() -> Array[Dictionary]:
	var right := entry.basis.x
	var up := entry.basis.y
	return [
		{"flag": Wall.LEFT, "name": "left", "offset": -right * half_width,
			"across": up, "half_across": half_height, "inward": right},
		{"flag": Wall.RIGHT, "name": "right", "offset": right * half_width,
			"across": up, "half_across": half_height, "inward": -right},
		{"flag": Wall.FLOOR, "name": "floor", "offset": -up * half_height,
			"across": right, "half_across": half_width, "inward": up},
		{"flag": Wall.CEILING, "name": "ceiling", "offset": up * half_height,
			"across": right, "half_across": half_width, "inward": -up},
	]
