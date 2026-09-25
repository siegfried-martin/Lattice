shader_type spatial;
render_mode unshaded, blend_mix, cull_disabled, depth_draw_opaque;

uniform vec3 tint : source_color = vec3(0.8, 0.7, 0.6);

void fragment() {
	vec2 p = UV * 2.0 - 1.0;
	float r = length(p);
	if (r > 1.0 || r < 0.55) discard;
	float bands = (sin(r * 80.0) * 0.5 + 0.5) * (sin(r * 23.0 + 1.0) * 0.5 + 0.5);
	ALBEDO = tint * (0.35 + bands * 0.5);
	ALPHA = (0.3 + bands * 0.45) * smoothstep(1.0, 0.95, r) * smoothstep(0.55, 0.6, r);
}
