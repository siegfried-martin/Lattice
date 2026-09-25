extends Node
## Dev aid: `godot --path . -- --tour` flies a scripted route and saves screenshots to
## user://tour/, then quits. Not used in normal play.

var main: Node


func _ready() -> void:
	# Keep running when the compositor throttles a hidden window.
	DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
	Engine.max_fps = 60
	DirAccess.make_dir_recursive_absolute("user://tour")
	_run()


func _shot(name: String) -> void:
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png("user://tour/%s.png" % name)
	print("tour: saved %s  mode=%d docked=%s pos=%s speed=%.0f t=%.1f" % [name, main.mode, main.docked, main.ship.position.snappedf(1.0), main.flight.velocity.length(), main.elapsed])


func _wait(t: float) -> void:
	await get_tree().create_timer(t).timeout


func _wait_until(cond: Callable, limit: float) -> bool:
	var t0: float = main.elapsed
	var last := -1
	while not cond.call() and main.elapsed - t0 < limit:
		await _wait(0.1)
		if int(main.elapsed) / 20 != last:
			last = int(main.elapsed) / 20
			print("tour: .. t=%.1f fps=%d mode=%d pos=%s" % [main.elapsed, Engine.get_frames_per_second(), main.mode, main.ship.position.snappedf(1.0)])
	return cond.call()


## Pitch toward a target angle (degrees) with fake mouse input; on the highway, also yaw to
## follow the road.
func _hold_pitch(target_deg: float, thrust := 1.0) -> void:
	var err: float = target_deg - rad_to_deg(main.flight.pitch)
	var mx := 0.0
	if main.mode == 1:
		var f: Vector3 = Galaxy.carr_fwd(main.hw_road, main.hw_d, main.hw_u)
		var yerr := rad_to_deg(wrapf(atan2(-f.x, -f.z) - main.flight.yaw, -PI, PI))
		mx = -clampf(yerr, -6.0, 6.0)
	main.input_override = {"thrust": thrust, "mouse": Vector2(mx, -clampf(err, -6.0, 6.0))}


func _run() -> void:
	if "--hop-only" in OS.get_cmdline_user_args():
		await _wait(1.0)
		await _hop_part()
		get_tree().quit()
		return
	main.input_override = {"thrust": 1.0}
	await _wait(1.5)
	await _shot("01_space_start")
	await _wait_until(func(): return main.mode == 1, 60.0)
	await _wait(1.0)
	await _shot("02_highway_free")
	main.input_override = {"thrust": 0.0}
	main.toggle_dock()
	await _wait(3.0)
	await _shot("03_docked")
	main.drive.change_lane(-1)
	await _wait(4.0)
	await _shot("04_docked_left_lane")

	main.toggle_dock()
	var t0: float = main.elapsed
	while main.elapsed - t0 < 5.0:
		_hold_pitch(35.0)
		await _wait(0.05)
	await _shot("05_highway_ceiling")
	# Docking works from anywhere under the ceiling.
	main.input_override = {"thrust": 0.0}
	main.toggle_dock()
	print("tour: re-dock docked=%s msg=%s" % [main.docked, main.msg])

	# Ride to the end of HWY 2 and keep left at the junction.
	main.drive.change_lane(-1)
	main.drive.change_lane(-1)
	if await _wait_until(func(): return not main.drive.on_road and main.drive.link.get("kind", "") == "turn", 120.0):
		await _wait(1.5)
		await _shot("06_junction_turn")
	# Then keep right for the first exit on HWY 1.
	await _wait_until(func(): return main.drive.on_road, 30.0)
	await _wait(9.0)
	for i in 3:
		main.drive.change_lane(1)
	await _wait_until(func(): return main.mode == 0, 150.0)
	await _wait(1.0)
	await _shot("07_space_after_exit")

	await _hop_part()
	get_tree().quit()


func _hop_part() -> void:
	# Take a hop lane (jump to one if this sector has none).
	var hop_gate := {}
	for g in Galaxy.gates:
		if g.kind == "hop_in" and (hop_gate.is_empty() or g.sector == main.space_world.current):
			hop_gate = g
	main.space_world.set_current_sector(hop_gate.sector)
	var xf: Transform3D = main.space_world.gate_local(hop_gate)
	main.flight.place(xf.origin + xf.basis.z * 1300.0, -xf.basis.z, 35.0)
	main.input_override = {"thrust": 1.0}
	await _wait(2.0)
	await _shot("08_hop_approach")
	await _wait_until(func(): return main.mode == 2, 60.0)
	await _wait(3.5)
	await _shot("09_hop_ride")
	await _wait_until(func(): return main.mode == 0, 120.0)
	await _wait(1.0)
	await _shot("10_hop_arrived")
