extends Node3D
## Open-space scene. This node is offset by -center of the current sector (floating origin),
## so the player always flies near the origin. Children are placed in world coordinates.
## Rendering rules: the current sector renders normally; neighbouring sectors show only their
## planets and moons, pushed out by a log factor and hazed; sectors two or more away render
## nothing. Asteroid clusters also show when the player is inside or near one.

const STRETCH_K := 2.4
const STRETCH_LEN := 1500.0
const WANDERERS := 6
const NPC_DESPAWN := Combat.SENSOR_RANGE * 1.5  # traffic this far away is handed back (removed)
const LANE_MARK_SPACING := 700.0
const GATE_NEAR := 5000.0

const WALL_SHADER := preload("res://shaders/sector_wall.gdshader")
const SKY_SHADER := preload("res://shaders/space_sky.gdshader")
const PLANET_SHADER := preload("res://shaders/planet.gdshader")
const PLANET_RING_SHADER := preload("res://shaders/planet_ring.gdshader")
const BEACON_SHADER := preload("res://shaders/beacon.gdshader")
const ASTEROID_SHADER := preload("res://shaders/asteroid.gdshader")
const NPC_FREIGHTER_NAMES := ["Bulk hauler", "Container ship", "Ore freighter", "Tanker"]
const NPC_FIGHTER_NAMES := ["Courier", "Patrol fighter", "Private fighter", "Scout"]
const NPC_HULLS := [Color(0.7, 0.3, 0.25), Color(0.55, 0.6, 0.65), Color(0.25, 0.35, 0.6), Color(0.8, 0.7, 0.3), Color(0.35, 0.5, 0.4)]

var current := Vector2i(-99, -99)
var center := Vector3.ZERO
var sector_vis := {}      # Vector2i -> {walls, lines, items: Array[Node3D]}
var gate_nodes: Array = []  # {gate, node}: shown in the current sector or when close by
var body_vis: Array = []  # {data, node, mat, appear}
var rocks: Array = []     # {data, nodes, offsets, omegas, radii}
var wall_mat: ShaderMaterial
var flash_mat: ShaderMaterial
var rock_mat: ShaderMaterial
var ast_time := 0.0
var npcs: Array = []      # {id, name, fighter, node, vel, age, life, kind, ...}
var _next_npc_id := 1
var gate_timer := 2.0
var rng := RandomNumberGenerator.new()
var combat: Combat
var _player_world := Vector3.ZERO


func build() -> void:
	rng.seed = 99
	_build_env()
	wall_mat = ShaderMaterial.new()
	wall_mat.shader = WALL_SHADER
	flash_mat = ShaderMaterial.new()
	flash_mat.shader = BEACON_SHADER
	var line_mat := StandardMaterial3D.new()
	line_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	line_mat.albedo_color = Color(0.1, 0.2, 0.45)
	line_mat.blend_mode = BaseMaterial3D.BLEND_MODE_ADD
	var wall_mesh := _hex_prism_mesh()
	var line_mesh := _hex_lines_mesh()
	for c in Galaxy.sectors:
		var root := Node3D.new()
		root.name = "Sector_" + Galaxy.sector_name(c)
		root.position = Galaxy.sector_center(c)
		add_child(root)
		var walls := MeshInstance3D.new()
		walls.mesh = wall_mesh
		walls.material_override = wall_mat
		walls.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		root.add_child(walls)
		var lines := MeshInstance3D.new()
		lines.mesh = line_mesh
		lines.material_override = line_mat
		root.add_child(lines)
		sector_vis[c] = {"walls": walls, "lines": lines, "items": []}
	for g in Galaxy.gates:
		var node := GateBuilder.build(g, true)
		node.transform = g.world
		add_child(node)
		gate_nodes.append({"gate": g, "node": node})
	_build_lane_markers()
	for b in Galaxy.bodies:
		body_vis.append(_build_body(b))
	_build_asteroids()
	combat = Combat.new()
	add_child(combat)


func _build_env() -> void:
	var sky_mat := ShaderMaterial.new()
	sky_mat.shader = SKY_SHADER
	var sky := Sky.new()
	sky.sky_material = sky_mat
	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	env.sky = sky
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.2, 0.24, 0.35)
	env.ambient_light_energy = 0.6
	env.reflected_light_source = Environment.REFLECTION_SOURCE_DISABLED
	env.tonemap_mode = Environment.TONE_MAPPER_ACES
	env.glow_enabled = true
	env.glow_intensity = 0.8
	env.glow_bloom = 0.03
	env.glow_blend_mode = Environment.GLOW_BLEND_MODE_ADDITIVE
	env.glow_hdr_threshold = 0.9
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)
	var sun := DirectionalLight3D.new()
	sun.light_color = Color(1.0, 0.93, 0.82)
	sun.light_energy = 1.6
	sun.rotation_degrees = Vector3(-35, -40, 0)
	add_child(sun)


