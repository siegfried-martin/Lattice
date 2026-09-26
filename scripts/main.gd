extends Node3D
## Owns the player ship, camera and HUD, and swaps between the open-space scene and the
## highway scene. Both scenes are built once at startup and kept in memory; only the active
## one is in the tree. Highway gates are where the two meet: the player's transform relative
## to a gate is carried across the swap.
##
## On the highway the ship either flies freely (same flight model as open space, bounded by
## the tunnel) or is docked to the road (C toggles). Hop lanes are ridden in open space.

const SpaceWorldScene := preload("res://scenes/space_world.tscn")
const HighwayScene := preload("res://scenes/highway.tscn")
const HudScript := preload("res://scripts/hud.gd")
const HighwayWorld := preload("res://scripts/highway_world.gd")
const SLEEVE_SHADER := preload("res://shaders/hop_sleeve.gdshader")

enum Mode { SPACE, HIGHWAY, HOP }
## Which job the player holds in open space. The ship holds its heading and speed while the
## player is at the turret or flying a missile.
enum Station { PILOT, TURRET, MISSILE }

const DOCK_TIME := 1.5
const DOCK_MAX_ALT := 40.0 * Galaxy.ROAD_SCALE
const HOP_SPEED := 600.0
const HOP_ACCEL := 80.0
const TURRET_MOUNT := Vector3(0.0, 4.1, 2.5) * ShipMesh.FREIGHTER_SCALE      # freighter turret, first-person eye point
const FREIGHTER_LAUNCHER := Vector3(0.0, 2.2, -9.5) * ShipMesh.FREIGHTER_SCALE
const FIGHTER_NOSE := Vector3(0.0, 0.0, -6.0)
const MISSILE_RETURN_DELAY := 0.9                  # s to watch the missile end before returning

var mode: int = Mode.SPACE
var space_world: Node3D
var highway: Node3D
var ship: Node3D
var camera: Camera3D
var hud: Control
var flash_rect: ColorRect
var sleeve: MeshInstance3D
var sleeve_mat: ShaderMaterial
var flight := SpaceFlight.new()
var drive := HighwayDrive.new()
var mouse_rel := Vector2.ZERO
var elapsed := 0.0
var msg := ""
var msg_t := 0.0
var edge_warn := 0.0
var _last_bounce := -10.0

# Highway state
var docked := false
var hw_road := 0     # road nearest the ship while flying freely
var hw_u := 0.0      # nearest position along that road
var hw_d := 1        # side of the road (carriageway direction) the ship is on
var hw_dv := 1       # direction the tunnel opens toward
var _other_u := 0.0
var _other_timer := 0.0
var blend_t := 1.0   # 0..1 ease from blend_ship / blend_cam to the live transforms
var blend_ship := Transform3D()
var blend_cam := Transform3D()

# Combat state
var station: int = Station.PILOT
var station_before_missile: int = Station.PILOT
var turret_yaw := 0.0
var turret_pitch := 0.0
var hull := 0.0
var cannon_t := 0.0
var blocker_cd := 0.0
var missile_cd := 0.0
var missile_end_t := -1.0
var _last_missile_cam := Transform3D()
var _laser_on := false

# Sensors and targeting (open space)
var contacts: Array = []   # space_world.contacts(), refreshed each frame
var target_id := ""

# Hop state
var hop: Dictionary = {}
var hop_s := 0.0
var hop_speed := 0.0

## Debug hook: when set, used instead of keyboard/mouse (see debug_tour.gd).
var input_override: Dictionary = {}


func _ready() -> void:
	space_world = SpaceWorldScene.instantiate()
	highway = HighwayScene.instantiate()
	space_world.build()
	highway.build()
	add_child(space_world)

	camera = Camera3D.new()
	camera.near = 0.3
	camera.far = 300000.0
	camera.fov = 70.0
	add_child(camera)
	camera.make_current()
	_set_ship_class(SpaceFlight.FREIGHTER)

	var cyl := CylinderMesh.new()
	cyl.top_radius = 45.0 * Galaxy.ROAD_SCALE
	cyl.bottom_radius = 45.0 * Galaxy.ROAD_SCALE
	cyl.height = 600.0 * Galaxy.ROAD_SCALE
	cyl.cap_top = false
	cyl.cap_bottom = false
	cyl.radial_segments = 32
	sleeve_mat = ShaderMaterial.new()
	sleeve_mat.shader = SLEEVE_SHADER
	sleeve = MeshInstance3D.new()
	sleeve.mesh = cyl
	sleeve.material_override = sleeve_mat
	sleeve.visible = false
	add_child(sleeve)

	var layer := CanvasLayer.new()
	add_child(layer)
	hud = HudScript.new()
	hud.set_anchors_preset(Control.PRESET_FULL_RECT)
	hud.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(hud)
	flash_rect = ColorRect.new()
	flash_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	flash_rect.color = Color(0.7, 0.9, 1.0, 0.0)
	flash_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(flash_rect)

	var start_pos: Vector3 = Galaxy.start.pos
	var gate_xf: Transform3D = Galaxy.start.gate.world
	space_world.set_current_sector(Galaxy.hex_of(start_pos))
	flight.place(start_pos - space_world.center, -gate_xf.basis.z, 25.0)

	if "--tour" in OS.get_cmdline_user_args():
		var tour: Node = load("res://scripts/debug_tour.gd").new()
		tour.main = self
		add_child(tour)
	else:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func _exit_tree() -> void:
	# The inactive scene is out of the tree, so it isn't freed with us.
	for w in [space_world, highway]:
		if is_instance_valid(w) and not w.is_inside_tree():
			w.free()


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		mouse_rel += event.relative
	elif event is InputEventKey and event.pressed and not event.echo:
		match event.physical_keycode:
			KEY_ESCAPE:
				Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
			KEY_C:
				toggle_dock()
			KEY_TAB:
				cycle_target()
			KEY_R:
				target_nearest()
			KEY_KP_1:
				switch_ship(SpaceFlight.FREIGHTER)
			KEY_KP_2:
				switch_ship(SpaceFlight.FIGHTER)
			KEY_A:
				if docked:
					drive.change_lane(-1)
				elif station == Station.MISSILE:
					space_world.combat.dodge_player_missile(-1)
			KEY_D:
				if docked:
					drive.change_lane(1)
				elif station == Station.MISSILE:
					space_world.combat.dodge_player_missile(1)
			KEY_G:
				to_turret()
			KEY_T:
				to_pilot()
			KEY_X:
				fire_missile()
			KEY_P, KEY_O, KEY_I:
				spawn_test_ship(event.physical_keycode)
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT \
			and station == Station.MISSILE and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		space_world.combat.detonate_player_missile()
	elif event is InputEventMouseButton and event.pressed and not docked and input_override.is_empty() \
			and Input.mouse_mode != Input.MOUSE_MODE_CAPTURED:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func _process(delta: float) -> void:
	elapsed += delta
	var rel := mouse_rel
	mouse_rel = Vector2.ZERO
	var thrust := float(Input.is_physical_key_pressed(KEY_W)) - float(Input.is_physical_key_pressed(KEY_S))
	if not input_override.is_empty():
		thrust = input_override.get("thrust", thrust)
		rel = input_override.get("mouse", Vector2.ZERO)
	msg_t = maxf(0.0, msg_t - delta)
	edge_warn = maxf(0.0, edge_warn - delta)
	match mode:
		Mode.SPACE:
			_process_space(delta, rel, thrust)
		Mode.HOP:
			_process_hop(delta)
		_:
			if docked:
				_process_docked(delta)
			else:
				_process_hw_free(delta, rel, thrust)
	flash_rect.color.a = move_toward(flash_rect.color.a, 0.0, delta * 2.0)


