# Milestones (MS)

The build order. Every milestone is built headless with tests, and each lands on a
development-size world (GEN-1). **Nothing is judged until all of them run** (SIM-2): the
runner's metrics exist from MS-2 to catch bugs, not to tune.

### MS-1 Skeleton and toolchain `v1`

CMake core library, runner, doctest with sanitizers, config loading, the event queue and
clock (TIM-1, TIM-2), metrics output, versioned saves, and a minimal godot-cpp binding
(`advance`, a snapshot) loaded by the Godot project to prove the toolchain (ARC-4).

*Done 2026-10-18.* The world has no systems yet: a daily cadence event and the
`debug.pings` scaffolding exercise the queue, the RNG streams and saves. Tests cover
event order, RNG streams, config errors, save round trips and rejection, determinism,
and that splitting an advance changes nothing.

### MS-2 World and economy `v1`

World generation (GEN-1 to GEN-7), population, production, labour, markets and prices,
events, treasuries and operating costs, osmosis (SYS-1 to SYS-6). Planets with no
characters yet: osmosis is the only trade.

*Built 2026-10-18.* World generation makes the map, regions, systems, groups, the
Lattice, hop lanes, what planets make, the seats of power (for fuel prices) and fuel
prices; each planet runs its daily economy, weekly labour shift and monthly costs; world
events and osmosis run daily. The runner writes the map and per-planet, per-good and
world series. Tests cover every generation promise over many seeds at both sizes, the
SYS-1 to SYS-6 formulas, and that money and goods add up over a year (MOD-12).

*Found:* with the starting values the world can't feed itself, so populations shrink
from the first day. A thousand people need 3 units of life support a day; at most they
make `prod.labour_per_capita` × productivity, 3.5 × 0.9 ≈ 3.2, and only if all their
labour is at high efficiency. Prices then climb to `price.max`, happiness falls to 0
and productivity to `pop.p_base`. Planets missing a life-support good live on osmosis
alone. Not tuned here: merchants (MS-3) change the picture, and tuning waits for Phase A
(SIM-2).

### MS-3 Characters, movement, information and trade `v1`

Characters and generation (SYS-18), ship variants, running costs and fuel (SYS-21),
Lattice transit and open-space movement with encounters (TIM-4 to TIM-6), facts and
sharing (SYS-15), independent and faction merchants (SYS-7), missions and careers
(SYS-8), contracts (SYS-9), automated couriers.

*Built 2026-10-18.*
- **What runs:** characters spawn toward `char.target` and generate ship catalogues
  (MOD-1). They move along routes over open space, the Lattice and hop lanes, paying
  fuel and running costs. They observe markets and share facts within their faction.
- **Work:** they trade as independents or salaried faction merchants (all three trip
  shapes), take courier and scout missions, buy ships with their savings, and sell them
  when broke. Automated couriers carry facts down and up the fealty trees.
- **Moved in:** fealty trees (GEN-8) and governors (part of GEN-9) came forward from
  MS-4, because faction merchants, sharing and couriers need them.
- **Moved out:** escorts and sightings move to MS-5, where risk and armies first exist.
- **Speed:** a v1-size world with 2,000 characters runs a year in about 7 s.
- **Found:** merchants move more life support than the planets make (goods change hands
  more than once) but can't fix MS-2's structural shortage: populations still shrink
  under the starting values. With `prod.labour_per_capita` at 7 a world settles at about
  60% of its start. Happiness then flips between 0 and 1 across planets: the happiness,
  productivity and budget loop reinforces itself. Both are for Phase A.

### MS-4 Politics `v1`

Fealty trees and starting factions (GEN-8, GEN-9), relationships and deeds (SYS-10),
tribute, patronage and defection (SYS-11), the governor's budget model (SYS-5), the Trade
Authority and dues (SYS-20).

