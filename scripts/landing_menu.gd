extends Control
## The menu shown after landing on a planet or putting in at a service station. The game is
## paused while it's open; this node keeps processing so its buttons and keys work.
## Main fills it in with open() and listens for refuel / take_off.

signal refuel
signal take_off

const CYAN := Color(0.5, 0.85, 1.0)
const AMBER := Color(1.0, 0.75, 0.4)

var _title: Label
var _kind: Label
var _fuel: Label
var _fuel_bar: ProgressBar
var _refuel_btn: Button
var _note: Label
var _take_off_btn: Button


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	visible = false
	var dim := ColorRect.new()
	dim.color = Color(0.0, 0.02, 0.05, 0.55)
	dim.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(dim)
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var panel := PanelContainer.new()
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.02, 0.05, 0.1, 0.92)
	style.border_color = Color(CYAN, 0.5)
	style.set_border_width_all(1)
	style.set_content_margin_all(24)
	panel.add_theme_stylebox_override("panel", style)
	panel.custom_minimum_size = Vector2(420, 0)
	center.add_child(panel)
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 10)
	panel.add_child(box)
	_kind = _label(box, 13, CYAN)
	_title = _label(box, 28, Color.WHITE)
	box.add_child(HSeparator.new())
	_fuel = _label(box, 16, Color(0.85, 0.9, 1.0))
	_fuel_bar = ProgressBar.new()
	_fuel_bar.show_percentage = false
	_fuel_bar.custom_minimum_size = Vector2(0, 10)
	box.add_child(_fuel_bar)
	_refuel_btn = _button(box, "Refuel  (F)")
	_refuel_btn.pressed.connect(func(): refuel.emit())
	_note = _label(box, 12, Color(0.6, 0.7, 0.8))
	box.add_child(HSeparator.new())
	_take_off_btn = _button(box, "Take off  (Enter)")
	_take_off_btn.pressed.connect(func(): take_off.emit())


func _label(parent: Control, size: int, color: Color) -> Label:
	var l := Label.new()
	l.add_theme_font_size_override("font_size", size)
	l.add_theme_color_override("font_color", color)
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	parent.add_child(l)
	return l


func _button(parent: Control, text: String) -> Button:
	var b := Button.new()
	b.text = text
	b.add_theme_font_size_override("font_size", 16)
	b.custom_minimum_size = Vector2(0, 36)
	parent.add_child(b)
	return b


## site: {name, kind}. fuel / capacity: capacity 0 means no Lattice Drive fitted.
func open(site: Dictionary, fuel: float, capacity: float) -> void:
	_kind.text = "LANDED" if site.kind == "planet" else "DOCKED  -  SERVICE STATION"
	_title.text = site.name
	show_fuel(fuel, capacity)
	visible = true
	_take_off_btn.grab_focus()


func show_fuel(fuel: float, capacity: float) -> void:
	var drive := capacity > 0.0
	_fuel_bar.visible = drive
	_refuel_btn.visible = drive
	if not drive:
		_fuel.text = "No Lattice Drive fitted"
		_note.text = "Lattice fuel is only for ships that can enter the Lattice."
		return
	_fuel.text = "Lattice fuel   %d / %d" % [roundi(fuel), roundi(capacity)]
	_fuel_bar.max_value = capacity
	_fuel_bar.value = fuel
	_refuel_btn.disabled = fuel >= capacity - 0.01
	_note.text = "Tank full." if _refuel_btn.disabled else "Free for now. Prices come with the economy."


func close() -> void:
	visible = false


func _unhandled_input(event: InputEvent) -> void:
	if not visible or not (event is InputEventKey and event.pressed and not event.echo):
		return
	match event.physical_keycode:
		KEY_F:
			if _refuel_btn.visible and not _refuel_btn.disabled:
				refuel.emit()
			get_viewport().set_input_as_handled()
		KEY_ENTER, KEY_KP_ENTER, KEY_SPACE:
			take_off.emit()
			get_viewport().set_input_as_handled()
