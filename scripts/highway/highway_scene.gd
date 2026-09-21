class_name HighwayScene
extends Node3D
## The highway harness: one road, one ship, and every number that could be wrong
## on the HUD. `make fly`.
##
## **Build order step 1** (`docs/HIGHWAY_BUILD_ORDER.md`). A scene of its own rather
## than a change to `exploration.tscn`, because the road has to be flyable before it
## has anywhere to be — and because a harness that loads in two seconds is what makes
## a bounce value worth iterating on. It joins the map at step 7.
##
## Nothing here is content. There is no system, no planet, no boundary, no portal:
## the only question this scene asks is what a square tube feels like to fly down and
## bump into. Everything is built in code from tuning; the `.tscn` is a shell.

## How far inside the mouth the ship starts, and where a reset puts it back.
## Infrastructure: far enough in to see the first rib, near enough to see the way in.
const START_INSIDE_METRES: float = 120.0

var _road: HighwayRoad
var _ship: Mothership
var _camera: ChaseCamera
var _hud: DebugHud
## Everything in road space hangs off one node, so a floating-origin shift is a move
## of this and nothing else has to know (ADR 0020).
var _root: Node3D


func _ready() -> void:
	_build_environment()
	_build_lights()
	_build_road()
	_build_ship()
	_build_hud()
	Tuning.reloaded.connect(_apply_tuning)
	_apply_tuning()
	DebugPanel.toggled.connect(func(open: bool) -> void:
		if not open:
			_apply_mouse_mode())
	_apply_mouse_mode()


# --- construction ------------------------------------------------------------

func _build_environment() -> void:
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	var world_env := WorldEnvironment.new()
	world_env.name = "WorldEnvironment"
	world_env.environment = env
	add_child(world_env)


func _build_lights() -> void:
	var key := DirectionalLight3D.new()
	key.name = "KeyLight"
	key.light_color = Color(1.0, 0.96, 0.9)
	add_child(key)
	var fill := DirectionalLight3D.new()
	fill.name = "FillLight"
	fill.light_color = Color(0.42, 0.56, 0.78)
	add_child(fill)


func _build_road() -> void:
	_root = Node3D.new()
	_root.name = "RoadRoot"
	add_child(_root)

	_road = HighwayRoad.new()
	_road.name = "Highway"
	_root.add_child(_road)


func _build_ship() -> void:
	_ship = Mothership.new()
	_ship.name = "Ship"
	_root.add_child(_ship)
	_ship.set_autopilot(false)
	_ship.piloted = true
	_road.ship = _ship
	_place_at_mouth()

	_camera = ChaseCamera.new()
	_camera.name = "ChaseCamera"
	_camera.subject = _ship
	_camera.tuning_prefix = "camera/ship"
	_camera.boom_scale = _ship.hull_scale()
	add_child(_camera)
	_camera.snap()
	_camera.current = true


## Put the ship back at the start of the road, at rest, pointing down it.
##
## The harness's one convenience. A bounce is a thing you do over and over at
## slightly different angles, and 4.8 km of turning around between attempts is how
## a feel question goes unanswered.
func _place_at_mouth() -> void:
	var mouth := _road.shell().mouth()
	_ship.transform = Transform3D(mouth.basis,
		mouth.origin + (-mouth.basis.z) * START_INSIDE_METRES)
	_ship.reset_motion()
	if _camera != null:
		_camera.snap()


