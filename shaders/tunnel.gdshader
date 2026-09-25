// Box tunnel plating. UV.x = metres across the face, UV.y = metres along the path,
// UV2.x = distance ahead of the ship, UV2.y = 0 at the closed back .. 1 at the throat,
// COLOR.r = face (0 floor, 1/3 right wall, 2/3 ceiling, 1 left wall).
shader_type spatial;
render_mode cull_disabled;

varying float face;

float hash(vec2 p) {
	return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void vertex() {
	face = COLOR.r * 3.0;
}

void fragment() {
	float fid = floor(face + 0.5);
	vec2 cell = fid == 0.0 ? vec2(6.0, 6.0) : vec2(12.0, 8.0);
	vec2 g = UV / cell;
	vec2 id = floor(g);
	vec2 f = fract(g);
	// Seams 0.12 m wide, faded out where they'd be thinner than a pixel.
	vec2 fw = max(fwidth(g), vec2(1e-4));
	vec2 w = 0.06 / cell;
	vec2 dd = min(f, 1.0 - f);
	vec2 sl = (1.0 - smoothstep(w, w + fw, dd)) * clamp(w / fw, 0.0, 1.0);
	float seam = max(sl.x, sl.y);
	float h = hash(id + fid * 17.0);
	vec3 base = mix(vec3(0.07, 0.072, 0.076), vec3(0.12, 0.12, 0.125), h);
	if (fid == 0.0) {
		base *= 0.55;
	}
	float fr = UV2.y;
	ALBEDO = base * (1.0 - seam * 0.6) * smoothstep(0.0, 0.15, fr);
	METALLIC = 0.55;
	ROUGHNESS = 0.45 + 0.3 * h;
	// A faint cold light seeping through the plating seams.
	float flow = 0.5 + 0.5 * sin(UV.y * 0.04 - TIME * 0.5 + h * 6.0);
	vec3 emit = vec3(0.12, 0.2, 0.3) * seam * (0.05 + 0.15 * flow) + vec3(0.008, 0.01, 0.014);
	float throat = smoothstep(0.72, 1.0, fr);
	emit += vec3(0.35, 0.45, 0.6) * throat * throat * 1.4;
	EMISSION = emit * smoothstep(0.0, 0.15, fr);
	// Dither the plating away right next to the camera so it can't block the view.
	if (smoothstep(4.0, 14.0, length(VERTEX)) < hash(FRAGCOORD.xy)) {
		discard;
	}
}
