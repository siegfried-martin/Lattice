extends Node3D
## Wormhole highway scene: the road network from Galaxy at HW_SCALE of the open-space map,
## so roads point the same way as the sectors they connect.
##
## A closed box tunnel is rebuilt around the ship every frame along its path: it opens up
## ahead into a throat and closes behind, hiding everything outside it. Lighting is a pool
## of lamps along the median that follows the ship, plus a faint glow from the wall seams.

const TUN_BACK := -130.0
const TUN_FRONT := 270.0
const TUN_RINGS := 56
const TUN_HALF_W := 72.0
const TUN_H := 32.0
const TUN_FLOOR := -0.3
const THROAT_Y := 4.0
const FACE_SEGS := [10, 4, 10, 4]   # floor, right wall, ceiling, left wall

# Open flying limits, metres from the median on the player's side / above the road.
const FLY_MIN_X := 5.5
const FLY_MAX_X := TUN_HALF_W - 5.0
const FLY_CEILING := 24.0
const CAM_CEILING := TUN_H - 3.0

const LAMP_SPACING := 36.0
const LAMP_COUNT := 12
const NPC_OPPOSING := 6
const NPC_SAME := 4
const NPC_COLORS := [Color(0.45, 0.3, 0.25), Color(0.45, 0.47, 0.5), Color(0.3, 0.33, 0.4), Color(0.5, 0.45, 0.3), Color(0.33, 0.38, 0.33)]

const ROAD_SHADER := preload("res://shaders/road.gdshader")
const BARRIER_SHADER := preload("res://shaders/barrier.gdshader")
const TUNNEL_SHADER := preload("res://shaders/tunnel.gdshader")
const WALL_SHADER := preload("res://shaders/sector_wall.gdshader")

var tube_mesh: ArrayMesh
var tube_idx := PackedInt32Array()
var tube_open := 1.0
var curtain_mat: ShaderMaterial
var lamps: Array = []
var npcs: Array = []
var npc_road := -1
var opp_timer := 0.0
var same_timer := 0.0
var rng := RandomNumberGenerator.new()


func build() -> void:
	rng.seed = 7
	_build_env()
	_build_roads()
	_build_links()
	_build_signs()
	_build_tube()
	for i in LAMP_COUNT:
		var l := OmniLight3D.new()
		l.light_color = Color(1.0, 0.82, 0.58)
		l.light_energy = 7.0
		l.omni_range = 75.0
		l.omni_attenuation = 1.2
		l.shadow_enabled = false
		add_child(l)
		lamps.append(l)


func _build_env() -> void:
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color.BLACK
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.35, 0.4, 0.5)
	env.ambient_light_energy = 0.22
	env.tonemap_mode = Environment.TONE_MAPPER_ACES
	env.glow_enabled = true
	env.glow_intensity = 0.5
	env.glow_bloom = 0.0
	env.glow_blend_mode = Environment.GLOW_BLEND_MODE_ADDITIVE
	env.glow_hdr_threshold = 1.0
	env.fog_enabled = true
	env.fog_light_color = Color(0.04, 0.045, 0.055)
	env.fog_density = 0.0025
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)


func _build_roads() -> void:
	var road_mat := ShaderMaterial.new()
	road_mat.shader = ROAD_SHADER
	var bar_mat := ShaderMaterial.new()
	bar_mat.shader = BARRIER_SHADER
	curtain_mat = ShaderMaterial.new()
	curtain_mat.shader = WALL_SHADER
	curtain_mat.set_shader_parameter("grid", 8.0)
	curtain_mat.set_shader_parameter("reach", 50.0)
	for road in Galaxy.roads:
		var t: Track = road.track
		var mi := MeshInstance3D.new()
		mi.mesh = MeshUtil.track_ribbon(t, 0.0, t.length, 4.0, -Galaxy.ROAD_HALF_W, Galaxy.ROAD_HALF_W, 0.0)
		mi.material_override = road_mat
		add_child(mi)
		var bar := MeshInstance3D.new()
		bar.mesh = MeshUtil.track_wall(t, 0.0, t.length, 4.0, 0.0, 0.0, 1.6)
		bar.material_override = bar_mat
		add_child(bar)
		# Shows the median limit when flying close to it.
		var curtain := MeshInstance3D.new()
		curtain.mesh = MeshUtil.track_wall(t, 0.0, t.length, 8.0, 0.0, 1.6, TUN_H)
		curtain.material_override = curtain_mat
		curtain.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		add_child(curtain)


