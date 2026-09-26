class_name Combat
extends Node3D
## Combat in open space: ships to fight, auto-cannon rounds, blockers, missiles and the
## fighter's laser. Lives under SpaceWorld and works in world coordinates, so it follows the
## floating origin like everything else there.
##
## The player is not a node here. main.gd passes the player's state into update() and
## applies the events it returns (damage taken, messages, the player's missile ending).

## Current tuning. Everything here is expected to move.
const T := {
	"cannon_interval": 0.5,        # s between rounds (2 per second)
	"cannon_speed": 700.0,
	"cannon_damage": 10.0,
	"cannon_life": 2.5,
	"blocker_speed": 150.0,        # fast enough to meet a missile, slow enough to dodge
	"blocker_cooldown": 1.5,
	"blocker_life": 5.0,
	"blocker_pellets": 16,
	"blocker_spread_delay": 0.35,  # s before the cloud starts to open
	"blocker_spread_rate": 9.0,    # m/s the cloud grows
	"blocker_max_radius": 18.0,
	"pellet_radius": 4.0,
	"missile_speed": 200.0,
	"missile_boost_speed": 300.0,
	"missile_brake_speed": 110.0,
	"missile_accel": 250.0,
	"missile_turn": 80.0,          # deg/s
	"missile_turn_boost": 40.0,
	"missile_turn_brake": 170.0,
	"missile_boost_time": 2.0,     # s of boost per missile; it does not refill
	"missile_fuse": 10.0,
	"missile_damage": 150.0,
	"missile_radius": 3.0,
	"missile_cooldown": 5.0,
	"blast_radius": 45.0,          # detonating early (left click) damages ships this far from the blast
	"blast_damage": 55.0,          # at the hull; falls to blast_edge_frac of this at the edge
	"blast_edge_frac": 0.3,
	"dodge_distance": 22.0,
	"dodge_time": 0.25,
	"dodge_cooldown": 1.2,
	"laser_range": 400.0,
	"laser_dps": 60.0,
	"laser_heat_rate": 0.35,       # heat per second firing; 1.0 overheats
	"laser_cool_rate": 0.25,
	"laser_restart_heat": 0.3,     # after overheating, usable again below this
	"npc_missile_turn": 60.0,
	"npc_aim_error_deg": 2.5,      # NPC gunnery is OK, not great
	"npc_reaction": Vector2(0.4, 1.1),
	"hp_dummy": 100.0,
	"hp_freighter": 300.0,
	"hp_fighter": 150.0,
}

## Ships farther than this from the player aren't rendered or targetable. Combat ships stay
## in play beyond it (they're in a fight); traffic is handed back.
const SENSOR_RANGE := 4000.0

const ROUND_PLAYER := Color(1.0, 0.75, 0.35)
const ROUND_ENEMY := Color(1.0, 0.35, 0.3)
const BLOCKER_PLAYER := Color(0.6, 0.85, 1.0)
const BLOCKER_ENEMY := Color(1.0, 0.5, 0.35)

var ships: Array = []      # {id, kind, name, node, pos, vel, dir, speed, max_speed, hp, max_hp, radius, ai}
var rounds: Array = []     # {pos, vel, life, dmg, player, node}
var blockers: Array = []   # {center, vel, age, dirs, nodes, player}
var missiles: Array = []   # see _new_missile
var player_missile: Dictionary = {}
var laser_heat := 0.0
var laser_locked := false
var _beams := {}           # owner key -> MeshInstance3D
var _next_id := 1
var _events: Array = []
var _player: Dictionary = {}
var rng := RandomNumberGenerator.new()


func _ready() -> void:
	rng.randomize()


# --- Spawning (demo keys) -----------------------------------------------------------------

func spawn_dummy(p: Vector3, freighter_speed: float) -> void:
	var ang := rng.randf() * TAU
	var dir := Vector3(cos(ang), 0.0, sin(ang))
	var s := _add_ship("dummy", "Target drone", ShipMesh.build_freighter(Color(0.5, 0.5, 0.52), Color(1.0, 0.8, 0.4), true),
		p, dir, freighter_speed * 0.75, T.hp_dummy, SpaceFlight.FREIGHTER.radius)
	s.speed = s.max_speed


