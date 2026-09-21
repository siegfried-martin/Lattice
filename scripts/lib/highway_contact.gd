class_name HighwayContact
extends RefCounted
## A wall, touching the ship: which way it pushes, and how far.
##
## Deliberately carries no velocity and no decision about what to do — a contact is
## a measurement. What a bounce *does* with it is a feel question and lives in
## `HighwayRoad` against tuned values.

## Unit, pointing the way the ship must move to be clear of the wall.
var normal: Vector3 = Vector3.UP
## Metres the ship has to move along `normal` to be exactly clear. Greater than the
## ship's radius means it had passed through the wall.
var depth: float = 0.0
## Which wall it was, for the HUD: "left", "ceiling", and so on.
var wall_name: String = "wall"
## Which tube the wall belongs to: "northbound", "northbound off-ramp 2".
var tube: String = ""


static func make(push: Vector3, how_deep: float, called: String,
		in_tube: String) -> HighwayContact:
	var contact := HighwayContact.new()
	contact.normal = push
	contact.depth = how_deep
	contact.wall_name = called
	contact.tube = in_tube
	return contact


## One wall, however many tiles it runs across. A bounce is the frame a wall starts
## touching, and a wall that re-started at every seam would kick the ship every tile.
func key() -> String:
	return tube + "/" + wall_name
