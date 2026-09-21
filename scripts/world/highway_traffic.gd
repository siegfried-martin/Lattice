class_name HighwayTraffic
extends Node3D
## Other ships on the road: both carriageways, in lanes, at their own speeds.
##
## **Build order step 2**, with the brief's traffic deferral lifted at the human's
## direction (2026-09-20). This is the least traffic that makes a road read as a
## road: something to overtake, something that overtakes you, and the other side's
## ships going the other way across the median.
##
## **Lanes, not drivers.** Each carriageway has `highway/traffic_lanes` lanes across
## it. The right-hand lane is the slowest and the left the fastest, spread by
## `exploration/road_traffic_speed_spread` around cruise — keep right except to
## overtake, which is how a road already works. Every ship in a lane goes at that
## lane's speed, so no two ever close on each other and there is no avoidance to
## write. The player is the only thing on the road that changes lanes.
##
## Traffic does not know the player exists. It does not slow, swerve, or stop for
## them, and it can never stop the player either: touching a ship glances off it
## through the same decaying push the walls use ("collisions glance off, no
## collision damage", `EXPLORATION_DESIGN.md`). Nothing here is an NPC deciding
## anything about the player's trip — no interdiction (ADR 0014).
##
## A ship that reaches the end of its carriageway is recycled to the start. Visible
## at the dead ends, and the dead ends are not what this step is about.

## After the ship has moved, for the same reason as the road. Infrastructure.
const AFTER_THE_SHIP: int = 10
## Where the traffic is scattered from. Fixed so a session is repeatable: "the ship
## in the slow lane just past the first junction" has to be the same ship twice.
const SEED: int = 20260920

var ship: Mothership
var road: HighwayRoad

## One entry per ship: `route` (int), `along` (m), `speed` (m/s), `across` (m to the
## right of the lane centre), `lane` (0 is the right-hand lane), `node`.
var _cars: Array[Dictionary] = []
var _touched: Dictionary = {}
var _seconds_since_contact: float = INF
var _cars_root: Node3D


func _ready() -> void:
	process_priority = AFTER_THE_SHIP
	_cars_root = Node3D.new()
	_cars_root.name = "Cars"
	add_child(_cars_root)
	rebuild()
	# Connected after the road's own rebuild, so traffic is laid onto the new road
	# rather than the old one.
	Tuning.reloaded.connect(rebuild)


func rebuild() -> void:
	for car in _cars:
		(car["node"] as Node).free()
	_cars.clear()
	_touched.clear()
	if road == null:
		return

	var rng := RandomNumberGenerator.new()
	rng.seed = SEED
	var cruise := Tuning.num("exploration/cruise_speed")
	var spread := clampf(Tuning.num("exploration/road_traffic_speed_spread"), 0.0, 0.95)
	var per_km := maxf(Tuning.num("exploration/road_traffic_per_km"), 0.0)
	var lanes := maxi(Tuning.integer("highway/traffic_lanes"), 1)
	var usable := maxf(Tuning.num("highway/tube_width") * 0.5 - traffic_radius(), 0.0)
	var hull := load("res://assets/models/carrier.obj") as Mesh
	var paint := StandardMaterial3D.new()
	paint.albedo_color = Tuning.color("highway/traffic_color")
	paint.emission_enabled = true
	paint.emission = Tuning.color("highway/traffic_color")
	paint.emission_energy_multiplier = Tuning.num("highway/traffic_emission")

	var routes := road.shell().routes
	for route_id in routes.size():
		var route := routes[route_id]
		if route.kind != HighwayRoute.Kind.CARRIAGEWAY:
			continue
		var count := roundi(route.length() / 1000.0 * per_km)
		for lane in lanes:
			var in_lane := count / lanes + (1 if lane < count % lanes else 0)
			if in_lane <= 0:
				continue
			# 0 is the right-hand lane: slowest, and furthest to the right.
			var share := 0.0 if lanes == 1 else float(lane) / float(lanes - 1)
			var across := 0.0 if lanes == 1 else lerpf(usable, -usable, share)
			var speed := cruise * (1.0 + spread * (0.0 if lanes == 1 \
				else lerpf(-1.0, 1.0, share)))
			var spacing := route.length() / float(in_lane)
			for k in in_lane:
				var node := MeshInstance3D.new()
				node.name = "Car%d" % _cars.size()
				node.mesh = hull
				node.material_override = paint
				_cars_root.add_child(node)
				# Scattered within each ship's own slot, never across into the next:
				# a lane at one speed keeps whatever spacing it starts with forever.
				_cars.append({"route": route_id, "lane": lane, "across": across,
					"speed": speed, "node": node,
					"along": (float(k) + rng.randf_range(0.15, 0.85)) * spacing})
	_place_all()


