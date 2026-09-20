extends Node
## `make roads`: build both worlds' roads from `data/routes.json` headless and print
## every problem the networks found, plus how long the whole structure takes to
## build. For authoring the map: save the file, run this, read the list, fix the one
## it names. The wormhole's legs are printed in seconds, which is what they are tuned
## in (`docs/WORMHOLE_PROTOTYPE.md`).

func _ready() -> void:
	var t0 := Time.get_ticks_msec()
	var space := RoadNetwork.new()
	space.name = "Road"
	add_child(space)
	var wormhole := RoadNetwork.new()
	wormhole.name = "Wormhole"
	add_child(wormhole)
	RoadNetwork.build_worlds(space, wormhole, Routes.data(), Routes.system_positions())
	var t1 := Time.get_ticks_msec()
	print("── roads: wormhole %d roads, %d tubes, %d ramps, %d chunks; space %d ramps, %d chunks; laid out in %d ms ──" % [
		wormhole.roads.size(), wormhole.tubes.size(), wormhole.ramps.size(),
		wormhole.chunk_count(), space.ramps.size(), space.chunk_count(), t1 - t0])
	var floor_r := Tuning.num("exploration/cruise_speed") / (clampf(Tuning.num("exploration/road_turn_share"), 0.05, 1.0)
		* deg_to_rad(Tuning.num("exploration/cruise_turn_rate_deg_per_sec")))
	for leg: Dictionary in WormholeLayout.lay(Routes.data(), Routes.system_positions())["legs"]:
		print("  %-8s %-9s > %-9s %6.1f km in the world, %5.0f m (%4.1f s) in the wormhole" % [
			leg["highway"], leg["from"], leg["to"], float(leg["world"]) / 1000.0,
			leg["metres"], leg["seconds"]])
	for network: RoadNetwork in [wormhole, space]:
		print("  in the %s:" % ("wormhole" if network == wormhole else "open space"))
		for road in network.roads:
			print("  %-40s %6.1f km  min bend %5.0f m  pitch %.1f deg" % [road.name,
				road.path.length / 1000.0, road.path.min_radius(), road.path.max_pitch_deg()])
			# Name the corner that is too tight, so the author knows which leg to lengthen.
			if road.kind == "highway" and road.path.min_radius() < floor_r:
				for c in road.path.corners:
					if float(c["radius"]) < floor_r:
						print("      corner at %s turns %.0f deg with r=%.0f (floor %.0f)" % [
							c["pos"], c["angle_deg"], c["radius"], floor_r])
	if OS.get_environment("ROAD_REPORT_MESH") == "1":
		var tris := 0
		for network: RoadNetwork in [wormhole, space]:
			var faces := network.build_all_now()
			for name: String in faces:
				tris += (faces[name] as PackedVector3Array).size() / 3
		print("  full mesh: %d triangles in %d ms" % [tris, Time.get_ticks_msec() - t1])
	var problems := wormhole.problems + space.problems
	if problems.is_empty():
		print("  no problems")
	for problem in problems:
		print("  PROBLEM  " + problem)
	var status := 0 if problems.is_empty() else 1
	space.free()
	wormhole.free()
	get_tree().quit(status)
