// Sector boundary: a grid that only shows up near the player.
shader_type spatial;
render_mode unshaded, blend_add, cull_disabled, depth_draw_never, shadows_disabled;

uniform vec3 player_pos;
uniform float reach = 1200.0;
uniform float grid = 250.0;
varying vec3 wpos;

void vertex() {
	wpos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
}

void fragment() {
	float d = distance(wpos, player_pos);
	float fade = 1.0 - smoothstep(reach * 0.15, reach, d);
	vec2 g = UV / grid;
	vec2 fw = fwidth(g);
	vec2 gl = 1.0 - smoothstep(vec2(0.0), fw * 1.5, abs(fract(g - 0.5) - 0.5));
	float lines = max(gl.x, gl.y);
	ALBEDO = vec3(0.12, 0.35, 0.8) * (lines * 0.6 + 0.02) * fade;
}
