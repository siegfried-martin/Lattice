// Asteroids drawn with MultiMesh. Each rock orbits its cluster centre (the node origin) about
// the vertical axis and tumbles about its own axis. The CPU uses the same orbit for
// collisions, driven by the same sim_time.
// INSTANCE_CUSTOM: x = orbital angular speed (rad/s), y = tumble speed (rad/s), z = seed.
shader_type spatial;
render_mode skip_vertex_transform;

uniform float sim_time = 0.0;
varying float seed;

mat3 rot_y(float a) {
	float c = cos(a);
	float s = sin(a);
	return mat3(vec3(c, 0.0, -s), vec3(0.0, 1.0, 0.0), vec3(s, 0.0, c));
}

mat3 axis_angle(vec3 axis, float a) {
	float c = cos(a);
	float s = sin(a);
	float t = 1.0 - c;
	vec3 n = axis;
	return mat3(
		vec3(t * n.x * n.x + c, t * n.x * n.y + s * n.z, t * n.x * n.z - s * n.y),
		vec3(t * n.x * n.y - s * n.z, t * n.y * n.y + c, t * n.y * n.z + s * n.x),
		vec3(t * n.x * n.z + s * n.y, t * n.y * n.z - s * n.x, t * n.z * n.z + c));
}

void vertex() {
	seed = INSTANCE_CUSTOM.z;
	vec3 axis = normalize(vec3(sin(seed * 12.1), cos(seed * 7.3), sin(seed * 3.7) + 0.2));
	mat3 tumble = axis_angle(axis, INSTANCE_CUSTOM.y * sim_time);
	vec3 w = (MODEL_MATRIX * vec4(tumble * VERTEX, 1.0)).xyz;
	vec3 wn = normalize(mat3(MODEL_MATRIX) * (tumble * NORMAL));
	mat3 orb = rot_y(INSTANCE_CUSTOM.x * sim_time);
	w = NODE_POSITION_WORLD + orb * (w - NODE_POSITION_WORLD);
	wn = orb * wn;
	VERTEX = (VIEW_MATRIX * vec4(w, 1.0)).xyz;
	NORMAL = normalize((VIEW_MATRIX * vec4(wn, 0.0)).xyz);
}

void fragment() {
	vec3 a = vec3(0.23, 0.21, 0.19);
	vec3 b = vec3(0.36, 0.32, 0.27);
	ALBEDO = mix(a, b, fract(seed * 7.13));
	ROUGHNESS = 0.95;
}
