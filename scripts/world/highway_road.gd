class_name HighwayRoad
extends Node3D
## The road, on screen and under the ship: it draws the shell's own walls and it
## keeps the ship inside them.
##
## **Build order step 1** (`docs/HIGHWAY_BUILD_ORDER.md`): one straight run of
## identical square tiles. It bends in step 2 and branches in step 3, and neither
## changes anything here — this node asks `HighwayShell` for quads and does not know
## what shape they make.
##
## The mesh is `HighwayShell.quads()` drawn and the collision is the same array
## collided, which is the brief's first lesson made structural rather than
## remembered: there is no rule about where a wall is open, because an open wall is
## a quad that was never made.
##
## Geometry is built in this node's own frame and the node placed, so the whole road
## moves as one thing and nothing caches a world position across frames (ADR 0020).

## Run after the ship has moved. The ship integrates in `_process`, and a wall that
## corrected last frame's position would let a fast hull sit visibly inside it for a
## frame. Infrastructure, not feel.
const AFTER_THE_SHIP: int = 10
## Metres a rib sits inside the wall it is drawn on, so the two do not z-fight.
## Infrastructure: it is the thickness of a coincidence, not a look.
const RIB_INSET: float = 0.35
## Seconds the headline bounce stays the headline before a smaller one can replace
## it. A readout detail — nothing in the world changes with it.
const BOUNCE_REPORT_HOLD: float = 0.2

## The ship this road is holding. Assigned by the scene; nothing is done without it.
var ship: Mothership

var _shell := HighwayShell.new()
var _wall: MeshInstance3D
var _ribs: MeshInstance3D
## Which walls were touching last frame, by name. A bounce is the frame a wall
## *starts* touching; while it keeps touching, the ship scrapes along instead of
## being kicked repeatedly by the same surface.
var _touched: Dictionary = {}
var _last_bounce_speed: float = 0.0
var _last_bounce_wall: String = ""
var _seconds_since_bounce: float = INF


func _ready() -> void:
	process_priority = AFTER_THE_SHIP

	_wall = MeshInstance3D.new()
	_wall.name = "Walls"
	_wall.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(_wall)

	_ribs = MeshInstance3D.new()
	_ribs.name = "Ribs"
	_ribs.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(_ribs)

	rebuild()
	Tuning.reloaded.connect(rebuild)


## Lay the road out from tuning, and draw exactly what was laid out.
func rebuild() -> void:
	_shell.build_chain(Transform3D.IDENTITY,
		Tuning.integer("highway/section_count"),
		Tuning.num("highway/section_length"),
		Tuning.num("highway/tube_width"),
		Tuning.num("highway/tube_height"))

	_wall.mesh = _mesh_of(_shell.quads())
	_wall.material_override = _wall_material()
	_ribs.mesh = _mesh_of(_shell.rib_quads(Tuning.num("highway/rib_width"), RIB_INSET))
	_ribs.material_override = _rib_material()


## One surface from a list of quads. The only place road geometry becomes a mesh,
## and it takes the quads whole — there is nothing here that could draw a wall the
## collider does not have.
func _mesh_of(quads: Array[HighwayQuad]) -> ArrayMesh:
	if quads.is_empty():
		return null
	var verts := PackedVector3Array()
	var normals := PackedVector3Array()
	for quad in quads:
		var tris := quad.triangles()
		verts.append_array(tris)
		for _i in tris.size():
			normals.append(quad.normal)
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = verts
	arrays[Mesh.ARRAY_NORMAL] = normals
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	return mesh


## Translucent, and lit by nothing. The lane has to stay visually open (ADR 0057) —
## a square tube you cannot see out of is the tunnel that design says not to build —
## so the wall reads as a surface without hiding what is past it.
func _wall_material() -> StandardMaterial3D:
	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	var color := Tuning.color("highway/wall_color")
	color.a = clampf(Tuning.num("highway/wall_alpha"), 0.0, 1.0)
	mat.albedo_color = color
	return mat


func _rib_material() -> StandardMaterial3D:
	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	var color := Tuning.color("highway/rib_color")
	color.a = clampf(Tuning.num("highway/rib_alpha"), 0.0, 1.0)
	mat.albedo_color = color
	return mat


func _process(delta: float) -> void:
	_seconds_since_bounce += delta
	if ship == null or not is_instance_valid(ship) or delta <= 0.0:
		return

	# In the road's own frame, both ways. Identity today; correct the day the road
	# is placed somewhere or the origin shifts under it.
	var to_road := global_transform.affine_inverse()
	var here: Vector3 = to_road * ship.global_position
	var radius := wall_clearance()

	var settled := _shell.settle(here, radius)
	var contacts: Array = settled["contacts"]
	if contacts.is_empty():
		_touched.clear()
		return

	ship.global_position = global_transform * Vector3(settled["position"])

	var moving: Vector3 = to_road.basis * ship.velocity()
	var decay := Tuning.num("highway/bounce_decay_seconds")
	var limit := ship.manual_max_speed() \
		* maxf(Tuning.num("highway/bounce_max_speed_fraction"), 0.0)
	var restitution := clampf(Tuning.num("highway/bounce_restitution"), 0.0, 1.0)
	var still_touching: Dictionary = {}

	for contact: HighwayContact in contacts:
		still_touching[contact.wall_name] = true
		var world_normal: Vector3 = global_transform.basis * contact.normal
		# Whatever the ship is still being pushed *into* the wall with is absorbed
		# by it. Without this, a ship held against a wall keeps spending a knock it
		# has already had, and the correction quietly fights it every frame.
		var into_external := ship.external_velocity().dot(world_normal)
		if into_external < 0.0:
			ship.push(world_normal * -into_external, decay, limit)
		if _touched.has(contact.wall_name):
			continue
		var into := moving.dot(contact.normal)
		if into >= 0.0:
			continue
		ship.push(world_normal * (-(1.0 + restitution) * into), decay, limit)
		if absf(into) > _last_bounce_speed or _seconds_since_bounce > BOUNCE_REPORT_HOLD:
			_last_bounce_speed = absf(into)
			_last_bounce_wall = contact.wall_name
			_seconds_since_bounce = 0.0

	_touched = still_touching


## How far the hull is held off a wall.
##
## Scaled by the hull, not fixed: a fighter drawn at a quarter size that stopped a
## gunboat's distance from the wall would be flying down the middle of a corridor it
## could see it was nowhere near.
func wall_clearance() -> float:
	if ship == null or not is_instance_valid(ship):
		return Tuning.num("highway/wall_clearance")
	return Tuning.num("highway/wall_clearance") * ship.hull_scale()


func shell() -> HighwayShell:
	return _shell


## What the HUD reports: the last knock, and how long ago. Speed is the component
## into the wall, not the ship's speed — a graze at full throttle is a small number
## and should read as one.
func last_bounce_speed() -> float:
	return _last_bounce_speed


func last_bounce_wall() -> String:
	return _last_bounce_wall


func seconds_since_bounce() -> float:
	return _seconds_since_bounce


## Where the ship is, in the road's frame. Every question the HUD asks is asked
## here so the scene never has to know which frame the road is drawn in.
func ship_in_road() -> Vector3:
	if ship == null or not is_instance_valid(ship):
		return Vector3.ZERO
	return global_transform.affine_inverse() * ship.global_position