func spawn_freighter(p: Vector3, player_pos: Vector3, speed: float) -> void:
	var s := _add_ship("freighter", "Hostile freighter", ShipMesh.build_freighter(Color(0.45, 0.28, 0.25), Color(1.0, 0.4, 0.3), true),
		p, (player_pos - p).normalized(), speed, T.hp_freighter, SpaceFlight.FREIGHTER.radius)
	s.ai = {"missile_at": rng.randf_range(10.0, 20.0), "age": 0.0, "fired_missile": false, "blocked": false, "react": -1.0}


func spawn_fighter(p: Vector3, player_pos: Vector3, speed: float, turn_deg: float) -> void:
	var s := _add_ship("fighter", "Hostile fighter", ShipMesh.build(Color(0.5, 0.3, 0.28), Color(1.0, 0.4, 0.3), true),
		p, (player_pos - p).normalized(), speed, T.hp_fighter, 5.0)
	s.ai = {"turn": turn_deg, "cannon_t": 0.0, "err": Vector3.ZERO, "err_t": 0.0, "break_t": 0.0, "heat": 0.0, "locked": false}


func _add_ship(kind: String, name: String, node: Node3D, p: Vector3, dir: Vector3, max_speed: float, hp: float, radius: float) -> Dictionary:
	add_child(node)
	var s := {"id": _next_id, "kind": kind, "name": name, "node": node, "pos": p, "vel": Vector3.ZERO, "dir": dir,
		"speed": max_speed * 0.6, "max_speed": max_speed, "hp": hp, "max_hp": hp, "radius": radius, "ai": {}}
	_next_id += 1
	ships.append(s)
	_place_ship(s)
	return s


func _place_ship(s: Dictionary) -> void:
	var node: Node3D = s.node
	node.position = s.pos
	node.basis = Basis.looking_at(s.dir, Vector3.UP)
	ShipMesh.set_engine_glow(node, (s.speed as float) / (s.max_speed as float))
	if _player.has("pos"):
		node.visible = (s.pos as Vector3).distance_to(_player.pos) <= SENSOR_RANGE


# --- Player weapons -----------------------------------------------------------------------

func fire_cannon(origin: Vector3, dir: Vector3, base_vel: Vector3, player: bool) -> void:
	var col := ROUND_PLAYER if player else ROUND_ENEMY
	var m := StandardMaterial3D.new()
	m.albedo_color = Color.BLACK
	m.emission_enabled = true
	m.emission = col
	m.emission_energy_multiplier = 5.0
	var node := MeshInstance3D.new()
	node.mesh = MeshUtil.box(0.5, 0.5, 8.0)
	node.material_override = m
	add_child(node)
	var r := {"pos": origin, "vel": dir * T.cannon_speed + base_vel, "life": T.cannon_life, "dmg": T.cannon_damage, "player": player, "node": node}
	rounds.append(r)
	_place_round(r)


func fire_blocker(origin: Vector3, dir: Vector3, base_vel: Vector3, player: bool) -> void:
	var col := BLOCKER_PLAYER if player else BLOCKER_ENEMY
	var m := StandardMaterial3D.new()
	m.albedo_color = Color.BLACK
	m.emission_enabled = true
	m.emission = col
	m.emission_energy_multiplier = 4.0
	var sphere := SphereMesh.new()
	sphere.radius = 1.2
	sphere.height = 2.4
	sphere.radial_segments = 8
	sphere.rings = 4
	var dirs: Array = []
	var nodes: Array = []
	for i in T.blocker_pellets:
		# Spread mostly sideways, filling the cloud rather than making a shell.
		var v := Vector3(rng.randfn(), rng.randfn(), rng.randfn()).normalized()
		v = (v - dir * v.dot(dir) * 0.7).normalized() * rng.randf_range(0.2, 1.0)
		dirs.append(v)
		var node := MeshInstance3D.new()
		node.mesh = sphere
		node.material_override = m
		add_child(node)
		nodes.append(node)
	blockers.append({"center": origin, "vel": dir * T.blocker_speed + base_vel, "age": 0.0, "dirs": dirs, "nodes": nodes, "player": player})


func launch_missile(origin: Vector3, dir: Vector3, player: bool, target: Variant) -> Dictionary:
	var m := _new_missile(origin, dir, player, target)
	missiles.append(m)
	if player:
		player_missile = m
	return m