func _build_links() -> void:
	for l in Galaxy.links:
		var tint := Color(0.8, 0.8, 0.75)
		if l.kind == "on":
			tint = GateBuilder.ON_TINT
		elif l.kind == "off":
			tint = GateBuilder.OFF_TINT
		var m := ShaderMaterial.new()
		m.shader = ROAD_SHADER
		m.set_shader_parameter("is_ramp", true)
		m.set_shader_parameter("tint", Vector3(tint.r, tint.g, tint.b))
		var tr: Track = l.track
		var mi := MeshInstance3D.new()
		mi.mesh = MeshUtil.track_ribbon(tr, 0.0, tr.length, 4.0, -Galaxy.RAMP_HALF_W, Galaxy.RAMP_HALF_W, 0.06)
		mi.material_override = m
		add_child(mi)
	for g in Galaxy.gates:
		if g.kind == "on" or g.kind == "off":
			var gate := GateBuilder.build(g, false)
			gate.transform = g.hw
			add_child(gate)


func _build_signs() -> void:
	for key in Galaxy.events:
		var road: int = key.x
		var d: int = key.y
		for e in Galaxy.events[key]:
			var text: String = e.text
			var side: float = e.side
			if side > 0.0:
				text += "  >"
			elif side < 0.0:
				text = "<  " + text
			var tint := GateBuilder.OFF_TINT.lerp(Color.WHITE, 0.3) if e.kind == "off" else Color(0.85, 0.83, 0.75)
			_sign(road, d, e.u - d * 300.0, text + "\n300 m", tint, side * 5.0)
			_sign(road, d, e.u - d * 40.0, text, tint, side * 10.0)
	for b in Galaxy.boundaries:
		var tint := Color(0.65, 0.75, 0.85)
		_sign(b.road, 1, b.u, "SECTOR %s" % Galaxy.sector_name(b.b), tint, -5.0)
		_sign(b.road, -1, b.u, "SECTOR %s" % Galaxy.sector_name(b.a), tint, -5.0)


func _sign(road: int, d: int, u: float, text: String, tint: Color, lat := 0.0) -> void:
	var t := Galaxy.road_track(road)
	if u < 0.0 or u > t.length:
		return
	var root := Node3D.new()
	root.transform = Transform3D(Basis.looking_at(Galaxy.carr_fwd(road, d, u), Vector3.UP), Galaxy.carr_point(road, d, u, lat, 15.0))
	var pm := StandardMaterial3D.new()
	pm.albedo_color = Color(0.1, 0.105, 0.11)
	pm.metallic = 0.6
	pm.roughness = 0.6
	MeshUtil.part(root, MeshUtil.box(22.0, 7.5, 0.4), Vector3.ZERO, pm)
	var label := Label3D.new()
	label.text = text
	label.font_size = 72
	label.pixel_size = 0.04
	label.modulate = tint
	label.outline_size = 0
	label.position = Vector3(0, 0, 0.25)
	root.add_child(label)
	add_child(root)


# --- Tunnel -------------------------------------------------------------------------------

func _build_tube() -> void:
	tube_mesh = ArrayMesh.new()
	tube_mesh.custom_aabb = AABB(Vector3(-1e5, -1e5, -1e5), Vector3(2e5, 2e5, 2e5))
	var mat := ShaderMaterial.new()
	mat.shader = TUNNEL_SHADER
	var mi := MeshInstance3D.new()
	mi.mesh = tube_mesh
	mi.material_override = mat
	mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(mi)
	var row := _ring_size()
	var base := 0
	for segs in FACE_SEGS:
		for k in TUN_RINGS - 1:
			for j in segs:
				var a: int = k * row + base + j
				var c: int = a + row
				tube_idx.append_array([a, c, a + 1, a + 1, c, c + 1])
		base += segs + 1


