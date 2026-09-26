extends Control
## Screen overlay. Main fills in the fields each frame; everything is drawn in _draw.

const CYAN := Color(0.5, 0.85, 1.0)
const DIM := Color(0.5, 0.65, 0.8, 0.7)
const PANEL := Color(0.02, 0.05, 0.1, 0.6)
const WARN := Color(1.0, 0.35, 0.25)

var show_flight := true      # pitch gauge
var left_lines: Array = []   # [text, color, size]
var right_lines: Array = []
var center_msg := ""
var help := ""
var reticle := Vector2(-1, -1)
var heading := Vector2(-1, -1)
var crosshair := false       # turret: fixed centre crosshair
var markers: Array = []      # {pos, text, color, arrow, bar (optional 0..1)}
var pitch_frac := 0.0        # ship pitch / max pitch
var speed_frac := -1.0       # speed / top speed; hidden when negative
var bars: Array = []         # {label, frac, color, text}
var warning := ""            # blinking alert, e.g. an incoming missile
var radar: Dictionary = {}   # {range: text, blips: [{p: Vector2 in -1..1 (up = heading), color, target, missile}]}
var target: Dictionary = {}  # {name, cls, stance, color, dist, speed, closing, hp}
var target_box: Dictionary = {}  # {pos, half, color}: box round the target in the view

var nav: Dictionary = {}      # {sector, route: {ahead, more, passed, frac}, on_link, lanes}

const RADAR_R := 100.0
const NAV_W := 330.0
const ROUTE_ROW := 30.0
const ROUTE_STOPS := 5        # stops ahead shown on the route line
const TARGET_W := 250.0


func _draw() -> void:
	var font := ThemeDB.fallback_font
	_panel(font, Vector2(16, 16), left_lines, false)
	if not right_lines.is_empty():
		_panel(font, Vector2(size.x - 16, 16), right_lines, true)

	for m in markers:
		_marker(font, m)
	if not target_box.is_empty():
		_box(target_box.pos, target_box.half, target_box.color)
	if not radar.is_empty():
		_radar(font)
	if not target.is_empty():
		_target_panel(font)
	if not nav.is_empty():
		_nav(font)
		if not (nav.lanes as Dictionary).is_empty():
			_lanes(font, nav.lanes)
	if heading.x >= 0.0:
		draw_circle(heading, 3.0, Color(1, 1, 1, 0.6))
	if reticle.x >= 0.0:
		draw_arc(reticle, 16.0, 0.0, TAU, 40, CYAN, 1.5)
		for a in 4:
			var d := Vector2.RIGHT.rotated(a * PI * 0.5)
			draw_line(reticle + d * 10.0, reticle + d * 22.0, CYAN, 1.5)
	if crosshair:
		var c := size * 0.5
		for a in 4:
			var d := Vector2.RIGHT.rotated(a * PI * 0.5)
			draw_line(c + d * 6.0, c + d * 18.0, CYAN, 2.0)
		draw_circle(c, 1.5, CYAN)
	if show_flight:
		_pitch_gauge(font)
	if speed_frac >= 0.0:
		_speed_meter(font)
	_bars(font)

	if warning != "" and fmod(Time.get_ticks_msec() / 1000.0, 0.6) < 0.4:
		draw_string(font, Vector2(0, size.y * 0.2), warning, HORIZONTAL_ALIGNMENT_CENTER, size.x, 26, WARN)
	if center_msg != "":
		draw_string(font, Vector2(0, size.y * 0.3), center_msg, HORIZONTAL_ALIGNMENT_CENTER, size.x, 22, Color(1.0, 0.75, 0.4))
	draw_string(font, Vector2(16, size.y - 16), help, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)


func _marker(font: Font, m: Dictionary) -> void:
	var p: Vector2 = m.pos
	var c: Color = m.color
	if m.arrow:
		var dir := (p - size * 0.5).normalized()
		draw_colored_polygon(PackedVector2Array([p, p - dir * 14 + dir.orthogonal() * 7, p - dir * 14 - dir.orthogonal() * 7]), c)
		draw_string(font, p - dir * 22 + Vector2(-40, 4), m.text, HORIZONTAL_ALIGNMENT_CENTER, 80, 12, c)
		return
	var pts := PackedVector2Array([p + Vector2(0, -7), p + Vector2(7, 0), p + Vector2(0, 7), p + Vector2(-7, 0), p + Vector2(0, -7)])
	draw_polyline(pts, c, 1.5)
	draw_string(font, p + Vector2(11, 4), m.text, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, c)
	if m.has("bar"):
		var r := Rect2(p + Vector2(-20, 11), Vector2(40, 4))
		draw_rect(r, Color(0, 0, 0, 0.6))
		draw_rect(Rect2(r.position, Vector2(r.size.x * clampf(m.bar, 0.0, 1.0), r.size.y)), c)


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
	var cy := size.y * 0.5 + 80.0   # below the radar / navigation panel
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