func _build_body(b: Dictionary) -> Dictionary:
	var node := Node3D.new()
	var sphere := SphereMesh.new()
	sphere.radius = 1.0
	sphere.height = 2.0
	sphere.radial_segments = 96 if b.kind == "planet" else 48
	sphere.rings = 48 if b.kind == "planet" else 24
	var pal: Array = b.palette
	var m := ShaderMaterial.new()
	m.shader = PLANET_SHADER
	m.set_shader_parameter("col_a", pal[0])
	m.set_shader_parameter("col_b", pal[1])
	m.set_shader_parameter("col_c", pal[2])
	m.set_shader_parameter("atmo", pal[3] if b.kind == "planet" else Color(0, 0, 0))
	m.set_shader_parameter("seed", b.seed)
	m.set_shader_parameter("banding", b.banding)
	MeshUtil.part(node, sphere, Vector3.ZERO, m)
	if b.ringed:
		var plane := PlaneMesh.new()
		plane.size = Vector2(4.6, 4.6)
		var rm := ShaderMaterial.new()
		rm.shader = PLANET_RING_SHADER
		rm.set_shader_parameter("tint", (pal[1] as Color).lerp(Color.WHITE, 0.3))
		var ring := MeshUtil.part(node, plane, Vector3.ZERO, rm)
		ring.rotation = b.ring_tilt
	node.position = b.world
	node.scale = Vector3.ONE * b.radius
	add_child(node)
	return {"data": b, "node": node, "mat": m, "appear": 1.0}


## Faint frames along each hop lane so the lanes can be followed by eye.
func _build_lane_markers() -> void:
	var m := StandardMaterial3D.new()
	m.albedo_color = Color.BLACK
	m.emission_enabled = true
	m.emission = GateBuilder.HOP_TINT
	m.emission_energy_multiplier = 0.7
	# Sized to the hop gates, which grow with Galaxy.ROAD_SCALE.
	var w := 40.0 * Galaxy.ROAD_SCALE
	var h := 26.0 * Galaxy.ROAD_SCALE
	var t := 0.6 * Galaxy.ROAD_SCALE
	var parts := [[MeshUtil.box(w, t, t), Vector3(0, h * 0.5, 0)], [MeshUtil.box(w, t, t), Vector3(0, -h * 0.5, 0)],
		[MeshUtil.box(t, h, t), Vector3(w * 0.5, 0, 0)], [MeshUtil.box(t, h, t), Vector3(-w * 0.5, 0, 0)]]
	for hop in Galaxy.hops:
		var by_sector := {}
		var basis := Basis.looking_at(hop.dir, Vector3.UP)
		var dist := LANE_MARK_SPACING
		while dist < hop.length - LANE_MARK_SPACING * 0.5:
			var p: Vector3 = hop.entry + hop.dir * dist
			var c := Galaxy.hex_of(p)
			if not by_sector.has(c):
				var st := SurfaceTool.new()
				st.begin(Mesh.PRIMITIVE_TRIANGLES)
				by_sector[c] = st
			for part in parts:
				(by_sector[c] as SurfaceTool).append_from(part[0], 0, Transform3D(basis, p + basis * (part[1] as Vector3)))
			dist += LANE_MARK_SPACING
		for c in by_sector:
			if not sector_vis.has(c):
				continue
			var mi := MeshInstance3D.new()
			mi.mesh = (by_sector[c] as SurfaceTool).commit()
			mi.material_override = m
			add_child(mi)
			sector_vis[c].items.append(mi)


func _hex_corners() -> Array:
	var pts: Array = []
	for k in 6:
		var a := deg_to_rad(30.0 + 60.0 * k)
		pts.append(Vector3(cos(a), 0.0, sin(a)) * Galaxy.HEX_R)
	return pts