func _new_missile(origin: Vector3, dir: Vector3, player: bool, target: Variant) -> Dictionary:
	var node := Node3D.new()
	var body := StandardMaterial3D.new()
	body.albedo_color = Color(0.75, 0.75, 0.78) if player else Color(0.5, 0.3, 0.28)
	body.metallic = 0.6
	body.roughness = 0.4
	var glow := Color(0.6, 0.85, 1.0) if player else Color(1.0, 0.35, 0.25)
	var gm := StandardMaterial3D.new()
	gm.albedo_color = Color.BLACK
	gm.emission_enabled = true
	gm.emission = glow
	gm.emission_energy_multiplier = 2.5
	MeshUtil.part(node, MeshUtil.cylinder(0.45, 0.45, 4.5), Vector3.ZERO, body, Vector3(90, 0, 0))
	MeshUtil.part(node, MeshUtil.cylinder(0.02, 0.45, 1.2, 8), Vector3(0, 0, -2.85), body, Vector3(-90, 0, 0))
	for k in 4:
		var fin := MeshUtil.part(node, MeshUtil.box(0.08, 1.4, 1.0), Vector3(0, 0, 1.7), body)
		fin.rotation.z = k * PI * 0.5
		fin.position += fin.basis.y * 0.5
	MeshUtil.part(node, MeshUtil.cylinder(0.35, 0.35, 0.15), Vector3(0, 0, 2.3), gm, Vector3(90, 0, 0))
	var b := MeshInstance3D.new()
	b.mesh = QuadMesh.new()
	var bm := ShaderMaterial.new()
	bm.shader = ShipMesh.BEACON_SHADER
	b.material_override = bm
	b.position = Vector3(0, 0, 2.6)
	b.extra_cull_margin = 200.0
	b.set_instance_shader_parameter("tint", Vector3(glow.r, glow.g, glow.b))
	b.set_instance_shader_parameter("base_size", 1.2)
	b.set_instance_shader_parameter("intensity", 0.5)
	node.add_child(b)
	add_child(node)
	var yaw := atan2(-dir.x, -dir.z)
	var pitch := asin(clampf(dir.y, -1.0, 1.0))
	var m := {"pos": origin, "prev": origin, "yaw": yaw, "pitch": pitch, "aim_yaw": yaw, "aim_pitch": pitch,
		"speed": T.missile_speed, "fuse": T.missile_fuse, "boost_left": T.missile_boost_time, "player": player,
		"target": target, "node": node, "boost_in": false, "brake_in": false, "boosting": false,
		"dodge_t": 0.0, "dodge_dir": 0.0, "dodge_cd": 0.0, "cam_shift": Vector3.ZERO,
		"cam_yaw": yaw, "cam_pitch": pitch, "lead": rng.randf_range(0.6, 1.0), "dead": false}
	_place_missile(m)
	return m


static func missile_basis(m: Dictionary) -> Basis:
	return Basis.from_euler(Vector3(m.pitch, m.yaw, 0.0))


## Mouse steers the player's missile: it turns toward the aim point at its turn rate.
func steer_player_missile(rel: Vector2, boost: bool, brake: bool) -> void:
	if player_missile.is_empty():
		return
	var m := player_missile
	m.aim_yaw -= rel.x * SpaceFlight.MOUSE_SENS
	m.aim_pitch = clampf(m.aim_pitch - rel.y * SpaceFlight.MOUSE_SENS, deg_to_rad(-80.0), deg_to_rad(80.0))
	m.boost_in = boost
	m.brake_in = brake


func dodge_player_missile(dir: int) -> void:
	var m := player_missile
	if m.is_empty() or m.dodge_cd > 0.0:
		return
	m.dodge_t = T.dodge_time
	m.dodge_dir = float(dir)
	m.dodge_cd = T.dodge_cooldown


## Left click while flying: blow the missile up where it is. Ships whose hull is inside the
## blast radius take a share of blast_damage, less the further out they are.
func detonate_player_missile() -> void:
	var m := player_missile
	if m.is_empty():
		return
	var at: Vector3 = m.pos
	var hits: Array = []
	for s in ships.duplicate():
		var gap := maxf(0.0, at.distance_to(s.pos) - (s.radius as float))
		if gap > T.blast_radius:
			continue
		var dmg := T.blast_damage * lerpf(1.0, T.blast_edge_frac, gap / T.blast_radius)
		hits.append("%s  %d" % [s.name, int(dmg)])
		_damage_ship(s, dmg)
	_flash(at, Color(1.0, 0.75, 0.45), T.blast_radius * 2.2)
	_end_missile(m, "Detonated  -  " + (", ".join(hits) if not hits.is_empty() else "nothing in the blast"))


