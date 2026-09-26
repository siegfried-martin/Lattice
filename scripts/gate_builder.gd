class_name GateBuilder
## Rectangular gates. A highway gate is the one element that exists in both scenes: it sits
## at gate.hw on the highway and at gate.world (the same spot, unscaled) in open space.
## In open space, entrances get a line of lead-up frames on the approach side and exits a
## few trailing frames, instead of a stretch of road.

const PORTAL_SHADER := preload("res://shaders/portal.gdshader")
const ON_TINT := Color(0.55, 0.9, 0.6)
const OFF_TINT := Color(1.0, 0.62, 0.28)
const HOP_TINT := Color(0.72, 0.55, 1.0)
const JCT_TINT := Color(0.45, 0.75, 1.0)


static func tint_for(g: Dictionary) -> Color:
	match g.kind:
		"on":
			return ON_TINT
		"off":
			return OFF_TINT
		"jct":
			return JCT_TINT
	return HOP_TINT


static func size_for(g: Dictionary) -> Vector2:
	if g.has("size"):
		return g.size
	if g.kind.begins_with("hop"):
		return Vector2(Galaxy.HOP_GATE_W, Galaxy.HOP_GATE_H)
	return Vector2(Galaxy.GATE_W, Galaxy.GATE_H)


static func is_entry(g: Dictionary) -> bool:
	return g.kind == "on" or g.kind == "hop_in"


static func build(g: Dictionary, space_side: bool) -> Node3D:
	var root := Node3D.new()
	var tint := tint_for(g)
	var sz := size_for(g)
	var steel := StandardMaterial3D.new()
	steel.albedo_color = Color(0.2, 0.21, 0.22)
	steel.metallic = 0.7
	steel.roughness = 0.5
	var k := Galaxy.ROAD_SCALE   # frames grow with the road and the freighter
	var t := 1.8 * k
	var depth := 3.0 * k
	for sy in [-1.0, 1.0]:
		MeshUtil.part(root, MeshUtil.box(sz.x + 2.0 * t, t, depth), Vector3(0, sy * (sz.y + t) * 0.5, 0), steel)
	for sx in [-1.0, 1.0]:
		MeshUtil.part(root, MeshUtil.box(t, sz.y, depth), Vector3(sx * (sz.x + t) * 0.5, 0, 0), steel)
	_outline(root, sz, 0.0, tint, 2.5, 0.35 * k)

	var q := QuadMesh.new()
	q.size = sz
	var pm := ShaderMaterial.new()
	pm.shader = PORTAL_SHADER
	pm.set_shader_parameter("tint", Vector3(tint.r, tint.g, tint.b))
	MeshUtil.part(root, q, Vector3.ZERO, pm)

	var label := Label3D.new()
	# Junction gates are signed on the HUD instead.
	label.text = g.get("label", "") if g.kind != "jct" else ""
	label.font_size = 96
	label.pixel_size = (0.05 if not g.kind.begins_with("hop") else 0.09) * k
	label.outline_size = 18
	label.outline_modulate = Color(0, 0, 0, 0.85)
	label.modulate = tint.lightened(0.3)
	label.position = Vector3(0, sz.y * 0.5 + t + 4.0 * k, 0)
	if space_side:
		label.billboard = BaseMaterial3D.BILLBOARD_FIXED_Y
	root.add_child(label)

	if space_side:
		if is_entry(g):
			var spacing := (160.0 if g.kind == "on" else 220.0) * k
			for i in range(1, 6):
				_outline(root, sz * (1.0 + 0.12 * i), spacing * i, tint, 1.4 - 0.2 * i, 0.8 * k)
		else:
			for i in range(1, 4):
				_outline(root, sz * (1.0 + 0.1 * i), -150.0 * k * i, tint, 1.0 - 0.25 * i, 0.8 * k)
	return root


## Glowing rectangular outline of inner size sz at local z.
static func _outline(parent: Node3D, sz: Vector2, z: float, tint: Color, energy: float, thick: float) -> void:
	var m := StandardMaterial3D.new()
	m.albedo_color = Color.BLACK
	m.emission_enabled = true
	m.emission = tint
	m.emission_energy_multiplier = maxf(energy, 0.2)
	for sy in [-1.0, 1.0]:
		MeshUtil.part(parent, MeshUtil.box(sz.x, thick, thick), Vector3(0, sy * sz.y * 0.5, z), m)
	for sx in [-1.0, 1.0]:
		MeshUtil.part(parent, MeshUtil.box(thick, sz.y, thick), Vector3(sx * sz.x * 0.5, 0, z), m)


## True if segment a -> b passes through the gate's opening from its approach side.
static func crossed(g: Dictionary, xf: Transform3D, a: Vector3, b: Vector3) -> bool:
	var inv := xf.affine_inverse()
	var la := inv * a
	var lb := inv * b
	var half := size_for(g) * 0.5 * 1.15
	return la.z > 0.0 and lb.z <= 0.0 and absf(lb.x) < half.x and absf(lb.y) < half.y
