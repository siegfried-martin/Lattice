class_name SystemMap
extends Node3D
## The test map: five systems on two crossing highways, joined by corridors
## (`docs/EXPLORATION_POC_IMPLEMENTATION.md`).
##
##     [ D ]
##        ╲
##  [ A ]──[ B ]────────[ C ]      A-377B runs A, B, C; K-112 runs D, B, E.
##           ╲
##           [ E ]
##
## **Where everything is comes from `data/routes.json`** (ADR 0096): the systems'
## positions, which systems each highway joins, and which ramps exist. This node owns
## three things nothing else should: the **layout** (placing the discs and corridors
## from that data), the **boundary** composed from every piece of it, and the **hot
## reload** of both — on `Tuning.reloaded` and on `Routes.reloaded`. The discs and
## links do not subscribe themselves: their geometry depends on a layout they do not own.
##
## **Two worlds** (`docs/WORMHOLE_PROTOTYPE.md`): open space, and the wormhole the
## highways run inside, far below it in the same frame. Only the ramps exist in both.
## The ship's collider knows every tube of both, so which world the ship is in is a
## fact about the tube it is in; the crossing between them happens on a ramp.
##
## The road is a place, not a mode (ADR 0057): the map knows where the ship is and the
## ship only ever receives a sample of the road under it. Which tube the ship is in is
## decided by the road's own geometry (`RoadCollider`), never by a rule here.

## Placeholder identity. The POC doc calls them A, B and C; D and E arrived with the
## crossing highway, and naming them anything more is content this POC does not test.
const NAMES: PackedStringArray = ["SYSTEM A", "SYSTEM B", "SYSTEM C",
	"SYSTEM D", "SYSTEM E"]
const LETTERS: PackedStringArray = ["A", "B", "C", "D", "E"]

## Relayed from whichever system's envelope fired, so the dock screen can say where
## it is without the scene tracking which envelope belongs to which planet.
signal arrived(place: String)
signal departed()
## The ship has just crossed between the worlds by `move`, in the map's frame: the
## rigid transform that took it from where it was to where it is. The scene moves the
## camera by the same, so the crossing is not a cut.
signal crossed(move: Transform3D)

var _discs: Array[SystemDisc] = []
var _planets: Array[Planet] = []
## One star per planet: below and beside it, the system's only light.
var _stars: Array[Star] = []
var _approaches: Array[ApproachEnvelope] = []
var _links: Array[SystemLink] = []
## Which two systems each link joins, in `NAMES` order, built from the highways.
var _link_ends: Array[Vector2i] = []
## The road in open space: the ramps' twins, and nothing else.
var _road: RoadNetwork
## The road inside the wormhole: every highway, and the ramps that join them.
var _wormhole: RoadNetwork
## Both networks' tubes and roads, for the one collider.
var _all_tubes: Array[Tube] = []
var _all_roads: Array[Road] = []
## Every piece of playable space, united (ADR 0062).
var _field: BoundaryField = BoundaryField.new()
## The playable space inside the wormhole: the tunnels around its roads.
var _void: BoundaryField = BoundaryField.new()
## Which world the ship is in. Derived from the tube it is in whenever it is in one,
## so a debug drop onto either world's road puts the map there; kept while it is in
## open space or in the wormhole's void.
var _in_wormhole: bool = false
## What is out there past the edge. Background-layer only: nothing in it is queryable
## and nothing in it collides (CLAUDE.md's LOD/collision rule).
var _deep: DeepField
## The map's legs, system to system, sampled: what the deep field is scattered along.
var _spine: PackedVector3Array = PackedVector3Array()

var _seconds_outside: float = 0.0
var _warning: float = 0.0
var _speed_scale: float = 1.0
var _outbound: float = 0.0
## The tube the player is riding with the cruise drive running, or null.
var _riding: Tube = null
## The berth on the roadway. One per map: it is a thing the player does, not a thing a
## stretch of road has (ADR 0082).
var _berth: RoadBerth = null
## THE SECTORS (`docs/SECTOR_PROTOTYPE.md`, prototype 1, behind `sectors_enabled`):
## the world as an open hex tiling inside one border, and which sector the ship is in
## deciding how every body is drawn. With the flag off neither is touched.
var _sectors: SectorLayer = SectorLayer.new()
var _border: HexRegion = HexRegion.new()

