// Camera-facing glow that never shrinks below a minimum on-screen size.
shader_type spatial;
render_mode unshaded, blend_add, depth_draw_never, cull_disabled, shadows_disabled;

instance uniform vec3 tint = vec3(0.5, 0.8, 1.0);
instance uniform float intensity = 1.0;
instance uniform float base_size = 3.0;
uniform float px_scale = 0.004;

void vertex() {
	vec4 cv = MODELVIEW_MATRIX * vec4(0.0, 0.0, 0.0, 1.0);
	float dist = max(-cv.z, 0.01);
	float size = max(base_size, dist * px_scale);
	cv.xy += VERTEX.xy * size;
	POSITION = PROJECTION_MATRIX * cv;
}

void fragment() {
	vec2 p = UV * 2.0 - 1.0;
	float r2 = dot(p, p);
	float g = exp(-r2 * 5.0) * (1.0 - smoothstep(0.8, 1.0, r2));
	ALBEDO = tint * g * intensity * 2.0;
}