func player_missile_aim_dir() -> Vector3:
	if player_missile.is_empty():
		return Vector3.FORWARD
	return -Basis.from_euler(Vector3(player_missile.aim_pitch, player_missile.aim_yaw, 0.0)).z


## Chase camera behind the player's missile. After a dodge the camera lags the sideways
## jump, so the missile is seen to move off to the side and then drift back to centre.
func player_missile_camera() -> Transform3D:
	var m := player_missile
	var cb := Basis.from_euler(Vector3(m.cam_pitch, m.cam_yaw, 0.0))
	var anchor: Vector3 = (m.pos as Vector3) - (m.cam_shift as Vector3)
	return Transform3D(cb * Basis(Vector3.RIGHT, deg_to_rad(-5.0)), anchor + cb * Vector3(0.0, 3.2, 13.0))


## Fighter laser, held. Returns true while the beam is firing.
func player_laser(origin: Vector3, dir: Vector3, held: bool, delta: float) -> bool:
	var firing := held and not laser_locked
	if firing:
		laser_heat += T.laser_heat_rate * delta
		if laser_heat >= 1.0:
			laser_heat = 1.0
			laser_locked = true
			firing = false
			_event("message", "Laser overheated")
	if not firing:
		laser_heat = maxf(0.0, laser_heat - T.laser_cool_rate * delta)
		if laser_locked and laser_heat < T.laser_restart_heat:
			laser_locked = false
		_beam("player", Vector3.ZERO, Vector3.ZERO, false)
		return false
	var end := origin + dir * T.laser_range
	var hit := _ray_ships(origin, dir, T.laser_range)
	if not hit.is_empty():
		end = hit.point
		_damage_ship(hit.ship, T.laser_dps * delta)
	_beam("player", origin, end, true)
	return true


# --- Per frame ----------------------------------------------------------------------------

## player: {pos, vel, radius, alive}. Returns events:
## {type: "player_hit", dmg} / {type: "message", text} / {type: "missile_ended"}.
func update(delta: float, player: Dictionary) -> Array:
	_player = player
	for s in ships.duplicate():
		_update_ship(s, delta)
	for m in missiles.duplicate():
		_update_missile(m, delta)
	for r in rounds.duplicate():
		_update_round(r, delta)
	for b in blockers.duplicate():
		_update_blocker(b, delta)
	# Includes events raised between frames, e.g. by detonate_player_missile().
	var out := _events
	_events = []
	return out


func incoming_missiles() -> Array:
	var out: Array = []
	for m in missiles:
		if not m.player and not m.dead:
			out.append(m)
	return out


func _event(type: String, value: Variant = null) -> void:
	var e := {"type": type}
	if type == "message":
		e.text = value
	elif type == "player_hit":
		e.dmg = value
	_events.append(e)


# --- Ships --------------------------------------------------------------------------------

func _update_ship(s: Dictionary, delta: float) -> void:
	match s.kind:
		"freighter":
			_ai_freighter(s, delta)
		"fighter":
			_ai_fighter(s, delta)
	s.vel = (s.dir as Vector3) * (s.speed as float)
	s.pos = (s.pos as Vector3) + (s.vel as Vector3) * delta
	_place_ship(s)


## Turn dir toward want by at most turn_deg * delta, keeping a level horizon.
static func _turn_toward(dir: Vector3, want: Vector3, turn_deg: float, delta: float) -> Vector3:
	var ang := dir.angle_to(want)
	if ang < 0.0001:
		return want
	var step := minf(1.0, deg_to_rad(turn_deg) * delta / ang)
	var out := dir.slerp(want, step).normalized()
	out.y = clampf(out.y, -0.85, 0.85)
	return out.normalized()


func _aim_error(dist: float) -> Vector3:
	var e := deg_to_rad(T.npc_aim_error_deg) * dist
	return Vector3(rng.randfn(), rng.randfn() * 0.5, rng.randfn()) * e