## Whether the map reads the real input devices, or is told what was pressed. See
## `Mothership.reads_input`. Harness switch only.
var reads_input: bool = true
var pressed_dock: bool = false
var pressed_click: bool = false
var _previous: Vector3 = Vector3.ZERO
var _has_previous: bool = false


func _ready() -> void:
	_build()
	relayout()
	Tuning.reloaded.connect(relayout)
	# Saving `data/routes.json` relays the map out under the ship, the way saving
	# `tuning.cfg` re-tunes it. The road is data, and data you cannot nudge while
	# looking at it is a worse instrument than a slider (ADR 0096).
	Routes.reloaded.connect(relayout)


func _build() -> void:
	_berth = RoadBerth.new()
	_berth.name = "Berth"
	add_child(_berth)
	for i in NAMES.size():
		var letter := LETTERS[i]
		var disc := SystemDisc.new()
		disc.name = "Disc" + letter
		add_child(disc)
		_discs.append(disc)

		var planet := Planet.new()
		planet.name = "Planet" + letter
		add_child(planet)
		_planets.append(planet)

		var star := Star.new()
		star.name = "Star" + letter
		star.index = i
		add_child(star)
		_stars.append(star)

		var approach := ApproachEnvelope.new()
		approach.name = "Approach" + letter
		approach.host = planet
		add_child(approach)
		_approaches.append(approach)
		var place := NAMES[i]
		approach.arrived.connect(func() -> void: arrived.emit(place))
		approach.departed.connect(func() -> void: departed.emit())

	_road = RoadNetwork.new()
	_road.name = "Road"
	add_child(_road)
	_wormhole = RoadNetwork.new()
	_wormhole.name = "Wormhole"
	add_child(_wormhole)

	_deep = DeepField.new()
	_deep.name = "DeepField"
	add_child(_deep)

	# One corridor per leg, across every highway. The corridor is what you fly when
	# you decline the road, so a leg without one is a leg you may only travel by
	# highway. The legs are read from the route data once; a highway added to
	# `routes.json` needs a restart to get its corridors, which is the one thing the
	# hot reload does not cover.
	for highway in Routes.highway_names():
		var on_route := Routes.systems_on(highway)
		for i in on_route.size() - 1:
			var from_system := NAMES.find(on_route[i])
			var to_system := NAMES.find(on_route[i + 1])
			if from_system < 0 or to_system < 0:
				continue
			var link := SystemLink.new()
			link.name = "Link%s%s" % [LETTERS[from_system], LETTERS[to_system]]
			link.link_name = "%s to %s" % [NAMES[from_system], NAMES[to_system]]
			link.from_name = NAMES[from_system]
			link.to_name = NAMES[to_system]
			add_child(link)
			_links.append(link)
			_link_ends.append(Vector2i(from_system, to_system))


