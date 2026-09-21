class_name HighwayRoad
extends Node3D
## The road, on screen and under the ship: it draws the shell's own walls, runs the
## cruise drive on them, and keeps the ship inside them.
##
## **Build order step 2** (`docs/HIGHWAY_BUILD_ORDER.md`): two carriageways that
## bend and climb, with an on-ramp and an off-ramp each way at every junction. The
## layout is `HighwayLayout`'s; this node asks `HighwayShell` for quads and does not
## know what shape they make.
##
## The mesh is the shell's triangles drawn and the collision is the same triangles
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

var _layout := HighwayLayout.new()
var _shell := HighwayShell.new()
var _wall: MeshInstance3D
## Ribs, one mesh per colour: each carriageway its own, so which way a tube runs is
## visible across the median, and the ramps a third.
var _ribs: Dictionary = {}
## Which walls were touching last frame, by `HighwayContact.key()`. A bounce is the
## frame a wall *starts* touching; while it keeps touching, the ship scrapes along
## instead of being kicked repeatedly by the same surface.
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

	for group: String in ["northbound", "southbound", "ramps"]:
		var ribs := MeshInstance3D.new()
		ribs.name = "Ribs" + group.capitalize()
		ribs.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		add_child(ribs)
		_ribs[group] = ribs

	rebuild()
	Tuning.reloaded.connect(rebuild)


## Every number the layout needs, read here so the layout itself stays pure.
static func layout_params() -> Dictionary:
	return {
		"tile": Tuning.num("highway/section_length"),
		"width": Tuning.num("highway/tube_width"),
		"height": Tuning.num("highway/tube_height"),
		"median": Tuning.num("highway/median_width"),
		"junctions": Tuning.integer("highway/junctions"),
		"bend_tiles": Tuning.integer("highway/bend_tiles"),
		"bend_deg": Tuning.num("highway/bend_deg_per_tile"),
		"climb_deg": Tuning.num("highway/climb_deg_per_tile"),
		"end_tiles": Tuning.integer("highway/end_run_tiles"),
		"ramp_width": Tuning.num("highway/ramp_width"),
		"taper_tiles": maxi(Tuning.integer("highway/ramp_taper_tiles"), 1),
		"diverge_tiles": maxi(Tuning.integer("highway/ramp_diverge_tiles"), 1),
		"ramp_gap": Tuning.num("highway/ramp_gap"),
		"tail_tiles": maxi(Tuning.integer("highway/ramp_tail_tiles"), 1),
		"tail_deg": Tuning.num("highway/ramp_tail_deg_per_tile"),
		"gap_tiles": maxi(Tuning.integer("highway/junction_gap_tiles"), 0),
	}


## Lay the road out from tuning, and draw exactly what was laid out.
func rebuild() -> void:
	_layout.build(layout_params())
	_shell.adopt(_layout.routes)

	_wall.mesh = _mesh_of(_shell.quads())
	_wall.material_override = _material("highway/wall_color", "highway/wall_alpha")

	var grouped := {}
	for group: String in _ribs:
		grouped[group] = [] as Array[HighwayQuad]
	for route in _shell.routes:
		var group := route.name if route.kind == HighwayRoute.Kind.CARRIAGEWAY \
			and grouped.has(route.name) else "ramps"
		for section in route.sections:
			(grouped[group] as Array[HighwayQuad]).append_array(
				section.ribs(Tuning.num("highway/rib_width"), RIB_INSET))
	for group: String in grouped:
		var ribs := _ribs[group] as MeshInstance3D
		ribs.mesh = _mesh_of(grouped[group])
		ribs.material_override = _material("highway/%s_rib_color" % (
			"ramp" if group == "ramps" else group), "highway/rib_alpha")


