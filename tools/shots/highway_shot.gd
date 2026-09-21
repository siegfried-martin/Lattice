extends Node
## Capture harness: parks the ship outside the mouth of the road, off to one side
## and above it, looking down its length — so `make shot` photographs the tube's
## shape without a human flying it.
##
## From OUTSIDE on purpose. Inside a tube, the walls fill the frame and a picture
## of them is a picture of a colour; the one thing a frame can settle is whether the
## thing built is a square tube made of tiles, and that is only visible from off the
## end of it. Flying it is how the inside gets judged, and that is the human's job.
##
## Lives in tools/ rather than in the scene: the game should not carry a code path
## that exists for screenshots.

## Where to sit, as a share of the tube's own half-extents, so the frame stays the
## same picture when the road is retuned to a different size.
const ACROSS_SHARE: float = -3.0
const UP_SHARE: float = 2.6
## How far back from the mouth, in tiles.
const TILES_BACK: float = 1.2

var _scene: HighwayScene


func _ready() -> void:
	_scene = (load("res://scenes/highway.tscn") as PackedScene).instantiate() \
		as HighwayScene
	add_child(_scene)


func _process(_delta: float) -> void:
	if _scene == null or _scene.ship() == null or _scene.road() == null:
		return
	# Parked rather than flown: at 15.5 m/s, flying anywhere honestly is most of a
	# minute of capture for one frame.
	var tile := Tuning.num("highway/section_length")
	var ship := _scene.ship()
	ship.set_process(false)
	ship.position = Vector3(
		Tuning.num("highway/tube_width") * 0.5 * ACROSS_SHARE,
		Tuning.num("highway/tube_height") * 0.5 * UP_SHARE,
		tile * TILES_BACK)
	# Aimed ALONG the road rather than at it. The camera rides behind the ship, so a
	# ship pointed at the tube puts its own hull between the lens and the thing
	# being photographed; flying parallel leaves the road running away down one
	# side of the frame, which is the picture that shows what it is.
	ship.basis = Basis.IDENTITY
