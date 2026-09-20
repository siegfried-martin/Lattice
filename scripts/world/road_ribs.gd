class_name RoadRibs
extends Node3D
## One road's rib collars and the lamp bars on them, as instances placed every frame
## rather than as part of the streamed chunk mesh. They have to move: in the highway
## gear the road is a treadmill (`Road.slip`). The ship goes through the world at the
## geared speed, and the structure it is measuring that speed against slides along
## with it by the surplus, so a collar passes at the felt speed while the world
## outside passes faster. The collision reads the same slipped positions
## (`Road.rib_margin_at`), so what you see is still what you hit.
##
## A collar that would stand inside a neighbouring tube — at a junction, or where a
## ramp's head runs inside its host — is hidden, which is the clip rule the chunk
## mesh applies, done per rib.

var road: Road

var _collars: MultiMeshInstance3D
var _lamps: MultiMeshInstance3D
var _neighbours: Array[Tube] = []
var _corners: Array[Vector2] = []


func setup(for_road: Road) -> void:
	road = for_road
	name = "Ribs " + road.name
	_collars = MultiMeshInstance3D.new()
	_collars.name = "Collars"
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.mesh = _collar_mesh()
	_collars.multimesh = mm
	_collars.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
	add_child(_collars)
	_lamps = RoadLamps.fixture_instance(road)
	add_child(_lamps)
	_neighbours.clear()
	for tube in road.tubes:
		for n in tube.neighbours:
			if not _neighbours.has(n):
				_neighbours.append(n)
	var w := road.half_width + 0.3 + road.rib_protrusion
	var top := road.half_height + 0.3 + road.rib_protrusion
	var bottom := -(road.half_height + 0.3 + Tuning.num("exploration/structure_floor_thickness")
		+ road.rib_protrusion)
	_corners = [Vector2(-w, top), Vector2(w, top), Vector2(-w, bottom), Vector2(w, bottom),
		Vector2(0.0, top), Vector2(0.0, bottom)]
	place()


## Put every collar and lamp bar where the road's ribs are this frame.
func place() -> void:
	if road == null:
		return
	var ribs := road.rib_positions()
	var mm := _collars.multimesh
	if mm.instance_count != ribs.size():
		mm.instance_count = ribs.size()
	for i in ribs.size():
		var f := road.path.frame(ribs[i])
		var right: Vector3 = f["right"]
		var up: Vector3 = f["up"]
		var xf := Transform3D(Basis(right, up, right.cross(up)), f["pos"])
		if _inside_a_neighbour(xf):
			mm.set_instance_transform(i, Transform3D(Basis().scaled(Vector3.ZERO), f["pos"]))
			continue
		mm.set_instance_transform(i, xf)
	RoadLamps.place_fixtures(_lamps, road, ribs)


func _inside_a_neighbour(xf: Transform3D) -> bool:
	if _neighbours.is_empty():
		return false
	for c in _corners:
		var p := xf.origin + xf.basis.x * c.x + xf.basis.y * c.y
		for n in _neighbours:
			if n.contains(p):
				return true
	return false


## The collar: four boxes around the section, `rib_thickness` along the road, in the
## rib's own frame (x across, y up, z along).
func _collar_mesh() -> ArrayMesh:
	var w := road.half_width + 0.3
	var h := road.half_height + 0.3
	var ft := Tuning.num("exploration/structure_floor_thickness")
	var pr := road.rib_protrusion
	var half := road.rib_thickness * 0.5
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	_box(st, Vector3(-w - pr, h, -half), Vector3(w + pr, h + pr, half))
	_box(st, Vector3(-w - pr, -h - ft - pr, -half), Vector3(w + pr, -h - ft, half))
	_box(st, Vector3(w, -h - ft, -half), Vector3(w + pr, h, half))
	_box(st, Vector3(-w - pr, -h - ft, -half), Vector3(-w, h, half))
	var mesh := st.commit()
	mesh.surface_set_material(0, RoadMesh.materials()[RoadMesh.MAT_METAL])
	return mesh


static func _box(st: SurfaceTool, lo: Vector3, hi: Vector3) -> void:
	var p := [
		Vector3(lo.x, lo.y, lo.z), Vector3(hi.x, lo.y, lo.z), Vector3(hi.x, hi.y, lo.z), Vector3(lo.x, hi.y, lo.z),
		Vector3(lo.x, lo.y, hi.z), Vector3(hi.x, lo.y, hi.z), Vector3(hi.x, hi.y, hi.z), Vector3(lo.x, hi.y, hi.z),
	]
	# Each face's corners counter-clockwise around its outward normal; see `_face`.
	_face(st, p[0], p[3], p[2], p[1], Vector3(0, 0, -1))
	_face(st, p[4], p[5], p[6], p[7], Vector3(0, 0, 1))
	_face(st, p[0], p[1], p[5], p[4], Vector3(0, -1, 0))
	_face(st, p[3], p[7], p[6], p[2], Vector3(0, 1, 0))
	_face(st, p[0], p[4], p[7], p[3], Vector3(-1, 0, 0))
	_face(st, p[1], p[2], p[6], p[5], Vector3(1, 0, 0))


## `a b c d` run counter-clockwise around `n` (the cross-product sense); emitted
## reversed, so the normal side is Godot's clockwise front, as `RoadMesh._emit` does.
static func _face(st: SurfaceTool, a: Vector3, b: Vector3, c: Vector3, d: Vector3, n: Vector3) -> void:
	for v: Vector3 in [a, c, b, a, d, c]:
		st.set_normal(n)
		st.add_vertex(v)
