class_name HighwayScene
extends Node3D
## The highway harness: one road, its traffic, one ship, and every number that
## could be wrong on the HUD. `make fly`.
##
## **Build order step 2** (`docs/HIGHWAY_BUILD_ORDER.md`): a road with somewhere to
## go. The ship starts in open space outside the mouth of the first northbound
## on-ramp, so the first thing flown is getting on.
##
## A scene of its own rather than a change to `exploration.tscn`, because the road
## has to be flyable before it has anywhere to be. It joins the map at step 5.
## Everything is built in code from tuning; the `.tscn` is a shell.

## How far outside the on-ramp's mouth the ship starts, and where a reset puts it.
## Infrastructure: far enough back to see the mouth as a thing to aim at.
const START_OUTSIDE_METRES: float = 260.0
## The ramp the harness starts at. The first junction northbound is the one nearest
## the start of the road.
const START_RAMP: String = "northbound on-ramp 1"

var _road: HighwayRoad
var _traffic: HighwayTraffic
var _ship: Mothership
var _camera: ChaseCamera
var _hud: DebugHud
## How much the camera is held to the road right now. Walked toward the tuned share
## on the road and toward zero off it, so leaving by a ramp mouth is a turn of the
## camera and not a cut.
var _camera_share: float = 0.0
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

	# A child of the road, so it is drawn in the road's frame and moves with it.
	_traffic = HighwayTraffic.new()
	_traffic.name = "Traffic"
	_traffic.road = _road
	_road.add_child(_traffic)


func _build_ship() -> void:
	_ship = Mothership.new()
	_ship.name = "Ship"
	_root.add_child(_ship)
	_ship.set_autopilot(false)
	_ship.piloted = true
	_road.ship = _ship
	_traffic.ship = _ship

	_camera = ChaseCamera.new()
	_camera.name = "ChaseCamera"
	_camera.subject = _ship
	_camera.tuning_prefix = "camera/ship"
	_camera.boom_scale = _ship.hull_scale()
	add_child(_camera)
	_place_at_start()
	_camera.current = true


## Put the ship back outside the first on-ramp, at rest, pointing into its mouth.
##
## The harness's one convenience: getting on, bouncing, and taking an exit are all
## things done over and over, and the road is too long to fly back along.
func _place_at_start() -> void:
	var ramp := _road.route_named(START_RAMP)
	var mouth := _road.shell().mouth() if ramp == null or ramp.sections.is_empty() \
		else ramp.sections[0].start
	_ship.transform = Transform3D(mouth.basis,
		mouth.origin + mouth.basis.z * START_OUTSIDE_METRES)
	_ship.reset_motion()
	_camera_share = 0.0
	_camera.reference_share = 0.0
	_camera.snap()


func _build_hud() -> void:
	_hud = DebugHud.new()
	_hud.name = "DebugHud"
	add_child(_hud)

	_hud.add_row("road", func() -> String:
		var routes := _road.shell().routes
		var carriageways := 0
		for route in routes:
			if route.kind == HighwayRoute.Kind.CARRIAGEWAY:
				carriageways += 1
		return "%d carriageways of %.1f km  ·  %d ramps  ·  %.0f x %.0f m tube  ·  median %.0f m" % [
			carriageways, routes[0].length() / 1000.0, routes.size() - carriageways,
			Tuning.num("highway/tube_width"), Tuning.num("highway/tube_height"),
			Tuning.num("highway/median_width")])
	# Where the ship is, in the road's own words. "Off the road" is worth seeing
	# rather than being rounded into the nearest tube: leaving through a ramp mouth is
	# the thing the ramps are for.
	_hud.add_row("on", func() -> String:
		var where := _road.shell().progress(_road.ship_in_road())
		if int(where["route"]) < 0:
			return "OFF THE ROAD"
		return "%s  ·  %.1f of %.1f km" % [String(where["name"]).to_upper(),
			float(where["along"]) / 1000.0, float(where["total"]) / 1000.0])
	_hud.add_row("next", func() -> String:
		return _next_junction())
	# The camera row says how much of what is on screen is the road's direction and
	# how much is the ship's, because the two diverge the moment the player looks
	# off-axis and "the camera is wrong" and "the ship is pointing there" look alike.
	_hud.add_row("heading", func() -> String:
		var road_basis: Variant = _road.road_basis_at_ship()
		if road_basis == null:
			return "free  ·  camera on the ship"
		var off := rad_to_deg((-_ship.global_basis.z).angle_to(
			-(road_basis as Basis).z))
		return "%.0f deg off the road's axis  ·  camera %.0f%% on the road" % [
			off, _camera_share * 100.0])
	_hud.add_row("clearance", func() -> String:
		var near := _road.shell().clearance(_road.ship_in_road())
		var distance := float(near["distance"])
		if distance == INF:
			return "no road nearby"
		return "%.0f m to the %s wall of the %s  ·  hull held %.0f m off" % [
			distance, String(near["wall"]), String(near["tube"]), _road.wall_clearance()])
	_hud.add_row("bounce", func() -> String:
		var wall := "no contact yet" if _road.seconds_since_bounce() == INF else \
			"%.1f m/s into the %s, %.1f s ago" % [_road.last_bounce_speed(),
				_road.last_bounce_wall(), _road.seconds_since_bounce()]
		return "%s  ·  drift %.1f m/s" % [wall, _ship.external_velocity().length()])
	_hud.add_row("traffic", func() -> String:
		var near := _traffic.nearest(_road.ship_in_road())
		if float(near["distance"]) == INF:
			return "none"
		var lanes := maxi(Tuning.integer("highway/traffic_lanes"), 1)
		var touched := "" if _traffic.seconds_since_contact() == INF else \
			"  ·  last glanced off one %.1f s ago" % _traffic.seconds_since_contact()
		return "%d ships  ·  nearest %.0f m, %s lane %d of %d, %.0f m/s%s" % [
			_traffic.count(), float(near["distance"]), String(near["route"]),
			int(near["lane"]) + 1, lanes, float(near["speed"]), touched])
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
		return "CRUISE  ·  %.0f m/s" % cruise)
	_hud.add_row("flight", func() -> String:
		return "throttle %3.0f%%  ·  %.0f m/s of %.0f" % [
			_ship.throttle() * 100.0, _ship.speed(), _ship.manual_max_speed()])
	_hud.add_row("class", func() -> String:
		return "%s  ·  %.1f m/s top  ·  %.0f deg/s turn" % [
			HullClass.name_of(_ship.hull_class).to_upper(),
			HullClass.max_speed(_ship.hull_class), _ship.turn_rate_deg_per_sec()])
	_hud.add_row("keys", func() -> String:
		return "W/S throttle · A/D thrusters · mouse steers · R back to the start · H hull · F1 hud · F2 tune")