## Place everything, and recompose the boundary from what was placed. Called on every
## tuning and route reload.
func relayout() -> void:
	var positions := Routes.system_positions()
	var radius := Tuning.num("exploration/system_diameter") * 0.5
	var facings: Array[Array] = []
	for i in _discs.size():
		facings.append([] as Array[float])
		_discs[i].position = positions.get(NAMES[i], Vector3.ZERO)
	# Apertures accumulate across highways: a system on two roads has a mouth facing
	# each way along each of them, which is what makes B an interchange.
	for i in _links.size():
		var ends := _link_ends[i]
		var from_at := _discs[ends.x].position
		var to_at := _discs[ends.y].position
		var step := (to_at - from_at).normalized()
		var bearing := SystemDisc.direction_to_bearing(step)
		(facings[ends.x] as Array[float]).append(bearing)
		(facings[ends.y] as Array[float]).append(bearing + 180.0)
		# The CORRIDOR is mouth to mouth: the bounded space between two systems.
		_links[i].follow(RoadPath.straight(from_at + step * radius, to_at - step * radius))
	for i in _discs.size():
		var disc := _discs[i]
		disc.system_name = NAMES[i]
		disc.bearings = facings[i] as Array[float]
		disc.rebuild()
		_planets[i].base = disc.position
		_planets[i].rebuild()
		_stars[i].base = disc.position
		_stars[i].planet_position = _planets[i].position
		_stars[i].rebuild()
		_approaches[i].position = _planets[i].position
		_approaches[i].rebuild()

	RoadNetwork.build_worlds(_road, _wormhole, Routes.data(), positions)
	_all_tubes = _road.tubes + _wormhole.tubes
	_all_roads = _road.roads + _wormhole.roads
	_spine = _legs(positions)
	for problem in _road.problems + _wormhole.problems:
		push_warning("[road] " + problem)

	_field.regions.clear()
	var open := Tuning.flag("exploration/sectors_enabled")
	if open:
		# ONE BORDER around everything; inside it, open space. The discs, corridors and
		# their funnels are hidden rather than removed, so the flag can go back off.
		_lay_the_border(positions)
		_field.regions.append(_border)
	else:
		for disc in _discs:
			_field.regions.append(disc.region())
		for link in _links:
			_field.regions.append(link.region())
		# THE ROAD CARRIES ITS OWN SPACE (ADR 0091). An interchange ramp cuts the corner
		# between two highways and is on no corridor; riding it put the boundary's red
		# across the view. A region per road: regions UNITE, so this only ever adds space.
		for road in _road.roads:
			_field.regions.append(_space_around(road))
	_field.warning_band = Tuning.num("exploration/bounds_warning_band")
	_field.stop_distance = Tuning.num("exploration/bounds_stop_distance")
	# Inside the wormhole the playable space is the road's own, and nothing else.
	_void.regions.clear()
	for road in _wormhole.roads:
		_void.regions.append(_space_around(road))
	_void.warning_band = _field.warning_band
	_void.stop_distance = _field.stop_distance
	_apply_world()

	# LAST, and it needs both of the things above it: the deep field is scattered
	# around the spine and rejected wherever the boundary says the point is playable.
	# In the open world the only wall is the far border, so the field is scattered
	# around every road instead, just past the road's own space: it is the thing beside
	# the road that the gear's speed is read against between systems.
	if open:
		var road_space := BoundaryField.new()
		for road in _road.roads:
			road_space.regions.append(_space_around(road))
		_deep.rebuild(_spine, road_space)
	else:
		_deep.rebuild(_spine, _field)


## Every highway's legs, system to system, as a sampled polyline. With the highways
## inside the wormhole this is the line between the systems in open space.
static func _legs(positions: Dictionary) -> PackedVector3Array:
	var out := PackedVector3Array()
	for highway in Routes.highway_names():
		var on_route := Routes.systems_on(highway)
		for i in on_route.size() - 1:
			if not positions.has(on_route[i]) or not positions.has(on_route[i + 1]):
				continue
			out.append_array(RoadPath.straight(positions[on_route[i]],
				positions[on_route[i + 1]]).points(200.0))
	return out


## The hex prism around the whole map: centred on the systems' centroid, its apothem
## reaching `sector_border_margin` past the farthest system or highway point, with the
## disc's ceiling and floor. The sectors are named after the systems centred in them.
func _lay_the_border(positions: Dictionary) -> void:
	var centroid := Vector3.ZERO
	var count := 0
	for system_name: String in positions:
		centroid += positions[system_name]
		count += 1
	if count > 0:
		centroid /= count
	centroid.y = 0.0
	var extent := 0.0
	for system_name: String in positions:
		var at: Vector3 = positions[system_name]
		extent = maxf(extent, Vector2(at.x - centroid.x, at.z - centroid.z).length())
	_border.center = centroid
	_border.circumradius = (extent + Tuning.num("exploration/sector_border_margin")) \
		/ (HexGrid.SQRT3 * 0.5)
	_border.ceiling = Tuning.num("exploration/system_ceiling_height")
	_border.floor_depth = Tuning.num("exploration/system_floor_depth")
	_sectors.name_cells(positions)


