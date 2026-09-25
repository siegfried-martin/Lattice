shader_type spatial;

uniform vec3 col_a : source_color;
uniform vec3 col_b : source_color;
uniform vec3 col_c : source_color;
uniform vec3 atmo : source_color;
uniform float seed = 0.0;
uniform float banding = 0.0;
// 0 = seen at its true distance, 1 = far away (neighbouring sector): dims and cools it.
uniform float haze = 0.0;
varying vec3 opos;

float hash(vec3 p) {
	p = fract(p * 0.3183099 + 0.1);
	p *= 17.0;
	return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x) {
	vec3 i = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix(hash(i), hash(i + vec3(1, 0, 0)), f.x),
				   mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x), f.y),
			   mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x),
				   mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

float fbm(vec3 p) {
	float a = 0.5;
	float s = 0.0;
	for (int i = 0; i < 6; i++) {
		s += a * noise(p);
		p *= 2.03;
		a *= 0.5;
	}
	return s;
}

void vertex() {
	opos = VERTEX;
}

void fragment() {
	vec3 p = normalize(opos);
	float n = fbm(p * 3.0 + seed);
	float bands = sin(p.y * 18.0 + fbm(p * 4.0 + seed) * 6.0) * 0.5 + 0.5;
	float v = mix(n, bands, banding);
	vec3 c = mix(col_a, col_b, smoothstep(0.38, 0.55, v));
	c = mix(c, col_c, smoothstep(0.62, 0.78, v));
	ALBEDO = mix(c, vec3(0.03, 0.04, 0.06), haze * 0.75);
	ROUGHNESS = 0.9;
	float fres = pow(1.0 - clamp(dot(NORMAL, VIEW), 0.0, 1.0), 3.0);
	EMISSION = atmo * fres * 0.6 * (1.0 - haze * 0.8);
}