## The next ramp ahead on the carriageway the ship is on, or where the ramp it is on
## goes. What a sign will say one day; for now the HUD says it.
func _next_junction() -> String:
	var where := _road.shell().progress(_road.ship_in_road())
	var route_id := int(where["route"])
	if route_id < 0:
		return "—"
	var routes := _road.shell().routes
	var here := routes[route_id]
	if here.kind == HighwayRoute.Kind.OFF_RAMP:
		return "on an exit  ·  the mouth is %.0f m ahead" % (
			here.length() - float(where["along"]))
	if here.kind == HighwayRoute.Kind.ON_RAMP:
		return "on an entrance  ·  merges into the %s in %.0f m" % [
			routes[here.carriageway].name, here.length() - float(where["along"])]
	var along := float(where["along"])
	var best_exit := INF
	var exit_name := ""
	var best_entry := INF
	for route in routes:
		if route.carriageway != route_id or route.joins_at < along:
			continue
		if route.kind == HighwayRoute.Kind.OFF_RAMP and route.joins_at - along < best_exit:
			best_exit = route.joins_at - along
			exit_name = route.name
		if route.kind == HighwayRoute.Kind.ON_RAMP and route.joins_at - along < best_entry:
			best_entry = route.joins_at - along
	var parts := PackedStringArray()
	parts.append("no more exits  ·  the road ends in %.0f m" % (here.length() - along) \
		if best_exit == INF else "exit in %.0f m (%s)" % [best_exit, exit_name])
	if best_entry != INF:
		parts.append("traffic joins in %.0f m" % best_entry)
	return "  ·  ".join(parts)


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

## Hold the camera to the road while the ship is on it. The camera only: the ship's
## nose is never touched (ADR 0012), so looking off-axis on the road shows the hull
## at an angle to a view that still runs down the tube.
func _process(delta: float) -> void:
	if _road == null or _ship == null:
		return
	var road_basis: Variant = _road.road_basis_at_ship()
	var wanted := 0.0 if road_basis == null \
		else clampf(Tuning.num("highway/camera_road_share"), 0.0, 1.0)
	_camera_share = move_toward(_camera_share, wanted,
		delta / maxf(Tuning.num("highway/camera_road_blend_seconds"), 0.001))
	if road_basis != null:
		_camera.reference_basis = road_basis
	_camera.reference_share = _camera_share


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
		_place_at_start()
	elif event.is_action_pressed("debug_cycle_hull"):
		_ship.set_hull_class(HullClass.next(_ship.hull_class))
		_camera.boom_scale = _ship.hull_scale()


func road() -> HighwayRoad:
	return _road


func traffic() -> HighwayTraffic:
	return _traffic


func ship() -> Mothership:
	return _ship


func camera() -> ChaseCamera:
	return _camera
