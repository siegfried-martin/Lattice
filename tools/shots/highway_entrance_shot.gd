extends Node
## Capture harness: parks the ship inside the first northbound entrance, at the tile
## where it first touches the road, looking ahead and a little left toward it — the
## spot of the human's 2026-09-21 screenshot, where a wall stood between two tubes
## that were touching. The frame should show one open space: no rib, no wall on the
## ramp's left from here on.
##
## Lives in tools/, because the game should not carry a code path that exists for
## screenshots.

const START_RAMP: String = "northbound on-ramp 1"
## How far to look left of straight ahead, toward the road. A view along the ramp's
## own axis would show its far end and nothing about the side it opens onto.
const LOOK_LEFT_DEG: float = 12.0

var _scene: HighwayScene


func _ready() -> void:
	_scene = (load("res://scenes/highway.tscn") as PackedScene).instantiate() \
		as HighwayScene
	add_child(_scene)


func _process(_delta: float) -> void:
	if _scene == null or _scene.ship() == null or _scene.road() == null:
		return
	var ramp := _scene.road().route_named(START_RAMP)
	if ramp == null:
		return
	# The first tile whose left wall is open is where the ramp starts touching.
	var touch := ramp.sections[0]
	for section in ramp.sections:
		if section.open & HighwaySection.Wall.LEFT:
			touch = section
			break
	var ship := _scene.ship()
	ship.set_process(false)
	var at := touch.start
	ship.transform = Transform3D(
		at.basis.rotated(at.basis.y, deg_to_rad(LOOK_LEFT_DEG)),
		at.origin + at.basis.z * 60.0)
