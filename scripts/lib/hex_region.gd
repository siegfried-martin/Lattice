class_name HexRegion
extends BoundaryRegion
## The outer border of an open world (`docs/SECTOR_PROTOTYPE.md`): one hexagonal
## prism around the whole map, with the same hard ceiling and floor a system disc has.
## Inside it, space is open; there are no discs, corridors or funnels. It is one
## region in the field, so ADR 0063's union of regions still holds, trivially.
##
## The hexagon is pointy-top like the sector grid it encloses, so its six walls are
## three pairs of planes and a corner falls out of the arithmetic the same way a
## disc's ceiling-and-rim corner does.

var center: Vector3 = Vector3.ZERO
## Centre to vertex.
var circumradius: float = 0.0
var ceiling: float = 0.0
var floor_depth: float = 0.0
## What the HUD prints for a point inside. The map keeps it set to the name of the
## sector the ship is in, since a region's label takes no point.
var name_of: String = "open space"


func label() -> String:
	return name_of


func constraints(point: Vector3) -> Array[BoundaryConstraint]:
	var local := point - center
	var list: Array[BoundaryConstraint] = []
	list.append(BoundaryConstraint.new(local.y - ceiling, Vector3.UP))
	list.append(BoundaryConstraint.new(-floor_depth - local.y, Vector3.DOWN))
	var apothem := circumradius * HexGrid.SQRT3 * 0.5
	for k in 3:
		var angle := deg_to_rad(60.0 * k)
		var normal := Vector3(cos(angle), 0.0, sin(angle))
		var along := local.dot(normal)
		list.append(BoundaryConstraint.new(along - apothem, normal))
		list.append(BoundaryConstraint.new(-along - apothem, -normal))
	return list
