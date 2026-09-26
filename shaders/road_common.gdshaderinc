// Shared road surface. x = lateral metres (0 = median / link centre), z = metres along,
// both divided by the road scale, so these are the design metres before scaling.
// Outputs albedo and a small emission (paint and reflector studs).
const float MEDIAN = 4.0;
const float LANE_W = 10.0;
const float CARR_W = 30.0;
const float SHOULDER = 3.0;

float aa_line(float x, float c, float hw) {
	float fw = max(fwidth(x), 1e-4);
	return 1.0 - smoothstep(hw, hw + fw * 1.5, abs(x - c));
}

float hash21(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

vec3 concrete(float x, float z) {
	float n = hash21(floor(vec2(x, z) * 0.5)) * 0.5 + hash21(floor(vec2(x, z) * 3.0)) * 0.5;
	float slab = aa_line(fract(z / 20.0) * 20.0, 0.0, 0.08);
	return vec3(0.075, 0.075, 0.072) * (0.8 + n * 0.4) * (1.0 - slab * 0.5);
}

float hazard(float x, float z) {
	return step(0.5, fract((x + z) / 3.0));
}

void main_road(float x, float z, out vec3 albedo, out vec3 emit) {
	float ax = abs(x);
	albedo = concrete(x, z);
	emit = vec3(0.0);
	vec3 paint = vec3(0.55, 0.55, 0.52);
	vec3 amber = vec3(0.5, 0.36, 0.12);
	albedo = mix(albedo, amber, aa_line(ax, MEDIAN + 0.6, 0.18));
	float dash = step(0.55, fract(z / 14.0));
	albedo = mix(albedo, paint, aa_line(ax, MEDIAN + LANE_W, 0.18) * dash);
	albedo = mix(albedo, paint, aa_line(ax, MEDIAN + 2.0 * LANE_W, 0.18) * dash);
	albedo = mix(albedo, paint, aa_line(ax, MEDIAN + CARR_W - 0.4, 0.2));
	// Hazard striping on the shoulder.
	float sh = step(MEDIAN + CARR_W, ax);
	albedo = mix(albedo, mix(vec3(0.05), amber * 0.8, hazard(x, z)), sh * 0.8);
	// Reflector studs along the lane lines.
	float stud = step(0.93, fract(z / 18.0)) * (aa_line(ax, MEDIAN + LANE_W, 0.25) + aa_line(ax, MEDIAN + 2.0 * LANE_W, 0.25));
	emit += vec3(0.9, 0.7, 0.4) * stud * 0.6;
}

void ramp_road(float x, float z, vec3 tint, out vec3 albedo, out vec3 emit) {
	float ax = abs(x);
	albedo = concrete(x, z);
	emit = vec3(0.0);
	float edge = step(LANE_W * 0.5, ax);
	albedo = mix(albedo, mix(vec3(0.05), tint * 0.45, hazard(x, z)), edge * 0.8);
	float chev = fract((z + ax * 0.8) / 12.0);
	float c = smoothstep(0.84, 0.92, chev) * step(ax, 3.5);
	albedo = mix(albedo, tint * 0.5, c);
	emit += tint * c * 0.12;
}