func _hex_prism_mesh() -> ArrayMesh:
	var h := Galaxy.SECTOR_HALF_H
	var c := _hex_corners()
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	for k in 6:
		var a: Vector3 = c[k]
		var b: Vector3 = c[(k + 1) % 6]
		var w := a.distance_to(b)
		var quad := [[a + Vector3.DOWN * h, Vector2(0, -h)], [b + Vector3.DOWN * h, Vector2(w, -h)],
			[b + Vector3.UP * h, Vector2(w, h)], [a + Vector3.UP * h, Vector2(0, h)]]
		for i in [0, 1, 2, 0, 2, 3]:
			st.set_uv(quad[i][1])
			st.add_vertex(quad[i][0])
	for y in [-h, h]:
		for k in 6:
			var a: Vector3 = c[k]
			var b: Vector3 = c[(k + 1) % 6]
			for v in [Vector3.ZERO, a, b]:
				st.set_uv(Vector2(v.x, v.z))
				st.add_vertex(v + Vector3.UP * y)
	return st.commit()


func _hex_lines_mesh() -> ArrayMesh:
	var h := Galaxy.SECTOR_HALF_H
	var c := _hex_corners()
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_LINES)
	for k in 6:
		var a: Vector3 = c[k]
		var b: Vector3 = c[(k + 1) % 6]
		for y in [-h, h]:
			st.add_vertex(a + Vector3.UP * y)
			st.add_vertex(b + Vector3.UP * y)
		st.add_vertex(a + Vector3.DOWN * h)
		st.add_vertex(a + Vector3.UP * h)
	return st.commit()


# --- Asteroids ----------------------------------------------------------------------------

static func _rock_mesh(noise_seed: int) -> ArrayMesh:
	var sm := SphereMesh.new()
	sm.radius = 1.0
	sm.height = 2.0
	sm.radial_segments = 12
	sm.rings = 7
	var arrays := sm.get_mesh_arrays()
	var verts: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
	var noise := FastNoiseLite.new()
	noise.seed = noise_seed
	noise.frequency = 0.9
	for i in verts.size():
		var n := verts[i].normalized()
		verts[i] = n * (1.0 + 0.4 * noise.get_noise_3dv(n) + 0.12 * noise.get_noise_3dv(n * 3.3))
	arrays[Mesh.ARRAY_VERTEX] = verts
	arrays[Mesh.ARRAY_NORMAL] = null
	arrays[Mesh.ARRAY_TANGENT] = null
	var st := SurfaceTool.new()
	st.create_from_arrays(arrays)
	st.deindex()
	st.generate_normals()
	return st.commit()


func _build_asteroids() -> void:
	rock_mat = ShaderMaterial.new()
	rock_mat.shader = ASTEROID_SHADER
	var meshes := [_rock_mesh(1), _rock_mesh(2), _rock_mesh(3)]
	for cl in Galaxy.clusters:
		var r := RandomNumberGenerator.new()
		r.seed = cl.seed
		var n: int = cl.count
		var offsets := PackedVector3Array()
		var omegas := PackedFloat32Array()
		var radii := PackedFloat32Array()
		var xforms: Array = [[], [], []]
		var customs: Array = [[], [], []]
		var sigma: float = cl.sigma
		for i in n:
			# Mostly small rocks, a few large ones.
			var size := clampf(4.0 * pow(1.0 - r.randf() * 0.999, -0.55), 3.0, 90.0)
			var off: Vector3
			var omega: float
			if cl.kind == "belt":
				var ang := r.randf() * TAU
				var rad: float = cl.radius + r.randfn(0.0, sigma)
				off = Vector3(cos(ang) * rad, r.randfn(0.0, sigma * 0.25), sin(ang) * rad)
				omega = r.randf_range(8.0, 16.0) / maxf(rad, 50.0)
			else:
				off = Vector3(r.randfn(0.0, sigma), r.randfn(0.0, sigma * 0.35), r.randfn(0.0, sigma))
				omega = r.randf_range(1.0, 10.0) / maxf(Vector2(off.x, off.z).length(), 150.0)
			var basis := Basis.from_euler(Vector3(r.randf() * TAU, r.randf() * TAU, r.randf() * TAU))
			basis = basis.scaled(Vector3(size, size * r.randf_range(0.6, 1.0), size * r.randf_range(0.7, 1.2)))
			offsets.append(off)
			omegas.append(omega)
			radii.append(size * 0.85)
			xforms[i % 3].append(Transform3D(basis, off))
			customs[i % 3].append(Color(omega, r.randf_range(0.05, 0.5), r.randf(), 0.0))
		var nodes: Array = []
		var ext: float = cl.extent
		for v in 3:
			var mm := MultiMesh.new()
			mm.transform_format = MultiMesh.TRANSFORM_3D
			mm.use_custom_data = true
			mm.mesh = meshes[v]
			mm.instance_count = xforms[v].size()
			for i in xforms[v].size():
				mm.set_instance_transform(i, xforms[v][i])
				mm.set_instance_custom_data(i, customs[v][i])
			var mmi := MultiMeshInstance3D.new()
			mmi.multimesh = mm
			mmi.material_override = rock_mat
			mmi.custom_aabb = AABB(Vector3(-ext, -ext, -ext), Vector3(ext, ext, ext) * 2.0)
			mmi.position = cl.center
			add_child(mmi)
			nodes.append(mmi)
		rocks.append({"data": cl, "nodes": nodes, "offsets": offsets, "omegas": omegas, "radii": radii})


