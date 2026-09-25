// Surface of a rectangular gate: a slow, muted swirl.
shader_type spatial;
render_mode unshaded, blend_add, cull_disabled, depth_draw_never, shadows_disabled;

uniform vec3 tint = vec3(0.55, 0.9, 0.6);

void fragment() {
	vec2 p = UV * 2.0 - 1.0;
	float r = length(p);
	float a = atan(p.y, p.x);
	float sw = sin(a * 4.0 + log(r + 0.01) * 6.0 - TIME * 1.5) * 0.5 + 0.5;
	float edge = smoothstep(1.0, 0.8, max(abs(p.x), abs(p.y)));
	ALBEDO = tint * (0.05 + sw * 0.18 * r + smoothstep(0.6, 0.0, r) * 0.25) * edge;
}
