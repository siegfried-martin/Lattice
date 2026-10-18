# Checks that the simulation binding loads and runs (ARC-4, MS-1). Run by `make check`:
#   godot --headless --path . --script res://tests/sim_binding.gd
extends SceneTree


func _initialize() -> void:
	quit(0 if _run() else 1)


func _run() -> bool:
	if not ClassDB.class_exists("LatticeSim"):
		printerr("sim binding: LatticeSim isn't registered (run make gdext)")
		return false
	var sim: RefCounted = ClassDB.instantiate("LatticeSim")
	if not sim.start(42, "[debug]\npings = 10\n"):
		return false
	# One real minute is one game day at the default clock (WLD-15), in uneven frames.
	for i in 3600:
		sim.advance(1.0 / 60.0)
	var snap: Dictionary = sim.snapshot()
	if snap.day != 1 or snap.seed != 42 or snap.events_processed < 1:
		printerr("sim binding: unexpected snapshot ", snap)
		return false
	print("sim binding: ok ", snap)
	return true