func _say(text: String, t := 3.0) -> void:
	msg = text
	msg_t = t


func _set_ship_class(cls: Dictionary) -> void:
	flight.cls = cls
	if is_instance_valid(ship):
		ship.queue_free()
	if cls == SpaceFlight.FREIGHTER:
		ship = ShipMesh.build_freighter(Color(0.6, 0.62, 0.66), Color(0.4, 0.8, 1.0))
	else:
		ship = ShipMesh.build(Color(0.62, 0.66, 0.72), Color(0.4, 0.8, 1.0))
	add_child(ship)
	hull = cls.hull
	station = Station.PILOT


## POC shortcut for trying both classes (numpad 1 / 2): switch ships in open space.
func switch_ship(cls: Dictionary) -> void:
	if mode != Mode.SPACE or flight.cls == cls:
		return
	_leave_combat()
	var speed := flight.forward_speed()
	_set_ship_class(cls)
	flight.velocity = flight.forward() * minf(speed, flight.cls.max_speed)
	flash_rect.color.a = 0.5
	_say("Now flying the %s" % flight.cls.name.to_lower(), 2.0)


## Hitting a limit: bounce back a little and lose speed. Thrust is untouched, so the ship
## accelerates back up afterwards. normal points back into the allowed space.
func _bounce(normal: Vector3) -> void:
	var vn := flight.velocity.dot(normal)
	if vn >= 0.0:
		return
	if elapsed - _last_bounce > 0.6:
		flight.velocity -= normal * vn * 1.3
		flight.velocity *= 0.6
	else:
		# Still pressed against the limit: just stop moving into it.
		flight.velocity -= normal * vn
	_last_bounce = elapsed


## Docked: the mouse is a free cursor. Flying: it steers.
func _set_mouse_for_mode() -> void:
	if input_override.is_empty():
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if docked else Input.MOUSE_MODE_CAPTURED


func _start_blend() -> void:
	blend_ship = ship.transform
	blend_cam = camera.transform
	blend_t = 0.0


func _apply_blend(delta: float, ship_xf: Transform3D, cam_xf: Transform3D) -> void:
	blend_t = minf(1.0, blend_t + delta / DOCK_TIME)
	var e := smoothstep(0.0, 1.0, blend_t)
	ship.transform = blend_ship.interpolate_with(ship_xf, e)
	camera.transform = blend_cam.interpolate_with(cam_xf, e)


# --- Open space ---------------------------------------------------------------------------

func _process_space(delta: float, rel: Vector2, thrust: float) -> void:
	# Only the pilot flies the ship; at the turret or on a missile it holds heading and speed.
	var piloting := station == Station.PILOT
	flight.aim(rel if piloting else Vector2.ZERO)
	var prev := flight.pos
	flight.update(delta, thrust if piloting else 0.0)
	_space_limits(prev)
	prev = _rebase(prev)
	if station == Station.TURRET:
		turret_yaw -= rel.x * SpaceFlight.MOUSE_SENS
		turret_pitch = clampf(turret_pitch - rel.y * SpaceFlight.MOUSE_SENS, deg_to_rad(-60.0), deg_to_rad(85.0))
	elif station == Station.MISSILE:
		space_world.combat.steer_player_missile(rel, _held(KEY_W, "boost"), _held(KEY_S, "brake"))

	for g in space_world.current_gates():
		if not GateBuilder.is_entry(g) or not GateBuilder.crossed(g, space_world.gate_local(g), prev, flight.pos):
			continue
		if g.kind == "hop_in":
			_enter_hop(g)
			return
		if flight.cls.highway:
			_enter_highway(g)
			return
		_say("No threader fitted  -  fighters can't enter the Lattice  (Numpad 1 for the freighter)")

	ship.transform = Transform3D(flight.ship_basis(), flight.pos)
	ShipMesh.set_engine_glow(ship, _speed_frac())
	if ship.has_meta("turret_head"):
		var head: Node3D = ship.get_meta("turret_head")
		var aim_basis := Basis.from_euler(Vector3(turret_pitch, turret_yaw, 0.0)) if station == Station.TURRET else flight.ship_basis()
		head.basis = flight.ship_basis().inverse() * aim_basis
	_weapons(delta)
	_update_combat(delta)
	camera.transform = _space_camera(delta)
	space_world.update(delta, flight.pos, camera.global_position)
	contacts = space_world.contacts(_to_world(flight.pos))
	if target_id != "" and target().is_empty():
		target_id = ""
	_hud_space()


