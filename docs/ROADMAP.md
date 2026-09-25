# Roadmap

*Written 2026-09-24. Order, not detail. Each step ends in something that can be
played or run.*

1. **Travel foundation.** Done, as the prototype this repo was rebuilt from. See
   `WORLD_AND_TRAVEL.md`.

2. **Simulation architecture (design).** How the world exists without being
   rendered: factions, named characters, fleets, stations, markets. How fleets move
   on the map's graph of Lattice roads, hop lanes and open-space sectors. How the
   economy produces, consumes and moves goods on actual ships. How an entity is
   handed to the rendered world near the player and taken back when they leave.

3. **Headless simulation prototype.** Run that world with nothing rendered, many
   times faster than real time, and check it behaves: trade flows, borders move
   without one faction snowballing, fleets persist and go somewhere.

4. **Traffic from the simulation.** Replace the prototype's cosmetic traffic with
   rendered simulation fleets, in open space and on the Lattice.

5. **Combat feel prototype.** Flying the munitions and blockers, built fresh against
   this world's scale and speeds. Turrets and hired gunners as the other way to
   fight.

6. **The playstyles, one at a time.** Each of `VISION.md`'s paths made playable end
   to end: merchant and escorts, explorer, warfighter with a faction, summoner, and
   casual missions.

Carried alongside: moving feel values out of code into a tuning file that can be
edited while the game runs, once there are enough of them to justify it.
