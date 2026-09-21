extends Node
## Capture harness: parks the camera outside the road, above and beside the first
## northbound junction, looking along it — so `make shot` photographs both
## carriageways, the median, the ramps and the traffic without a human flying there.
##
## From OUTSIDE on purpose. Inside a tube the walls fill the frame, and the thing a
## frame can settle is whether what was built is the shape that was meant: two tubes
## side by side, ramps peeling off to the right, the gore between. Flying it is how
## the inside gets judged, and that is the human's job.
##
## The ship is hidden rather than moved out of shot: the chase camera rides behind
## it, so it is the tripod. Lives in tools/, because the game should not carry a code
## path that exists for screenshots.

## Where the tripod stands, relative to the start of the first exit, in metres along
## the northbound road's own axes: back along it, out to its right, and up.
const BACK_METRES: float = 300.0
const RIGHT_METRES: float = 1300.0
const UP_METRES: float = 1500.0
## What it looks at: this far down the road past the exit, and this far across
## toward the median, so both carriageways are in frame.
const AHEAD_METRES: float = 250.0
const TOWARD_MEDIAN_METRES: float = -150.0

var _scene: HighwayScene


func _ready() -> void:
	_scene = (load("res://scenes/highway.tscn") as PackedScene).instantiate() \
		as HighwayScene
	add_child(_scene)


func _process(_delta: float) -> void:
	if _scene == null or _scene.ship() == null or _scene.road() == null:
		return
	var road := _scene.road()
	var north := road.route_named("northbound")
	var exit_ramp := road.route_named("northbound off-ramp 1")
	if north == null or exit_ramp == null:
		return
	var at := north.sample(exit_ramp.joins_at)
	var ship := _scene.ship()
	ship.set_process(false)
	ship.visible = false
	var stand := at.origin - at.basis.z * -BACK_METRES + at.basis.x * RIGHT_METRES \
		+ Vector3.UP * UP_METRES
	var target := north.sample(exit_ramp.joins_at + AHEAD_METRES).origin \
		- at.basis.x * TOWARD_MEDIAN_METRES
	ship.position = stand
	ship.basis = FlightGeometry.basis_from_forward((target - stand).normalized())
