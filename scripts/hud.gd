extends Control
## Screen overlay. Main fills in the fields each frame; everything is drawn in _draw.

const CYAN := Color(0.5, 0.85, 1.0)
const DIM := Color(0.5, 0.65, 0.8, 0.7)
const PANEL := Color(0.02, 0.05, 0.1, 0.6)

var show_flight := true      # reticle, heading pip, pitch gauge
var left_lines: Array = []    # [text, color]
var right_lines: Array = []
var center_msg := ""
var help := ""
var reticle := Vector2(-1, -1)
var heading := Vector2(-1, -1)
var markers: Array = []       # {pos: Vector2, text, color, arrow: bool}
var pitch_frac := 0.0         # ship pitch / max pitch


func _draw() -> void:
	var font := ThemeDB.fallback_font
	_panel(font, Vector2(16, 16), left_lines, false)
	if not right_lines.is_empty():
		_panel(font, Vector2(size.x - 16, 16), right_lines, true)

	if show_flight:
		for m in markers:
			var p: Vector2 = m.pos
			var c: Color = m.color
			if m.arrow:
				var dir := (p - size * 0.5).normalized()
				var tip := p
				draw_colored_polygon(PackedVector2Array([tip, tip - dir * 14 + dir.orthogonal() * 7, tip - dir * 14 - dir.orthogonal() * 7]), c)
				draw_string(font, p - dir * 22 + Vector2(-40, 4), m.text, HORIZONTAL_ALIGNMENT_CENTER, 80, 12, c)
			else:
				var pts := PackedVector2Array([p + Vector2(0, -7), p + Vector2(7, 0), p + Vector2(0, 7), p + Vector2(-7, 0), p + Vector2(0, -7)])
				draw_polyline(pts, c, 1.5)
				draw_string(font, p + Vector2(11, 4), m.text, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, c)
		if heading.x >= 0.0:
			draw_circle(heading, 3.0, Color(1, 1, 1, 0.6))
		if reticle.x >= 0.0:
			draw_arc(reticle, 16.0, 0.0, TAU, 40, CYAN, 1.5)
			for a in 4:
				var d := Vector2.RIGHT.rotated(a * PI * 0.5)
				draw_line(reticle + d * 10.0, reticle + d * 22.0, CYAN, 1.5)
		_pitch_gauge(font)

	if center_msg != "":
		draw_string(font, Vector2(0, size.y * 0.3), center_msg, HORIZONTAL_ALIGNMENT_CENTER, size.x, 22, Color(1.0, 0.75, 0.4))
	draw_string(font, Vector2(16, size.y - 16), help, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)


func _panel(font: Font, anchor: Vector2, lines: Array, right_align: bool) -> void:
	var w := 0.0
	for l in lines:
		w = maxf(w, font.get_string_size(l[0], HORIZONTAL_ALIGNMENT_LEFT, -1, l[2] if l.size() > 2 else 16).x)
	w += 24.0
	var h := 14.0
	for l in lines:
		h += (l[2] if l.size() > 2 else 16) + 8
	var origin := anchor - Vector2(w, 0) if right_align else anchor
	draw_rect(Rect2(origin, Vector2(w, h)), PANEL)
	draw_rect(Rect2(origin, Vector2(w, h)), Color(CYAN, 0.35), false, 1.0)
	var y := origin.y + 8
	for l in lines:
		var fs: int = l[2] if l.size() > 2 else 16
		y += fs + 4
		draw_string(font, Vector2(origin.x + 12, y), l[0], HORIZONTAL_ALIGNMENT_LEFT, -1, fs, l[1])
		y += 4


## Vertical gauge of ship pitch; the ends turn amber as the climb/dive limit gets close.
func _pitch_gauge(font: Font) -> void:
	var x := size.x - 40.0
	var cy := size.y * 0.5
	var h := 110.0
	draw_line(Vector2(x, cy - h), Vector2(x, cy + h), DIM, 2.0)
	for k in [-1.0, -0.5, 0.0, 0.5, 1.0]:
		var y: float = cy - k * h
		draw_line(Vector2(x - 6, y), Vector2(x + 6, y), DIM, 1.0)
	var near_limit := absf(pitch_frac) > 0.8
	var c := Color(1.0, 0.65, 0.3) if near_limit else CYAN
	var py := cy - clampf(pitch_frac, -1.0, 1.0) * h
	draw_colored_polygon(PackedVector2Array([Vector2(x - 8, py), Vector2(x - 18, py - 6), Vector2(x - 18, py + 6)]), c)
	draw_string(font, Vector2(x - 30, cy - h - 10), "PITCH", HORIZONTAL_ALIGNMENT_LEFT, -1, 11, DIM)
