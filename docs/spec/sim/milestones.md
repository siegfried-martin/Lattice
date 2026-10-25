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
- **Round 3, built** (`feat/sim-round-3`): steps 1 to 6 as planned, then tuning on 4 seeds
  over 3 years, then the round 3 run (v1, 8 seeds, 5 years). What the runs found, and
  what changed (DECISIONS):
  - First run: luxuries glutted (made 10 times faster than used, every price at the
    floor), so the network sat near the happiness ceiling and fringe planets starved.
    Fixed by a per-good `good.yield` (regional luxuries 1/6, fringe 1/3) and fringe
    planets weighting life support × 3 (`gen.fringe_ls_mult`).
  - Rich planets overspent on security into debt (a unit's value grew as income
    squared), and debt drove rebellions on happy network planets. Fixed by valuing a
    unit at `value.security_ratio` × its cost × threat × reserves.
  - `rebel.scale` 0.007.

  | Measure | Round 2 | Round 3 |
  |---|---|---|
  | Each faction's time at war | 62% | 64% |
  | Median war | 71 days | 66 days |
  | Planets changing hands a year | 22% | 42% |
  | Most-traded planet's captures a year | 1.9 | 2.3 |
  | Conquests held by their army | all | about 30% |
  | Blockades per faction a year | 3.7 | 1.9 |
  | Pirate flips a year | 5.9% | 6.8% |
  | Secessions a year | 0.8% | 2.0% |
  | Pirate-planet time on the fringe | 93% | 68% |
  | Money growth a year, second half | 1.17 | 1.05 |
  | Median treasury months, end over halfway | 1.12 | 0.93 |
  | Population, end over start | 95% | 126% |
  | Happiness samples near either end | 67% | 5% |
  | Median happiness, network and fringe | (0 to 1 scale) | +0.5 to +0.7; fringe −0.7 to +0.4 by world |
  | Luxury prices, median over base | 0.2 (the floor) | 0.8 to 1.4; fringe 2.4 to 3.6 |
  | Life-support prices, median over base | | 0.24 (near the floor) |
  | Contraband reaching lawful populations a year | 0 | 465 |

  16 of 31 checks pass on every seed (round 2 under the same checks: 17, its happiness
  checks read on its 0-to-1 scale). Newly passing: CHK-8, CHK-12c, CHK-23. Newly
  failing on some seeds: CHK-6 (0.42, band to 0.4), CHK-8b, CHK-12b, CHK-20b.
- **Still failing after round 3:**
  - Life support is overproduced: its prices sit near the floor everywhere, and price
    dispersion (CHK-14, 1.16) comes from that floor against the cap at planets that
    lack a good. Lowering `prod.labour_per_capita` eases it but starves the fringe.
  - The fringe still shrinks against the network over five years (CHK-11, 0.33), and a
    third of pirate-planet time is on the network (CHK-4, 68%): network planets that
    rebel are mostly fresh conquests (the capture shock) or in debt.
  - Flips (CHK-9, 6.8%), the most-traded planet (CHK-6c, 2.3), the price index (CHK-7b,
    1.6, inflation in some worlds), smuggling runs (CHK-24, 0.4).
  - Budgets read gross income (market receipts), not net of imports, so a big
    importer's budget overstates what it can afford (OPN-39).
