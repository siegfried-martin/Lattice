shader_type sky;

float hash13(vec3 p) {
	p = fract(p * 0.1031);
	p += dot(p, p.zyx + 31.32);
	return fract((p.x + p.y) * p.z);
}

float noise3(vec3 x) {
	vec3 i = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix(hash13(i), hash13(i + vec3(1, 0, 0)), f.x),
				   mix(hash13(i + vec3(0, 1, 0)), hash13(i + vec3(1, 1, 0)), f.x), f.y),
			   mix(mix(hash13(i + vec3(0, 0, 1)), hash13(i + vec3(1, 0, 1)), f.x),
				   mix(hash13(i + vec3(0, 1, 1)), hash13(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

float fbm(vec3 p) {
	float a = 0.5;
	float s = 0.0;
	for (int i = 0; i < 5; i++) {
		s += a * noise3(p);
		p *= 2.03;
		a *= 0.5;
	}
	return s;
}

vec3 star_layer(vec3 dir, float scale, float thresh) {
	vec3 p = dir * scale;
	vec3 cell = floor(p);
	vec3 f = fract(p);
	float h = hash13(cell);
	if (h < thresh) return vec3(0.0);
	vec3 sp = vec3(hash13(cell + 1.7), hash13(cell + 4.3), hash13(cell + 9.1)) * 0.6 + 0.2;
	float d = length(f - sp);
	float b = (h - thresh) / (1.0 - thresh);
	float s = smoothstep(0.14, 0.0, d) * (0.4 + b * 3.0);
	vec3 tint = mix(vec3(0.7, 0.8, 1.0), vec3(1.0, 0.85, 0.7), hash13(cell + 2.2));
	return tint * s;
}

void sky() {
	vec3 d = EYEDIR;
	vec3 c = vec3(0.002, 0.003, 0.008);
	float n = fbm(d * 2.5);
	float n2 = fbm(d * 5.0 + 3.0);
	c += vec3(0.05, 0.02, 0.09) * smoothstep(0.45, 0.8, n);
	c += vec3(0.0, 0.03, 0.06) * smoothstep(0.5, 0.85, n2);
	c += star_layer(d, 120.0, 0.97);
	c += star_layer(d, 260.0, 0.985) * 0.7;
	COLOR = c;
}
