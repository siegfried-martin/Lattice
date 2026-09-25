shader_type spatial;
render_mode cull_disabled;
#include "res://shaders/road_common.gdshaderinc"

uniform bool is_ramp = false;
uniform vec3 tint = vec3(0.55, 0.9, 0.6);

void fragment() {
	vec3 albedo;
	vec3 emit;
	if (is_ramp) {
		ramp_road(UV.x, UV.y, tint, albedo, emit);
	} else {
		main_road(UV.x, UV.y, albedo, emit);
	}
	ALBEDO = albedo;
	EMISSION = emit;
	ROUGHNESS = 0.85;
}
