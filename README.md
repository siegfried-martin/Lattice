# Lattice

A space sandbox set in a living simulation: factions at war over territory, a real
economy moving real goods, and named characters whose fleets persist whether or not
you're watching. Play it your way, as a trader, explorer, mercenary, fleet commander
or casual pilot, and the world responds. Its signature is combat where you **fly the
missile** yourself, and **the Lattice**, a wormhole highway network that makes a vast
map reachable.

Rebuilt on 2026-09-24 from a travel prototype that got the speed and scale of space
right. The earlier build (working title Missile Rider / TerminalGuidance) is kept at
tag `legacy-v1`.

## Run

Godot 4.7 (`godot` on your PATH).

```
make run        # play
make check      # parse every script and build both worlds headless
make tour       # scripted fly-through with screenshots
make tour-combat  # scripted combat run with screenshots
```

## Controls

| | |
|---|---|
| Mouse | aim (the ship, the turret or the missile) |
| W / S | thrust / brake; on a missile, boost / slow-and-turn |
| T / G / X | pilot / turret (freighter) / fire a missile (from any station) |
| Left / right mouse | auto-cannon / blocker at the turret; auto-cannon / laser on the fighter; left detonates a missile you're flying |
| A / D | missile dodge; change lane while docked |
| P / O / I | release a target drone / hostile freighter / hostile fighter (testing) |
| Tab | next ship in sensor range |
| R | nearest enemy in combat, otherwise nearest ship; again for the second nearest |
| Numpad 1 / 2 | fly the freighter / fighter (prototype shortcut) |
| C | dock to / undock from the road, on the Lattice |
| Esc | release the mouse |

Fly through a **green** frame to enter the Lattice (freighter only), an **amber** one
to leave it, and a **violet** one to ride a hop lane.

## Documents

| | |
|---|---|
| `docs/VISION.md` | what the game is, the playstyles it supports, how it gets built |
| `docs/WORLD_AND_TRAVEL.md` | the world's layout and how travel works and feels (design of record) |
| `docs/COMBAT.md` | the combat prototype: stations, weapons, enemies |
| `docs/ROADMAP.md` | what comes next, in order |
| `CLAUDE.md` | working notes for AI-assisted sessions |