## Show one world and hide the other. Open space is the discs and corridors (when
## the sectors are off), the planets and stars, the deep field and the ramps' twins;
## the wormhole is its own network. Both stay resident: nothing loads at a crossing.
func _apply_world() -> void:
	var open := Tuning.flag("exploration/sectors_enabled")
	for disc in _discs:
		disc.visible = not open and not _in_wormhole
	for link in _links:
		link.visible = not open and not _in_wormhole
	for planet in _planets:
		planet.visible = not _in_wormhole
	for star in _stars:
		star.visible = not _in_wormhole
	_deep.visible = not _in_wormhole
	_road.visible = not _in_wormhole
	_wormhole.visible = _in_wormhole


func _enter_world(wormhole: bool) -> void:
	if wormhole == _in_wormhole:
		return
	_in_wormhole = wormhole
	_apply_world()


## The bounded space around one road. Radius is DERIVED — the section's own corner to
## corner plus the warning band — so riding a road can never redden it.
func _space_around(road: Road) -> TubeRegion:
	var tube := TubeRegion.new()
	tube.name_of = road.name
	tube.follow(road.path)
	tube.radius = sqrt(road.half_width * road.half_width + road.half_height * road.half_height) \
		+ Tuning.num("exploration/bounds_warning_band")
	tube.mouth_radius = tube.radius
	tube.flare_length = 0.0
	return tube


## Give the ship its collider against this map's road. Called once by the scene; the
## collider is the ship's because "which tube am I in" is a fact about the hull.
func attach(ship: Mothership) -> void:
	if ship.road == null:
		ship.road = RoadCollider.new()
	ship.road.setup(_all_tubes, _all_roads)


## Run the whole map against the ship for this frame.
##
## The order matters and is the whole treatment: paint first so the player is
## looking at red before anything else happens, work out the strain second so the
## ship slows rather than turns, and only then start counting toward damage.
func observe(ship: Mothership, delta: float) -> void:
	_speed_scale = 1.0
	_outbound = 0.0
	if ship == null or not is_instance_valid(ship) or delta <= 0.0:
		return
	if ship.road == null or ship.road.tubes != _all_tubes:
		var was := ship.road.tube.name if ship.road != null and ship.road.tube != null else ""
		attach(ship)
		if not was.is_empty():
			ship.road.tube = tube_named(was)
	var here := to_local(ship.global_position)
	var travelled := here.distance_to(_previous) if _has_previous else 0.0
	var took_dock := Input.is_action_just_pressed("dock") if reads_input \
		else pressed_dock
	pressed_dock = false
	pressed_click = false

	# The world is where the tube is (`_in_wormhole`).
	if ship.road != null and ship.road.tube != null:
		_enter_world(_wormhole.tubes.has(ship.road.tube))
	var bounds := field()
	_warning = bounds.warning(here)
	for disc in _discs:
		if disc.visible:
			disc.paint(_warning)
	for link in _links:
		if link.visible:
			link.paint(_warning)

	var heading := _heading_of(ship)
	_outbound = BoundaryField.outbound_fraction(heading, bounds.outward(here))
	_speed_scale = bounds.speed_ceiling_scale(here, heading)

	if not _in_wormhole:
		for approach in _approaches:
			approach.observe(ship, delta)
			_speed_scale = minf(_speed_scale, approach.speed_scale())

	_ride_the_road(ship, here)
	here = _cross_if_due(ship, here)
	# AFTER the road, because a berth binds to the tube the ship is in. A ramp that has
	# ENDED hands the berth to the road it merged into (ADR 0096).
	var onward: Tube = null
	if _riding != null and _riding.is_ramp():
		var record := ramp_of(_riding)
		if not record.is_empty():
			onward = record["to_tube"]
	_berth.observe(ship, _riding, onward, took_dock, delta)
	# An exit taken from the strip is an exit lined up for, whichever side of the
	# lane the ship is on.
	if ship.cruise != null and _berth.taking() != null:
		ship.cruise.downshift = true
	# HIGHWAY METRES, whoever is doing the steering (ADR 0086).
	if ship.is_cruising():
		ship.cruise_tank.burn(travelled)
	_name_the_ways()
	for gate in _road.gates() + _wormhole.gates():
		gate.repaint(delta)
	_road.set_active(_riding)
	_wormhole.set_active(_riding)
	_road.stream(here)
	_wormhole.stream(here)
	# THE TREADMILL (`Road.slip`): the ridden road's ribs slide along with the ship by
	# the gear's surplus, so they pass at the felt speed.
	if _riding != null and _riding.road != null:
		var fwd: Vector3 = _riding.travel_frame(_riding.local(here)["t"])["fwd"]
		(_riding.road as Road).roll(ship.gear_moved().dot(fwd) * _riding.direction)
	_road.roll()
	_wormhole.roll()
	_road.light(here, _riding)
	_wormhole.light(here, _riding)
	_deep.follow(here)
	_compress_the_distance(here, delta)
	_previous = here
	_has_previous = true

	if bounds.overshoot(here) <= 0.0:
		_seconds_outside = 0.0
		return
	_seconds_outside += delta
	var rate := BoundaryField.damage_per_second(_seconds_outside,
		Tuning.num("exploration/bounds_grace_seconds"),
		Tuning.num("exploration/bounds_damage_ramp_seconds"),
		Tuning.num("exploration/bounds_damage_per_second"))
	if rate > 0.0:
		ship.take_hit(rate * delta)