## Vertical sector limits, planets and moons, asteroids.
func _space_limits(prev: Vector3) -> void:
	var lim := Galaxy.SECTOR_HALF_H - 40.0
	if absf(flight.pos.y) > lim:
		_bounce(Vector3.DOWN * signf(flight.pos.y))
		flight.pos.y = clampf(flight.pos.y, -lim, lim)
		edge_warn = 1.5
	for b in space_world.solid_bodies():
		var to_ship: Vector3 = flight.pos - b.pos
		var min_d: float = b.radius * 1.02 + 30.0
		if to_ship.length() < min_d:
			var n := to_ship.normalized()
			flight.pos = b.pos + n * min_d
			_bounce(n)
	var push: Vector3 = space_world.collide_asteroids(flight.pos, flight.cls.radius)
	if push != Vector3.ZERO:
		flight.pos += push
		_bounce(push.normalized())


## Move the floating origin when the ship crosses into another sector; returns prev shifted
## to match. Leaving the map bounces the ship back.
func _rebase(prev: Vector3) -> Vector3:
	var world: Vector3 = flight.pos + space_world.center
	var hex := Galaxy.hex_of(world)
	if hex == space_world.current:
		return prev
	if Galaxy.is_valid(hex):
		var old_center: Vector3 = space_world.center
		space_world.set_current_sector(hex)
		var shift: Vector3 = old_center - space_world.center
		flight.pos += shift
		return prev + shift
	var back: Vector3 = (Galaxy.sector_center(space_world.current) - world) * Vector3(1, 0, 1)
	flight.pos = prev
	_bounce(back.normalized())
	edge_warn = 2.5
	return prev


func _enter_highway(g: Dictionary) -> void:
	_leave_combat()
	var local: Transform3D = space_world.gate_local(g).affine_inverse() * ship.transform
	var speed := flight.forward_speed()
	remove_child(space_world)
	add_child(highway)
	move_child(highway, 0)
	highway.reset_on_enter()
	var xf: Transform3D = (g.hw as Transform3D) * local
	flight.place(xf.origin, -xf.basis.z, speed)
	var entry: Dictionary = g.terminal if g.has("terminal") else g.link.to
	hw_road = entry.road
	hw_d = entry.d
	hw_dv = hw_d
	hw_u = Galaxy.road_track(hw_road).project_global(xf.origin)
	_other_timer = 0.0
	docked = false
	blend_t = 1.0
	mode = Mode.HIGHWAY
	flash_rect.color.a = 0.9
	_say("Entered the Lattice  -  press C near the road to dock", 4.0)
	_process_hw_free(0.0, Vector2.ZERO, 0.0)


func _exit_to_space(g: Dictionary) -> void:
	var local: Transform3D = (g.hw as Transform3D).affine_inverse() * ship.transform
	var speed := drive.speed if docked else flight.forward_speed()
	remove_child(highway)
	add_child(space_world)
	move_child(space_world, 0)
	space_world.set_current_sector(g.sector)
	var xf: Transform3D = space_world.gate_local(g) * local
	var fwd := -xf.basis.z
	fwd.y = 0.0
	flight.place(xf.origin, fwd.normalized(), minf(speed, flight.cls.max_speed))
	docked = false
	_set_mouse_for_mode()
	blend_t = 1.0
	mode = Mode.SPACE
	flash_rect.color.a = 0.9
	_process_space(0.0, Vector2.ZERO, 0.0)


# --- Combat -------------------------------------------------------------------------------

func _held(key: Key, override_name: String) -> bool:
	if not input_override.is_empty():
		return input_override.get(override_name, false)
	return Input.is_physical_key_pressed(key)


func _mouse_held(button: MouseButton, override_name: String) -> bool:
	if not input_override.is_empty():
		return input_override.get(override_name, false)
	return Input.mouse_mode == Input.MOUSE_MODE_CAPTURED and Input.is_mouse_button_pressed(button)


func _speed_frac() -> float:
	return flight.velocity.length() / (flight.cls.max_speed as float)


## Sector-local position to the world coordinates combat works in.
func _to_world(p: Vector3) -> Vector3:
	return p + space_world.center


func _turret_dir() -> Vector3:
	return -Basis.from_euler(Vector3(turret_pitch, turret_yaw, 0.0)).z


func to_turret() -> void:
	if mode != Mode.SPACE or station == Station.MISSILE:
		return
	if not flight.cls.turret:
		_say("No turret on the fighter  -  its guns fire forward from the pilot's seat", 2.5)
		return
	if station != Station.TURRET:
		turret_yaw = flight.yaw
		turret_pitch = flight.pitch
	station = Station.TURRET


func to_pilot() -> void:
	if mode == Mode.SPACE and station == Station.TURRET:
		station = Station.PILOT


## X from any seat: the view goes with the missile until it ends.
func fire_missile() -> void:
	if mode != Mode.SPACE or station == Station.MISSILE:
		return
	if missile_cd > 0.0:
		_say("Launcher reloading  -  %.1f s" % missile_cd, 1.2)
		return
	var offset := FREIGHTER_LAUNCHER if flight.cls.turret else FIGHTER_NOSE
	var origin := _to_world(ship.transform * offset)
	space_world.combat.launch_missile(origin, flight.forward(), true, null)
	missile_cd = Combat.T.missile_cooldown
	station_before_missile = station
	station = Station.MISSILE
	missile_end_t = -1.0