*Built 2026-10-18.*
- **What runs:**
  - opinions with deeds and fade;
  - tribute with the cascade;
  - the liege's tribute tools;
  - defection to a rival or to independence;
  - the weekly budget over commanders and patronage;
  - commander hiring and stipends;
  - starting commanders sworn to every governor (GEN-9);
  - Authority dues.
- **Not yet:** keep-or-grant and choosing a governor need capture (MS-5).
- **Seen in a peaceful run:**
  - tribute flows up the trees and patronage reaches vassals in the red;
  - loyalty rises with patronage;
  - commanders follow income: hired when governors are rich and let go as populations
    (and incomes) shrink under the starting values;
  - nobody defects: only war deeds push opinion that low.

### MS-5 War `v1`

Diplomacy (SYS-12), combat resolution (SYS-14), escorts, armies, invasions, sieges,
calls for help, relief and capture with keep-or-grant (SYS-13), scouts, planetary
defence and security missions, risk (SYS-19).

*Part 1 built 2026-10-18.*
- **What runs:**
  - diplomacy: the war vote, suing for peace, ceasefire tribute, and what war changes
    (osmosis, faction trade, gates);
  - battles in rounds, with pods;
  - static defence bought through the budget;
  - invasions as a budget option, with muster, siege, calls for help and relief;
  - capture with keep-or-grant and a new governor.
- **Tests caught two bugs:**
  - a merchant re-planning the same doomed trip within one instant;
  - leftover cargo relabelled as a new good.
  A sanitizer run caught a contract ended twice.
- **Seen:**
  - In development worlds, conquest is fast: under the starting values one or two
    factions remain after two to three years, and populations collapse as before.
  - A v1 world keeps five factions after three years, with reliefs breaking about one
    siege in seven.
  - CHK-1 and CHK-6 are for Phase A.
*Part 2 built 2026-10-18.*
- **What runs:**
  - risk per sector, with beliefs;
  - raids at war, engaging in open space, with loot;
  - escorts hired by the expected-loss formula;
  - threat scouting sorties; sightings, warnings and garrisons;
  - security patrols;
  - war and peace travelling as facts (DIP-5).
- **Bug caught:** an optimised ASan run caught an army replanning from mid-flight when a
  garrison's time ran out during a battle.
- **Seen in five development years:** about 800 raids, 6,000 escorts hired, 300 warnings,
  17 garrisons and 27 reliefs.

### MS-6 Piracy and suspicion `v1`

Pirate planets and flips, broke commanders, raiding and loot, pirate missions and
smugglers, the pirate relation, contraband in lawful markets, policing and suspicion
(SYS-16, SYS-17).

*Part 1 built 2026-10-18.*
- **What runs:**
  - pirate planets: flips, contraband production, low costs, no dues, recapture;
  - starting pirate planets with crews;
  - the pirate faction's hostility, and its exclusion from the Lattice;
  - pirate raids, loot sold at home;
  - broke commanders turning pirate;
  - the pirate relation;
  - Authority clearing contracts.
- **Changed while building:**
  - Pirate planets were made known: flips are seen nearby, and a landing shows the whole
    system. Otherwise no broke commander could ever find one.
  - Pirates were given raiding reach (`pirate.range_days` 20); at 6 days of open space
    few havens had a target.
  - An engagement cooldown was added: a raider holding station had re-engaged hundreds
    of times.
  - Invasions wait out the warm-up, like declarations.
- **Seen over five development years:** pirates take hold in some seeds, not others. In
  seed 3 one fringe haven persists, with 14 broke commanders joining it and 50 pirate
  sorties.
*Part 2 built 2026-10-18.*
- **What runs:**
  - pirate missions (smuggling and legal goods), offered on contact to bold independents;
  - contraband sold untaxed to lawful populations, with its productivity cost;
  - suspicion and inspections, with confiscation;
  - policing as a budget option.
- **Seen in seed 3 over five years:** about 400 offers, 67 legal-goods and 15 smuggling
  runs, 4,400 units of contraband reaching lawful populations, and 4 smugglers caught.