## Push-out needed to separate a sphere at p_local from any rock; ZERO if clear.
func collide_asteroids(p_local: Vector3, ship_r: float) -> Vector3:
	var p := p_local + center
	var push := Vector3.ZERO
	for rk in rocks:
		var c: Vector3 = rk.data.center
		if p.distance_to(c) > (rk.data.extent as float) + 100.0:
			continue
		var offsets: PackedVector3Array = rk.offsets
		var omegas: PackedFloat32Array = rk.omegas
		var radii: PackedFloat32Array = rk.radii
		var rel := p - c
		for i in offsets.size():
			var rp := offsets[i].rotated(Vector3.UP, omegas[i] * ast_time)
			var dv := rel - rp
			var min_d := radii[i] + ship_r
			var dd := dv.length_squared()
			if dd < min_d * min_d:
				var dist := sqrt(dd)
				push += (dv / maxf(dist, 0.001)) * (min_d - dist)
	return push


# --- Sector switching -----------------------------------------------------------------------

func set_current_sector(c: Vector2i) -> void:
	current = c
	center = Galaxy.sector_center(c)
	position = -center
	for k in sector_vis:
		var dist := Galaxy.hex_distance(k, c)
		var v: Dictionary = sector_vis[k]
		v.walls.visible = dist == 0
		v.lines.visible = dist <= 1
		for item in v.items:
			item.visible = dist == 0
	for b in body_vis:
		var node: Node3D = b.node
		var show := Galaxy.hex_distance(b.data.sector, c) <= 1
		if show and not node.visible:
			b.appear = 0.0
		node.visible = show


func gate_local(g: Dictionary) -> Transform3D:
	var t: Transform3D = g.world
	return Transform3D(t.basis, t.origin - center)


## Gates the player can interact with: those in the current sector plus any close by, so a
## gate near a sector border still works from the neighbouring sector.
func current_gates() -> Array:
	var out: Array = []
	for gn in gate_nodes:
		if gn.node.visible:
			out.append(gn.gate)
	return out


## Bodies of the current sector, at their true sector-local position.
func solid_bodies() -> Array:
	var out: Array = []
	for b in Galaxy.sectors[current].bodies:
		out.append({"pos": b.world - center, "radius": b.radius})
	return out


## Visible bodies with their displayed (possibly pushed-out) global positions.
func visible_bodies() -> Array:
	var out: Array = []
	for b in body_vis:
		if b.node.visible:
			out.append({"pos": b.node.global_position, "name": b.data.name, "kind": b.data.kind, "sector": b.data.sector})
	return out


# --- Per frame ----------------------------------------------------------------------------

func update(delta: float, player_local: Vector3, cam_local: Vector3) -> void:
	var player_world := player_local + center
	wall_mat.set_shader_parameter("player_pos", player_local)
	for gn in gate_nodes:
		var g: Dictionary = gn.gate
		gn.node.visible = g.sector == current or (g.world.origin as Vector3).distance_to(player_world) < GATE_NEAR
	ast_time += delta
	rock_mat.set_shader_parameter("sim_time", ast_time)
	for rk in rocks:
		var cl: Dictionary = rk.data
		var show: bool = cl.sector == current or player_world.distance_to(cl.center) < (cl.extent as float) * 1.5
		for n in rk.nodes:
			n.visible = show
	for b in body_vis:
		var node: Node3D = b.node
		if not node.visible:
			continue
		b.appear = minf(1.0, b.appear + delta * 0.7)
		var data: Dictionary = b.data
		var world: Vector3 = data.world
		var haze := 0.0
		if data.sector == current:
			node.position = world
		else:
			# Push neighbour bodies out along the view ray. The factor grows with the log of
			# how far the player is from that sector, and is 1 at its border.
			var excess := maxf(0.0, Galaxy.hex_sdf(player_world, data.sector))
			var factor := 1.0 + STRETCH_K * log(1.0 + excess / STRETCH_LEN)
			var rel := (world - center) - cam_local
			node.global_position = cam_local + rel * factor
			haze = clampf((factor - 1.0) / 4.0, 0.0, 0.85)
		(b.mat as ShaderMaterial).set_shader_parameter("haze", haze)
		node.scale = Vector3.ONE * data.radius * ease(b.appear, 0.5)
	_update_npcs(delta, player_world)