func _ai_freighter(s: Dictionary, delta: float) -> void:
	var ai: Dictionary = s.ai
	ai.age += delta
	var to_p: Vector3 = (_player.pos as Vector3) - (s.pos as Vector3)
	var d := to_p.length()
	# Hold a standoff: close in when far, open up when near, circle in between.
	var want := to_p.normalized()
	if d < 500.0:
		want = -want
	elif d < 900.0:
		want = want.cross(Vector3.UP).normalized()
	s.dir = _turn_toward(s.dir, want, 22.0, delta)
	s.speed = s.max_speed
	if not ai.fired_missile and ai.age >= ai.missile_at and _player.alive:
		ai.fired_missile = true
		launch_missile((s.pos as Vector3) + (s.dir as Vector3) * 12.0, to_p.normalized(), false, null)
		_event("message", "Missile launch detected")
	# One blocker, at the first missile that comes for it.
	if not ai.blocked and not player_missile.is_empty():
		var m := player_missile
		var mp: Vector3 = m.pos
		if mp.distance_to(s.pos) < 1100.0:
			if ai.react < 0.0:
				ai.react = rng.randf_range(T.npc_reaction.x, T.npc_reaction.y)
			ai.react -= delta
			if ai.react <= 0.0:
				ai.blocked = true
				var mv := -missile_basis(m).z * (m.speed as float)
				var t := mp.distance_to(s.pos) / (T.blocker_speed + (m.speed as float))
				var aim := (mp + mv * t + _aim_error(mp.distance_to(s.pos)) - (s.pos as Vector3)).normalized()
				fire_blocker(s.pos, aim, s.vel, false)


func _ai_fighter(s: Dictionary, delta: float) -> void:
	var ai: Dictionary = s.ai
	var pp: Vector3 = _player.pos
	var sp: Vector3 = s.pos
	var d := sp.distance_to(pp)
	# Aim point: lead the player, with an error that wanders.
	ai.err_t -= delta
	if ai.err_t <= 0.0:
		ai.err_t = 0.7
		ai.err = _aim_error(d)
	var lead: Vector3 = pp + (_player.vel as Vector3) * (d / T.cannon_speed) + (ai.err as Vector3)
	var want := (lead - sp).normalized()
	ai.break_t -= delta
	if d < 220.0 and ai.break_t <= -2.0:
		ai.break_t = 2.5
	if ai.break_t > 0.0:
		want = (want.cross(Vector3.UP).normalized() + Vector3.UP * 0.2 - want * 0.5).normalized()
	s.dir = _turn_toward(s.dir, want, ai.turn, delta)
	s.speed = s.max_speed
	if not _player.alive:
		_beam(str(s.id), Vector3.ZERO, Vector3.ZERO, false)
		return
	var off := rad_to_deg((s.dir as Vector3).angle_to((lead - sp).normalized()))
	var nose: Vector3 = sp + (s.dir as Vector3) * 6.0
	ai.cannon_t -= delta
	if ai.cannon_t <= 0.0 and off < 5.0 and d < 900.0:
		ai.cannon_t = T.cannon_interval
		fire_cannon(nose, s.dir, s.vel, false)
	# Laser when close and lined up; it overheats like the player's.
	var lasing: bool = d < T.laser_range * 0.9 and off < 3.5 and not ai.locked
	if lasing:
		ai.heat += T.laser_heat_rate * delta
		if ai.heat >= 1.0:
			ai.locked = true
			lasing = false
	if not lasing:
		ai.heat = maxf(0.0, ai.heat - T.laser_cool_rate * delta)
		if ai.locked and ai.heat < T.laser_restart_heat:
			ai.locked = false
		_beam(str(s.id), Vector3.ZERO, Vector3.ZERO, false)
		return
	var end := nose + (s.dir as Vector3) * T.laser_range
	var t := _ray_sphere(nose, s.dir, pp, _player.radius)
	if t >= 0.0 and t <= T.laser_range:
		end = nose + (s.dir as Vector3) * t
		_event("player_hit", T.laser_dps * delta)
	_beam(str(s.id), nose, end, true)


func _damage_ship(s: Dictionary, dmg: float) -> void:
	s.hp = (s.hp as float) - dmg
	if s.hp <= 0.0 and ships.has(s):
		_flash(s.pos, Color(1.0, 0.7, 0.4), 120.0)
		_event("message", "%s destroyed" % s.name)
		_beam(str(s.id), Vector3.ZERO, Vector3.ZERO, false)
		(s.node as Node3D).queue_free()
		ships.erase(s)