## Getting on and off the road.
##
## **Where the ship is, the geometry decides.** The collider says which tube the hull
## is in: it entered one through a mouth, crossed from one to another through an open
## wall, or left through a mouth into open space. Nothing here chooses a road.
##
## Entry is **on contact and instant** (ADR 0057): being inside a tube with a drive
## that can run puts the cruise drive on. Leaving is flying out of a mouth. Every
## portal's colour is set from the hull the player is flying THIS frame (ADR 0060).
func _ride_the_road(ship: Mothership, here: Vector3) -> void:
	var allowed := ship.has_cruise_drive()
	# ENGAGING needs fuel; STAYING does not (ADR 0086).
	var can_engage := allowed and not ship.cruise_tank.is_dry()
	_road.set_permitted(can_engage)
	_wormhole.set_permitted(can_engage)
	var clearance := ship.lane_clearance()
	var inside: Tube = ship.road.tube
	if inside == null:
		if _riding != null:
			_riding = null
			ship.cruise = null
			ship.leave_road()
			ship.reset_reticle()
		return
	if _riding == null:
		if not can_engage:
			return
		_riding = inside
		ship.cruise = inside.sample(here, clearance, ship.speed())
		_forgive_the_junction(ship, here, clearance)
		ship.adopt_road_axis(ship.cruise.axis)
		ship.reset_reticle()
		return
	if not allowed:
		# Losing the drive mid-road (the debug roster) leaves you in the tube at hull
		# speed. That is the honest reading of the drive belonging to the hull.
		_riding = null
		ship.cruise = null
		ship.leave_road()
		ship.reset_reticle()
		return
	# Merging and diverging, with no junction logic: the hull crossed an open wall and
	# the tube on the far side is the road now (ADR 0063's rule, applied to lanes).
	_riding = inside
	ship.cruise = _riding.sample(here, clearance, ship.speed())
	_forgive_the_junction(ship, here, clearance)


## THE CROSSING (`docs/WORMHOLE_PROTOTYPE.md`). A ship in a ramp that has a twin in
## the other world, past the ramp's `cross_at`, is moved to the same (t, u, v) in the
## twin by the rigid transform between the two ramps' frames there: the same tube,
## the same place in it, the same speed and heading against the road; only the world
## around it changes. Forward only: a ship that drifts back past the point stays
## where it is, held by the same walls, so nothing can flap between the worlds.
## Returns where the ship is now, in the map's frame.
func _cross_if_due(ship: Mothership, here: Vector3) -> Vector3:
	var inside: Tube = ship.road.tube if ship.road != null else null
	if inside == null or not inside.is_ramp():
		return here
	var record := ramp_of(inside)
	if record.is_empty() or record["twin"] == null or float(record["cross_at"]) < 0.0:
		return here
	var l := inside.local(here)
	if float(l["t"]) < float(record["cross_at"]):
		return here
	return _cross(ship, inside, record["twin"], l)


