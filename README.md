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
```

## Controls

| | |
|---|---|
| Mouse | aim (the ship turns toward the reticle) |
| W / S | thrust / brake |
| Tab | swap fighter / freighter (prototype shortcut) |
| C | dock to / undock from the road, on the Lattice |
| A / D | change lane while docked |
| Esc | release the mouse |

Fly through a **green** frame to enter the Lattice (freighter only), an **amber** one
to leave it, and a **violet** one to ride a hop lane.

## Documents

| | |
|---|---|
| `docs/VISION.md` | what the game is, the playstyles it supports, how it gets built |
| `docs/WORLD_AND_TRAVEL.md` | the world's layout and how travel works and feels (design of record) |
| `docs/ROADMAP.md` | what comes next, in order |
| `CLAUDE.md` | working notes for AI-assisted sessions |