func spawn_test_ship(key: Key) -> void:
	if mode != Mode.SPACE:
		return
	var here := _to_world(flight.pos)
	var fwd := flight.forward()
	var combat: Combat = space_world.combat
	var freighter_speed: float = SpaceFlight.FREIGHTER.max_speed
	match key:
		KEY_P:
			combat.spawn_dummy(here + fwd * 300.0, freighter_speed)
			_say("Target drone released", 2.0)
		KEY_O:
			combat.spawn_freighter(here + fwd * 900.0 + fwd.cross(Vector3.UP) * 200.0, here, freighter_speed)
			_say("Hostile freighter inbound  -  it carries one missile and one blocker", 3.0)
		KEY_I:
			combat.spawn_fighter(here + fwd * 1200.0, here, SpaceFlight.FIGHTER.max_speed, SpaceFlight.FIGHTER.turn_yaw)
			_say("Hostile fighter inbound", 2.0)


## Turret (freighter): left = auto-cannon, right = blocker. Pilot seat (fighter): left =
## auto-cannon straight ahead, right = laser (held, with heat).
func _weapons(delta: float) -> void:
	var combat: Combat = space_world.combat
	cannon_t -= delta
	blocker_cd -= delta
	missile_cd = maxf(0.0, missile_cd - delta)
	var lmb := _mouse_held(MOUSE_BUTTON_LEFT, "fire")
	var rmb := _mouse_held(MOUSE_BUTTON_RIGHT, "alt")
	var vel := flight.velocity
	var nose := _to_world(ship.transform * FIGHTER_NOSE)
	_laser_on = false
	if station == Station.TURRET:
		var aim := _turret_dir()
		var origin := _to_world(ship.transform * TURRET_MOUNT) + aim * 4.0
		if lmb and cannon_t <= 0.0:
			cannon_t = Combat.T.cannon_interval
			combat.fire_cannon(origin, aim, vel, true)
		if rmb and blocker_cd <= 0.0:
			blocker_cd = Combat.T.blocker_cooldown
			combat.fire_blocker(origin, aim, vel, true)
	elif station == Station.PILOT and not flight.cls.turret:
		if lmb and cannon_t <= 0.0:
			cannon_t = Combat.T.cannon_interval
			combat.fire_cannon(nose, flight.forward(), vel, true)
		_laser_on = rmb
	# Always called, so the laser cools when it isn't firing.
	combat.player_laser(nose, flight.forward(), _laser_on, delta)


func _update_combat(delta: float) -> void:
	var combat: Combat = space_world.combat
	var events: Array = combat.update(delta, {"pos": _to_world(flight.pos), "vel": flight.velocity,
		"radius": flight.cls.radius, "alive": true})
	for e in events:
		match e.type:
			"player_hit":
				hull -= e.dmg
				flash_rect.color = Color(1.0, 0.3, 0.2, maxf(flash_rect.color.a, minf(0.5, e.dmg / 60.0)))
				if hull <= 0.0:
					hull = flight.cls.hull
					space_world.flash(_to_world(flight.pos), Color(1.0, 0.6, 0.3), 80.0)
					_say("Hull destroyed  -  restored for testing", 3.0)
			"message":
				_say(e.text, 2.5)
			"missile_ended":
				missile_end_t = MISSILE_RETURN_DELAY
	if station == Station.MISSILE and missile_end_t >= 0.0:
		missile_end_t -= delta
		if missile_end_t < 0.0:
			station = station_before_missile


func _space_camera(delta: float) -> Transform3D:
	match station:
		Station.TURRET:
			return Transform3D(Basis.from_euler(Vector3(turret_pitch, turret_yaw, 0.0)), ship.transform * TURRET_MOUNT)
		Station.MISSILE:
			var combat: Combat = space_world.combat
			if not combat.player_missile.is_empty():
				var c := combat.player_missile_camera()
				_last_missile_cam = Transform3D(c.basis, c.origin - space_world.center)
			return _last_missile_cam
	return flight.camera_transform()


func _leave_combat() -> void:
	space_world.combat.cancel_player_missile()
	space_world.combat.player_laser(Vector3.ZERO, Vector3.FORWARD, false, 0.0)
	station = Station.PILOT
	target_id = ""
	contacts = []


# --- Targeting ----------------------------------------------------------------------------

func target() -> Dictionary:
	for c in contacts:
		if c.id == target_id:
			return c
	return {}


## Combat means a hostile ship on sensors or an enemy missile in flight.
func in_combat() -> bool:
	for c in contacts:
		if c.hostile:
			return true
	return not space_world.combat.incoming_missiles().is_empty()


## Tab: next ship in sensor range, nearest first, wrapping round.
func cycle_target() -> void:
	if mode != Mode.SPACE:
		return
	if contacts.is_empty():
		_say("No ships in sensor range", 1.5)
		return
	var i := -1
	for k in contacts.size():
		if contacts[k].id == target_id:
			i = k
	target_id = contacts[(i + 1) % contacts.size()].id


## R: nearest enemy in combat, otherwise nearest ship. Pressing it again takes the second
## nearest, so repeated presses flip between the two closest.
func target_nearest() -> void:
	if mode != Mode.SPACE:
		return
	var hostile_only := in_combat()
	var pool: Array = contacts.filter(func(c): return c.hostile or not hostile_only)
	if pool.is_empty():
		_say("No ships in sensor range", 1.5)
		return
	target_id = pool[1].id if pool[0].id == target_id and pool.size() > 1 else pool[0].id


# --- Hop lanes ----------------------------------------------------------------------------

func _enter_hop(g: Dictionary) -> void:
	_leave_combat()
	hop = g.hop
	hop_s = 0.0
	hop_speed = maxf(flight.forward_speed(), 20.0)
	mode = Mode.HOP
	sleeve.visible = true
	flash_rect.color = Color(0.8, 0.7, 1.0, 0.7)
	_say("Hop lane  -  %s" % hop.to.name, 3.0)