## Speed as a share of top speed, filling from the bottom.
func _speed_meter(font: Font) -> void:
	var r := Rect2(Vector2(30.0, size.y * 0.5 - 110.0), Vector2(12.0, 220.0))
	draw_rect(r, Color(0, 0, 0, 0.45))
	var f := clampf(speed_frac, 0.0, 1.0)
	var fill := Rect2(Vector2(r.position.x, r.end.y - r.size.y * f), Vector2(r.size.x, r.size.y * f))
	draw_rect(fill, CYAN.lerp(Color(1.0, 0.85, 0.5), f))
	draw_rect(r, DIM, false, 1.0)
	draw_string(font, Vector2(r.position.x - 6, r.position.y - 10), "SPEED", HORIZONTAL_ALIGNMENT_LEFT, -1, 11, DIM)
	draw_string(font, Vector2(r.position.x - 6, r.end.y + 16), "%d%%" % int(f * 100.0), HORIZONTAL_ALIGNMENT_LEFT, -1, 12, CYAN)


## Status bars stacked above the help line, centred.
func _bars(font: Font) -> void:
	var w := 220.0
	var y := size.y - 44.0
	for i in range(bars.size() - 1, -1, -1):
		var b: Dictionary = bars[i]
		var r := Rect2(Vector2(size.x * 0.5 - w * 0.5, y), Vector2(w, 8.0))
		draw_rect(r, Color(0, 0, 0, 0.5))
		draw_rect(Rect2(r.position, Vector2(w * clampf(b.frac, 0.0, 1.0), r.size.y)), b.color)
		draw_rect(r, Color(b.color, 0.5), false, 1.0)
		draw_string(font, Vector2(r.position.x - 110, y + 8), b.label, HORIZONTAL_ALIGNMENT_RIGHT, 100, 12, b.color)
		draw_string(font, Vector2(r.end.x + 10, y + 8), b.get("text", ""), HORIZONTAL_ALIGNMENT_LEFT, -1, 12, b.color)
		y -= 20.0


## Corner brackets round a point.
func _box(c: Vector2, h: float, col: Color) -> void:
	var k := minf(h * 0.45, 12.0)
	for sx in [-1.0, 1.0]:
		for sy in [-1.0, 1.0]:
			var corner := c + Vector2(sx, sy) * h
			draw_line(corner, corner - Vector2(sx * k, 0), col, 2.0)
			draw_line(corner, corner - Vector2(0, sy * k), col, 2.0)


func _radar_center() -> Vector2:
	return Vector2(size.x - 24.0 - RADAR_R, 24.0 + RADAR_R)


## Top-down view of sensor range; the ship sits in the middle, heading up.
func _radar(font: Font) -> void:
	var c := _radar_center()
	draw_circle(c, RADAR_R + 6.0, PANEL)
	draw_arc(c, RADAR_R + 6.0, 0.0, TAU, 64, Color(CYAN, 0.35), 1.0)
	draw_arc(c, RADAR_R * 0.5, 0.0, TAU, 48, Color(DIM, 0.3), 1.0)
	draw_line(c + Vector2(0, -RADAR_R), c + Vector2(0, RADAR_R), Color(DIM, 0.2), 1.0)
	draw_line(c + Vector2(-RADAR_R, 0), c + Vector2(RADAR_R, 0), Color(DIM, 0.2), 1.0)
	for b in radar.blips:
		var p: Vector2 = b.p
		if p.length() > 1.0:
			p = p.normalized()
		var bp := c + p * RADAR_R
		var col: Color = b.color
		if b.missile:
			draw_circle(bp, 2.0, col)
		else:
			draw_rect(Rect2(bp - Vector2(2.5, 2.5), Vector2(5, 5)), col)
		if b.target:
			_box(bp, 6.0, col)
	draw_colored_polygon(PackedVector2Array([c + Vector2(0, -7), c + Vector2(5, 5), c + Vector2(-5, 5)]), CYAN)
	draw_string(font, c + Vector2(-RADAR_R, RADAR_R + 22.0), "RADAR", HORIZONTAL_ALIGNMENT_LEFT, -1, 11, DIM)
	draw_string(font, c + Vector2(-RADAR_R, RADAR_R + 22.0), radar.range, HORIZONTAL_ALIGNMENT_RIGHT, RADAR_R * 2.0, 11, DIM)