# --- Missiles -----------------------------------------------------------------------------

func _update_missile(m: Dictionary, delta: float) -> void:
	var turn: float = T.missile_turn
	var target_speed: float = T.missile_speed
	m.boosting = false
	if m.player:
		if m.boost_in and m.boost_left > 0.0:
			target_speed = T.missile_boost_speed
			turn = T.missile_turn_boost
			m.boost_left = maxf(0.0, m.boost_left - delta)
			m.boosting = true
		elif m.brake_in:
			target_speed = T.missile_brake_speed
			turn = T.missile_turn_brake
	else:
		# Home on the player, leading a little less than it should.
		turn = T.npc_missile_turn
		var pp: Vector3 = _player.pos
		var tt := (m.pos as Vector3).distance_to(pp) / (m.speed as float)
		var want: Vector3 = (pp + (_player.vel as Vector3) * tt * (m.lead as float) - (m.pos as Vector3)).normalized()
		m.aim_yaw = atan2(-want.x, -want.z)
		m.aim_pitch = asin(clampf(want.y, -1.0, 1.0))
	m.speed = move_toward(m.speed, target_speed, T.missile_accel * delta)
	var tr := deg_to_rad(turn)
	m.yaw += clampf(wrapf(m.aim_yaw - m.yaw, -PI, PI) * 4.0, -tr, tr) * delta
	m.pitch = clampf(m.pitch + clampf((m.aim_pitch - m.pitch) * 4.0, -tr, tr) * delta, deg_to_rad(-80.0), deg_to_rad(80.0))
	var b := missile_basis(m)
	m.prev = m.pos
	m.pos = (m.pos as Vector3) - b.z * (m.speed as float) * delta
	m.dodge_cd = maxf(0.0, m.dodge_cd - delta)
	if m.dodge_t > 0.0:
		var step := b.x * (m.dodge_dir as float) * (T.dodge_distance / T.dodge_time) * minf(delta, m.dodge_t)
		m.pos = (m.pos as Vector3) + step
		# The camera takes most of the jump at once and lags the rest, so the missile is
		# seen to slide off to the side and then drift back to centre.
		m.cam_shift = (m.cam_shift as Vector3) + step * 0.3
		m.dodge_t -= delta
	m.cam_shift = (m.cam_shift as Vector3) * exp(-2.2 * delta)
	m.cam_yaw = lerp_angle(m.cam_yaw, m.yaw, 1.0 - exp(-8.0 * delta))
	m.cam_pitch = lerpf(m.cam_pitch, m.pitch, 1.0 - exp(-8.0 * delta))
	m.fuse -= delta
	_place_missile(m)

	var a: Vector3 = m.prev
	var z: Vector3 = m.pos
	if m.player:
		for s in ships:
			if _seg_hits(a, z, s.pos, (s.radius as float) + T.missile_radius):
				_damage_ship(s, T.missile_damage)
				_end_missile(m, "Direct hit")
				return
	elif _player.alive and _seg_hits(a, z, _player.pos, (_player.radius as float) + T.missile_radius):
		_event("player_hit", T.missile_damage)
		_end_missile(m, "")
		return
	if m.fuse <= 0.0:
		_end_missile(m, "Fuse expired" if m.player else "")


func _place_missile(m: Dictionary) -> void:
	var node: Node3D = m.node
	node.position = m.pos
	node.basis = missile_basis(m)


func _end_missile(m: Dictionary, why: String) -> void:
	if m.dead:
		return
	m.dead = true
	_flash(m.pos, Color(1.0, 0.8, 0.5) if m.player else Color(1.0, 0.45, 0.3), 45.0)
	(m.node as Node3D).queue_free()
	missiles.erase(m)
	if m.player:
		player_missile = {}
		_event("missile_ended")
		if why != "":
			_event("message", why)


func cancel_player_missile() -> void:
	if not player_missile.is_empty():
		_end_missile(player_missile, "")


# --- Rounds and blockers ------------------------------------------------------------------