- **Round 4** (agreed with the human 2026-10-18; built on branch `feat/sim-round-4`):
  the cost of making goods (PRD-7, ECO-5, SYS-2) replaces efficiency as output; slots
  idle when their bid doesn't cover their cost; labour moves by margin (PRD-9); events
  move a slot's cost a tier (SYS-4); CHK-14c. What the runs found, and what changed
  (DECISIONS):
  - With only that and `prod.labour_per_capita` 5, the world fell apart in two years:
    population to 0.54, flips 21% of planets a year. Three causes, fixed in turn:
  - **Debt.** About 40% of gross receipts now go on making goods, but budgets were
    sized on gross and tribute paid on it: 55% to 80% of planets ended in the red, and
    debt drove rebellion. The budget and tribute now read net income (OPN-39, SYS-5);
    war, capture value, credit and rebellion keep gross, because on net the war vote's
    targets term blew up (factions declared 8 to 9 wars a year). Rebellion counts debt
    in months of income (`pirate.flip_debt` 1). `econ.op_per_capita` 250.
  - **Growth.** Stocks rest where the bid meets the cost, 15% to 40% over reserve, so
    almost nothing grew against a whole reserve and nothing replaced war's losses.
    `pop.full_excess` 0.5.
  - **Distribution.** Merchants carried 87% of life support (round 3: 18%, with osmosis
    carrying the glut). In war their trips fell 60% and importers went short.
    `osmosis.rate` 0.2 (0.05 and 0.1 left populations falling over five years).

  | Measure | Round 3 | Round 4 |
  |---|---|---|
  | Each faction's time at war | 64% | 46% |
  | War declarations per faction a year | 3.7 | 3.0 |
  | Median war | 66 days | 49 days |
  | Planets changing hands a year | 42% | 33% |
  | Most-traded planet's captures a year | 2.3 | 2.9 |
  | Conquests held by their army | about 30% | about 40% |
  | Blockades per faction a year | 1.9 | 4.7 |
  | Pirate flips a year | 6.8% | 5.5% |
  | Secessions a year | 2.0% | 0.6% |
  | Pirate-planet time on the fringe | 68% | 96% |
  | Money growth a year, second half | 1.05 | 1.26 |
  | Price index, end over start | 1.6 | 1.23 |
  | Population, end over start | 126% | 94% |
  | Life-support prices, median over base | 0.24 (near the floor) | 0.55 to 0.65 |
  | Luxury prices, median over base | 0.8 to 1.4 | 1.0 to 1.2 |
  | Prices at `price.min` (CHK-14c) | 11% | under 0.01% |
  | Price dispersion (CHK-14) | 1.16 | 0.95 |
  | Merchant profit per trip | 1,510 | 116 |
  | Characters idle | 5% | 23% |
  | Pirates, share of characters | 1.6% | 15% |

  22 of 32 checks pass on every seed (round 3: 16). Newly passing on every seed: CHK-4,
  CHK-7b, CHK-8b, CHK-12, CHK-12b, CHK-14c, CHK-20b. Newly failing: CHK-17b and CHK-23.
- **Still failing after round 4:**
  - **Merchants are squeezed.** With prices resting on costs and osmosis carrying the
    bulk, a trip earns a tenth of what it did; a quarter of characters sit idle
    (CHK-17b) and the broke turn pirate, 15% of characters (CHK-23, band to 10%).
    Lower osmosis didn't fix it (11% to 16% pirates at 0.05 to 0.2) and let
    populations fall. This is the next thing to design: what merchants are for, once
    a glut no longer hands them margins.
  - **Money** grows 1.26 a year in the second half (CHK-7, 25% of seeds pass): debts
    written off when planets flip count as money created (ECO-13), and a few worlds
    create far more than they sink in their last years.
  - **The fringe** still shrinks against the network (CHK-11, 0.29): in some worlds
    its life support costs 2 to 5 × base and it loses most of its people.
  - Price dispersion (CHK-14, 0.95, band to 0.6), war share (CHK-8, 0.46 against 0.5
    to 0.8), flips (CHK-9, 5.5% against 5%), the most-traded planet (CHK-6c, 2.9),
    smuggling runs (CHK-24, 75% of seeds).
- **Where round 4's money goes** (eight seeds, second half, share of money created):
  making goods 45%, operating costs about 24%, the osmosis margin 15%, patronage 7.7%,
  tribute 4.5%, static defence 3.0%, ships bought 0.9%, stipends 0.3%, ship running
  costs and fuel 0.2%. Military spending of every kind is about 4%; raising an army
  moves no money; patronage paid down is 1.3 to 2.7 times tribute paid up; an invasion's
  counted value is on the order of a thousand times its counted cost. War can't drain a
  treasury, so CHK-8's cycle can't happen.
- **Where a run's time goes** (one v1 world, a year, profiled 2026-10-19): character
  decisions are about 60% of it, merchants planning trips most of that; the open-space
  step 18%. One function, `curve_at` (a merchant's believed price curve, with a `pow`
  per call), is 34% on its own and could be computed once per fact; weighing a salary
  plans the best trip twice. Sweeps already use every core, one seed each, so threads
  inside a world wouldn't make them faster; cheaper decisions would. No action yet
  (TIM-8 holds: about 15 seconds a year).

**Now: the roles and war round** *(roles designed 2026-10-22 and built 2026-10-24 as
stage 1; war and fealty money designed 2026-10-19 and built 2026-10-24 as stage 2; tuning
next)*. Governors who sail change who defends, who dies and who
decides, so they're built before any more balancing. Two building stages, then tuning
once, against targets written down first (CHK-26 to CHK-29):