func _process_hop(delta: float) -> void:
	var length: float = hop.length
	var exit_speed := minf(flight.cls.max_speed, 30.0)
	var brake_v := sqrt(2.0 * HOP_ACCEL * maxf(length - hop_s, 0.0) + exit_speed * exit_speed)
	hop_speed = move_toward(hop_speed, minf(HOP_SPEED, brake_v), HOP_ACCEL * delta)
	hop_s += hop_speed * delta
	var dir: Vector3 = hop.dir
	var world: Vector3 = hop.entry + dir * minf(hop_s, length)
	var hex := Galaxy.hex_of(world)
	if hex != space_world.current and Galaxy.is_valid(hex):
		space_world.set_current_sector(hex)
	flight.pos = world - space_world.center
	flight.yaw = atan2(-dir.x, -dir.z)
	flight.pitch = asin(clampf(dir.y, -1.0, 1.0))
	flight.aim_yaw = flight.yaw
	flight.aim_pitch = flight.pitch
	flight.velocity = dir * hop_speed
	flight.cam_yaw = lerp_angle(flight.cam_yaw, flight.yaw, 1.0 - exp(-4.0 * delta))
	flight.cam_pitch = lerpf(flight.cam_pitch, flight.pitch * 0.6, 1.0 - exp(-4.0 * delta))
	if hop_s >= length:
		flight.place(flight.pos, dir, exit_speed)
		mode = Mode.SPACE
		sleeve.visible = false
		flash_rect.color = Color(0.8, 0.7, 1.0, 0.6)
		_say("Arrived  -  %s" % hop.to.name, 3.0)
		hop = {}
		return
	ship.transform = Transform3D(flight.ship_basis(), flight.pos)
	ShipMesh.set_engine_glow(ship, 1.0)
	camera.transform = flight.camera_transform()
	sleeve.transform = Transform3D(Basis.looking_at(dir, Vector3.UP) * Basis(Vector3.RIGHT, PI * 0.5), flight.pos + dir * 150.0 * Galaxy.ROAD_SCALE)
	sleeve_mat.set_shader_parameter("intensity", clampf(hop_speed / HOP_SPEED, 0.0, 1.0))
	space_world.update(delta, flight.pos, camera.global_position)
	_hud_hop(length)


# --- Highway: free flight -----------------------------------------------------------------

func _road_dist(road: int, u: float, p: Vector3) -> float:
	var q := Galaxy.road_track(road).pos(u)
	return Vector2(p.x - q.x, p.z - q.z).length()


func _process_hw_free(delta: float, rel: Vector2, thrust: float) -> void:
	flight.aim(rel)
	var prev := flight.pos
	flight.update(delta, thrust)

	# Follow whichever road the ship is over; switch when the other one is clearly nearer.
	var tr := Galaxy.road_track(hw_road)
	hw_u = tr.project(flight.pos, hw_u)
	var other := 1 - hw_road
	_other_timer -= delta
	if _other_timer <= 0.0:
		_other_timer = 0.25
		_other_u = Galaxy.road_track(other).project_global(flight.pos)
	else:
		_other_u = Galaxy.road_track(other).project(flight.pos, _other_u)
	if _road_dist(other, _other_u, flight.pos) + 15.0 < _road_dist(hw_road, hw_u, flight.pos):
		hw_road = other
		hw_u = _other_u
		tr = Galaxy.road_track(hw_road)
		hw_d = 1 if (flight.pos - tr.pos(hw_u)).dot(tr.right(hw_u)) >= 0.0 else -1

	# Keep to our side of the median, inside the tunnel walls, under the ceiling, and short
	# of HWY 1's dead ends.
	var right := tr.right(hw_u) * hw_d
	var x := (flight.pos - tr.pos(hw_u)).dot(right)
	var cx := clampf(x, HighwayWorld.FLY_MIN_X, HighwayWorld.FLY_MAX_X)
	if cx != x:
		flight.pos += right * (cx - x)
		_bounce(right * signf(cx - x))
	if flight.pos.y > HighwayWorld.FLY_CEILING or flight.pos.y < 2.0:
		_bounce(Vector3.DOWN if flight.pos.y > HighwayWorld.FLY_CEILING else Vector3.UP)
		flight.pos.y = clampf(flight.pos.y, 2.0, HighwayWorld.FLY_CEILING)
		edge_warn = 1.0
	if hw_road == 0:
		for end_u in [0.0, tr.length]:
			var out := tr.tangent(end_u) * (-1.0 if end_u == 0.0 else 1.0)
			var past := (flight.pos - tr.pos(end_u)).dot(out)
			if past > 0.0:
				flight.pos -= out * past
				_bounce(-out)

	var along := flight.forward().dot(tr.tangent(hw_u))
	if absf(along) > 0.3:
		hw_dv = 1 if along > 0.0 else -1

	for g in Galaxy.gates:
		if g.kind == "off" and (g.hw.origin as Vector3).distance_to(flight.pos) < 500.0 and GateBuilder.crossed(g, g.hw, prev, flight.pos):
			_exit_to_space(g)
			return

	var cam := flight.camera_transform()
	cam.origin.y = clampf(cam.origin.y, 1.0, HighwayWorld.CAM_CEILING)
	_apply_blend(delta, Transform3D(flight.ship_basis(), flight.pos), cam)
	ShipMesh.set_engine_glow(ship, _speed_frac())
	var r := hw_road
	var d := hw_d
	var dv := hw_dv
	var u0 := hw_u
	var frame := func(t: float) -> Dictionary:
		var t2 := Galaxy.road_track(r)
		var uu := u0 + dv * t
		var fwd := t2.tangent(uu) * dv
		return {"center": t2.pos(uu), "carr": t2.point(uu, d * Galaxy.CARR_CENTER), "fwd": fwd, "right": fwd.cross(Vector3.UP)}
	highway.update(delta, {"frame": frame, "phase": hw_u * hw_dv, "road": hw_road, "d": hw_d, "u": hw_u, "dv": hw_dv,
		"speed": flight.forward_speed(), "ship": flight.pos, "show_limits": true})
	_hud_highway()


func toggle_dock() -> void:
	if mode != Mode.HIGHWAY:
		return
	if docked:
		_undock()
	else:
		_try_dock()


