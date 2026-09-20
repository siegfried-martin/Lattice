extends Node
## Fly the drunk pilot on ONE tube, at the taxi's hull scale and at 0.5, and name the
## triangle a failing step crossed. The gate's road suite says which tube failed and
## where; this says whose face it was, in a minute rather than six:
##
##   REPRO_TUBE="A-377B L SYSTEM A out" godot --headless --scene res://tools/tests/repro_drunk.tscn
##
## `REPRO_FROM` / `REPRO_TO` (as "x,y,z", from the suite's "moving A -> B" in its
## failure line) test that one step against every road's triangles and print the
## road and the triangle it crossed.


func _expect(condition: bool, what: String, detail: String) -> void:
	print(("ok   " if condition else "FAIL ") + what + ("" if condition else " — " + detail))


func _ready() -> void:
	var suite := RoadSuite.new(self)
	var faces := await suite._prepare()
	var from := _vec(OS.get_environment("REPRO_FROM"))
	var to := _vec(OS.get_environment("REPRO_TO"))
	if from != to:
		for road_name: String in faces:
			var tris: PackedVector3Array = faces[road_name]
			var i := 0
			while i + 2 < tris.size():
				var hit: Variant = Geometry3D.segment_intersects_triangle(from, to,
					tris[i], tris[i + 1], tris[i + 2])
				if hit != null:
					print("[face] %s at %s: %s %s %s" % [road_name, hit, tris[i], tris[i + 1], tris[i + 2]])
				i += 3
	var hull := load("res://assets/models/carrier.obj") as Mesh
	var tube: Tube = suite.tube_named(OS.get_environment("REPRO_TUBE"))
	if tube == null:
		push_error("no tube named '%s'" % OS.get_environment("REPRO_TUBE"))
	else:
		for scale: float in [Tuning.num("ship/hull_scale"), 0.5]:
			suite._probe.half = hull.get_aabb().size * scale * 0.5
			print("--- hull scale %.2f half %s" % [scale, suite._probe.half])
			suite._drunk_on("drunk on %s" % tube.name, tube,
				float(tube.road.get("draw_from")) + 100.0 if tube.is_ramp() else 100.0, 30.0)
	suite._probe.collider.setup([], [])
	for network in suite._networks:
		network._release()
	get_tree().quit(0)


static func _vec(text: String) -> Vector3:
	var parts := text.split(",")
	if parts.size() != 3:
		return Vector3.ZERO
	return Vector3(float(parts[0]), float(parts[1]), float(parts[2]))
