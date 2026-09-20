class_name SectorLayer
extends RefCounted
## Which sector the player is in, and how a body elsewhere is drawn because of it
## (`docs/SECTOR_PROTOTYPE.md`, prototype 1). A body's tier is the ring distance from
## the player's sector to the body's: home, next, far, or not drawn. Each tier has its
## own compression curve on top of the far layer's, so the size a body is drawn at
## says which tier it is in.
##
## A body never changes sector, so the tiers only change when the player crosses a
## sector edge. That moment is made an event rather than a snap: every body's scale
## tweens from its old tier to its new one over `sector_crossing_seconds`, while the
## HUD shows the sector's name for `sector_banner_seconds`. The road-trip sign.
##
## Not a node: the map ticks it and asks it for scales, so it can be tested without a
## scene tree. It reads tuning, which is why it is not in `scripts/lib`.

## The sector the player is in, and the one they came from while the tween runs.
var cell: Vector2i = Vector2i.ZERO
var previous: Vector2i = Vector2i.ZERO
## 0 to 1 across the crossing tween. 1 means settled.
var blend: float = 1.0
## Seconds left on the banner. Zero means no sign showing.
var banner_left: float = 0.0
## How many edges have been crossed since the layer was made. For the HUD and tests.
var crossings: int = 0
## Names for cells that hold a body, keyed by cell. Other cells are named by the grid.
var names: Dictionary = {}

var _radius: float = 1.0
var _started: bool = false


func retune() -> void:
	_radius = maxf(Tuning.num("exploration/sector_radius"), 1.0)


## Put the player at a point. The first call settles there without an event; every
## later call that lands in a different cell starts a crossing. Returns whether one
## started.
func tick(eye: Vector3, delta: float) -> bool:
	retune()
	var now := HexGrid.cell_of(eye, _radius)
	var crossed := false
	if not _started:
		_started = true
		cell = now
		previous = now
	elif now != cell:
		previous = cell
		cell = now
		blend = 0.0
		banner_left = Tuning.num("exploration/sector_banner_seconds")
		crossings += 1
		crossed = true
	var tween := Tuning.num("exploration/sector_crossing_seconds")
	blend = 1.0 if tween <= 0.0 else minf(blend + delta / tween, 1.0)
	banner_left = maxf(banner_left - delta, 0.0)
	return crossed


## Which tier a point is in, relative to a reference cell: 0 home, 1 next, 2 far, and
## anything past `sector_far_rings` is not drawn.
func tier_of(point: Vector3, from: Vector2i) -> int:
	return HexGrid.ring_distance(HexGrid.cell_of(point, _radius), from)


## The far-layer power a tier compresses with. Home is the far layer's own
## `far_compress_power`, so a flag flipped off changes nothing about the home sector.
static func power_for(tier: int) -> float:
	match tier:
		0:
			return Tuning.num("exploration/far_compress_power")
		1:
			return Tuning.num("exploration/sector_next_power")
		_:
			return Tuning.num("exploration/sector_far_power")


## What to scale a body at `point` by, seen from `eye`, this frame: the far layer's
## factor for its tier, and zero past the last drawn ring. During a crossing the old
## and new tiers' scales are blended, which is the tween.
func scale_for(point: Vector3, eye: Vector3) -> float:
	var start := Tuning.num("exploration/road_detail_radius")
	var rings := Tuning.integer("exploration/sector_far_rings")
	var distance := eye.distance_to(point)
	var now := _scale(distance, start, tier_of(point, cell), rings)
	if blend >= 1.0:
		return now
	var before := _scale(distance, start, tier_of(point, previous), rings)
	return lerpf(before, now, blend)


static func _scale(distance: float, start: float, tier: int, rings: int) -> float:
	if tier > rings:
		return 0.0
	return FarLayer.factor(distance, start, power_for(tier))


## The name of a cell: a body's system if one is centred in it, else the grid's.
func name_of(which: Vector2i) -> String:
	return String(names.get(which, HexGrid.label(which)))


## The name of the sector the player is in.
func here_name() -> String:
	return name_of(cell)


## How visible the sign is, 0 to 1: a quick fade in, a hold, a slower fade out.
func banner_alpha() -> float:
	var total := Tuning.num("exploration/sector_banner_seconds")
	if banner_left <= 0.0 or total <= 0.0:
		return 0.0
	var shown := total - banner_left
	var fade_in := minf(0.3, total * 0.25)
	var fade_out := minf(0.8, total * 0.4)
	if shown < fade_in:
		return shown / fade_in
	if banner_left < fade_out:
		return banner_left / fade_out
	return 1.0


## Register the bodies' cells so their sectors carry their names.
func name_cells(centres: Dictionary) -> void:
	retune()
	names.clear()
	for system_name: String in centres:
		var at: Vector3 = centres[system_name]
		names[HexGrid.cell_of(at, _radius)] = system_name


func radius() -> float:
	return _radius