## One surface from a list of quads. The only place road geometry becomes a mesh,
## and it takes each quad's own triangles whole — there is nothing here that could
## draw a wall the collider does not have.
func _mesh_of(quads: Array[HighwayQuad]) -> ArrayMesh:
	var verts := PackedVector3Array()
	var normals := PackedVector3Array()
	for quad in quads:
		verts.append_array(quad.triangles())
		for n in quad.normals():
			normals.append_array(PackedVector3Array([n, n, n]))
	if verts.is_empty():
		return null
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = verts
	arrays[Mesh.ARRAY_NORMAL] = normals
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	return mesh


## Translucent, and lit by nothing. The lane has to stay visually open (ADR 0057) —
## a square tube you cannot see out of is the tunnel that design says not to build.
func _material(color_key: String, alpha_key: String) -> StandardMaterial3D:
	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	var color := Tuning.color(color_key)
	color.a = clampf(Tuning.num(alpha_key), 0.0, 1.0)
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
	drive_cruise(here, delta)
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
		still_touching[contact.key()] = true
		var world_normal: Vector3 = global_transform.basis * contact.normal
		# Whatever the ship is still being pushed *into* the wall with is absorbed
		# by it. Without this, a ship held against a wall keeps spending a knock it
		# has already had, and the correction quietly fights it every frame.
		var into_external := ship.external_velocity().dot(world_normal)
		if into_external < 0.0:
			ship.push(world_normal * -into_external, decay, limit)
		if _touched.has(contact.key()):
			continue
		var into := moving.dot(contact.normal)
		if into >= 0.0:
			continue
		ship.push(world_normal * (-(1.0 + restitution) * into), decay, limit)
		if absf(into) > _last_bounce_speed or _seconds_since_bounce > BOUNCE_REPORT_HOLD:
			_last_bounce_speed = absf(into)
			_last_bounce_wall = "%s wall of the %s" % [contact.wall_name, contact.tube]
			_seconds_since_bounce = 0.0

	_touched = still_touching


## Run the cruise drive while the ship is on the road — carriageway or ramp — and
## wind it down when it is not. Spooled rather than switched, so taking an on-ramp is
## a climb and not a launch: the brief's "no big acceleration on entering".
##
## The throttle is still the player's. This raises what full throttle means; it never
## moves the lever, so a ship at rest on the road stays at rest.
##
## Public so the gate can step it by hand; `_process` is the only caller in the game.
func drive_cruise(here: Vector3, delta: float) -> void:
	var cruise := Tuning.num("exploration/cruise_speed")
	var wanted := cruise if _shell.on_road(here) and ship.has_cruise_drive() else 0.0
	var rate := cruise / maxf(Tuning.num("highway/cruise_spool_seconds"), 0.001)
	# Spooled between the hull's own speed and cruise, never from zero: below the
	# hull's speed the cruise drive changes nothing, and a spool that spent its
	# first second there would read as lag.
	var hull := HullClass.max_speed(ship.hull_class)
	var next := move_toward(maxf(ship.cruise_ceiling, hull), maxf(wanted, hull),
		rate * delta)
	ship.cruise_ceiling = 0.0 if wanted <= 0.0 and next <= hull else next


## Is the cruise drive running, and how far up is it? For the HUD.
func cruise_share() -> float:
	var cruise := Tuning.num("exploration/cruise_speed")
	return 0.0 if ship == null or cruise <= 0.0 \
		else clampf(ship.cruise_ceiling / cruise, 0.0, 1.0)


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


func layout() -> HighwayLayout:
	return _layout


## The route named, or null. Tests and the harness find ramps this way.
func route_named(called: String) -> HighwayRoute:
	for route in _shell.routes:
		if route.name == called:
			return route
	return null


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


## The road's own heading where the ship is, in world space, or null off the road.
## What the camera is held to.
func road_basis_at_ship() -> Variant:
	var frame: Variant = _shell.frame_at(ship_in_road())
	if frame == null:
		return null
	return global_transform.basis * (frame as Transform3D).basis