## Target info, beside the radar.
func _target_panel(font: Font) -> void:
	var col: Color = target.color
	var x := _radar_center().x - RADAR_R - 18.0 - TARGET_W
	var r := Rect2(Vector2(x, 18.0), Vector2(TARGET_W, 150.0))
	draw_rect(r, PANEL)
	draw_rect(r, Color(col, 0.45), false, 1.0)
	var lx := x + 12.0
	draw_string(font, Vector2(lx, 36), "TARGET", HORIZONTAL_ALIGNMENT_LEFT, -1, 11, DIM)
	draw_string(font, Vector2(lx, 36), target.stance, HORIZONTAL_ALIGNMENT_RIGHT, TARGET_W - 24.0, 11, col)
	draw_string(font, Vector2(lx, 60), target.name, HORIZONTAL_ALIGNMENT_LEFT, TARGET_W - 24.0, 17, Color.WHITE)
	draw_string(font, Vector2(lx, 80), target.cls, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, col)
	var rows := [["RANGE", target.dist], ["SPEED", target.speed], ["CLOSING", target.closing]]
	var y := 102.0
	for row in rows:
		draw_string(font, Vector2(lx, y), row[0], HORIZONTAL_ALIGNMENT_LEFT, -1, 12, DIM)
		draw_string(font, Vector2(lx, y), row[1], HORIZONTAL_ALIGNMENT_RIGHT, TARGET_W - 24.0, 13, Color(0.85, 0.9, 1.0))
		y += 17.0
	var bar := Rect2(Vector2(lx + 50.0, y - 3.0), Vector2(TARGET_W - 74.0, 6.0))
	draw_string(font, Vector2(lx, y + 3.0), "HULL", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, DIM)
	draw_rect(bar, Color(0, 0, 0, 0.6))
	draw_rect(Rect2(bar.position, Vector2(bar.size.x * clampf(target.hp, 0.0, 1.0), bar.size.y)), col)


static func _event_color(kind: String) -> Color:
	match kind:
		"off", "end":
			return GateBuilder.OFF_TINT
		"jct", "jct_end":
			return GateBuilder.JCT_TINT
	return Color(0.85, 0.85, 0.8)