func _try_dock() -> void:
	var p := flight.pos
	if p.y > DOCK_MAX_ALT:
		_say("Too high to dock  -  get within %d m of the road" % int(DOCK_MAX_ALT))
		return
	var speed := maxf(flight.forward_speed(), 0.0)
	var tr := Galaxy.road_track(hw_road)
	var x := (p - tr.pos(hw_u)).dot(tr.right(hw_u) * hw_d)
	var on_road := hw_u > 0.5 and hw_u < tr.length - 0.5
	if on_road and x <= Galaxy.MEDIAN + Galaxy.CARR_W + Galaxy.SHOULDER:
		if flight.forward().dot(Galaxy.carr_fwd(hw_road, hw_d, hw_u)) < 0.3:
			_say("Face the direction of traffic to dock")
			return
		_start_blend()
		drive.dock_road(hw_road, hw_d, hw_u, x - Galaxy.CARR_CENTER, speed, flight.cls.max_speed)
		_docked()
		return
	# Off the carriageway: try the nearest link (ramp or junction turn).
	var best := {}
	var best_d := Galaxy.RAMP_HALF_W + 10.0
	var best_s := 0.0
	for l in Galaxy.links:
		var lt: Track = l.track
		if (lt.pos(lt.length * 0.5) - p).length() > lt.length:
			continue
		var s := lt.project(p, lt.length * 0.5, 8)
		var q := lt.pos(s)
		var dist := Vector2(p.x - q.x, p.z - q.z).length()
		if dist < best_d and flight.forward().dot(lt.tangent(s)) > 0.3:
			best = l
			best_d = dist
			best_s = s
	if best.is_empty():
		_say("Too far from the road to dock")
		return
	var bt: Track = best.track
	_start_blend()
	drive.dock_link(best, best_s, (p - bt.pos(best_s)).dot(bt.right(best_s)), speed, flight.cls.max_speed)
	_docked()


func _docked() -> void:
	docked = true
	_set_mouse_for_mode()
	_say("Docked  -  cruising", 2.0)


func _undock() -> void:
	_start_blend()
	var xf := drive.ship_xform
	var fwd := -xf.basis.z
	fwd.y = 0.0
	flight.place(xf.origin, fwd.normalized(), minf(drive.speed, flight.cls.max_speed))
	var cw := drive.ref_carriageway()
	hw_road = cw.x
	hw_d = cw.y
	hw_dv = hw_d
	hw_u = Galaxy.road_track(hw_road).project(xf.origin, drive.ref_u())
	docked = false
	_set_mouse_for_mode()
	_say("Undocked  -  free flight", 2.0)


# --- Highway: docked ----------------------------------------------------------------------

func _process_docked(delta: float) -> void:
	drive.update(delta)
	if not drive.exit_gate.is_empty():
		ship.transform = drive.ship_xform
		_exit_to_space(drive.exit_gate)
		return
	_apply_blend(delta, drive.ship_xform, drive.cam_xform)
	ShipMesh.set_engine_glow(ship, drive.speed / (flight.cls.max_speed as float))
	var cw := drive.ref_carriageway()
	hw_road = cw.x
	hw_d = cw.y
	hw_u = drive.ref_u()
	var phase := drive.u * drive.d if drive.on_road else drive.odo
	highway.update(delta, {"frame": drive.frame_at, "phase": phase, "road": cw.x, "d": cw.y, "u": hw_u, "dv": cw.y,
		"speed": drive.speed, "ship": ship.position, "show_limits": false})
	_hud_highway()


# --- HUD ----------------------------------------------------------------------------------

static func _fmt_dist(m: float) -> String:
	return "%.1f km" % (m / 1000.0) if m >= 1000.0 else "%d m" % int(m)


func _center_msg(fallback: String) -> String:
	return msg if msg_t > 0.0 else fallback


func _hud_space() -> void:
	var combat: Combat = space_world.combat
	var seat: String = ["PILOT", "GUNNER", "MISSILE"][station]
	hud.left_lines = [
		["OPEN SPACE  -  %s  -  %s" % [flight.cls.name, seat], Color(0.6, 0.9, 1.0), 13],
		["SECTOR  %s" % Galaxy.sector_name(space_world.current), Color.WHITE, 22],
		["%d m/s" % int(flight.velocity.length()), Color(0.8, 0.9, 1.0), 16],
	]
	hud.right_lines = []
	hud.center_msg = _center_msg("Edge of charted space" if edge_warn > 0.0 else "")
	_hud_flight(station == Station.PILOT)
	hud.speed_frac = _speed_frac()
	hud.crosshair = station == Station.TURRET
	hud.bars = _combat_bars(combat)

	var incoming := combat.incoming_missiles()
	hud.warning = ""
	if not incoming.is_empty():
		var nearest := INF
		for m in incoming:
			nearest = minf(nearest, _to_world(flight.pos).distance_to(m.pos))
		hud.warning = "MISSILE INBOUND  %s" % _fmt_dist(nearest) + ("  -  G for the turret" if flight.cls.turret and station != Station.TURRET else "")

	match station:
		Station.TURRET:
			hud.help = "Mouse: aim turret   Left: auto-cannon   Right: blocker   X: missile   T: back to the helm   Tab / R: target"
		Station.MISSILE:
			hud.help = "Mouse: steer   W: boost   S: slow and turn tighter   A / D: dodge   Left: detonate"
			var m: Dictionary = combat.player_missile
			if not m.is_empty():
				var mp: Vector3 = (m.pos as Vector3) - space_world.center
				var aim_pt := mp + combat.player_missile_aim_dir() * 800.0
				var fwd_pt := mp - Combat.missile_basis(m).z * 800.0
				hud.reticle = camera.unproject_position(aim_pt) if not camera.is_position_behind(aim_pt) else Vector2(-1, -1)
				hud.heading = camera.unproject_position(fwd_pt) if not camera.is_position_behind(fwd_pt) else Vector2(-1, -1)
		_:
			var weapons := "Left: cannon   Right: laser   " if not flight.cls.turret else "G: turret   "
			hud.help = "Mouse: aim   W / S: thrust / brake   " + weapons + "X: missile   Tab / R: target   P / O / I: drone / freighter / fighter   Numpad 1 / 2: freighter / fighter"

	# Combat ships on sensors get a marker; the target gets a box instead.
	var markers: Array = hud.markers
	for c in contacts:
		if not (c.id as String).begins_with("c") or c.id == target_id:
			continue
		var p: Vector3 = (c.pos as Vector3) - space_world.center
		var mk_count := markers.size()
		_marker(markers, p, "%s  %s" % [c.name, _fmt_dist(c.dist)], _contact_color(c), true)
		if markers.size() > mk_count and not markers[-1].arrow:
			markers[-1].bar = c.hp_frac
	_hud_target()
	_hud_radar(combat)
	for m in incoming:
		_marker(markers, (m.pos as Vector3) - space_world.center, "MISSILE", GateBuilder.OFF_TINT.lerp(Color.RED, 0.6), true)
	for g in space_world.current_gates():
		if not GateBuilder.is_entry(g):
			continue
		var p: Vector3 = space_world.gate_local(g).origin
		_marker(markers, p, "%s  %s" % [g.label, _fmt_dist(p.distance_to(flight.pos))], GateBuilder.tint_for(g), true)
	for b in space_world.visible_bodies():
		var cur: bool = b.sector == space_world.current
		var dist := flight.pos.distance_to(b.pos)
		if b.kind == "moon" and (not cur or dist > 10000.0):
			continue
		var txt: String = b.name + ("  " + _fmt_dist(dist) if cur else "  (%s)" % Galaxy.sector_name(b.sector))
		var col := Color(1.0, 0.85, 0.6) if cur else Color(0.7, 0.7, 0.8, 0.6)
		if b.kind == "moon":
			col = Color(0.85, 0.8, 0.7, 0.7)
		_marker(markers, b.pos, txt, col, cur and b.kind == "planet")
	hud.queue_redraw()