func _cross(ship: Mothership, from_tube: Tube, twin: Tube, l: Dictionary) -> Vector3:
	var t := float(l["t"])
	var fa := from_tube.travel_frame(t)
	var fb := twin.travel_frame(t)
	var ba := Basis(fa["right"] as Vector3, fa["up"] as Vector3, -(fa["fwd"] as Vector3))
	var bb := Basis(fb["right"] as Vector3, fb["up"] as Vector3, -(fb["fwd"] as Vector3))
	var rot := (bb * ba.inverse()).orthonormalized()
	var from := to_local(ship.global_position)
	var to := twin.world(t, float(l["u"]), float(l["v"]))
	var move := Transform3D(rot, to - rot * from)
	ship.global_position = to_global(to)
	ship.carry_across(rot)
	ship.road.tube = twin
	if _riding == from_tube:
		_riding = twin
		ship.cruise = twin.sample(to, ship.lane_clearance(), ship.speed())
		ship.adopt_road_axis(ship.cruise.axis)
	_berth.rebind(from_tube, twin)
	# The metres of the jump are not travelled (fuel, ADR 0086).
	_previous = to
	_enter_world(_wormhole.tubes.has(twin))
	crossed.emit(move)
	return to


## Inside a junction and on the approach to an exit the lane does not penalise
## (`Tube.forgive`, shared with the gate's probe).
func _forgive_the_junction(ship: Mothership, here: Vector3, _clearance: Vector2) -> void:
	if ship.cruise == null or _riding == null:
		return
	_riding.forgive(ship.cruise, here)


## THE FAR LAYER (`FarLayer`): beyond the road's detail radius, planets and stars are
## drawn smaller than perspective makes them, by the same rule the road's far mesh
## uses in its shader. Scaled about their own centres, so nothing moves.
##
## With sectors on, the power is the body's tier (`SectorLayer`): home, next, far, or
## not drawn, tweened across a sector crossing. The ship's position, not the camera's,
## decides the sector, so the sign says where the ship is.
func _compress_the_distance(here: Vector3, delta: float) -> void:
	var camera := get_viewport().get_camera_3d() if get_viewport() != null else null
	if camera == null:
		return
	var eye := to_local(camera.global_position)
	var open := Tuning.flag("exploration/sectors_enabled")
	if _in_wormhole:
		# No sectors, no bodies: the wormhole is its own place. The sector layer keeps
		# the cell the ship left, and names the one it arrives in on the way out.
		RoadMesh.set_sector_globals(false, _sectors, global_position)
		return
	if open:
		_sectors.tick(here, delta)
		_border.name_of = _sectors.here_name()
		for planet in _planets:
			_scale_body(planet, _sectors.scale_for(planet.position, eye))
		for star in _stars:
			_scale_body(star, _sectors.scale_for(star.position, eye))
	else:
		var start := Tuning.num("exploration/road_detail_radius")
		var power := Tuning.num("exploration/far_compress_power")
		for planet in _planets:
			_scale_body(planet, FarLayer.factor(eye.distance_to(planet.position), start, power))
		for star in _stars:
			_scale_body(star, FarLayer.factor(eye.distance_to(star.position), start, power))
	RoadMesh.set_sector_globals(open, _sectors, global_position)


## A body past the last drawn ring is hidden rather than scaled to nothing, which
## would leave it with a basis nothing can invert.
static func _scale_body(body: Node3D, factor: float) -> void:
	body.visible = factor > 0.0001
	body.scale = Vector3.ONE * maxf(factor, 0.0001)


func sectors() -> SectorLayer:
	return _sectors


func in_wormhole() -> bool:
	return _in_wormhole


func border() -> HexRegion:
	return _border


