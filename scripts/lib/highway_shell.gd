class_name HighwayShell
extends RefCounted
## The whole road as a list of walls: every quad of every tile, and the arithmetic
## that keeps a ship inside them.
##
## The shell is the only description of the road there is. `quads()` is what gets
## drawn and `settle()` is what gets collided, off the same array — so "where is
## this wall open" is not a rule the mesh and the collider have to agree about, it
## is a quad that does not exist. The gate asserts the drawn triangles and this
## array are the same set.
##
## Pure: no scene tree, no tuning, no disk.

## How many times `settle()` re-measures while pushing a sphere clear. A corner is
## two walls at once and the first push moves the sphere relative to the second, so
## one pass is not enough; three converges on every shape this road can make.
## Infrastructure, not feel.
const SOLVER_PASSES: int = 3

var sections: Array[HighwaySection] = []

var _quads: Array[HighwayQuad] = []


## A straight run of identical tiles from `start`. The tileset at its simplest: one
## tile, repeated, each beginning where the last ended. Step 2 gives the tiles turn
## deltas and this becomes a road that bends, without the chaining changing.
func build_chain(start: Transform3D, count: int, section_length: float,
		width: float, height: float) -> void:
	sections.clear()
	var frame := start
	for i in maxi(count, 0):
		var section := HighwaySection.make(frame, section_length, width, height,
			0, i)
		sections.append(section)
		frame = section.exit()
	_rebuild_quads()


func _rebuild_quads() -> void:
	_quads.clear()
	for section in sections:
		_quads.append_array(section.walls())


## Every wall of the road. Drawn from this; collided against this.
func quads() -> Array[HighwayQuad]:
	return _quads


## Every triangle of the road's surface, in the order the mesh emits them.
func triangles() -> PackedVector3Array:
	var out := PackedVector3Array()
	for quad in _quads:
		out.append_array(quad.triangles())
	return out


func rib_quads(width: float, inset: float) -> Array[HighwayQuad]:
	var list: Array[HighwayQuad] = []
	for section in sections:
		list.append_array(section.ribs(width, inset))
	return list


## A sphere put back inside the road, with every wall that had to push it.
##
## Returns `{"position": Vector3, "contacts": Array[HighwayContact]}`. The contacts
## are measured at the sphere's *original* position, so what the caller reads is the
## impact that happened rather than the geometry after the correction — a bounce is
## about the former.
##
## Every quad is tested. At a few hundred walls that is nothing; a broadphase by
## tile lands when the road is long enough to need one, and belongs here rather than
## in the caller.
func settle(point: Vector3, radius: float) -> Dictionary:
	var found := contacts(point, radius)
	var here := point
	for _pass in SOLVER_PASSES:
		var moved := false
		for contact in contacts(here, radius):
			if contact.depth <= 0.0:
				continue
			here += contact.normal * contact.depth
			moved = true
		if not moved:
			break
	return {"position": here, "contacts": found}


## Every wall touching a sphere at this point, deepest first.
func contacts(point: Vector3, radius: float) -> Array[HighwayContact]:
	var list: Array[HighwayContact] = []
	for quad in _quads:
		var contact := quad.contact(point, radius)
		if contact != null:
			list.append(contact)
	list.sort_custom(func(a: HighwayContact, b: HighwayContact) -> bool:
		return a.depth > b.depth)
	return list


## How far the nearest wall is, and which one — `{"distance": float, "wall": String}`.
## Negative distance means the point is outside that wall. `INF` means no road.
func clearance(point: Vector3) -> Dictionary:
	var best := INF
	var name_of := "—"
	for quad in _quads:
		var d := quad.distance(point)
		if d < best:
			best = d
			name_of = quad.wall_name
	return {"distance": best, "wall": name_of}


## Where along the road a point is — `{"section": int, "along": float,
## "total": float}`. Section -1 means it is not inside any tile: off an end, or out
## through an opening.
func progress(point: Vector3) -> Dictionary:
	var run := 0.0
	for section in sections:
		var t := section.along(point)
		if t >= 0.0 and t <= section.length:
			return {"section": section.index, "along": run + t,
				"total": total_length()}
		run += section.length
	return {"section": -1, "along": 0.0, "total": total_length()}


func total_length() -> float:
	var run := 0.0
	for section in sections:
		run += section.length
	return run


## The frame at the very start of the road, for placing a ship on it.
func mouth() -> Transform3D:
	return Transform3D.IDENTITY if sections.is_empty() else sections[0].entry