## Lattice navigation, top right: the road you're on as a subway line, the next stops at the
## top and the ship between the last stop passed and the next one.
func _nav(font: Font) -> void:
	var route: Dictionary = nav.route
	var ahead: Array = route.ahead
	var rows := ahead.size() + 1 + (1 if route.more > 0 else 0)
	var x := size.x - 16.0 - NAV_W
	var y := 16.0
	var h := 44.0 + rows * ROUTE_ROW + 26.0 + (20.0 if nav.on_link != "" else 0.0)
	draw_rect(Rect2(x, y, NAV_W, h), PANEL)
	draw_rect(Rect2(x, y, NAV_W, h), Color(CYAN, 0.35), false, 1.0)
	draw_string(font, Vector2(x + 12, y + 20), "NAVIGATION", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(0.6, 0.9, 1.0))
	draw_string(font, Vector2(x + 12, y + 20), "SECTOR  %s" % nav.sector, HORIZONTAL_ALIGNMENT_RIGHT, NAV_W - 24, 15, Color.WHITE)
	var lx := x + 26.0
	var top := y + 48.0
	# Rows from the top: "+N more", the stops ahead (farthest first), then the last stop passed.
	var row_y := func(i: int) -> float:
		return top + i * ROUTE_ROW + ROUTE_ROW * 0.5
	var r0 := 1 if route.more > 0 else 0
	var you_row := r0 + ahead.size()   # the passed stop's row
	var line_top: float = row_y.call(0)
	var line_bot: float = row_y.call(you_row) if not route.passed.is_empty() else row_y.call(you_row) - ROUTE_ROW * 0.5
	draw_line(Vector2(lx, line_top), Vector2(lx, line_bot), Color(CYAN, 0.8), 3.0)
	if route.more > 0:
		for k in 3:
			draw_circle(Vector2(lx, row_y.call(0) - 6.0 + k * 6.0), 1.5, CYAN)
		draw_string(font, Vector2(lx + 18, row_y.call(0) + 5), "+%d more" % route.more, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, DIM)
	for i in ahead.size():
		var e: Dictionary = ahead[ahead.size() - 1 - i]
		_stop(font, Vector2(lx, row_y.call(r0 + i)), e, false, x)
	if not route.passed.is_empty():
		_stop(font, Vector2(lx, row_y.call(you_row)), route.passed, true, x)
	if ahead.is_empty():
		draw_string(font, Vector2(lx + 18, row_y.call(r0) + 5), "end of the line", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	# The ship, between the last stop passed and the next.
	var from_y: float = row_y.call(you_row) if not route.passed.is_empty() else row_y.call(you_row) + ROUTE_ROW * 0.4
	var to_y: float = row_y.call(you_row - 1) if not ahead.is_empty() else from_y - ROUTE_ROW
	var sy := lerpf(from_y, to_y, clampf(route.frac, 0.1, 0.9))
	draw_colored_polygon(PackedVector2Array([Vector2(lx, sy - 8), Vector2(lx + 6, sy + 5), Vector2(lx - 6, sy + 5)]), Color.WHITE)
	var ly := top + rows * ROUTE_ROW + 18.0
	if nav.on_link != "":
		draw_string(font, Vector2(x + 12, ly), nav.on_link, HORIZONTAL_ALIGNMENT_LEFT, NAV_W - 24, 13, GateBuilder.ON_TINT)


## One stop on the route line: a glyph for its kind, its name, and the distance to it.
func _stop(font: Font, p: Vector2, e: Dictionary, passed: bool, panel_x: float) -> void:
	var col := _event_color(e.kind)
	if passed:
		col = Color(col, 0.45)
	match e.kind:
		"end":
			draw_line(p + Vector2(-8, 0), p + Vector2(8, 0), col, 4.0)
		"jct_end":
			draw_line(p, p + Vector2(-9, -8), col, 3.0)
			draw_line(p, p + Vector2(9, -8), col, 3.0)
			draw_circle(p, 5.0, col)
		_:
			# A branch off to the right, the side you leave from.
			draw_line(p, p + Vector2(10, -7), col, 3.0)
			draw_circle(p, 5.0, col)
	var name: String = e.text
	match e.kind:
		"off":
			name = name.trim_prefix("EXIT ")
		"jct":
			name = name.trim_prefix("JCT ")
		"jct_end":
			name = "< %s  |  %s >" % [e.left.get_slice(" >", 0), e.right.get_slice(" >", 0)]
		"end":
			name = name.replace("END  -  EXIT", "END  -")
	var dist := absf(e.dist)
	var dtxt := ("%.1f km" % (dist / 1000.0) if dist >= 1000.0 else "%d m" % int(dist)) + (" back" if passed else "")
	draw_string(font, Vector2(p.x + 18, p.y + 5), name, HORIZONTAL_ALIGNMENT_LEFT, NAV_W - 130, 13, col)
	draw_string(font, Vector2(panel_x + 12, p.y + 5), dtxt, HORIZONTAL_ALIGNMENT_RIGHT, NAV_W - 24, 13,
		Color(0.85, 0.9, 1.0, 0.45 if passed else 1.0))


## Lane guidance for the next fork, top centre: one box per lane, the ship's lane outlined.
func _lanes(font: Font, g: Dictionary) -> void:
	var col := _event_color(g.kind)
	var bw := 46.0
	var bh := 50.0
	var lanes: Array = g.lanes
	var total := bw * lanes.size() + 6.0 * (lanes.size() - 1)
	var x0 := size.x * 0.5 - total * 0.5
	var y0 := 18.0
	var text: String = g.text
	if g.kind == "jct_end":
		text = "<  %s        %s  >" % [g.left, g.right]
	var pw := maxf(total, font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, 14).x) + 24.0
	draw_rect(Rect2(size.x * 0.5 - pw * 0.5, y0 - 8, pw, bh + 50), PANEL)
	for i in lanes.size():
		var r := Rect2(x0 + i * (bw + 6.0), y0, bw, bh)
		var what: String = lanes[i]
		var lit := what != ""
		var c := col if lit else Color(DIM, 0.8)
		draw_rect(r, Color(c, 0.15))
		var mid := r.get_center()
		var base := Vector2(mid.x, r.end.y - 8)
		var tip := Vector2(mid.x, r.position.y + 10)
		match what:
			"ramp", "right":
				tip = Vector2(r.end.x - 10, r.position.y + 14)
			"left":
				tip = Vector2(r.position.x + 10, r.position.y + 14)
		if what == "end":
			draw_line(Vector2(r.position.x + 8, r.position.y + 12), Vector2(r.end.x - 8, r.position.y + 12), c, 3.0)
			draw_line(base, Vector2(mid.x, r.position.y + 18), c, 3.0)
		else:
			var knee := Vector2(mid.x, mid.y + 2)
			draw_polyline(PackedVector2Array([base, knee, tip]), c, 3.0)
			var dir := (tip - knee).normalized()
			draw_colored_polygon(PackedVector2Array([tip + dir * 4.0, tip - dir * 7.0 + dir.orthogonal() * 6.0, tip - dir * 7.0 - dir.orthogonal() * 6.0]), c)
		if i == g.current:
			draw_rect(r.grow(2.0), CYAN, false, 2.0)
	var ty := y0 + bh + 20.0
	draw_string(font, Vector2(0, ty), text, HORIZONTAL_ALIGNMENT_CENTER, size.x, 14, col)
	draw_string(font, Vector2(0, ty + 17), g.dist, HORIZONTAL_ALIGNMENT_CENTER, size.x, 13, Color(0.85, 0.9, 1.0))