### MS-7 Spies `v1`

Spies (INF-13, SYS-17). Last because suspicion already exists for smuggling, so a spy
only adds the role and what it can learn behind enemy lines.

*Built 2026-10-18.*
- **What runs:**
  - spies hired through the budget at war, on secret contracts;
  - rounds to enemy markets with cover trades;
  - sightings of armies forming, which reach home and raise warnings;
  - blown covers, with an attack;
  - EXP-4's switch.
- **Seen in seed 1 over five years:** spies saw about 320 musters, and warnings and
  garrisons rose with them. Hiring churns as budgets move: about 5,300 hires for about
  100 posts, and 680 caught.

### MS-8 Full scale and Phase A `v1`

v1-size worlds, the parallel experiment harness (many worlds across cores), plotting,
and tuning until the checks pass (`checks.md`).

*In progress (2026-10-18).*
- **Built:**
  - the harness (`sim/tools/experiment.py`: seeds and config variants across cores);
  - the checks (`sim/tools/checks.py`, with proposed bands);
  - an HTML report with charts (`sim/tools/report.py`).
  The tools use only the standard library.
- **Bugs caught:**
  - a fealty cycle: a planet that defected to its besieger was captured anyway, so its
    keeper ended up beneath it, and the war vote looped forever;
  - a partly written CSV broke the checks script.
- **Round 1** (v1 size, 8 seeds, 5 years), against the baseline:
  - `prod.labour_per_capita` 3.5 → 8: the world couldn't feed itself;
  - war-vote weights cut (strength 1 → 0.1, targets 0.3 → 0.05, idle 0.2 → 0.1,
    threshold 1 → 1.5): the unbounded strength ratio had every faction declaring on
    every smaller neighbour.

  | | Baseline | Round 1 |
  |---|---|---|
  | Population, end over start | 7% | 116% |
  | Planets changing hands a year | 139% | 26% |
  | War declarations per faction a year | 19 | 2.8 |
  | Pirate flips a year | 12% | 1.6% |
- **Still failing:**
  - money grows about 40% a year (CHK-7);
  - some war is on most of the time (CHK-8);
  - pirates and smugglers are now scarce, because they came from collapse (CHK-10,
    CHK-23, CHK-24);
  - fringe planets grow slower than the network (CHK-11).
- **Coupled:** raising operating costs fixes money growth but sets off flips and wars.
  The next round needs a view on which outcomes are intended.