func _build_hud() -> void:
	_hud = DebugHud.new()
	_hud.name = "DebugHud"
	add_child(_hud)

	_hud.add_row("tube", func() -> String:
		var shell := _road.shell()
		return "%.0f x %.0f m square  ·  %d tiles of %.0f m  ·  %.0f m of road" % [
			Tuning.num("highway/tube_width"), Tuning.num("highway/tube_height"),
			shell.sections.size(), Tuning.num("highway/section_length"),
			shell.total_length()])
	# Where on the road the ship is. Section -1 is the honest answer for "out of the
	# end", and it is worth seeing rather than being rounded into the nearest tile:
	# a road you can leave is the thing steps 3 and 6 are going to cut holes in.
	_hud.add_row("road", func() -> String:
		var where := _road.shell().progress(_road.ship_in_road())
		if int(where["section"]) < 0:
			return "OFF THE ROAD  ·  past an end"
		return "tile %d of %d  ·  %.0f m of %.0f" % [
			int(where["section"]) + 1, _road.shell().sections.size(),
			float(where["along"]), float(where["total"])])
	# The clearance row is the instrument the wall is judged with. A negative number
	# means the hull is outside a wall it should be inside, which is a bug and should
	# be readable as one without a debugger.
	_hud.add_row("clearance", func() -> String:
		var near := _road.shell().clearance(_road.ship_in_road())
		var distance := float(near["distance"])
		if distance == INF:
			return "no road"
		return "%.0f m to the %s wall  ·  hull held %.0f m off" % [
			distance, String(near["wall"]), _road.wall_clearance()])
	_hud.add_row("bounce", func() -> String:
		if _road.seconds_since_bounce() == INF:
			return "no contact yet"
		return "%.1f m/s into the %s wall, %.1f s ago  ·  drift %.1f m/s" % [
			_road.last_bounce_speed(), _road.last_bounce_wall(),
			_road.seconds_since_bounce(), _ship.external_velocity().length()])
	# The row that was missing: without it the road is a 4.8 km corridor at taxi
	# speed and there is no way to tell that cruise is the thing absent.
	_hud.add_row("cruise", func() -> String:
		var cruise := Tuning.num("exploration/cruise_speed")
		if not _ship.has_cruise_drive():
			return "NO CRUISE DRIVE  ·  a %s flies the road at its own %.1f m/s" % [
				HullClass.name_of(_ship.hull_class), _ship.engine_max_speed()]
		if _ship.cruise_ceiling <= 0.0:
			return "off  ·  only runs inside the road"
		if _road.cruise_share() < 0.995:
			return "SPOOLING  ·  full throttle is %.0f of %.0f m/s" % [
				_ship.cruise_ceiling, cruise]
		return "CRUISE  ·  %.0f m/s  ·  the whole road in %.0f s" % [
			cruise, _road.shell().total_length() / maxf(cruise, 0.001)])
	_hud.add_row("flight", func() -> String:
		return "throttle %3.0f%%  ·  %.0f m/s of %.0f" % [
			_ship.throttle() * 100.0, _ship.speed(), _ship.manual_max_speed()])
	_hud.add_row("class", func() -> String:
		return "%s  ·  %.1f m/s top  ·  %.0f deg/s turn" % [
			HullClass.name_of(_ship.hull_class).to_upper(),
			HullClass.max_speed(_ship.hull_class), _ship.turn_rate_deg_per_sec()])
	_hud.add_row("keys", func() -> String:
		return "W/S throttle · A/D thrusters · mouse steers · R back to the start · H hull · F1 hud · F2 tune")


func _apply_tuning() -> void:
	var env := (get_node("WorldEnvironment") as WorldEnvironment).environment
	env.background_color = Tuning.color("arena/background_color")
	env.ambient_light_color = Tuning.color("arena/background_color").lightened(0.35)
	env.ambient_light_energy = Tuning.num("arena/ambient_energy")
	env.glow_enabled = Tuning.flag("arena/glow_enabled")
	env.glow_intensity = Tuning.num("arena/glow_intensity")

	var key := get_node("KeyLight") as DirectionalLight3D
	key.light_energy = Tuning.num("arena/key_light_energy")
	key.rotation_degrees = Tuning.vec3("arena/key_light_angles_deg")
	var fill := get_node("FillLight") as DirectionalLight3D
	fill.light_energy = Tuning.num("arena/fill_light_energy")
	fill.rotation_degrees = Tuning.vec3("arena/fill_light_angles_deg")


# --- flying it ---------------------------------------------------------------

func _apply_mouse_mode() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if DebugPanel.is_open() \
		else Input.MOUSE_MODE_CAPTURED


func _unhandled_input(event: InputEvent) -> void:
	if DebugPanel.is_open():
		return
	if event is InputEventMouseMotion:
		_ship.add_mouse_steer((event as InputEventMouseMotion).relative)
		return
	if event.is_action_pressed("quit"):
		get_tree().quit()
	elif event.is_action_pressed("debug_toggle_hud"):
		_hud.toggle()
	elif event.is_action_pressed("debug_reload_tuning"):
		Tuning.reload()
	elif event.is_action_pressed("debug_reset_road"):
		_place_at_mouth()
	elif event.is_action_pressed("debug_cycle_hull"):
		_ship.set_hull_class(HullClass.next(_ship.hull_class))
		_camera.boom_scale = _ship.hull_scale()


func road() -> HighwayRoad:
	return _road


func ship() -> Mothership:
	return _ship