*Stage 1, roles: the model.*
1. **One fitness function** for governors, stewards and commanders (CHR-20, SYS-22),
   replacing the hiring score and the governor score.
2. **Stewards** (FAC-16): appointed, paid, holding the planet's information while the
   governor is away; **one budget, two deciders** (CHR-4, SYS-5).
3. **Governors' own fleets** (MIL-20): seeded governors get ships; at home they defend;
   with a steward they join the armies they commission and answer calls.
4. **Succession** (FAC-17): governor and root deaths.
5. **The mission rules** (MSN-3): deadlines, failure and abandonment as a bad deed, and
   spend tracked per issuer.

Then check the world still runs and that round 4's checks haven't regressed, before
stage 2.

*Stage 1 results (built 2026-10-24; v1 config, 8 seeds, 5 years, against main at 7ec7152
on the same seeds; no tuning yet):*
- **It runs, and every seat stays filled.** 0.4 to 2.8 governors die a year; a vacant
  seat waits 5 days on median (CHK-29), and no run ends with one empty. Successions cost
  no defections within three months (CHK-29b). About 13 of 101 governors are away with
  their fleets at any time. Speed is unchanged (CHK-25).
- **Round 4's checks:** 20 of 32 pass on every seed, against 21 on main. Better: CHK-6
  (captures in band, 62% of seeds to 100%), CHK-8b (75% to 100%), CHK-9 (pirate flips,
  12% to 75%), CHK-17b (idle characters 23% to 16%, 12% to 88% of seeds) and CHK-24.
  Worse, each now failing on one or two seeds: CHK-4 (pirate planets on the fringe, 0.96
  to 0.72 of pirate-planet time), CHK-7c (treasury months, 0.73 to 0.56) and CHK-7b
  (price index, 1.23 to 1.33). Battles a year doubled (510 to 960), as every planet now
  has a governor's fleet to fight for it; deaths per battle fell. Pirates are 14% of
  characters (15%). Merchant profit per trip doubled (116 to 234).
- **The new checks:** CHK-27 passes on 75% of seeds (13% of governors came from trade);
  57% of stewards came from combat work (CHK-27b); 5% of missions fail (CHK-28).
  CHK-28b fails on 75% of seeds: two-thirds of pirate runs fail, nearly all through the
  failure paths they always had (nothing to load, caught on landing), which are only now
  counted. Governors' and roots' missions fail 3% to 4% of the time.
- **For tuning: stewards churn.** About 82 planets have a steward at any time, but 313 are
  appointed and 282 quit a year, so a tenure is about three months: at
  `steward.salary_share` 0.02 of net income, trade beats the salary at the first monthly
  check. Their pay, 8.1 million a year, is nearly twice every commander's stipend
  together (4.5 million), and is the likeliest cause of CHK-7c's lower treasuries.

*Stage 2, war and fealty money (designed 2026-10-19):*
1. **Bounties for kills,** paid by the root (MIL-18), and **spoils of capture** split by
   power × days in the siege (MIL-19).
2. **Fleet loans** in both directions (MIL-3, MIL-11), replacing the automatic call to
   arms; **commanders choose their objectives** as missions (MIL-2, MIL-9), governors
   with fleets among them (MIL-20), behind a config flag so a run with and without shows
   what autonomy alone does.
3. **Tribute relief requests** on fixed need tests (FAC-15), with **patronage narrowed**
   to rebuilding captured planets (FAC-7).
4. **Reputation in stipends** (MIL-2).
5. **Messages go one way** (INF-16): requests carry deadlines and defaults.

Watch: CHK-26's bands, the root's treasury at war (CHK-8), money growth (CHK-7), time
at war, captures, desertions (objectives declined), relief granted and refused,
governors killed and successions (CHK-29), and where governors and stewards came from
(CHK-27). Munitions (MIL-4) and MIL-3's liege paying for a whole campaign wait until
bounties show whether war is still too cheap.

