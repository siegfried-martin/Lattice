class_name HighwayShell
extends RefCounted
## The whole road as a list of walls: every quad of every tile of every tube, and
## the arithmetic that keeps a ship inside them.
##
## The shell is the only description of the road there is. `triangles()` is what
## gets drawn and `settle()` is what gets collided, off the same quads — so "where is
## this wall open" is not a rule the mesh and the collider have to agree about, it is
## a quad that does not exist. The gate asserts the drawn triangles and this list are
## the same set, vertex for vertex.
##
## It does not know what a carriageway or a ramp is. It holds tiles; `HighwayRoute`
## gives them an order and `HighwayLayout` decides where they go.
##
## Pure: no scene tree, no tuning, no disk.

## How many times `settle()` re-measures while pushing a sphere clear. Only the
## deepest contact is resolved each pass, then everything is measured again — two
## walls that are the same plane either side of a seam would otherwise both push and
## double the correction. A corner takes two passes; eight covers a corner at a seam
## in a bend. Infrastructure, not feel.
const SOLVER_PASSES: int = 8
## How far `clearance()` looks for a wall before saying there is no road. Only the
## HUD asks, and only to name the nearest wall. Infrastructure.
const CLEARANCE_REACH: float = 400.0

var routes: Array[HighwayRoute] = []
## Every tile, flat, in route order. What the walls and the broadphase walk.
var sections: Array[HighwaySection] = []


## Take a finished layout.
func adopt(layout_routes: Array[HighwayRoute]) -> void:
	routes = layout_routes
	sections.clear()
	for route in routes:
		sections.append_array(route.sections)


## A straight run of identical tiles from `start`: the tileset at its simplest, and
## what the gate's step-1 checks still use.
func build_chain(start: Transform3D, count: int, section_length: float,
		width: float, height: float) -> void:
	var route := HighwayRoute.new()
	route.name = "chain"
	var frame := start
	for i in maxi(count, 0):
		var section := HighwaySection.straight(frame, section_length, width, height,
			0, i)
		section.route_id = 0
		route.sections.append(section)
		frame = section.end
	route.finish()
	adopt([route])


## Every wall of the road. Drawn from this; collided against this.
func quads() -> Array[HighwayQuad]:
	var list: Array[HighwayQuad] = []
	for section in sections:
		list.append_array(section.walls())
	return list


## Every triangle of the road's surface, in the order the mesh emits them.
func triangles() -> PackedVector3Array:
	var out := PackedVector3Array()
	for section in sections:
		for quad in section.walls():
			out.append_array(quad.triangles())
	return out


## The tiles whose bounds, grown by `margin`, contain this point. The broadphase: a
## ship is only ever near a handful of the road's tiles.
func nearby(point: Vector3, margin: float) -> Array[HighwaySection]:
	var list: Array[HighwaySection] = []
	for section in sections:
		if section.aabb().grow(margin).has_point(point):
			list.append(section)
	return list


## A sphere put back inside the road, with every wall that had to push it.
##
## Returns `{"position": Vector3, "contacts": Array[HighwayContact]}`. The contacts
## are measured at the sphere's *original* position, so what the caller reads is the
## impact that happened rather than the geometry after the correction.
func settle(point: Vector3, radius: float) -> Dictionary:
	var found := contacts(point, radius)
	var here := point
	if not found.is_empty():
		for _pass in SOLVER_PASSES:
			var now := contacts(here, radius)
			if now.is_empty():
				break
			here += now[0].normal * now[0].depth
	return {"position": here, "contacts": found}


## Every wall touching a sphere at this point, deepest first — one contact per
## wall, however many tiles that wall spans here.
func contacts(point: Vector3, radius: float) -> Array[HighwayContact]:
	var by_wall := {}
	for section in nearby(point, radius):
		for quad in section.walls():
			var contact := quad.contact(point, radius)
			if contact == null:
				continue
			var key := contact.key()
			if not by_wall.has(key) or contact.depth > (by_wall[key] as HighwayContact).depth:
				by_wall[key] = contact
	var list: Array[HighwayContact] = []
	for contact: HighwayContact in by_wall.values():
		list.append(contact)
	list.sort_custom(func(a: HighwayContact, b: HighwayContact) -> bool:
		return a.depth > b.depth)
	return list


## How far the nearest wall is, and which one — `{"distance": float, "wall": String,
## "tube": String}`. Negative is outside that wall; `INF` means no road within
## `CLEARANCE_REACH`.
func clearance(point: Vector3) -> Dictionary:
	var best := INF
	var wall := "—"
	var tube := "—"
	for section in nearby(point, CLEARANCE_REACH):
		for quad in section.walls():
			var d := quad.distance(point)
			if best == INF or absf(d) < absf(best):
				best = d
				wall = quad.wall_name
				tube = quad.tube
	return {"distance": best, "wall": wall, "tube": tube}


## Which tile a point is in — `{"route": int, "section": int, "t": float}`, route
## -1 when it is on no road at all. Where tubes run side by side the carriageway is
## preferred, so a ship in the open stretch beside a ramp is on the road it is on.
func locate(point: Vector3) -> Dictionary:
	var found := {"route": -1, "section": -1, "t": 0.0}
	for section in nearby(point, 0.5):
		if not section.contains(point):
			continue
		var t := section.t_of(point)
		var here := {"route": section.route_id, "section": section.index, "t": t}
		if section.route_id >= 0 and section.route_id < routes.size() \
				and routes[section.route_id].kind == HighwayRoute.Kind.CARRIAGEWAY:
			return here
		if int(found["route"]) < 0:
			found = here
	return found


## Is this point on the road? The cruise drive's question.
func on_road(point: Vector3) -> bool:
	return int(locate(point)["route"]) >= 0


## The road's frame where this point is: which way the tube runs here. Null off the
## road.
func frame_at(point: Vector3) -> Variant:
	var where := locate(point)
	var route_id := int(where["route"])
	if route_id < 0:
		return null
	return routes[route_id].sections[int(where["section"])].frame_at(float(where["t"]))


## Where along its route a point is — `{"route": int, "name": String, "along":
## float, "total": float}`. Route -1 is off the road.
func progress(point: Vector3) -> Dictionary:
	var where := locate(point)
	var route_id := int(where["route"])
	if route_id < 0:
		return {"route": -1, "name": "", "along": 0.0, "total": 0.0}
	var route := routes[route_id]
	return {"route": route_id, "name": route.name,
		"along": route.distance_at(int(where["section"]), float(where["t"])),
		"total": route.length()}


func total_length() -> float:
	var run := 0.0
	for route in routes:
		run += route.length()
	return run


## The frame at the very start of the first route, for placing a ship on it.
func mouth() -> Transform3D:
	return Transform3D.IDENTITY if sections.is_empty() else sections[0].start
