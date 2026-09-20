class_name HexGrid
extends RefCounted
## A tiling of the map's horizontal plane into pointy-top hexagonal sectors
## (`docs/SECTOR_PROTOTYPE.md`). Pure — no scene tree, no tuning, no disk.
##
## Axial coordinates `(q, r)`; `radius` is centre to vertex. The plane is x/z, y is
## ignored throughout, because a sector is a column of the whole playable height.
## Ring distance is the number of sector edges between two cells, which is the number
## the far layer tiers on: 0 is the player's own sector, 1 a neighbour, 2 the ring
## past that.

const SQRT3 := 1.7320508075688772


## The cell containing a point.
static func cell_of(point: Vector3, radius: float) -> Vector2i:
	if radius <= 0.0:
		return Vector2i.ZERO
	var q := (SQRT3 / 3.0 * point.x - point.z / 3.0) / radius
	var r := (2.0 / 3.0 * point.z) / radius
	return _round(q, r)


## Where a cell's centre sits, on y = 0.
static func center(cell: Vector2i, radius: float) -> Vector3:
	return Vector3(
		radius * (SQRT3 * cell.x + SQRT3 * 0.5 * cell.y),
		0.0,
		radius * 1.5 * cell.y)


## How many cell edges lie between two cells. Zero for the same cell.
static func ring_distance(a: Vector2i, b: Vector2i) -> int:
	var dq := a.x - b.x
	var dr := a.y - b.y
	return (absi(dq) + absi(dr) + absi(dq + dr)) / 2


## Signed distance from a point to the boundary of one cell, in the plane: negative
## inside, positive outside, measured to the nearest edge. A pointy-top hex has its
## three edge normals at 0°, 60° and 120° from +x.
static func edge_distance(point: Vector3, cell: Vector2i, radius: float) -> float:
	var local := point - center(cell, radius)
	var apothem := radius * SQRT3 * 0.5
	var worst := -INF
	for k in 3:
		var angle := deg_to_rad(60.0 * k)
		var normal := Vector3(cos(angle), 0.0, sin(angle))
		worst = maxf(worst, absf(local.dot(normal)) - apothem)
	return worst


## A short name for a cell that has no body to be named after.
static func label(cell: Vector2i) -> String:
	return "SECTOR %d,%d" % [cell.x, cell.y]


static func _round(q: float, r: float) -> Vector2i:
	var s := -q - r
	var rq := roundf(q)
	var rr := roundf(r)
	var rs := roundf(s)
	var dq := absf(rq - q)
	var dr := absf(rr - r)
	var ds := absf(rs - s)
	if dq > dr and dq > ds:
		rq = -rr - rs
	elif dr > ds:
		rr = -rq - rs
	return Vector2i(int(rq), int(rr))