## The exits ahead on the tube being ridden, nearest first, as
## `[[ramp, label, metres_ahead, may_take], …]`. Empty off the road.
##
## A **closed** exit is listed and refused rather than hidden (ADR 0084). Measured to
## where the ramp begins to DIVERGE from the carriageway, which is the turning the
## player is deciding about. An exit already taken stays listed a little past zero.
func upcoming_exits(here: Vector3) -> Array:
	var found: Array = []
	if _riding == null:
		return found
	var t_ship: float = _riding.local(here)["t"]
	var horizon := Tuning.num("exploration/nav_exit_horizon_metres")
	var taking := _berth.taking()
	for j in _riding.junctions:
		if j["kind"] != "exit":
			continue
		var ramp: Tube = j["ramp"]
		# To where the wall OPENS, which is the turning the player is deciding about.
		var metres: float = _riding.path.ahead(t_ship, float(j.get("opens_at", j["t"])),
			_riding.direction)
		if _riding.path.closed and metres > _riding.path.length * 0.5:
			metres -= _riding.path.length
		if metres > horizon:
			continue
		if metres < 0.0 and ramp != taking:
			continue
		found.append([ramp, j["label"], metres, ramp.passable])
	found.sort_custom(func(a: Array, b: Array) -> bool: return (a[2] as float) < (b[2] as float))
	return found


## Take an exit, from the strip. The rail rebind is the berth's and happens when the
## ramp arrives rather than when the button is pressed (ADR 0083).
func take_exit(ramp: Tube) -> void:
	if ramp != null and not ramp.passable:
		return
	_berth.take_exit(null if ramp == _berth.taking() else ramp)


## Which mouths say where they go. Off the road, the ways ON are the choices and they
## are named; on it, the way OFF the road you are riding is (ADR 0096).
func _name_the_ways() -> void:
	for record in _road.ramps:
		var tube: Tube = (record["road"] as Road).tubes[0]
		var entry: Portal = record["entry_portal"]
		var exit: Portal = record["exit_portal"]
		if entry != null:
			entry.set_named(_riding == null)
		if exit != null:
			exit.set_named(_riding == tube)


## The tube the player is riding, or null. For the HUD and for tests.
func riding() -> Tube:
	return _riding


func berth() -> RoadBerth:
	return _berth


func portals() -> Array[Portal]:
	return _road.portals()


## The road in open space: the ramps' twins.
func road() -> RoadNetwork:
	return _road


## The road inside the wormhole: the highways and their ramps.
func wormhole() -> RoadNetwork:
	return _wormhole


func tube_named(n: String) -> Tube:
	var t := _road.tube_named(n)
	return t if t != null else _wormhole.tube_named(n)


## The ramp record whose tube this is, in whichever world, or empty.
func ramp_of(tube: Tube) -> Dictionary:
	var record := _road.ramp_of(tube)
	return record if not record.is_empty() else _wormhole.ramp_of(tube)


func deep_field() -> DeepField:
	return _deep


func spine() -> PackedVector3Array:
	return _spine


## The nearest way on or off the road. For the HUD.
func nearest_portal(point: Vector3) -> Portal:
	var best: Portal = null
	var best_distance := INF
	for portal in portals():
		var distance := point.distance_to(portal.position)
		if distance < best_distance:
			best_distance = distance
			best = portal
	return best


func _heading_of(ship: Mothership) -> Vector3:
	var moving := ship.velocity()
	if moving.length_squared() > 0.01:
		return moving
	return -ship.basis.z


# --- what the scene and the HUD ask it ---------------------------------------

## The boundary of the world the ship is in.
func field() -> BoundaryField:
	return _void if _in_wormhole else _field


func systems() -> Array[SystemDisc]:
	return _discs


func links() -> Array[SystemLink]:
	return _links


func planets() -> Array[Planet]:
	return _planets


func stars() -> Array[Star]:
	return _stars


func approaches() -> Array[ApproachEnvelope]:
	return _approaches


func place_of(point: Vector3) -> String:
	return field().label(point)


func nearest_system(point: Vector3) -> int:
	var best := -1
	var best_distance := INF
	for i in _discs.size():
		var distance := point.distance_to(_discs[i].position)
		if distance < best_distance:
			best_distance = distance
			best = i
	return best


func system_center(index: int) -> Vector3:
	return Vector3.ZERO if index < 0 or index >= _discs.size() \
		else _discs[index].position


func system_name(index: int) -> String:
	return "—" if index < 0 or index >= NAMES.size() else NAMES[index]