# --- NPC traffic --------------------------------------------------------------------------

func _update_npcs(delta: float, player_world: Vector3) -> void:
	_player_world = player_world
	var wanderers := 0
	for n in npcs.duplicate():
		var node: Node3D = n.node
		n.age += delta
		node.position += n.vel * delta
		node.scale = Vector3.ONE * clampf(n.age / 0.8, 0.01, 1.0)
		if n.kind == "into_gate":
			var gw: Transform3D = n.gate.world
			if (gw.affine_inverse() * node.position).z <= 0.0:
				flash(node.position, GateBuilder.tint_for(n.gate), 40.0)
				_remove_npc(n)
				continue
		elif n.age > n.life or node.position.distance_to(player_world) > NPC_DESPAWN:
			if node.visible:
				flash(node.position, Color(0.6, 0.8, 1.0), 30.0)
			_remove_npc(n)
			continue
		# Traffic outside sensor range exists but isn't rendered.
		node.visible = node.position.distance_to(player_world) <= Combat.SENSOR_RANGE
		if n.kind == "wander":
			wanderers += 1

	if wanderers < WANDERERS:
		var ang := rng.randf() * TAU
		var p := player_world + Vector3(cos(ang), 0.0, sin(ang)) * rng.randf_range(700.0, NPC_DESPAWN * 0.9) + Vector3.UP * rng.randf_range(-300.0, 300.0)
		var hd := rng.randf() * TAU
		var dir := Vector3(cos(hd), rng.randf_range(-0.15, 0.15), sin(hd)).normalized()
		var fighter := rng.randf() < 0.5
		_spawn_npc(p, dir * (rng.randf_range(60.0, 100.0) if fighter else rng.randf_range(25.0, 40.0)), "wander", {"fighter": fighter})

	gate_timer -= delta
	if gate_timer <= 0.0:
		gate_timer = rng.randf_range(4.0, 9.0)
		var near: Array = []
		for g in current_gates():
			if (g.world.origin as Vector3).distance_to(player_world) < 5000.0:
				near.append(g)
		if not near.is_empty():
			var g: Dictionary = near[rng.randi() % near.size()]
			var gw: Transform3D = g.world
			var fwd := -gw.basis.z
			var fighter: bool = g.kind.begins_with("hop") and rng.randf() < 0.5
			var spd := 80.0 if fighter else 36.0
			if GateBuilder.is_entry(g):
				_spawn_npc(gw.origin - fwd * rng.randf_range(400.0, 800.0), fwd * spd, "into_gate", {"gate": g, "fighter": fighter})
			else:
				_spawn_npc(gw.origin + fwd * 5.0, fwd * spd, "from_gate", {"fighter": fighter})


func _spawn_npc(p: Vector3, vel: Vector3, kind: String, extra: Dictionary) -> void:
	var hull: Color = NPC_HULLS[rng.randi() % NPC_HULLS.size()]
	var glow := Color(1.0, 0.55, 0.3) if rng.randf() < 0.5 else Color(0.5, 0.8, 1.0)
	var node := ShipMesh.build(hull, glow, true) if extra.get("fighter", false) else ShipMesh.build_freighter(hull, glow, true)
	ShipMesh.set_engine_glow(node, vel.length() / (SpaceFlight.FIGHTER.max_speed if extra.get("fighter", false) else SpaceFlight.FREIGHTER.max_speed))
	node.position = p
	node.basis = Basis.looking_at(vel.normalized(), Vector3.UP)
	node.scale = Vector3.ONE * 0.01
	add_child(node)
	var fighter: bool = extra.get("fighter", false)
	var names: Array = NPC_FIGHTER_NAMES if fighter else NPC_FREIGHTER_NAMES
	var reg := "%s%s-%03d" % [char(65 + rng.randi() % 26), char(65 + rng.randi() % 26), rng.randi() % 1000]
	var n := {"id": "t%d" % _next_npc_id, "name": "%s %s" % [names[rng.randi() % names.size()], reg], "node": node, "vel": vel,
		"age": 0.0, "life": rng.randf_range(40.0, 90.0), "kind": kind}
	_next_npc_id += 1
	n.merge(extra)
	npcs.append(n)
	if _player_world.distance_to(p) <= Combat.SENSOR_RANGE:
		flash(p, glow, 30.0)
	else:
		node.visible = false