func _ring_size() -> int:
	var n := 0
	for segs in FACE_SEGS:
		n += segs + 1
	return n


## Called when the player enters: the tunnel opens up from a short throat.
func reset_on_enter() -> void:
	tube_open = 0.0
	for n in npcs:
		n.node.queue_free()
	npcs.clear()


## view: {frame: Callable(t) -> {center, carr, fwd, right}, phase: path distance at the ship,
##        road, d (side / carriageway), u, dv (direction the tunnel opens toward), speed,
##        ship: position, show_limits}
func update(delta: float, view: Dictionary) -> void:
	tube_open = minf(1.0, tube_open + delta * 1.0)
	curtain_mat.set_shader_parameter("player_pos", view.ship if view.show_limits else Vector3(0, 1e6, 0))
	var front := lerpf(70.0, TUN_FRONT, ease(tube_open, 0.4))
	_update_tube(view, front)
	_update_lamps(view, front)
	_update_npcs(delta, view)


func _update_tube(view: Dictionary, front: float) -> void:
	var frame: Callable = view.frame
	var phase: float = view.phase
	var row := _ring_size()
	var count := TUN_RINGS * row
	var verts := PackedVector3Array()
	var norms := PackedVector3Array()
	var uvs := PackedVector2Array()
	var uv2s := PackedVector2Array()
	var cols := PackedColorArray()
	verts.resize(count)
	norms.resize(count)
	uvs.resize(count)
	uv2s.resize(count)
	cols.resize(count)
	for k in TUN_RINGS:
		var t := lerpf(TUN_BACK, front, float(k) / (TUN_RINGS - 1))
		var f: Dictionary = frame.call(t)
		var fwd: Vector3 = f.fwd
		var right: Vector3 = fwd.cross(Vector3.UP).normalized()
		var back_k := clampf((t - TUN_BACK) / (-50.0 - TUN_BACK), 0.0, 1.0)
		var front_k := clampf((front - t) / (front - 40.0), 0.0, 1.0)
		var sc := sqrt(minf(back_k, front_k))
		# The throat converges onto the ship's carriageway at road level.
		var c: Vector3 = (f.carr as Vector3).lerp(f.center, sc)
		var hw := TUN_HALF_W * sc
		var y0 := lerpf(THROAT_Y, TUN_FLOOR, sc)
		var y1 := lerpf(THROAT_Y, TUN_H, sc)
		var frac := (t - TUN_BACK) / (front - TUN_BACK)
		var i := k * row
		var faces := [
			[Vector2(-1, 0), Vector2(1, 0), Vector3.UP],     # floor
			[Vector2(1, 0), Vector2(1, 1), -right],          # right wall
			[Vector2(1, 1), Vector2(-1, 1), Vector3.DOWN],   # ceiling
			[Vector2(-1, 1), Vector2(-1, 0), right],         # left wall
		]
		for fi in 4:
			var segs: int = FACE_SEGS[fi]
			var a: Vector2 = faces[fi][0]
			var b: Vector2 = faces[fi][1]
			for j in segs + 1:
				var q := a.lerp(b, float(j) / segs)
				verts[i] = c + right * (q.x * hw) + Vector3.UP * lerpf(y0, y1, q.y)
				norms[i] = faces[fi][2]
				# Panel coordinates in metres of the full-size tunnel, so seams stay put.
				var across := q.x * TUN_HALF_W if fi % 2 == 0 else lerpf(TUN_FLOOR, TUN_H, q.y)
				uvs[i] = Vector2(across, phase + t)
				uv2s[i] = Vector2(t, frac)
				cols[i] = Color(fi / 3.0, 0, 0)
				i += 1
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = verts
	arrays[Mesh.ARRAY_NORMAL] = norms
	arrays[Mesh.ARRAY_TEX_UV] = uvs
	arrays[Mesh.ARRAY_TEX_UV2] = uv2s
	arrays[Mesh.ARRAY_COLOR] = cols
	arrays[Mesh.ARRAY_INDEX] = tube_idx
	tube_mesh.clear_surfaces()
	tube_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)


