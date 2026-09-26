// Median barrier: concrete with a lamp housing every 36 m (UV.x = height, UV.y = road u),
// in design metres: UV is divided by the road scale.
shader_type spatial;
render_mode cull_disabled;

uniform float scale = 1.0;

void fragment() {
	float y = UV.x / scale;
	float along = fract(UV.y / scale / 36.0 + 0.5) - 0.5;
	vec3 c = vec3(0.12, 0.12, 0.115);
	c *= 0.85 + 0.15 * step(0.5, fract(UV.y / scale / 4.0));
	float lamp = step(abs(along * 36.0), 0.8) * step(0.9, y) * step(y, 1.4);
	ALBEDO = mix(c, vec3(0.2), lamp);
	EMISSION = vec3(1.0, 0.8, 0.55) * lamp * 3.0;
	ROUGHNESS = 0.9;
}
