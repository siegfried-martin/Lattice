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

### Step 1 — One tube, and the walls push back  ← *building now*

The tile, the chain, and the bounce. A straight run of identical square sections in
their own harness scene, the ship at one end, walls you can bump.

- `HighwayQuad` / `HighwaySection` / `HighwayShell` in `scripts/lib/` — pure, no
  scene tree. A section is a box with four walls and two open ends; the shell is
  every wall of every section, and resolves a moving sphere against them.
- `HighwayRoad` in `scripts/world/` — builds the sections from tuning, draws the
  shell's own quads, and constrains the ship after it has moved.
- `Mothership` gets one new channel: an external velocity that decays. The wall may
  move the ship and may push back; **it may never turn it.** Heading stays the
  player's (ADR 0012, magnitude never direction).
- `scenes/highway.tscn` + `make fly`, an instrument HUD, and a `make shot` harness.

**Fly:** `make fly`. Down the tube, into a wall at a shallow angle, into one head-on,
along one while holding throttle.
**I will ask:** how big the tube should be against the ship, and what a bounce should
do — kick, scrape, or stop.

### Step 2 — It connects: the road curves and climbs

The tile gains a turn: each section's exit frame is its entry frame advanced and
rotated by a yaw and a pitch. A road becomes a list of those deltas. The seam between
two sections is open by construction — neither emits a wall there — so *connect* is
the same one rule as *open*.

**Fly:** a road that bends and climbs, and the same wall test through a bend, where
the inside wall is the one that catches you.
**I will ask:** how tight a bend is too tight, and whether curvature is the thing
that makes a straight road worth driving.

### Step 3 — It branches

A section may open a side wall onto a spur. Same declaration, same mesh, same
collider: a branch is a wall that is not there. This is the step where the resolver
stops being trivially correct, and where the first road's bug lived — so it gets a
gate check that flies through the opening and along the lip of it.

**Fly:** take the branch, miss the branch, clip the corner of it.
**I will ask:** whether a branch is legible from far enough back to choose.

*After step 3 the one-sentence requirement is met: a tileset of square tubes that
connect and branch, with bounce collision on the walls. Everything below is the
place it sits in.*

### Step 4 — Two carriageways, and the median

The road becomes a pair in one tunnel, traffic on the right, median between
(`EXPLORATION_DESIGN.md`, locked). The cruise drive works here and the camera is held
to the road's direction.

**Fly:** the length of it, and look across the median at where the other side's
traffic will be.

### Step 5 — The wormhole around it

The streak cylinder that rides with the ship and closes to a throat a tuned distance
ahead, nearer than the concept art. The road is the slow part; the streaks are the
speed.

**Look:** `make shot` for the throat; fly for the streaks.

### Step 6 — The ramps, in both worlds

One ramp shape, placed twice: once against the carriageway inside, once in open space
with its mouth at the planet's. The crossing happens partway along it, carrying
position-in-ramp, speed and heading. No fade, no cut.

**Fly:** on at one end and off at the other, and out again from the middle of a ramp
to see what it looks like from the wrong side.

### Step 7 — Wired to the map

Highway nodes at the map's own bearings, each leg a tuned fraction of the world leg,
dead-ending a short run past the first and last systems.

**Fly:** A to B on the road against A to B by hand, back to back.

---

## How each step is verified

- `make check` — every step extends it in the same PR. Step 1 adds: the mesh-equals-
  collider set check, the bounce reflection arithmetic, and a headless flight down
  the road asserting the ship is never outside the drawn surfaces.
- `make fly` — the harness scene, a HUD row per thing that could be wrong.
- `make shot SCENE=res://tools/shots/highway_shot.tscn` — a frame, so a visual change
  is checked here before it is handed over.

## Deferred, and staying deferred

Traffic, the comms channel, signs and the map inset, fuel per hop, interchanges as
junctions inside the wormhole, what is past the last exit. All listed in the brief;
none of them are in these seven steps.