## Put a ship in a system: a short way behind the system's first entry mouth, facing
## into it, so the first thing on screen is the way onto the road. One placement
## rule, used by the spawn and by the debug teleport. A system with no entry (none
## on this map) falls back to the middle of the disc facing its aperture.
func place_ship(ship: Node3D, index: int) -> void:
	if ship == null or index < 0 or index >= _discs.size():
		return
	_enter_world(false)
	var disc := _discs[index]
	for record in _road.ramps:
		var portal: Portal = record["entry_portal"]
		if portal == null or String(record["mouth_of"]) != NAMES[index]:
			continue
		var back := Tuning.num("exploration/spawn_behind_mouth_metres")
		ship.global_position = to_global(portal.position - portal.travel * back)
		_face(ship, to_global(portal.position))
		return
	var home := disc.position
	var mouth := disc.aperture_mouth(disc.aperture_count() - 1)
	var out := (mouth - home).normalized()
	ship.global_position = to_global(home - out * disc.radius() * 0.45)
	_face(ship, to_global(mouth))


## Point a placed ship at something. The nose follows the reticle every frame, so a
## bare `look_at` is undone within a second as the ship turns back to where the
## reticle was left; the reticle has to come with it, as the debug drop's does.
func _face(ship: Node3D, target: Vector3) -> void:
	ship.look_at(target, Vector3.UP)
	if ship is Mothership:
		(ship as Mothership).reset_reticle()


## Take the ship off whatever it was on. Being moved kilometres while a lane sample
## from the old road is still attached would arrive as an engine running in open space.
func _lift_off(ship: Mothership) -> void:
	if ship.road == null:
		attach(ship)
	_berth.release(ship)
	_riding = null
	ship.cruise = null
	ship.leave_road()
	ship.reset_reticle()
	if ship.road != null:
		ship.road.tube = null


## THE DEBUG TELEPORT (POC step 7). Debug only: nothing in the game may call it, and
## the HUD says loudly that it was used.
func warp_to_system(ship: Mothership, index: int) -> void:
	if ship == null or index < 0 or index >= _discs.size():
		return
	_lift_off(ship)
	place_ship(ship, index)
	_has_previous = false
	_previous = to_local(ship.global_position)


## THE DEBUG DROP: put the ship on a tube, pointing along it, with the drive running
## if the hull has one. Stands in for flying to a bend or a junction to look at it.
func drop_on_road(ship: Mothership, tube: Tube, t: float) -> void:
	if ship == null or tube == null:
		return
	_lift_off(ship)
	_enter_world(_wormhole.tubes.has(tube))
	var f := tube.travel_frame(tube.path.wrap_t(t))
	ship.global_position = to_global(f["pos"])
	ship.look_at(to_global((f["pos"] as Vector3) + (f["fwd"] as Vector3) * 1000.0), Vector3.UP)
	ship.road.tube = tube
	_previous = f["pos"]
	_has_previous = true
	if not ship.has_cruise_drive():
		return
	_riding = tube
	ship.cruise = tube.sample(f["pos"], ship.lane_clearance())
	ship.adopt_road_axis(ship.cruise.axis)
	ship.reset_reticle()


## The road's teleport spots in both worlds: every mouth and arrival in space, every
## bend, exit and merge in the wormhole.
func spots() -> Array[Dictionary]:
	return _road.spots + _wormhole.spots


func active_approach(point: Vector3) -> ApproachEnvelope:
	for approach in _approaches:
		if approach.state() != ApproachEnvelope.State.CLEAR:
			return approach
	var index := nearest_system(point)
	return null if index < 0 else _approaches[index]


func is_docked() -> bool:
	for approach in _approaches:
		if approach.is_docked():
			return true
	return false


func depart() -> void:
	for approach in _approaches:
		if approach.is_docked():
			approach.depart()


func ramp_sites() -> Array[Vector3]:
	var sites: Array[Vector3] = []
	for portal in _road.portals():
		sites.append(portal.position)
	return sites


func warning() -> float:
	return _warning


func speed_scale() -> float:
	return _speed_scale


func outbound() -> float:
	return _outbound


func seconds_outside() -> float:
	return _seconds_outside


func marker_count() -> int:
	var total := 0
	for disc in _discs:
		total += disc.marker_count()
	for link in _links:
		total += link.marker_count()
	return total
