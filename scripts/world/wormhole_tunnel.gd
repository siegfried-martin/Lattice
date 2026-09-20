class_name WormholeTunnel
extends Node3D
## The inside of the wormhole (`docs/WORMHOLE_PROTOTYPE.md`, step 4): a spindle of
## streaking light around the road's centreline that rides with the ship and closes to
## a throat `wormhole_throat_metres` ahead and the same behind. The streaks are fixed
## in the wormhole — they slide back past the ship by exactly what it travels — and
## on top of that they flow toward it at `wormhole_streak_speed`, so the wormhole is
## alive at rest and faster when moving. The sense of speed is out here; the road
## inside it is the slow part.
##
## Background layer, in the sense CLAUDE.md means: nothing here is queryable, it has
## no body and hands out no position. The map places it every frame (`follow`) from
## the road the ship is on; it owns nothing but its own shape.

## How finely the spindle is tessellated. The rings crowd toward the throats by
## construction (they are spaced in the angle whose sine is the position along the
## axis), so the point where the tunnel closes stays round. Infrastructure.
const RINGS := 48
const SEGMENTS := 64

const SHADER := """
shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_opaque;

uniform vec4 fill : source_color = vec4(0.04, 0.02, 0.09, 1.0);
uniform vec4 streak : source_color = vec4(0.56, 0.82, 1.0, 1.0);
uniform float density = 96.0;
uniform float streak_length = 240.0;
uniform float throat = 500.0;
uniform float phase = 0.0;

float hash(float n) {
	return fract(sin(n * 127.1 + 311.7) * 43758.5453);
}

void fragment() {
	// One lane per streak around the tunnel; each lane has its own rhythm and length.
	float lane = floor(UV.x * density);
	float h = hash(lane);
	float h2 = hash(lane + 57.0);
	// Metres along the axis, fixed in the wormhole: UV.y is -1 at the throat ahead
	// and +1 at the one behind, and phase is what the ship has travelled plus the flow.
	float along = -UV.y * throat + phase;
	float period = streak_length * (2.0 + 4.0 * h);
	float p = fract((along + h2 * period) / period) * period;
	float len = streak_length * (0.3 + 0.7 * h2);
	float body = (1.0 - smoothstep(0.0, len, p)) * smoothstep(0.0, len * 0.15, p);
	// A streak is a thin line down its lane, not the whole lane lit.
	float across = abs(fract(UV.x * density) - 0.5);
	float line = 1.0 - smoothstep(0.08, 0.16, across);
	// The throats glow: the streaks converge there and the light piles up.
	float glow = pow(abs(UV.y), 8.0);
	ALBEDO = fill.rgb + streak.rgb * (body * line * 1.6 + glow * 1.2);
}
"""

var _mesh: MeshInstance3D
var _material: ShaderMaterial
var _phase: float = 0.0


func _ready() -> void:
	_mesh = MeshInstance3D.new()
	_mesh.name = "Streaks"
	_mesh.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	_mesh.mesh = _spindle()
	var shader := Shader.new()
	shader.code = SHADER
	_material = ShaderMaterial.new()
	_material.shader = shader
	_mesh.material_override = _material
	add_child(_mesh)
	rebuild()
	Tuning.reloaded.connect(rebuild)


## Size and colour from the keys. The shape is a unit spindle scaled, so a reload is
## a scale change rather than a rebuild.
func rebuild() -> void:
	var radius := Tuning.num("exploration/wormhole_tunnel_radius")
	var throat := Tuning.num("exploration/wormhole_throat_metres")
	_mesh.scale = Vector3(radius, radius, throat)
	_material.set_shader_parameter("fill", Tuning.color("exploration/wormhole_background_color"))
	_material.set_shader_parameter("streak", Tuning.color("exploration/wormhole_streak_color"))
	_material.set_shader_parameter("density", float(Tuning.integer("exploration/wormhole_streak_density")))
	_material.set_shader_parameter("streak_length", Tuning.num("exploration/wormhole_streak_length"))
	_material.set_shader_parameter("throat", throat)


## Ride with the ship: centred on the road at `frame` (`pos`, `fwd`, `up`, `right` in
## the parent's space, `fwd` the direction of travel), having moved `moved` metres
## along it since the last frame. Called every frame by the map while the ship is
## inside; with an empty frame it stays where it was.
func follow(frame: Dictionary, moved: float, delta: float) -> void:
	if not frame.is_empty():
		var fwd: Vector3 = frame["fwd"]
		var up: Vector3 = frame["up"]
		transform = Transform3D(Basis(fwd.cross(up), up, -fwd), frame["pos"])
	_phase += moved + Tuning.num("exploration/wormhole_streak_speed") * delta
	_material.set_shader_parameter("phase", _phase)


## How far the streaks have slid past, in metres. For the HUD.
func phase() -> float:
	return _phase


## A unit spindle about the local z axis: radius cos θ at z = sin θ, so it is round at
## the middle and closes to a point at z = ±1. UV.x is the angle around, UV.y is z.
static func _spindle() -> ArrayMesh:
	var verts := PackedVector3Array()
	var norms := PackedVector3Array()
	var uvs := PackedVector2Array()
	var idx := PackedInt32Array()
	for i in RINGS + 1:
		var theta := -PI * 0.5 + PI * float(i) / float(RINGS)
		var z := sin(theta)
		var r := maxf(cos(theta), 0.0)
		for j in SEGMENTS + 1:
			var a := TAU * float(j) / float(SEGMENTS)
			verts.append(Vector3(cos(a) * r, sin(a) * r, z))
			norms.append(Vector3(-cos(a), -sin(a), 0.0))
			uvs.append(Vector2(float(j) / float(SEGMENTS), z))
	for i in RINGS:
		for j in SEGMENTS:
			var a := i * (SEGMENTS + 1) + j
			var b := a + SEGMENTS + 1
			idx.append_array(PackedInt32Array([a, b, a + 1, a + 1, b, b + 1]))
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = verts
	arrays[Mesh.ARRAY_NORMAL] = norms
	arrays[Mesh.ARRAY_TEX_UV] = uvs
	arrays[Mesh.ARRAY_INDEX] = idx
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	return mesh
