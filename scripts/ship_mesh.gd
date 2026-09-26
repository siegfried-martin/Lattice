class_name ShipMesh
## Procedural ship models from primitive meshes. Noses point down -Z.
## meta "glow_mats": engine materials whose brightness follows speed (see set_engine_glow).

const BEACON_SHADER := preload("res://shaders/beacon.gdshader")
## The freighter model is built at 20 m long and scaled up by this.
const FREIGHTER_SCALE := 2.5


const CONTAINER_COLORS := [Color(0.7, 0.35, 0.15), Color(0.2, 0.45, 0.5), Color(0.5, 0.5, 0.48), Color(0.75, 0.6, 0.2), Color(0.35, 0.2, 0.2)]


static func _mat(c: Color, metallic := 0.5, rough := 0.45) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = c
	m.metallic = metallic
	m.roughness = rough
	return m


static func _glow_mat(glow: Color) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = Color.BLACK
	m.emission_enabled = true
	m.emission = glow
	m.emission_energy_multiplier = 6.0
	return m


static func _engine(root: Node3D, at: Vector3, r: float, length: float, dark: Material, glow: Material) -> void:
	MeshUtil.part(root, MeshUtil.cylinder(r, r, length), at, dark, Vector3(90, 0, 0))
	MeshUtil.part(root, MeshUtil.cylinder(r * 0.75, r * 0.75, 0.2), at + Vector3(0, 0, length * 0.5 + 0.05), glow, Vector3(90, 0, 0))


static func _beacon(root: Node3D, at: Vector3, glow: Color, size: float) -> void:
	var b := MeshInstance3D.new()
	b.mesh = QuadMesh.new()
	var bm := ShaderMaterial.new()
	bm.shader = BEACON_SHADER
	b.material_override = bm
	b.position = at
	b.extra_cull_margin = 200.0
	b.set_instance_shader_parameter("tint", Vector3(glow.r, glow.g, glow.b))
	b.set_instance_shader_parameter("base_size", size)
	root.add_child(b)


## Bulky hauler, about 50 m long. The only class that can use the highway. The parts are
## laid out at 1/FREIGHTER_SCALE and sit under a scaled "hull" node, so the root stays
## unscaled for the code that sets its transform.
static func build_freighter(hull: Color, glow: Color, beacon := false) -> Node3D:
	var root := Node3D.new()
	var body := Node3D.new()
	body.name = "hull"
	body.scale = Vector3.ONE * FREIGHTER_SCALE
	root.add_child(body)
	var hull_mat := _mat(hull, 0.5, 0.5)
	var dark := _mat(hull.darkened(0.6))
	var glass := _mat(Color(0.04, 0.08, 0.14), 0.9, 0.1)
	glass.emission_enabled = true
	glass.emission = Color(0.15, 0.4, 0.7)
	glass.emission_energy_multiplier = 0.6
	var glow_mat := _glow_mat(glow)

	MeshUtil.part(body, MeshUtil.box(4.5, 3.6, 16.0), Vector3.ZERO, hull_mat)
	MeshUtil.part(body, MeshUtil.box(3.6, 2.8, 2.2), Vector3(0, -0.2, -9.0), dark)
	MeshUtil.part(body, MeshUtil.box(3.2, 1.6, 3.2), Vector3(0, 2.5, -5.2), hull_mat)
	MeshUtil.part(body, MeshUtil.box(3.0, 0.6, 0.2), Vector3(0, 2.7, -6.85), glass)
	MeshUtil.part(body, MeshUtil.box(0.3, 2.6, 2.6), Vector3(0, 3.0, 6.4), dark)
	var ci := 0
	for sx in [-1.0, 1.0]:
		for i in 4:
			var cm := _mat(CONTAINER_COLORS[(ci * 3 + int(hull.r * 10.0)) % CONTAINER_COLORS.size()], 0.2, 0.7)
			MeshUtil.part(body, MeshUtil.box(2.2, 2.6, 3.3), Vector3(sx * 3.4, 0.2, -4.2 + i * 3.6), cm)
			ci += 1
	for ex in [-1.3, 1.3]:
		for ey in [-0.9, 0.9]:
			_engine(body, Vector3(ex, ey, 9.4), 0.75, 3.0, dark, glow_mat)
	# Roof turret; its head turns with the gunner's aim (meta "turret_head").
	MeshUtil.part(body, MeshUtil.cylinder(1.3, 1.5, 0.6), Vector3(0, 2.1, 2.5), dark)
	var head := Node3D.new()
	head.position = Vector3(0, 2.7, 2.5)
	body.add_child(head)
	var dome := SphereMesh.new()
	dome.radius = 1.0
	dome.height = 1.4
	MeshUtil.part(head, dome, Vector3.ZERO, hull_mat)
	for bx in [-0.35, 0.35]:
		MeshUtil.part(head, MeshUtil.cylinder(0.14, 0.14, 2.6), Vector3(bx, 0.1, -1.6), dark, Vector3(90, 0, 0))
	root.set_meta("turret_head", head)
	root.set_meta("glow_mats", [glow_mat])
	root.set_meta("glow_color", glow)
	if beacon:
		_beacon(root, Vector3(0, 0, 11.5 * FREIGHTER_SCALE), glow, 4.0 * FREIGHTER_SCALE)
	return root