*Stage 2 results (built 2026-10-24; v1 config, 8 seeds, 5 years, against stage 1 on the
same seeds; no tuning yet). EXP-15 is the same build with `objective.choose` off:*
- **It runs, and more checks pass.** 27 of 43 pass on every seed (stage 1: 24; EXP-15:
  26). Of round 4's 32, 22 pass on every seed (stage 1: 20).
  - Newly passing: CHK-7 (money growth 1.18 to 0.95 a year, 38% of seeds to 100%), CHK-7c
    and CHK-8 (time at war 0.48 to 0.66). CHK-6c is at 88% of seeds (from 25%).
  - Worse: CHK-9 (pirate flips 0.038 to 0.086 of planets a year, 75% of seeds to 0%),
    CHK-7b (price index 1.33 to 1.62, 75% to 38%), CHK-23 (pirates 14% to 16% of
    characters) and CHK-11 (the fringe, 0.29 to 0.18).
  - Speed holds: 14 days a second.
- **CHK-26 is nowhere near its bands yet.** These are the targets the tuning pass is for:

  | | Built | Band |
  |---|---|---|
  | Tribute paid up | 11.6% at peace, 11.5% at war | 20% to 30% |
  | Military | 11.7% at peace, 19.9% at war | 10% to 20%, 40% to 60% |
  | Patronage | 2.1% | under 5% |

  Patronage fell from 77 million a year to 7 million, now that it only goes to rebuilding.
  Tribute fell with it, from 53 million to 34 million a year: lieges raise tribute only
  when in the red, and they no longer go into the red paying patronage.
- **Bounties:** 4.7 million owed a year, 3.8 million paid. Roots end owing 4.6 million,
  about 1,500 unpaid tallies a year, each a bad deed.
  - Spoils are 2.2 million a year.
  - Stipends doubled (4.5 to 9.6 million a year), now with reputation in them; the
    split of that rise isn't measured.
  - Military spending of every kind is still well under tribute plus security.
- **Commanders mostly decline.** 4,700 objectives are declined a year, against 520 taken
  and 25 stays at home for defence. Each decline is `deed.objective_declined`, so mean
  loyalty fell from 7.6 to 1.9.
  - Captures fell from 29 a year to 20, and blockades from 3.9 a year per faction to 1.0.
  - Battles rose to 1,400 a year and deaths per battle halved; why isn't measured yet.
  - EXP-15 (everyone joins) has 37 captures, 5.2 blockades per faction, money growth of
    1.3 a year and patronage at 8% (more captured planets to rebuild).
- **Loans:** 320 a year, and 230 requests refused.
  - The fees, 3.9 million a year, are paid even for commanders who then decline, so a
    liege pays to borrow fleets that stay at its planet. With everyone joining (EXP-15)
    there are 75 loans a year and 0.3 million in fees.
  - Relief loans: 28 a year.
- **Tribute relief:** 62 requests a year, 33 granted, 24 refused; about 18 planets are on
  relief at any time.
- **For tuning:**
  - **Tribute:** `fealty.tribute_rate`, and when a liege raises it.
  - **What an objective pays against its risk:** `bounty.share`, `spoils.per_prosperity`
    and `combat.retreat_loss` in the expected loss. With them goes whether a gate's
    objective should be worth anything to a commander.
  - **The loan fee:** charged before anyone chooses. It could be charged only for those
    who join, which would be a rule change.
  - **What roots can afford** in bounties.
  - **Stewards' churn,** from stage 1.

**Later rounds from the roles design,** one area each:
- **The Authority's board and contract merchants** (MSN-7), as the response to the
  merchant squeeze (round 4: 23% of characters idle, 15% pirates).
- **Spies:** EXP-4 first; then spy missions and per-trip suspicion against stealth and
  detection (MSN-11, INF-14), if spies earn their place.
- **Spawned bounties** (MSN-6, MSN-10), with the hunt counting as patrol time.
- **Delivery top-ups** (MSN-8), only if tribute relief leaves poor planets unfed.

**Discussed, not decided: the economy after this round.** Starvation and merchants'
margins point at the same causes. In round 4 a full ration of life support would take
about a third of the median planet's budget (US households spend about a tenth on food),
and the poorest tenth would need 2 to 3 times their whole budget, so they starve on
price, not on stock; about 38% of purchasing power goes unspent, because luxury buying
is capped by how full the market is; and growth eats every surplus. Levers discussed,
for a later round with its own targets: life support cheap enough to be 10% to 15% of the
budget; a price curve steep only below the reserve; growth that follows happiness up to
a capacity per planet instead of food surplus; luxuries bought from whatever budget is
left. Governors capping population was discussed and set aside as not market driven.

### MS-9 Phase B `v1`

The experiments (`experiments.md`), and the spec changes that follow from them.

After MS-9 the roadmap continues with connecting the simulation to the rendered game
(ARC-10, ARC-13).
