# The highway — build order

*Written 2026-09-20, on `feat/highway-clean`. The spec is `docs/HIGHWAY_BRIEF.md`;
this is the order the work lands in and what each step gives you to fly.*

**Nothing here is a decision.** No ADR comes out of this document. Each step ends
with a thing on screen and a question for the human, and the answer to that question
is allowed to change every step after it. A step is done when `make check` passes
*and* the human has flown it.

---

## The one mechanism choice, and why it is forced

Both lessons in the brief point at the same thing, from opposite ends: *the mesh and
the collider must share one rule for where a wall is open*, and *a road is verified
by flying it against its own rendered surfaces*. The cheapest way to make both true
is to stop having two things.

**The wall list is the road.** A section of tube emits a list of quads — one per
wall it actually has. The mesh is those quads drawn. The collider is those quads
bounced off. An opening is a quad that was never emitted, so there is no rule about
openings to keep in sync; there is no second rule at all.

That gives the gate a check it could not otherwise make: the triangles in the
rendered `ArrayMesh` and the quads the ship bounces off are asserted to be **the same
set**. A wall you can see and cannot hit, or hit and cannot see, fails the build
rather than the flight.

It costs a per-quad test instead of a physics server. At the sizes here — tens of
quads near the ship — that is nothing, and it keeps the road inside the floating
origin with no bodies to re-anchor.

## One conflict, flagged rather than resolved quietly

`EXPLORATION_DESIGN.md` says the cross-section is *"a rounded lozenge rather than a
rectangle"*. The brief's one-sentence requirement says **square tubes**, and says to
judge every choice against that sentence first. So it is built square, and the
lozenge stays unbuilt until the human asks for it. Square is also what makes a
tileset a tileset: flat walls meet flat walls at a branch, and a lozenge does not.

Second, smaller one: the existing `exploration/lane_*`, `portal_*` and
`deck_separation` keys in `tuning.cfg` describe the *old* in-world road. They are
left alone and untouched; the new road gets its own `[highway]` section. Retiring
them is a later step's job, once something has replaced them.

---

## The steps

Each one ends in something to fly (`make fly`) or look at (`make shot`), and each is
one PR.

*Re-cut 2026-09-20 after the first flight.* The human's verdict on step 1: it works,
it is far too slow, and there is nothing in it worth testing — no ramps, no exits, no
opposing lane, no other ships. Two lessons. The cruise drive belonged in step 1, not
step 4; without it the road is a corridor at taxi speed. And steps were cut too thin:
a straight tube proves the mechanism and gives a flight nothing to judge. So the steps
below are fewer and each carries a whole road-shaped thing.

### Step 1 — One tube, the walls push back, and the cruise drive ✅

A straight run of square tiles in its own harness scene; walls you bounce off; the
wall list is the road (drawn and collided off one array). The cruise drive runs inside
the road and nowhere else, spooled from hull speed over `highway/cruise_spool_seconds`
to `exploration/cruise_speed`, and a fighter — no cruise drive — flies it at its own
speed. `Mothership` has two new channels: a decaying knock from the walls, and the
cruise ceiling the road raises. Neither can turn the ship.

### Step 2 — A road with somewhere to go  ← *next*

The step that makes the road worth driving. One PR, because every piece is only
judgeable next to the others:

- **It bends and climbs.** Each tile gets a yaw and a pitch; the road is a list of
  them. The seam between tiles is open because neither tile makes a wall there, so
  *connect* is the same rule as *open*.
- **Both carriageways and the median**, traffic on the right, in one tunnel shape, so
  there is an opposing lane to look across.
- **Exits and entrances.** An off-ramp is a branch — a side wall that is not there,
  opening onto a spur tile that peels away — and an on-ramp is the same shape merging
  in. Several along a longer road, so a missed exit costs the next one.
- **The camera held to the road's direction** while cruising (`EXPLORATION_DESIGN.md`,
  locked), which only makes sense once the road bends.

In this step the ramps end in open space in the same world. Crossing into the
wormhole is step 4.

**Fly:** on at one ramp, down a bending road, off at a chosen exit, back on the other
carriageway. Clip the lip of an exit on the way past.

### Step 3 — Interchanges

The brief defers interchanges in favour of *"taking one's off-ramp and the other's
on-ramp"*. With step 2's ramps that is already buildable: two roads, and a system
where one's exit sits by the other's entrance. Built that way unless the human wants
the deferral lifted and a junction built inside the tunnel instead.

### Step 4 — The wormhole, and the ramps in both worlds

The streak cylinder that rides with the ship and closes to a throat ahead; the road
the slow part and the streaks the speed. Each ramp placed twice — once against its
carriageway, once in open space with its mouth at the planet's — and the ship carried
across partway along it, keeping its place in the ramp, its speed and its heading.

### Step 5 — Wired to the map

Highway nodes at the map's bearings, each leg a tuned fraction of the world leg,
dead-ending a short run past the first and last systems. A to B on the road against A
to B by hand, back to back.

### Not in any step: traffic

The brief defers traffic: *"Traffic is not in this prototype; the tunnel is shaped for
it."* The human listed other ships among what step 1 was missing. Until that deferral
is lifted, the opposing lane in step 2 is empty.

---

## How each step is verified

- `make check` — every step extends it in the same PR. Step 1 adds: the mesh-equals-
  collider set check, the bounce reflection arithmetic, a headless flight down the
  road asserting the ship is never outside the drawn surfaces, and the cruise drive
  spooling on the road and off it.
- `make fly` — the harness scene, a HUD row per thing that could be wrong.
- `make shot SCENE=res://tools/shots/highway_shot.tscn` — a frame, so a visual change
  is checked here before it is handed over.

## Deferred, and staying deferred

Traffic (above), the comms channel, signs and the map inset, fuel per hop,
interchanges as junctions inside the wormhole, what is past the last exit. All
listed in the brief; none of them are in these five steps.
