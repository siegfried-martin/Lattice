// Streaks around the ship while riding a hop lane. UV.x = around, UV.y = along.
shader_type spatial;
render_mode unshaded, blend_add, cull_disabled, depth_draw_never, shadows_disabled;

uniform float intensity = 0.0;
uniform vec3 tint = vec3(0.6, 0.45, 1.0);

float h11(float n) { return fract(sin(n * 12.9898) * 43758.5453); }

void fragment() {
	float a = UV.x * 70.0;
	float id = floor(a);
	float line = smoothstep(0.3, 0.0, abs(fract(a) - 0.5));
	float streak = smoothstep(0.65, 1.0, fract(UV.y * 3.0 + TIME * 2.5 * (0.6 + h11(id)) + h11(id + 7.1)));
	float ends = sin(UV.y * 3.14159);
	ALBEDO = tint * line * streak * ends * intensity * 0.7;
}