- **Round 2** (design changes agreed with the human, then tuning; v1 size, 8 seeds, 5
  years):
  - design: war pacing (weariness, the winner's hope, money votes for war), hunger and
    rebellion (pirate or secession), and gate blockades (DIP-3, DIP-4, ECO-13, MIL-17);
  - tuning: `value.aftermath_months` 2 → 8, `defence.per_spend` 0.005 → 0.01,
    `value.blockade_months` 8, `rebel.shortfall` 0.5 and `rebel.days` 90, and the
    conqueror garrisoning its prize (`capture.hold_days` 30);
  - bug caught: in scoring gates, a flag shadowed the army's attack power, so every gate
    looked unwinnable.

  | | Round 1 | Round 2 |
  |---|---|---|
  | Population, end over start | 116% | 95% |
  | Planets changing hands a year | 26% | 22% |
  | Each faction's time at war | n/a | 62% |
  | Median war | 7 days | 71 days |
  | War declarations per faction a year | 2.8 | 2.9 |
  | Blockades per faction a year | n/a | 3.7 |
  | Pirate flips a year | 1.6% | 5.9% |
  | Pirate-planet time on the fringe | 47% | 93% |
  | Pirates, share of characters | 0.6% | 1.6% |
  | Money supply growth a year | 1.39 | 1.34 |
- **Still failing after round 2:**
  - money growth (CHK-7). It's structural: a planet keeps what its own population pays
    for its own goods, about 79% of that spending; operating costs take about 21% and
    the budget spends about 1%, because defence units are fixed amounts (1,000 credits)
    against incomes near 1.4 million a month. Higher operating costs push poor planets
    into failure while rich ones keep piling up;
  - planets changing hands (CHK-6, 22% against at most 20%) and pirate flips (CHK-9,
    5.9% against at most 5%), each passing on a quarter of seeds;
  - fringe growth (CHK-11, 0.40), price dispersion (CHK-14, 0.67), and smuggling, which
    runs in three-year runs but fades to almost nothing over five (CHK-10, CHK-24).
- **Found after round 2:**
  - **A poverty trap in the constants.** Purchasing power is `econ.wage` 40 × P a day per
    thousand, with P from 0.6 (unhappy) to 1.2. Life support at reserve prices costs 30
    (3 goods × 1.0 × 10). At P 0.6 a planet can't feed itself, so it starves and stays
    unhappy: 55% of planet-samples sat at happiness 0 and 33% at 1.
  - **A knife edge:** at full luxury use, daily joy (0.5 × 0.02) equals the drain (0.01).
  - **A luxury glut:** about 90% of planets make one of the three luxuries, and luxury
    prices sit at the floor (0.2 × base) nearly everywhere, fringe included.
  - Under ECO-1, more luxury buying creates money rather than sinking it; luxury trade
    moves money from consumers' treasuries to merchants and producers. The sinks are
    military, fuel and ships.
- **Round 3 plan** (agreed with the human 2026-10-18; spec written, not built), on
  branch `feat/sim-round-3`, in this order:
  1. **Happiness and productivity** (PRD-4, PRD-5, PRD-2, SYS-1): signed H with 0
     neutral, settling toward a target, per-luxury log returns, consumption falling as
     H rises, F(H) and G(H) through tanh with the productivity floor, capture as a drop
     of `capture.happiness_drop`, `gen.start_happiness` 0. Contraband demand's
     `(1 − H)` becomes u of unhappiness (SYS-16). Retire the old `pop.*` and `lux.*`
     keys. Check Planet fields (happiness's meaning changes; the save version bumps).
  2. **Eight luxuries** (PRD-1, PRD-8, GEN-6): six regional (wine, spice, silk, tea,
     perfume, glassware: placeholders) and two fringe (crystal, amber: placeholders).
     `Good` and `kLuxuries` grow; `goods` defaults, the market, osmosis, facts' price
     arrays and the save follow. GEN-6's regional weights and repair rule as written.
     Luxury reserves rise with unhappiness (`happy.want`, SYS-3).
  3. **Rebellion as a weekly chance** (ECO-13, SYS-16): `rebel.scale`, `rebel.power`,
     `rebel.w_debt` replace `rebel.happiness`, `rebel.shortfall`, `rebel.days` and the
     deficit counter's role in failure (Planet.unrest_days goes).
  4. **Garrisoning as a choice** (MIL-12, SYS-13): hold or go home at capture.
  5. **Defence sized to income, and reserves** (MIL-5, SYS-5): `value.security_share`,
     `budget.hoard`; move `dip.reserve_months` and `dip.treasury_cap` to `budget.*`.
     Retune `defence.per_spend`.
  6. **Checks** (`checks.py`): CHK-6 band 0.05 to 0.4 and CHK-6c (the most-traded
     planet's captures a year, at most 2, from `planet.captured` events); CHK-7 as
     second-half growth at most 1.1 and median treasury months levelling off; CHK-12c
     (happiness at the ends at most 20% of planet-days; fringe median below the
     network's, above the floor). The runner may need series for treasury months or F.
  7. **Run round 3** (v1, 8 seeds, 5 years), compare with round 2, tune, report.

### MS-9 Phase B `v1`

The experiments (`experiments.md`), and the spec changes that follow from them.

After MS-9 the roadmap continues with connecting the simulation to the rendered game
(ARC-10, ARC-13).