static func _contact_color(c: Dictionary) -> Color:
	if c.hostile:
		return Color(1.0, 0.42, 0.35)
	if (c.id as String).begins_with("c"):
		return Color(1.0, 0.85, 0.5)    # test drone
	return Color(0.75, 0.85, 0.8)


## The target's info panel, and a box round it in the view (an edge arrow when off screen).
func _hud_target() -> void:
	hud.target = {}
	hud.target_box = {}
	var t := target()
	if t.is_empty():
		return
	var col := _contact_color(t)
	var to_t: Vector3 = (t.pos as Vector3) - _to_world(flight.pos)
	var closing := -((t.vel as Vector3) - flight.velocity).dot(to_t.normalized())
	hud.target = {"name": t.name, "cls": t.cls, "stance": "HOSTILE" if t.hostile else "NEUTRAL", "color": col,
		"dist": _fmt_dist(t.dist), "speed": "%d m/s" % int((t.vel as Vector3).length()),
		"closing": "%+d m/s" % int(closing), "hp": t.hp_frac}
	var p: Vector3 = (t.pos as Vector3) - space_world.center
	var vp := get_viewport().get_visible_rect().size
	var sp := camera.unproject_position(p)
	if camera.is_position_behind(p) or sp.x < 0.0 or sp.y < 0.0 or sp.x > vp.x or sp.y > vp.y:
		_marker(hud.markers, p, t.name, col, true)
		return
	var edge := camera.unproject_position(p + camera.global_basis.x * (t.radius as float))
	hud.target_box = {"pos": sp, "half": maxf(sp.distance_to(edge) * 1.25, 14.0), "color": col}


## Top-down radar of sensor range, with the ship's heading always up.
func _hud_radar(combat: Combat) -> void:
	var me := _to_world(flight.pos)
	var fwd := flight.forward()
	fwd.y = 0.0
	fwd = fwd.normalized() if fwd.length() > 0.001 else Vector3.FORWARD
	var right := fwd.cross(Vector3.UP)
	var to_radar := func(p: Vector3) -> Vector2:
		var d := p - me
		return Vector2(d.dot(right), -d.dot(fwd)) / Combat.SENSOR_RANGE
	var blips: Array = []
	for c in contacts:
		blips.append({"p": to_radar.call(c.pos), "color": _contact_color(c), "target": c.id == target_id, "missile": false})
	for m in combat.missiles:
		var mp: Vector3 = m.pos
		if mp.distance_to(me) <= Combat.SENSOR_RANGE:
			blips.append({"p": to_radar.call(mp), "color": Color(0.6, 0.85, 1.0) if m.player else Color(1.0, 0.3, 0.25),
				"target": false, "missile": true})
	hud.radar = {"range": _fmt_dist(Combat.SENSOR_RANGE), "blips": blips}


func _combat_bars(combat: Combat) -> Array:
	var bars: Array = [{"label": "HULL", "frac": hull / (flight.cls.hull as float), "color": Color(0.55, 0.9, 0.6), "text": "%d" % int(hull)}]
	var m: Dictionary = combat.player_missile
	if station == Station.MISSILE and not m.is_empty():
		bars.append({"label": "BOOST  (W)", "frac": (m.boost_left as float) / Combat.T.missile_boost_time, "color": Color(1.0, 0.7, 0.3)})
		bars.append({"label": "FUSE", "frac": (m.fuse as float) / Combat.T.missile_fuse, "color": Color(0.8, 0.8, 0.85), "text": "%.1f s" % m.fuse})
	else:
		var ready := missile_cd <= 0.0
		bars.append({"label": "MISSILE  (X)", "frac": 1.0 - missile_cd / Combat.T.missile_cooldown,
			"color": Color(0.6, 0.85, 1.0) if ready else Color(0.45, 0.5, 0.6), "text": "ready" if ready else "%.1f s" % missile_cd})
	if station == Station.TURRET:
		var b_ready := blocker_cd <= 0.0
		bars.append({"label": "BLOCKER", "frac": 1.0 - maxf(blocker_cd, 0.0) / Combat.T.blocker_cooldown,
			"color": Combat.BLOCKER_PLAYER if b_ready else Color(0.45, 0.5, 0.6)})
	if not flight.cls.turret:
		bars.append({"label": "LASER HEAT", "frac": combat.laser_heat,
			"color": Color(1.0, 0.3, 0.25) if combat.laser_locked else Color(1.0, 0.7, 0.4), "text": "OVERHEATED" if combat.laser_locked else ""})
	return bars


