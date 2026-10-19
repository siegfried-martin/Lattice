# Roadmap

*Reset 2026-09-27; updated 2026-10-19.*

Done so far: the travel foundation (`WORLD_AND_TRAVEL.md`), a first combat prototype
(`COMBAT.md`), and four rounds of design, now formalised as the spec (`docs/spec/`).

**Current step:** the headless simulation, built in C++ in the order of
`docs/spec/sim/milestones.md` (MS-1 to MS-9): every v1 system first, then tuning until
the world is stable (Phase A), then experiments testing whether each mechanic earns its
cost (Phase B). Connecting the simulation to the rendered game follows.

MS-1, the skeleton and toolchain, is done: the core, runner, tests, saves and a Godot
binding the project loads. MS-2 to MS-7 are built: world generation and the economy,
characters, movement, information and trade, politics, war, piracy and suspicion, and
spies. Every v1 system runs. MS-8 is in progress: the experiment harness is built, and
Phase A has had four rounds. Round 2 added war pacing, rebellion and gate blockades.
Round 3 rebuilt happiness and productivity on a signed scale, eight regional and fringe
luxuries, rebellion as a weekly chance, garrisoning as a choice, and defence sized to
income. Round 4 made production cost money, with efficiency as that cost, and moved
budgets and tribute to net income; its results and what still fails (merchants
squeezed, so idle characters and pirates are too many) are in `sim/milestones.md`
(MS-8).

Next is designed but not built: a round on paying for war and fealty. Round 4 showed war
costs a treasury almost nothing and patronage outweighs tribute, so the round adds
bounties for kills paid by the root, spoils of capture, fleet loans between governors,
commanders choosing their own objectives, tribute relief for poor vassals and reputation
in stipends, tuned against target bands written first (CHK-26). The human is settling a
question about character roles with the planning agent first (OPN-42). The economic
round (starvation and the merchant squeeze) follows.
