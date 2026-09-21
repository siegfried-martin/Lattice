class_name HighwayRoute
extends RefCounted
## One tube, in travel order: a carriageway, or a ramp. The tiles it is made of and
## the distance down it.
##
## The shell collides tiles and does not care which tube they are in; this is what
## gives a tile an "ahead" — for traffic, for the HUD's next-exit, and for the camera
## to know which way the road runs.
##
## Pure: no scene tree, no tuning, no disk.

enum Kind { CARRIAGEWAY, OFF_RAMP, ON_RAMP }

var name: String = ""
var kind: Kind = Kind.CARRIAGEWAY
## For a ramp, the carriageway it leaves or joins, and the distance down that
## carriageway where it does. -1 on a carriageway.
var carriageway: int = -1
var joins_at: float = 0.0
var sections: Array[HighwaySection] = []

var _cumulative: PackedFloat32Array = PackedFloat32Array()


## Call once the tiles are in. Distances are summed tile by tile, so the inside lane
## of a bend is genuinely shorter than the outside — which is what a divided road is.
func finish() -> void:
	_cumulative = PackedFloat32Array([0.0])
	var run := 0.0
	for section in sections:
		run += section.length()
		_cumulative.append(run)


func length() -> float:
	return 0.0 if _cumulative.is_empty() else _cumulative[_cumulative.size() - 1]


## How far down the route this tile starts.
func distance_to_section(index: int) -> float:
	return 0.0 if index < 0 or index >= _cumulative.size() else _cumulative[index]


## How far down the route a point is, given the tile it is in and how far through.
func distance_at(index: int, t: float) -> float:
	if index < 0 or index >= sections.size():
		return 0.0
	return _cumulative[index] + sections[index].length() * t


## The frame at a distance down the route, clamped to its ends.
func sample(distance: float) -> Transform3D:
	if sections.is_empty():
		return Transform3D.IDENTITY
	var at := clampf(distance, 0.0, length())
	var lo := 0
	var hi := sections.size() - 1
	while lo < hi:
		var mid := (lo + hi + 1) / 2
		if _cumulative[mid] <= at:
			lo = mid
		else:
			hi = mid - 1
	var section := sections[lo]
	var span := maxf(section.length(), 0.001)
	return section.frame_at(clampf((at - _cumulative[lo]) / span, 0.0, 1.0))


func half_width_at_distance(distance: float) -> float:
	if sections.is_empty():
		return 0.0
	for i in sections.size():
		if _cumulative[i + 1] >= distance:
			var span := maxf(sections[i].length(), 0.001)
			return sections[i].half_width_at(clampf(
				(distance - _cumulative[i]) / span, 0.0, 1.0))
	return sections[sections.size() - 1].end_half_width