func _update_round(r: Dictionary, delta: float) -> void:
	var a: Vector3 = r.pos
	var z: Vector3 = a + (r.vel as Vector3) * delta
	r.pos = z
	r.life -= delta
	_place_round(r)
	if r.player:
		for s in ships:
			if _seg_hits(a, z, s.pos, s.radius):
				_damage_ship(s, r.dmg)
				_remove_round(r)
				return
	elif _player.alive and _seg_hits(a, z, _player.pos, _player.radius):
		_event("player_hit", r.dmg)
		_remove_round(r)
		return
	# Rounds knock down the other side's missiles.
	for m in missiles:
		if m.player != r.player and _seg_hits(a, z, m.pos, T.missile_radius + 1.5):
			_end_missile(m, "Missile shot down")
			_remove_round(r)
			return
	if r.life <= 0.0:
		_remove_round(r)


func _place_round(r: Dictionary) -> void:
	var node: Node3D = r.node
	node.position = r.pos
	node.basis = Basis.looking_at((r.vel as Vector3).normalized(), Vector3.UP if absf((r.vel as Vector3).normalized().y) < 0.99 else Vector3.RIGHT)


func _remove_round(r: Dictionary) -> void:
	(r.node as Node3D).queue_free()
	rounds.erase(r)


func _update_blocker(b: Dictionary, delta: float) -> void:
	b.age += delta
	b.center = (b.center as Vector3) + (b.vel as Vector3) * delta
	var rad := clampf((b.age - T.blocker_spread_delay) * T.blocker_spread_rate, 0.0, T.blocker_max_radius)
	var dirs: Array = b.dirs
	var nodes: Array = b.nodes
	for i in dirs.size():
		(nodes[i] as Node3D).position = (b.center as Vector3) + (dirs[i] as Vector3) * rad
	# A missile that touches a pellet is caught.
	for m in missiles.duplicate():
		if m.player == b.player:
			continue
		for i in dirs.size():
			if _seg_hits(m.prev, m.pos, (nodes[i] as Node3D).position, T.pellet_radius):
				_end_missile(m, "Caught by a blocker" if m.player else "")
				if b.player:
					_event("message", "Blocker caught the missile")
				break
	if b.age >= T.blocker_life:
		for n in nodes:
			(n as Node3D).queue_free()
		blockers.erase(b)


# --- Helpers ------------------------------------------------------------------------------

static func _seg_hits(a: Vector3, b: Vector3, c: Vector3, r: float) -> bool:
	var ab := b - a
	var t := clampf((c - a).dot(ab) / maxf(ab.length_squared(), 1e-6), 0.0, 1.0)
	return (a + ab * t).distance_squared_to(c) < r * r


## Distance along a unit ray to a sphere, or -1.
static func _ray_sphere(o: Vector3, d: Vector3, c: Vector3, r: float) -> float:
	var t := (c - o).dot(d)
	if t < 0.0:
		return -1.0
	var q := (o + d * t).distance_squared_to(c)
	if q > r * r:
		return -1.0
	return t - sqrt(r * r - q)


func _ray_ships(o: Vector3, d: Vector3, max_t: float) -> Dictionary:
	var best := {}
	for s in ships:
		var t := _ray_sphere(o, d, s.pos, s.radius)
		if t >= 0.0 and t <= max_t and (best.is_empty() or t < best.t):
			best = {"ship": s, "t": t, "point": o + d * t}
	return best


func _beam(key: String, a: Vector3, b: Vector3, on: bool) -> void:
	var node: MeshInstance3D = _beams.get(key)
	if node == null:
		if not on:
			return
		node = MeshInstance3D.new()
		var cyl := MeshUtil.cylinder(0.15, 0.15, 1.0, 6)
		node.mesh = cyl
		var m := StandardMaterial3D.new()
		m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		m.albedo_color = Color(0.6, 0.9, 1.0) if key == "player" else Color(1.0, 0.4, 0.35)
		m.albedo_color *= 1.6
		node.material_override = m
		add_child(node)
		_beams[key] = node
	node.visible = on
	if not on:
		return
	var len := a.distance_to(b)
	var dir := (b - a) / maxf(len, 0.001)
	var up := Vector3.UP if absf(dir.y) < 0.99 else Vector3.RIGHT
	# Cylinder axis is Y: point it along the beam.
	var basis := Basis.looking_at(dir, up) * Basis(Vector3.RIGHT, -PI * 0.5)
	node.transform = Transform3D(basis * Basis.from_scale(Vector3(1.0, len, 1.0)), (a + b) * 0.5)


func _flash(p: Vector3, tint: Color, size: float) -> void:
	var sw := get_parent()
	if sw != null and sw.has_method("flash"):
		sw.flash(p, tint, size)