## Small, quick ship. Cannot use the highway.
static func build(hull: Color, glow: Color, beacon := false) -> Node3D:
	var root := Node3D.new()
	var hull_mat := StandardMaterial3D.new()
	hull_mat.albedo_color = hull
	hull_mat.metallic = 0.6
	hull_mat.roughness = 0.35
	var dark := StandardMaterial3D.new()
	dark.albedo_color = hull.darkened(0.6)
	dark.metallic = 0.5
	dark.roughness = 0.5
	var glass := StandardMaterial3D.new()
	glass.albedo_color = Color(0.04, 0.08, 0.14)
	glass.metallic = 0.9
	glass.roughness = 0.1
	glass.emission_enabled = true
	glass.emission = Color(0.15, 0.4, 0.7)
	glass.emission_energy_multiplier = 0.5
	var glow_mat := _glow_mat(glow)

	MeshUtil.part(root, MeshUtil.box(1.8, 0.9, 6.0), Vector3(0, 0, 0.4), hull_mat)
	MeshUtil.part(root, MeshUtil.cylinder(0.05, 0.85, 3.2, 8), Vector3(0, 0, -4.0), hull_mat, Vector3(-90, 0, 0), Vector3(1, 1, 0.55))
	MeshUtil.part(root, MeshUtil.box(8.0, 0.2, 2.4), Vector3(0, -0.15, 1.4), hull_mat)
	MeshUtil.part(root, MeshUtil.box(3.0, 0.15, 1.2), Vector3(0, 0.1, -1.9), dark)
	for sx in [-1.0, 1.0]:
		MeshUtil.part(root, MeshUtil.box(0.25, 1.4, 1.8), Vector3(sx * 3.9, 0.55, 1.9), dark)
		MeshUtil.part(root, MeshUtil.cylinder(0.6, 0.6, 2.4), Vector3(sx * 1.45, 0.0, 2.7), dark, Vector3(90, 0, 0))
		MeshUtil.part(root, MeshUtil.cylinder(0.45, 0.45, 0.2), Vector3(sx * 1.45, 0.0, 3.95), glow_mat, Vector3(90, 0, 0))
	MeshUtil.part(root, MeshUtil.box(0.8, 0.4, 0.2), Vector3(0, 0, 3.45), glow_mat)
	var cockpit := SphereMesh.new()
	cockpit.radius = 0.6
	cockpit.height = 1.2
	MeshUtil.part(root, cockpit, Vector3(0, 0.5, -1.2), glass, Vector3.ZERO, Vector3(1.0, 0.7, 2.2))

	root.set_meta("glow_mats", [glow_mat])
	root.set_meta("glow_color", glow)

	if beacon:
		_beacon(root, Vector3(0, 0, 4.2), glow, 2.5)
	return root


## Engine brightness follows speed: dark when stopped, full at top speed.
static func set_engine_glow(ship: Node3D, speed_frac: float) -> void:
	var f := clampf(speed_frac, 0.0, 1.0)
	var glow: Color = ship.get_meta("glow_color", Color.WHITE)
	for m in ship.get_meta("glow_mats", []):
		var sm := m as StandardMaterial3D
		sm.emission_energy_multiplier = 7.0 * f
		sm.albedo_color = glow * (0.15 * f)
