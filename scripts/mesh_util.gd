class_name MeshUtil
## Small procedural mesh helpers.


## Flat ribbon along a track. UV = (lateral x in metres, s in metres).
static func track_ribbon(track: Track, s0: float, s1: float, step: float, x0: float, x1: float, y: float) -> ArrayMesh:
	var verts := PackedVector3Array()
	var uvs := PackedVector2Array()
	var norms := PackedVector3Array()
	var idx := PackedInt32Array()
	var n := maxi(1, int(ceil((s1 - s0) / step)))
	for i in n + 1:
		var s := s0 + (s1 - s0) * i / n
		var p := track.pos(s) + Vector3.UP * y
		var r := track.right(s)
		verts.append(p + r * x0)
		verts.append(p + r * x1)
		uvs.append(Vector2(x0, s))
		uvs.append(Vector2(x1, s))
		norms.append(Vector3.UP)
		norms.append(Vector3.UP)
		if i < n:
			var b := i * 2
			idx.append_array([b, b + 2, b + 1, b + 1, b + 2, b + 3])
	return _mesh(verts, uvs, norms, idx)


## Vertical ribbon along a track at lateral offset x. UV = (height, s).
static func track_wall(track: Track, s0: float, s1: float, step: float, x: float, y0: float, y1: float) -> ArrayMesh:
	var verts := PackedVector3Array()
	var uvs := PackedVector2Array()
	var norms := PackedVector3Array()
	var idx := PackedInt32Array()
	var n := maxi(1, int(ceil((s1 - s0) / step)))
	for i in n + 1:
		var s := s0 + (s1 - s0) * i / n
		var p := track.point(s, x)
		var r := track.right(s)
		verts.append(p + Vector3.UP * y0)
		verts.append(p + Vector3.UP * y1)
		uvs.append(Vector2(y0, s))
		uvs.append(Vector2(y1, s))
		norms.append(r)
		norms.append(r)
		if i < n:
			var b := i * 2
			idx.append_array([b, b + 2, b + 1, b + 1, b + 2, b + 3])
	return _mesh(verts, uvs, norms, idx)


static func _mesh(verts: PackedVector3Array, uvs: PackedVector2Array, norms: PackedVector3Array, idx: PackedInt32Array) -> ArrayMesh:
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = verts
	arrays[Mesh.ARRAY_TEX_UV] = uvs
	arrays[Mesh.ARRAY_NORMAL] = norms
	arrays[Mesh.ARRAY_INDEX] = idx
	var m := ArrayMesh.new()
	m.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	return m


static func part(parent: Node3D, mesh: Mesh, pos: Vector3, mat: Material, rot_deg := Vector3.ZERO, scl := Vector3.ONE) -> MeshInstance3D:
	var mi := MeshInstance3D.new()
	mi.mesh = mesh
	mi.position = pos
	mi.rotation_degrees = rot_deg
	mi.scale = scl
	mi.material_override = mat
	parent.add_child(mi)
	return mi


static func box(x: float, y: float, z: float) -> BoxMesh:
	var b := BoxMesh.new()
	b.size = Vector3(x, y, z)
	return b


static func cylinder(r_top: float, r_bottom: float, h: float, segs := 12) -> CylinderMesh:
	var c := CylinderMesh.new()
	c.top_radius = r_top
	c.bottom_radius = r_bottom
	c.height = h
	c.radial_segments = segs
	c.rings = 1
	return c
