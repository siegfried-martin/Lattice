class_name SpaceFlight
extends RefCounted
## Free flight (open space, and open flying on the highway): thrust with momentum, a
## mouse-driven aim point the ship turns toward at a limited rate, no roll, and a pitch
## limit the camera only partly follows. Handling comes from the ship class.

const FIGHTER := {
	"name": "FIGHTER", "highway": false,  # "highway": has a threader, so can enter the Lattice
	"max_speed": 100.0, "accel": 12.0, "brake": 20.0, "reverse": -15.0,
	"turn_yaw": 50.0, "turn_pitch": 40.0, "damp": 1.2, "cam": Vector3(0.0, 4.2, 16.0), "radius": 5.0,
	"hull": 150.0, "turret": false,
}
const FREIGHTER := {
	"name": "FREIGHTER", "highway": true,
	"max_speed": 40.0, "accel": 4.0, "brake": 8.0, "reverse": -8.0,
	"turn_yaw": 22.0, "turn_pitch": 16.0, "damp": 0.8, "cam": Vector3(0.0, 9.0, 34.0), "radius": 9.0,
	"hull": 400.0, "turret": true,
}

const MAX_PITCH := deg_to_rad(60.0)
const CAM_MAX_PITCH := deg_to_rad(38.0)
const AIM_LEAD_YAW := deg_to_rad(40.0)
const AIM_LEAD_PITCH := deg_to_rad(28.0)
const MOUSE_SENS := 0.0022

var cls: Dictionary = FREIGHTER
var pos := Vector3.ZERO
var velocity := Vector3.ZERO
var yaw := 0.0
var pitch := 0.0
var aim_yaw := 0.0
var aim_pitch := 0.0
var cam_yaw := 0.0
var cam_pitch := 0.0
var throttle_vis := 0.0


func place(p: Vector3, fwd: Vector3, speed: float) -> void:
	pos = p
	yaw = atan2(-fwd.x, -fwd.z)
	pitch = clampf(asin(clampf(fwd.y, -1.0, 1.0)), -MAX_PITCH, MAX_PITCH)
	aim_yaw = yaw
	aim_pitch = pitch
	cam_yaw = yaw
	cam_pitch = _cam_pitch_for(pitch)
	velocity = forward() * speed


func aim(mouse_rel: Vector2) -> void:
	aim_yaw -= mouse_rel.x * MOUSE_SENS
	aim_pitch -= mouse_rel.y * MOUSE_SENS


func ship_basis() -> Basis:
	return Basis.from_euler(Vector3(pitch, yaw, 0.0))


func forward() -> Vector3:
	return -ship_basis().z


func aim_dir() -> Vector3:
	return -Basis.from_euler(Vector3(aim_pitch, aim_yaw, 0.0)).z


func forward_speed() -> float:
	return velocity.dot(forward())


func update(delta: float, thrust: float) -> void:
	# Keep the aim point within reach of the ship so the reticle stays on screen.
	aim_yaw = yaw + clampf(wrapf(aim_yaw - yaw, -PI, PI), -AIM_LEAD_YAW, AIM_LEAD_YAW)
	aim_pitch = clampf(pitch + clampf(aim_pitch - pitch, -AIM_LEAD_PITCH, AIM_LEAD_PITCH), -MAX_PITCH, MAX_PITCH)

	var turn_yaw := deg_to_rad(cls.turn_yaw)
	var turn_pitch := deg_to_rad(cls.turn_pitch)
	yaw += clampf(wrapf(aim_yaw - yaw, -PI, PI) * 2.5, -turn_yaw, turn_yaw) * delta
	pitch += clampf((aim_pitch - pitch) * 2.5, -turn_pitch, turn_pitch) * delta
	pitch = clampf(pitch, -MAX_PITCH, MAX_PITCH)

	var fwd := forward()
	var fspeed := velocity.dot(fwd)
	var lateral := velocity - fwd * fspeed
	var target_throttle := 0.15
	if thrust > 0.0:
		fspeed = move_toward(fspeed, cls.max_speed, cls.accel * delta)
		target_throttle = 1.0
	elif thrust < 0.0:
		fspeed = move_toward(fspeed, cls.reverse, cls.brake * delta)
		target_throttle = 0.0
	fspeed = minf(fspeed, cls.max_speed)
	lateral *= exp(-cls.damp * delta)
	velocity = fwd * fspeed + lateral
	pos += velocity * delta
	throttle_vis = lerpf(throttle_vis, target_throttle, 1.0 - exp(-4.0 * delta))

	cam_yaw = lerp_angle(cam_yaw, yaw, 1.0 - exp(-5.0 * delta))
	cam_pitch = lerpf(cam_pitch, _cam_pitch_for(pitch), 1.0 - exp(-5.0 * delta))


## The camera follows pitch near 1:1 around level but eases off toward the limit,
## so near max climb/dive the ship visibly tilts away from the view.
func _cam_pitch_for(p: float) -> float:
	return CAM_MAX_PITCH * sin(clampf(p / MAX_PITCH, -1.0, 1.0) * PI * 0.5)


func camera_transform() -> Transform3D:
	var cb := Basis.from_euler(Vector3(cam_pitch, cam_yaw, 0.0))
	var cp := pos + cb * (cls.cam as Vector3)
	return Transform3D(cb * Basis(Vector3.RIGHT, deg_to_rad(-6.0)), cp)