func _remove_npc(n: Dictionary) -> void:
	n.node.queue_free()
	npcs.erase(n)


func flash(p: Vector3, tint: Color, size: float) -> void:
	var q := MeshInstance3D.new()
	q.mesh = QuadMesh.new()
	q.material_override = flash_mat
	q.extra_cull_margin = 500.0
	q.position = p
	add_child(q)
	q.set_instance_shader_parameter("tint", Vector3(tint.r, tint.g, tint.b))
	var tw := q.create_tween()
	tw.tween_method(func(v: float): q.set_instance_shader_parameter("intensity", v), 4.0, 0.0, 0.8)
	tw.parallel().tween_method(func(v: float): q.set_instance_shader_parameter("base_size", v), size * 0.2, size, 0.8)
	tw.tween_callback(q.queue_free)


# --- Collisions ---------------------------------------------------------------------------

## Keeps ships from passing through each other. Other ships that overlap are pushed apart;
## the player, a capsule in world coordinates, is pushed out by the returned amount (they
## take half when it's another ship's fault as much as theirs; main bounces them).
func separate_ships(player: Dictionary) -> Vector3:
	var bodies: Array = []
	for s in combat.ships:
		var cls: Dictionary = SpaceFlight.FIGHTER if s.kind == "fighter" else SpaceFlight.FREIGHTER
		bodies.append({"ship": s, "cap": SpaceFlight.capsule(cls, s.pos, s.dir)})
	for n in npcs:
		var cls: Dictionary = SpaceFlight.FIGHTER if n.get("fighter", false) else SpaceFlight.FREIGHTER
		var node: Node3D = n.node
		bodies.append({"npc": n, "cap": SpaceFlight.capsule(cls, node.position, -node.basis.z)})
	var player_push := Vector3.ZERO
	var me := player.duplicate()
	# A few passes, so a pile of ships sorts itself out rather than shuffling the overlap on.
	for pass_i in 4:
		for i in bodies.size():
			var a: Dictionary = bodies[i]
			var pp := SpaceFlight.capsule_push(me, a.cap)
			if pp != Vector3.ZERO:
				player_push += pp * 0.5
				me.pos = (me.pos as Vector3) + pp * 0.5
				_move_body(a, -pp * 0.5)
			for j in range(i + 1, bodies.size()):
				var b: Dictionary = bodies[j]
				var push := SpaceFlight.capsule_push(a.cap, b.cap)
				if push != Vector3.ZERO:
					_move_body(a, push * 0.5)
					_move_body(b, -push * 0.5)
	return player_push


func _move_body(b: Dictionary, by: Vector3) -> void:
	b.cap.pos = (b.cap.pos as Vector3) + by
	if b.has("ship"):
		b.ship.pos = (b.ship.pos as Vector3) + by
	else:
		(b.npc.node as Node3D).position += by


# --- Sensors ------------------------------------------------------------------------------

## Every ship within sensor range of p (world coordinates), nearest first:
## {id, name, cls, pos, vel, hp_frac, hostile, radius, dist}.
func contacts(p: Vector3) -> Array:
	var out: Array = []
	for s in combat.ships:
		out.append({"id": "c%d" % s.id, "name": s.name, "cls": "Fighter" if s.kind == "fighter" else "Freighter",
			"pos": s.pos, "vel": s.vel, "hp_frac": (s.hp as float) / (s.max_hp as float), "hostile": s.kind != "dummy",
			"radius": s.radius})
	for n in npcs:
		var fighter: bool = n.get("fighter", false)
		out.append({"id": n.id, "name": n.name, "cls": "Fighter" if fighter else "Freighter", "pos": n.node.position,
			"vel": n.vel, "hp_frac": 1.0, "hostile": false,
			"radius": SpaceFlight.FIGHTER.radius if fighter else SpaceFlight.FREIGHTER.radius})
	var near: Array = []
	for c in out:
		c.dist = p.distance_to(c.pos)
		if c.dist <= Combat.SENSOR_RANGE:
			near.append(c)
	near.sort_custom(func(a, b): return a.dist < b.dist)
	return near