func _process(delta: float) -> void:
	_seconds_since_contact += delta
	if road == null or delta <= 0.0:
		return
	var routes := road.shell().routes
	for car in _cars:
		var length := routes[int(car["route"])].length()
		var along := float(car["along"]) + float(car["speed"]) * delta
		car["along"] = fmod(along, maxf(length, 0.001))
	_place_all()
	_glance(delta)


func _place_all() -> void:
	var routes := road.shell().routes
	for car in _cars:
		var node := car["node"] as MeshInstance3D
		var frame := routes[int(car["route"])].sample(float(car["along"]))
		frame.origin += frame.basis.x * float(car["across"])
		node.transform = frame.scaled_local(
			Vector3.ONE * Tuning.num("highway/traffic_hull_scale"))


## The player touching a ship: pushed clear and knocked off it, the same way a wall
## does. The ship does not notice.
func _glance(_delta: float) -> void:
	if ship == null or not is_instance_valid(ship):
		return
	var reach := road.wall_clearance() + traffic_radius()
	var here := road.ship_in_road()
	var moving := road.global_transform.basis.inverse() * ship.velocity()
	var decay := Tuning.num("highway/bounce_decay_seconds")
	var limit := ship.manual_max_speed() \
		* maxf(Tuning.num("highway/bounce_max_speed_fraction"), 0.0)
	var restitution := clampf(Tuning.num("highway/bounce_restitution"), 0.0, 1.0)
	var still: Dictionary = {}
	var routes := road.shell().routes
	for i in _cars.size():
		var car := _cars[i]
		var at := car_position(i)
		var apart := here - at
		var gap := apart.length()
		if gap >= reach:
			continue
		still[i] = true
		var n := Vector3.UP if gap <= 0.001 else apart / gap
		here += n * (reach - gap)
		ship.global_position = road.global_transform * here
		if _touched.has(i):
			continue
		var frame := routes[int(car["route"])].sample(float(car["along"]))
		var closing := (moving - (-frame.basis.z) * float(car["speed"])).dot(n)
		if closing < 0.0:
			ship.push(road.global_transform.basis * n * (-(1.0 + restitution) * closing),
				decay, limit)
			_seconds_since_contact = 0.0
	_touched = still


## How far a traffic ship's hull reaches, from the same clearance the walls hold the
## player's hull at — so a traffic ship is held off a wall exactly as the player is.
func traffic_radius() -> float:
	return Tuning.num("highway/wall_clearance") * Tuning.num("highway/traffic_hull_scale")


func car_position(index: int) -> Vector3:
	var car := _cars[index]
	var frame := road.shell().routes[int(car["route"])].sample(float(car["along"]))
	return frame.origin + frame.basis.x * float(car["across"])


func count() -> int:
	return _cars.size()


func cars() -> Array[Dictionary]:
	return _cars


## The nearest ship to a point — `{"distance": float, "speed": float, "route":
## String, "lane": int}`, distance INF when the road is empty.
func nearest(point: Vector3) -> Dictionary:
	var best := {"distance": INF, "speed": 0.0, "route": "—", "lane": -1}
	for i in _cars.size():
		var d := point.distance_to(car_position(i))
		if d < float(best["distance"]):
			best = {"distance": d, "speed": float(_cars[i]["speed"]),
				"route": road.shell().routes[int(_cars[i]["route"])].name,
				"lane": int(_cars[i]["lane"])}
	return best


func seconds_since_contact() -> float:
	return _seconds_since_contact
