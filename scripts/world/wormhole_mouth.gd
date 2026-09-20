class_name WormholeMouth
extends Node3D
## The opening between the worlds, drawn at a ramp's crossing point
## (`docs/WORMHOLE_PROTOTYPE.md`). In open space it is the wormhole's mouth ahead on an
## on-ramp: a disc of the wormhole's own dark with its streaks converging, rimmed in
## their light. Inside, on an off-ramp, it is space opening ahead: the same rim
## around open space's colour. Behind the ship, after a crossing, it is the mouth just
## come through.
##
## ONE-SIDED. The camera lags the ship by a few dozen metres, so for a moment after
## every crossing the mouth stands between the two; a disc seen from both sides would
## hide the ship. Each mouth is shown only to a camera on the side of the ramp its
## world draws, which is the side it is ever looked at from.
##
## Background layer: no body, nothing queryable, placed by the network that owns it.

const SEGMENTS := 64

const SHADER := """
shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_opaque;

uniform vec4 fill : source_color = vec4(0.04, 0.02, 0.09, 1.0);
uniform vec4 rim : source_color = vec4(0.56, 0.82, 1.0, 1.0);
uniform float density = 96.0;
uniform float swirl = 1.0;
uniform float phase = 0.0;

float hash(float n) {
	return fract(sin(n * 127.1 + 311.7) * 43758.5453);
}

void fragment() {
	vec2 p = UV * 2.0 - 1.0;
	float r = length(p);
	if (r > 1.0) {
		discard;
	}
	float a = atan(p.y, p.x) / TAU + 0.5;
	float lane = floor(a * density);
	float h = hash(lane);
	// Streaks running in to the centre, the same lanes as the tunnel's, drawn inward.
	float along = r * 4.0 + h * 3.0 + phase * 0.002;
	float body = pow(fract(along), 3.0) * (1.0 - smoothstep(0.0, 0.85, r));
	float across = abs(fract(a * density) - 0.5);
	float line = 1.0 - smoothstep(0.08, 0.16, across);
	float ring = smoothstep(0.84, 0.94, r) * (1.0 - smoothstep(0.97, 1.0, r));
	ALBEDO = fill.rgb + rim.rgb * (ring * 1.6 + swirl * body * line * 1.2);
}
"""

var _mesh: MeshInstance3D
var _material: ShaderMaterial
var _elapsed: float = 0.0


func _ready() -> void:
	_mesh = MeshInstance3D.new()
	_mesh.name = "Disc"
	_mesh.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	_mesh.mesh = _disc()
	var shader := Shader.new()
	shader.code = SHADER
	_material = ShaderMaterial.new()
	_material.shader = shader
	_mesh.material_override = _material
	add_child(_mesh)


## Place it on the ramp's axis at `at`, its front toward `facing` (the side it is
## seen from), `up` the ramp's up there. `into_wormhole` paints it as the wormhole's
## mouth (its dark, its streaks); otherwise as space beyond the rim.
func place(at: Vector3, facing: Vector3, up: Vector3, into_wormhole: bool) -> void:
	transform = Transform3D(Basis(up.cross(facing), up, facing), at)
	_mesh.scale = Vector3.ONE * Tuning.num("exploration/wormhole_mouth_radius")
	_material.set_shader_parameter("fill", Tuning.color("exploration/wormhole_background_color")
		if into_wormhole else Tuning.color("arena/background_color"))
	_material.set_shader_parameter("rim", Tuning.color("exploration/wormhole_streak_color"))
	_material.set_shader_parameter("density", float(Tuning.integer("exploration/wormhole_streak_density")))
	_material.set_shader_parameter("swirl", 1.0 if into_wormhole else 0.0)


func _process(delta: float) -> void:
	_elapsed += Tuning.num("exploration/wormhole_streak_speed") * delta
	_material.set_shader_parameter("phase", _elapsed)
	# One-sided, decided here rather than in the shader: shown only to a camera on
	# the side it faces.
	var camera := get_viewport().get_camera_3d()
	_mesh.visible = camera != null \
		and (camera.global_position - global_position).dot(global_transform.basis.z) > 0.0


## The side it is seen from, in the parent's space.
func facing() -> Vector3:
	return transform.basis.z


## A unit disc in the local xy plane, front toward +z, UV from 0 to 1 across it.
static func _disc() -> ArrayMesh:
	var verts := PackedVector3Array([Vector3.ZERO])
	var norms := PackedVector3Array([Vector3(0.0, 0.0, 1.0)])
	var uvs := PackedVector2Array([Vector2(0.5, 0.5)])
	var idx := PackedInt32Array()
	for j in SEGMENTS + 1:
		var a := TAU * float(j) / float(SEGMENTS)
		verts.append(Vector3(cos(a), sin(a), 0.0))
		norms.append(Vector3(0.0, 0.0, 1.0))
		uvs.append(Vector2(0.5 + 0.5 * cos(a), 0.5 + 0.5 * sin(a)))
	for j in SEGMENTS:
		idx.append_array(PackedInt32Array([0, j + 1, j + 2]))
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = verts
	arrays[Mesh.ARRAY_NORMAL] = norms
	arrays[Mesh.ARRAY_TEX_UV] = uvs
	arrays[Mesh.ARRAY_INDEX] = idx
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	return mesh
