# Combat: the first prototype

*Written 2026-09-24. Rebuilt from the concept alone (fly the munitions, blockers,
turrets, NPCs that are beatable), against this world's scale. Numbers live in
`Combat.T` in `scripts/combat.gd` and are all expected to move.*

## Stations

In open space the player holds one job at a time. While they're away from the helm,
the ship holds its heading and speed.

| Key | Station | What the view does |
|---|---|---|
| **T** | Pilot | chase camera behind the ship (the travel view) |
| **G** | Gunner (freighter only) | first person from the roof turret. The mouse aims it directly and the turret head turns with it. |
| **X** | Missile (from any station) | chase camera behind the missile until it ends, then back to the station you fired from |

Weapons are off inside the Lattice and on hop lanes. Entering either ends a missile in
flight.

## Weapons

**Auto-cannon.** Left mouse, 2 rounds per second. On the freighter it fires from the
turret wherever you aim. On the fighter it fires straight ahead from the nose. Rounds
also knock down enemy missiles.

**Blocker (freighter turret).** Right mouse. A slow canister that opens into a cloud
of pellets after a moment. Any missile that touches a pellet is caught. It's quick
enough to put in front of an incoming missile, and slow enough that a missile you're
flying can dodge an enemy's. The cloud fills its volume unevenly, so the gaps are
worth aiming for.

**Laser (fighter).** Right mouse, held. Short range and much higher damage, straight
ahead. It builds heat while firing. At full heat it locks out until it has cooled most
of the way.

**Missile.** X from any station, one in flight at a time, then a reload.

- The mouse steers: the missile turns toward the reticle at its turn rate.
- **W** boosts: faster, but it turns more slowly. Each missile has a short boost
  meter that doesn't refill.
- **S** slows the missile and lets it turn tighter.
- **A / D** dodge: a quick sideways jump. The camera takes up most of the jump at
  once and lags the rest, so you see the missile move to the side and drift back to
  centre.
- It has a fuse. When the fuse runs out, the missile ends.
- **Left click** detonates it early. Ships whose hull is inside the blast radius
  take some damage, most at the hull and less toward the edge, and always well short
  of a direct hit. It's a way to salvage a near miss.

## Enemies (demo keys)

| Key | Ship | Behaviour |
|---|---|---|
| **P** | Target drone | Released ahead at 75% of freighter speed on a random straight line. No weapons. |
| **O** | Hostile freighter | Holds a standoff and circles. Fires one missile 10–20 s after appearing, and one blocker at the first missile you send at it. No guns. |
| **I** | Hostile fighter | Same speed and weapons as the player's fighter. Makes attack runs and breaks off when close. |

NPCs are deliberately beatable. Their aim wanders a few degrees off, they react late,
and enemy missiles lead their target less than they should. Later, some NPCs will be
better or worse at particular jobs, and the same goes for hired crew.

When an enemy missile is inbound, the HUD flashes **MISSILE INBOUND** with the range
and marks the missile, so there's time to get to the turret.

## Sensors and targeting

Ships exist whether or not they're drawn. Only those within **sensor range** (4 km)
are rendered, shown on the radar and targetable. Traffic beyond it is handed back
(removed) once it's well outside. Ships in active combat stay in play beyond it. For
now that means every combat test ship, and later it will also cover NPCs fighting each
other.

| Key | Does |
|---|---|
| **Tab** | next ship in sensor range, nearest first, wrapping round. Any ship. |
| **R** | in combat, the nearest enemy; otherwise the nearest ship. Press again for the second nearest, so repeated presses flip between the two closest. |

Combat means a hostile ship on sensors or an enemy missile in flight. The target gets
corner brackets in the view (an edge arrow when it's off screen) and an info panel:
name, class, stance, range, speed, closing speed, hull. The target is dropped when it
leaves sensor range or is destroyed, and when the player enters the Lattice or a hop
lane or switches ship.

This is deliberately close to Escape Velocity.

## HUD

- **Speed meter (left):** speed as a share of top speed. Engine glow follows it:
  dark when stopped, brightest at full speed.
- **Status bars (bottom):** hull; missile ready or reloading, or boost and fuse while
  flying one; blocker ready at the turret; laser heat on the fighter.
- **Radar (top right):** a top-down view of sensor range with the ship's heading
  always up, like a navigation map. Hostiles are red, test drones amber, traffic
  grey-green. Missiles show as dots, yours blue and enemies' red. The target is
  bracketed.
- **Target panel (beside the radar):** see above.
- **Combat ships** on sensors are marked with range and a health bar.

More will be added to the HUD. It isn't meant to be balanced yet.

## Not decided yet

- Whether the fighter should carry missiles. For now both ships do.
- Whether you can shoot down your own missile's blockers (you can't at the moment),
  and whether blockers hurt ships (they don't).
- Losing the ship: for testing, the hull is restored when it runs out.
- Whether a missile blast hurts the player, knocks down missiles or clears blockers
  (it doesn't yet), and whether the fuse running out should detonate it too (it
  fizzles).
- Whether the radar should show height (it's flat for now), gates and bodies.
- Whether the target should feed the weapons, e.g. a missile reticle or a lead
  indicator.
- Whether sensor range varies by ship class.