func _hud_hop(length: float) -> void:
	hud.left_lines = [
		["HOP LANE", Color(0.8, 0.7, 1.0), 13],
		["> %s" % hop.to.name, Color.WHITE, 22],
		["%d m/s   %s to go" % [int(hop_speed), _fmt_dist(length - hop_s)], Color(0.8, 0.9, 1.0), 16],
	]
	hud.right_lines = []
	hud.center_msg = _center_msg("")
	hud.help = "Riding a hop lane"
	_hud_flight(false)
	hud.queue_redraw()


## Reticle, heading pip and pitch gauge, shown whenever the ship is flying freely.
func _hud_flight(show: bool) -> void:
	hud.markers = []
	hud.radar = {}
	hud.target = {}
	hud.target_box = {}
	hud.show_flight = show
	hud.crosshair = false
	hud.bars = []
	hud.warning = ""
	hud.speed_frac = _speed_frac() if mode != Mode.HIGHWAY or not docked else drive.speed / (flight.cls.max_speed as float)
	hud.reticle = Vector2(-1, -1)
	hud.heading = Vector2(-1, -1)
	if not show:
		return
	var fwd_pt := flight.pos + flight.forward() * 800.0
	var aim_pt := flight.pos + flight.aim_dir() * 800.0
	hud.heading = camera.unproject_position(fwd_pt) if not camera.is_position_behind(fwd_pt) else Vector2(-1, -1)
	hud.reticle = camera.unproject_position(aim_pt) if not camera.is_position_behind(aim_pt) else Vector2(-1, -1)
	hud.pitch_frac = flight.pitch / SpaceFlight.MAX_PITCH


func _marker(list: Array, p: Vector3, text: String, color: Color, clamp_to_edge: bool) -> void:
	var vp := get_viewport().get_visible_rect().size
	var behind := camera.is_position_behind(p)
	var sp := camera.unproject_position(p)
	var margin := 40.0
	var onscreen := not behind and sp.x > margin and sp.y > margin and sp.x < vp.x - margin and sp.y < vp.y - margin
	if onscreen:
		list.append({"pos": sp, "text": text, "color": color, "arrow": false})
	elif clamp_to_edge:
		var c := vp * 0.5
		var dir := (sp - c) * (-1.0 if behind else 1.0)
		if dir.length() < 1.0:
			dir = Vector2.DOWN
		var k := minf((c.x - margin) / maxf(absf(dir.x), 0.001), (c.y - margin) / maxf(absf(dir.y), 0.001))
		list.append({"pos": c + dir * k, "text": text.get_slice("  ", 0), "color": color, "arrow": true})


func _hud_highway() -> void:
	_hud_flight(not docked)
	var fwd := Galaxy.carr_fwd(hw_road, hw_d, hw_u)
	var sec := Galaxy.hex_of(Galaxy.world_of_hw(ship.position))
	var speed := drive.speed if docked else flight.velocity.length()
	var status := "DOCKED" if docked else "FREE FLIGHT"
	hud.left_lines = [
		["THE LATTICE  -  %s" % status, Color(0.6, 0.9, 1.0), 13],
		["%s %s  >  %s" % [Galaxy.ROAD_NAMES[hw_road], Galaxy.compass(fwd), Galaxy.carr_destination(hw_road, hw_d)], Color.WHITE, 22],
		["%d m/s   (~%d m/s in open space)" % [int(speed), int(speed / Galaxy.HW_SCALE)], Color(0.8, 0.9, 1.0), 16],
	]
	var lines: Array = [["CURRENT SECTOR", Color(0.6, 0.9, 1.0), 13], [Galaxy.sector_name(sec), Color.WHITE, 22]]
	if docked and not drive.on_road:
		var lk: Dictionary = drive.link
		var col := GateBuilder.OFF_TINT if lk.kind == "off" else GateBuilder.ON_TINT
		lines.append([("TAKING  " if lk.kind != "on" else "MERGING ONTO  ") + (lk.label as String), col, 15])
	lines.append(["UPCOMING", Color(0.6, 0.9, 1.0), 13])
	var ev := Galaxy.upcoming(hw_road, hw_d, hw_u, 4)
	for e in ev:
		var col := GateBuilder.OFF_TINT if e.kind == "off" or e.kind == "end" else Color(0.85, 0.85, 0.8)
		lines.append(["%-36s %8s" % [e.text, _fmt_dist(e.dist)], col, 15])
	var hint := ""
	if not ev.is_empty() and ev[0].dist < 600.0:
		var e: Dictionary = ev[0]
		if docked:
			match e.kind:
				"off":
					hint = "%s in %s  -  keep right to take it" % [e.text, _fmt_dist(e.dist)]
				"turn":
					hint = "%s in %s" % [e.text, _fmt_dist(e.dist)]
				"jct_end":
					hint = "Junction in %s  -  keep left or right" % _fmt_dist(e.dist)
		elif e.kind == "off":
			hint = "%s in %s  -  fly through the amber frame to take it" % [e.text, _fmt_dist(e.dist)]
	hud.right_lines = lines
	hud.center_msg = _center_msg("Ceiling" if edge_warn > 0.0 and not docked else hint)
	if docked:
		hud.help = "C: undock   A / D: change lane   Right lane at an exit takes it   Left / right lane picks the turn at a junction"
	else:
		hud.help = "C: dock to the road   Mouse: aim   W / S: thrust / brake   Fly through an amber frame to exit   Esc: release mouse"
	hud.queue_redraw()