## Median lamps sit every LAMP_SPACING metres of path, matching the lamp housings on the
## barrier, and the pool is re-placed around the ship each frame.
func _update_lamps(view: Dictionary, front: float) -> void:
	var frame: Callable = view.frame
	var offset := fposmod(view.phase as float, LAMP_SPACING)
	for k in LAMP_COUNT:
		var t := LAMP_SPACING * (k - 3) - offset
		var l: OmniLight3D = lamps[k]
		l.visible = t > TUN_BACK + 30.0 and t < front - 30.0
		if l.visible:
			var f: Dictionary = frame.call(t)
			l.position = (f.center as Vector3) + Vector3.UP * 1.4


# --- Traffic ------------------------------------------------------------------------------

func _update_npcs(delta: float, view: Dictionary) -> void:
	var road: int = view.road
	var up: float = view.u
	var d: int = view.d
	var dv: int = view.dv
	var t := Galaxy.road_track(road)
	if road != npc_road:
		for n in npcs:
			n.node.queue_free()
		npcs.clear()
		npc_road = road
	var opp := 0
	var same := 0
	for n in npcs:
		if n.dir == d:
			same += 1
		else:
			opp += 1
	opp_timer -= delta
	same_timer -= delta
	if opp < NPC_OPPOSING and opp_timer <= 0.0:
		_spawn(road, -d, up + dv * (TUN_FRONT + rng.randf_range(10.0, 30.0)))
		opp_timer = rng.randf_range(0.8, 2.5)
	if same < NPC_SAME and same_timer <= 0.0:
		var ahead := rng.randf() < 0.5
		_spawn(road, d, up + dv * (TUN_FRONT + 10.0 if ahead else TUN_BACK - 10.0))
		same_timer = rng.randf_range(2.0, 5.0)

	for n in npcs.duplicate():
		n.u += n.dir * n.speed * delta
		n.bob += delta
		var rel: float = (n.u - up) * dv
		if rel < TUN_BACK - 40.0 or rel > TUN_FRONT + 60.0 or n.u < 0.0 or n.u > t.length:
			n.node.queue_free()
			npcs.erase(n)
			continue
		var lat: float = -Galaxy.RIGHT_LANE_LAT + Galaxy.LANE_W * n.lane
		var node: Node3D = n.node
		node.position = Galaxy.carr_point(road, n.dir, n.u, lat, HighwayDrive.HOVER + sin(n.bob * 1.7) * 0.2)
		node.basis = Basis.looking_at(Galaxy.carr_fwd(road, n.dir, n.u), Vector3.UP)


## Docked freighters run at 80% of top speed; loads vary a little.
func _docked_speed() -> float:
	return SpaceFlight.FREIGHTER.max_speed * HighwayDrive.CRUISE_FRACTION * rng.randf_range(0.85, 1.1)


func _spawn(road: int, dir: int, u: float) -> void:
	if u < 0.0 or u > Galaxy.road_track(road).length:
		return
	var col: Color = NPC_COLORS[rng.randi() % NPC_COLORS.size()]
	# Only freighters can use the highway.
	var node := ShipMesh.build_freighter(col, Color(1.0, 0.6, 0.35) if rng.randf() < 0.5 else Color(0.6, 0.8, 1.0))
	ShipMesh.set_engine_glow(node, 0.5)
	add_child(node)
	npcs.append({"node": node, "dir": dir, "u": u, "speed": _docked_speed(), "lane": rng.randi_range(0, 1) if rng.randf() < 0.8 else 2, "bob": rng.randf() * 10.0})
