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

*Tuning pass, part 1 (2026-10-24):* objectives with purses and tribute at 0.25 (DECISIONS).
Two runs, 8 seeds × 5 years:

| | Stage 2 | Purse from 10% to 50% | Purse from 2% to 30% (kept) | Band |
|---|---|---|---|---|
| Tribute, peace and war | 11.6%, 11.5% | 20.4%, 19.0% | 19.3%, 18.2% | 20% to 30% |
| Military, peace and war | 11.7%, 19.9% | 26.7%, 74.8% | 18.1%, 36.7% | 10% to 20%, 40% to 60% |
| Patronage | 2.1% | 3.8% | 6.5% | under 5% |
| Objectives declined, taken, a year | 4,700, 520 | 16, 860 | 40, 1,070 | |
| Purses paid a year | — | 178 million | 49 million | |
| Captures a year | 20 | 17 | 22 | |
| Planets' treasuries at the end, median | 168 million | −575 million | −13 million | |
| Characters' wealth at the end, median | 92 million | 902 million | 273 million | |
| Seeds whose planets end net in debt | 0 of 8 | 7 | 4 | |

- **CHK-26 is close** with the purse from 2% to 30%: tribute is just under 20%, and
  military at war just under 40%.
- **But the war money doesn't leave.**
  - Purses, bounties and spoils move money from treasuries to commanders. Commanders
    have nothing to spend it on beyond ships, and their fleets are capped.
  - Their wealth grows from about 20 million to 270 million, while half the worlds'
    planets end net in debt.
  - In one seed the world's total money goes negative.
  - Planets can promise purses and owe bounties they can't pay: the purse is paid from
    whatever the treasury holds, months after the campaign was funded.
- **CHK-7c's band misses this.** It passes a negative ratio, a median treasury that ends
  in debt.
- *Open, for the human:* a sink on the commanders' side (munitions, MIL-4, held back
  until bounties showed whether war was too cheap), and whether a purse must be held in
  escrow when it's offered, so a planet can't promise what it can't pay.

*Tuning pass, part 2 (2026-10-25): what a fleet costs.* The human's answer: a small
spend for munitions and a larger one for keeping the fleet, since every ship has to be
maintained, crewed and refuelled. Crews aren't tracked person by person: each class has
an average crew, scaled by tier (CHR-9, MIL-4, SYS-21). The purse is held when it's
offered (MIL-2). Targets, written before the run:

- **Upkeep:** a sworn gunship's upkeep is 50% to 70% of its base stipend at every tier,
  so service still pays at peace and the profit is in war. Merchants' upkeep stays about
  where it was.
- **Munitions** after a battle are about a tenth of a month's base stipend.
- **Characters' wealth levels off:** at the end of year 5 it's at most 1.5 times what it
  was at the end of year 2 (tune 2: 2.6 to 3.4 times).
- **No planet promises what it can't pay:** no seed's planets end net in debt, and the
  world's money stays positive on every seed.
- **CHK-26 holds:** no band that passed before fails.
- **A lost fleet is rebuilt (CHR-16):** at most 5% of characters have no ship at the end.
  Tune 2 left about half with none, because a character who lost its last ship could
  never buy another: a bug, fixed in this part.

The first run found the debt wasn't the purses. **Ceasefire tribute** (DIP-2) was paid
by the loser's root alone, at up to 30% of its whole faction's gross monthly income,
whatever its treasury held. In seed 7 it moved 5.6 billion in five years, against 0.4
billion of fealty tribute, and left its payers billions in debt, which then funded the
winners' spending. It's now paid only from what the payer holds, like bounties (SYS-12).
Results, 8 seeds × 5 years:

| | Tune 2 | Part 2 | Part 2, fleets of 8 | Target |
|---|---|---|---|---|
| Characters' wealth, year 5 over year 2, median | 2.9 | 1.39 (2 seeds over 1.5) | 1.37 (1 over) | ≤ 1.5 |
| Characters' wealth at the end, median | 273 million | 96 million | 73 million | |
| Planets in debt at the end, total, worst seed | −3.6 billion | −228 million | | |
| Most indebted planet, worst seed | −2.5 billion | −91 million | | |
| Seeds whose planets end net in debt (CHK-7d) | 4 of 8 | 3 | 3 | 0 |
| Ceasefire tribute paid, unpaid, a year | not measured | 44 million, 234 million | | |
| Upkeep, munitions a year | 6.9 million, 0 | 16.0 million, 1.1 million | 16.8, 1.4 | |
| Purses held, paid, a year | —, 49 million | 81 million, 11 million | 104, 10 | |
| Tribute, peace and war | 19.3%, 18.2% | 20.7%, 21.1% | 21.4%, 20.6% | 20% to 30% |
| Military, peace and war | 18.1%, 36.7% | 13.1%, 22.7% | 15.7%, 22.8% | 10% to 20%, 40% to 60% |
| Patronage | 6.5% | 1.7% | 1.5% | under 5% |
| Planets changing hands a year, share | 0.22 | 0.14 | 0.16 | |
| Characters with no ship at the end | about 1,000 | 974 | 1,054 | ≤ 100 |
| Checks passing on every seed | 26 of 44 | 27 | 26 | |

- **Met:** upkeep and munitions at their sizes; characters' wealth levels off at the
  median; tribute and patronage are in their bands, and military at peace on every seed.
- **Not met:**
  - **Debt is small now but not gone.** About half the planets end slightly below zero
    (median −0.1 million), as in tune 2, so CHK-7c fails. Those are poor planets living
    at zero, an economy question older than this round.
  - **Military at war fell to 23%.** Treasuries can no longer overdraw into war, so wars
    are leaner: fewer captures, fewer blockades.
  - **Ceasefire offers are mostly unpayable:** 234 million a year owed and not paid. How
    they're sized (from the whole faction's gross income, paid by the root alone) is for
    the human.
  - **Shipless characters stay at about 1,000.** Seed 1 saw about 900 rebuilds, but most of the
    rest are ruined: in seed 7 of the first run, 811 of 932 hold under a courier's 8,000 (median 1,650;
    a new character starts with 2,000). They fill the population and stop new characters
    from spawning. What a ruined character does is for the human. Governors who lose
    their fleet never buy another either.
  - **Idle characters rose to 20%** (CHK-17b fails on half the seeds): rebuilt couriers
    with little to do.
- **Fleets of 8** (`fleet.max_ships`) sink a little more into ships but change little
  else, so the cap stays at 4.

*Tuning pass, part 3 (2026-11-24): insurance and the bottom rung.* The human's answer
to the shipless and ruined characters: a ruined character starts over in the cheapest
hull, with bottom-rung work (courier runs, passenger runs, small trades), and a character
at the top falls a peg at a time, not all at once. Insurance is built into a ship's price
(CHR-21): 30% premium, 50% of the price paid out if it's destroyed, the payout free money
(MSN-5). Insurance also lowers what fighting costs, so combat can stay frequent. Passenger
runs (MSN-13) are new free-money work for a character with one tier-1 hull. Governors
rebuy their fleets (MIL-20). Targets, written before the run:

- **Shipless characters at the end:** at most 100 (part 2: 974).
- **Idle characters** under 15% (CHK-17b; part 2: 20%).
- **Characters' wealth levels off:** year 5 over year 2 still at most 1.5.
- **Free money** small against the economy: under 10% of ship spending.
- **Fewer objectives declined** than part 2.
- **CHK-26 holds:** no band that passed on every seed fails.

The first run (shipless 241, idle 24%) turned up two bugs, each exposed by fleets now
being rebuilt:
- **A shipless character at a besieged planet flew off exploring** with no ship. It now
  waits for the siege to end.
- **Sieges never ended** once a governor rebuilt at home: nothing attacked a fleet that
  came home mid-siege, and a weaker one that retreated stayed at home, "ready" again the
  next round. One siege passed 1,150 days, with 650 stranded characters at the planet.
  Now a defender strong enough to stand is fought, and a weaker one can't block the
  capture (SYS-13).

Results, 8 seeds × 5 years:

| | Part 2 | Part 3 | Part 3, 4 passenger runs a planet | Target |
|---|---|---|---|---|
| Characters with no ship at the end, median | 974 | 38 | 34 | ≤ 100 |
| Free hulls given a year | — | 412 | 348 | |
| Characters idle, CHK-17b (seeds passing) | 20.5% (4 of 8) | 20.5% (2) | | < 15% |
| Idle at the end, median | 19.5% | 27.8% | 28.9% | |
| Characters' wealth, year 5 over year 2, median | 1.30 | 1.11 | 1.08 | ≤ 1.5 |
| Insurance paid, premiums, a year | — | 14.6 million, 16.0 million | 13.3, 15.6 | |
| Passenger fares a year (runs) | — | 0.8 million (6,300) | 1.4 million (11,400) | |
| Free money, share of ship spending | 0 | 24% gross; about 0 net of premiums | | < 10% |
| Objectives declined, share | 8.2% | 5.6% | 4.4% | fewer |
| Planets changing hands a year, share | 0.14 | 0.20 | | |
| Battles a year; deaths per battle | 1,910; 0.013 | 1,540; 0.029 | | |
| Tribute, peace and war | 20.7%, 21.1% | 19.6%, 19.1% | | 20% to 30% |
| Military, peace and war | 13.1%, 22.7% | 13.8%, 21.6% | | 10% to 20%, 40% to 60% |
| Seeds whose planets end net in debt (CHK-7d) | 3 of 8 | 1 | | 0 |
| Checks passing on every seed | 27 of 44 | 28 | 28 | |

- **Met:**
  - **Shipless characters fall from 974 to 38.** What's left are mostly stewards, who
    stay at their planet, and characters waiting out a siege.
  - **Wealth levels off;** objectives are declined less; planets in debt fall to one
    seed.
  - **Insurance pays for itself in money terms:** premiums (16.0 million a year) are a
    little more than payouts (14.6 million). The free money is gross; net, it's about
    nothing.
- **Not met:**
  - **Idle characters rose.** The ruined now have ships, but many find nothing worth
    doing. Doubling passenger runs didn't help, so it isn't a shortage of runs. The
    merchant squeeze from round 4 is the likelier cause, for the Authority's board
    (MSN-7).
  - **Tribute slipped just under its band** (19.6%, 19.1% against 20%), and CHK-26c
    fails one seed it passed before. Military at war is still about 22% against 40% to
    60%.
  - **Deaths per battle doubled** (0.013 to 0.029), still well inside CHK-20b. Sieges
    that end are deadlier than sieges that never do.
- **Not built:** a faction's courier work gated on its opinion of the character (the
  human's "courier if good faction relation"); planet-level couriers stay open to anyone.
  The free hull has a Lattice Drive until ships without one exist (TRD-18).
- **Passenger runs stay at 2 a planet;** 4 changed nothing that matters.

*Tuning pass, part 4 (2026-11-25): pirates and bankruptcy.* Breaking down part 3's idle
characters by why they were idle found the cause: at the end, 17% of all characters were
pirates sitting at a haven for 90+ days, in debt. A broke commander rolled its risk
tolerance every day it stayed broke, so nearly every one who knew a pirate planet turned
within weeks; the haven hired it, couldn't afford it at the next budget, and let it go;
and a pirate with no salary had no work it was allowed to take, couldn't afford to move,
and never fought, so it never died. Pirates were 31% of characters by year 5 (CHK-23:
1% to 10%). The human's direction: piracy is terminal and rare enough to be a story arc;
a pirate raids until destroyed, rebuilds in a free fighter and raids again until it dies,
at the ordinary pod death chance, since it loses often; a lucky one can still get rich;
many characters never turn (PIR-4, PIR-16).

Built: the turn weighed once a broke spell, only for the bold (`pirate.turn_min_risk`
0.5, chance `pirate.turn_p` 0.5 × risk); debts written off on renouncing; freelance
raiding; a free tier-1 scout as a pirate's bottom rung, gunships to rebuild with. The
first run fixed the pirates and moved the trap: the broke who no longer turned kept their
last hull and sat in debt, 22% of characters by the end. So, also built: **bankruptcy**
(CHR-16): still in debt with one hull, it's swapped for the bottom rung's and the debt
written off; and the bottom rung looks for work without keeping a reserve. A legal-goods
run taken at the haven with nowhere known to buy the good crashed the dev build once
bottom-rung characters started taking them; it now fails.

Results, 8 seeds × 5 years:

| | Part 3 | Part 4 | Part 4, `pirate.turn_p` 1 | Target |
|---|---|---|---|---|
| Characters idle, CHK-17b (seeds passing) | 20.5% (2 of 8) | 12.2% (8) | 12.4% (8) | ≤ 20% |
| Idle at the end, median | 27.8% | 13.1% | | |
| Trading, mean | 25.7% | 32.8% | | |
| Pirates, CHK-23 (seeds passing) | 15.3% (1 of 8) | 0.9% (2) | 1.1% (5) | 1% to 10% |
| Pirates at the end, median | 618 | 46 | 47 | |
| Turning pirate a year | 122 | 14 | 20 | rare |
| Pirate lifespan after turning, median days | — (3 in 4 never died) | 102 | 94 | |
| Pirate raids a year (freelance) | 142 | 297 (284) | 416 (385) | |
| Bankruptcies a year; debt written off a year | — | 98; 14,000 | 76; 10,000 | |
| Free hulls a year | 412 | 564 | 765 | |
| Battles a year; deaths per battle | 1,540; 0.029 | 1,830; 0.030 | 2,240; 0.028 | |
| Smuggling runs a year (CHK-24) | 12.7 | 5.5 | 13.3 | ≥ 1 |
| Worst issuer's failed share (CHK-28b) | 0.61 | 0.75 | | ≤ 0.5 |
| Checks passing on every seed | 28 of 44 | 30 | 29 | |

- **Met:** idleness passes CHK-17b on every seed for the first time, and trading is up
  by a third. Pirates are a flow: they turn rarely, raid twice as often as all of part
  3's pirates did, and die about three months in. Debt written off is small, about a
  seventh of what new characters bring in.
- **Not met:** pirates sit at the floor of CHK-23. The stock is set by how fast pirates
  die more than by how many turn: doubling `pirate.turn_p` adds a tenth. Whether the band
  or the pirates should move is the human's call.
- **Pirate missions fail more** (CHK-28b, already failing in part 3): more bold
  independents now take pirate runs, and a run that can't be done fails instead of
  pretending to finish. Pirate missions are their own job.
- **Fewer pirates offer fewer smuggling runs** (CHK-24 still passes, CHK-10 halves).

*Tuning pass, part 5 (2026-11-25): ships without a Lattice Drive, and standing.* The
human's direction: a ruined character's free hull follows who it was, standing winning
over a merchant record (CHR-16); a ship without a drive keeps off the Lattice and one
with a drive drifts to it, with no preference rule (TRD-18); and standing is seeded so a
character keeps working for its faction and builds more (REL-5).

Built: every class with and without a drive (no drive: 70% of the price, 125% of the
cargo); routes, trip times and fuel by what the fleet can fly, from a second, open-space
travel table; passenger runs as open-space hops, the only missions a hull without a
drive can take; the free-hull ladder (pirate scout; courier with a drive for the
trusted; small freighter without one for a merchant record; else a shuttle); a shuttle
owner's first purchase is the same hull with a drive. Standing: a new character is born
with 15 in its faction; a planet's job or a passenger run also raises the faction root's
opinion (`deed.faction_job`); courier runs need 10.

What the runs turned up:
- **Standing barely existed.** Measured directly, nearly every character's best opinion
  from any governor in its faction was 0 to 5, so the trusted rung never fired (0 a
  year). Seeding alone didn't hold: 15 fades below 10 in about 13 weeks, during the
  warm-up.
- **A shuttle owner had money but no way up:** its upgrade was a freighter it couldn't
  afford, never the courier it could. Fixed by buying the same hull with a drive first.
- **The passenger supply is the shuttle rung's ceiling:** about 9,300 runs a year, all
  taken.
- **`deed.faction_job` decides whether the loop runs** (4 seeds): at +3, 6% of
  characters clear the courier gate and 0.7% the trusted rung; at +10, 21% and 7.7%,
  and 100 trusted free hulls a year. A slower fade (1% a week) got 18% and 2.6% but moves
  every opinion in the game, so +10 it is.

Results, 8 seeds × 5 years:

| | Part 4 | Part 5 |
|---|---|---|
| Characters idle, CHK-17b (seeds passing) | 12.2% (8 of 8) | 10.9% (8) |
| Clearing the courier gate; the trusted rung, mean | — | 20.8%; 7.3% |
| Free hulls a year: scout, courier with a drive, freighter, shuttle | 564 (no ladder) | 171, 94, 65, 391 |
| Lawful fleets without a drive, end | — | 20% |
| Bankruptcies a year | 98 | 123 |
| Exploring, mean | 11.8% | 19.7% |
| Checks passing on every seed | 30 of 44 | 29 |

- **Met:** the ladder runs, standing is a number that moves, and a trusted character
  climbs back on the Lattice.
- **Not met:** a fifth of lawful fleets end without a drive, and bankruptcies are up a
  quarter: the shuttle rung is crowded and capped by the passenger supply. Scaling
  passenger runs with the shuttles waiting is the next lever, if the human wants it.
- **Exploring is up** because a fleet without a drive is slow in open space, so looking
  for work takes longer. It reads as busy, not idle.
- **CHK-8 fails one seed** (war share), noise-sized; CHK-7d now passes every seed.
- **Not built:** route risk for a fleet without a drive still reads the reference route's
  sectors. Bought ships all have drives; who would buy one without is the rest of
  TRD-18.

*Part 5, then: moving on after a day (TRD-4).* The human: 10% idle isn't terrible, and a
character that finds nothing for a day should go to a nearby planet, which also makes
the world feel alive. Built as TRD-4 already said: within 3 days of travel, busier
planets likelier, so long as the fuel puts it in no debt. A breakdown of who's idle afterwards: of
10.9%, 9.1 points are characters in port for under a day, between jobs; solvent
characters idle a day or more are 0.2%. About 770 moves a year. Idle stays at 11.0%;
checks passing on every seed are 29 of 44 (CHK-8 fails one seed). A first version
required the fuel to be paid from savings, which held every indebted shuttle in port
though its fuel off the Lattice is nothing; it now goes so long as the fuel adds no
debt.

**Bankruptcies swing widely by seed** (48 to 726 a year within one run; 67 to 352 in the
last), but across part 5's runs the median has climbed from part 4's 98 to about 170, 8%
of characters a year. The likeliest cause is the crowded shuttle rung (passenger runs are
its only work and all are taken). Worth its own look. *(Looked at in part 6, below.)*

*Tuning pass, part 6 (2026-11-26): bankruptcy, and errands (MSN-14).* A diagnostic
logged every bankruptcy with what put the character in debt. **The cause, in layers:**

1. **A part 4 rule leaked.** The bottom rung explored without a reserve, so in debt it
   kept exploring across the Lattice on credit: it can't buy cargo in debt, found no
   trade, and explored again. The broke clock ran only for characters docked at the daily
   tick, and these were almost always in transit: at day 180 on seed 6, about 380 lawful
   characters (19%) had been in debt 15 days or more, every one in transit. Debts grew
   for months, into the thousands, and were written off when the ship was finally lost
   and the pod landed (over half of all bankruptcies were characters with no ship).
2. **Under it, too little work for the bottom rung.** Closing the leak alone doubled the
   count: 70% became debts under 100, the same jobless characters going bankrupt every
   ~16 days (bankruptcy leaves them at exactly zero; a day's upkeep puts them back in
   debt).
3. **Traps where a character can't leave:** planets open space doesn't get out of
   (the Lattice the only way, and no fuel money in debt); shuttles without a drive given
   at those planets; lawful characters whose pod landed at a pirate planet; pirate
   scouts without a drive landed at a lawful planet with no open-space route to a pirate
   planet (one sat there for years, bankrupt 117 times, never raiding).

**The human's rule:** after a random 12 to 48 hours with nothing, a bottom-rung
character's planet posts it a free-money errand to a nearby planet (MSN-14), which
mirrors the player's experience: a board is never empty. Built with the fixes: the
bottom rung explores only with money in hand; the broke clock runs at landing too;
errands go by the Lattice (fare covering fuel) or to the nearest reachable lawful
planet where open space doesn't get out, and pirate planets post them for the lawful;
a free hull can always leave (a drive where needed; bankruptcy swaps a driveless hull).
A goods-conservation test's tolerance was relative to the net change; it's now
relative to the goods that flowed (the residual was 5e-8 on 6 million).

| | Part 5 | Part 6 |
|---|---|---|
| Bankruptcies a year (median of 8 seeds) | 171 | 134 |
| Range over seeds, 5 years | 335 to 1,755 | 438 to 1,150 |
| Characters gone bankrupt, a year | — | 65 (3%) |
| Debt written off a year | 81k | 15k |
| Errands a year | — | 2,359 |
| Passenger runs a year (with errands) | 9,079 | 11,293 |
| Idle | 11.0% | 11.9% |
| Pirates, mean (end) | 1.1% (2.0%) | 1.4% (2.8%) |
| Turned pirate a year | 12 | 16 |
| Checks passing on every seed | 29 of 44 | 28 (CHK-23 now passes; CHK-8, 8b, 27 lose one seed each) |

What's left: pirates are about a third of bankruptcies (raiding until destroyed, by
design); spies, governors and stewards in debt the rest.

*Tuning pass, part 7 (2026-11-26): life support on the fringe, and supply orders
(MSN-15).* At wage 40 life support took a median 37% of a Lattice planet's budget and
177% of a fringe planet's. Raising `econ.wage` (80, 120; not yet chosen, the human's
call) brings that to 13% and 47%, but the fringe still shrank. **Why:** the fringe does
make life support (93% of fringe planets make at least one of the three, 47% two or
more) and sits on a surplus of what it makes (a median 0.66 of base), but a good it
doesn't make costs 3.6 times base, 31% of the time at the cap. Of 158 fringe shortages
sampled, 150 had no surplus within osmosis's 3 days. Merchants plan among the 12 nearest
planets they know (TRD-4); no Lattice planet (0 of 252) has a fringe planet among its 12
nearest, so no merchant ever weighed the trip, at any price. **The human's design:** a
short planet sends an order by courier mission to the nearest surplus it has heard of
(untaken, it arrives anyway), and the producer posts the delivery for merchants to weigh
as a trip (MSN-15).

**What the wage is.** `econ.wage` is a population's purchasing power, per thousand a day,
times productivity: created from nothing, the same on every planet, and money only when
spent on goods in stock (life support first, then luxuries, whose buying scales with how
full their stock is). Across the world, populations spent 32 a thousand a day at wage 40
(about two thirds of it) and 34.5 at wage 120 (about a quarter). Tripling the wage added
8% to real spending. It matters only where life support is dear, which is on the poor
fringe. **A confound, fixed:** the starting world sized each planet's income, and from it
the starting commanders, the first credit limit and budgets, as if the whole wage were
spent. At 120 that meant three times the starting military. The starting income is now
what the population would spend with its markets at reserve, within its wage
(`economy::init_state`). At wage 40 nothing changes; the runs below all use it.

One variable at a time: the wage alone, then orders at the planet's full price, then at
half of it (`order.price_share` 0.5, the human's call: "they shouldn't advertise the full
price they will pay at that moment, but reduced significantly. The planet is taking the
risk"). 8 seeds, 5 years, medians.

| | Wage 40 | Wage 120 | + orders, full price | + orders, half price |
|---|---|---|---|---|
| Fringe population, end over start | 0.20 | 0.26 | 0.59 | **0.54** |
| Fringe happiness | −0.44 | 0.11 | 0.23 | 0.23 |
| Fringe life-support share, median (p90) | 177% (510%) | 46% (104%) | 43% (96%) | 41% (113%) |
| Lattice life-support share, median | 37% | 13% | 12% | 13% |
| Fringe treasury, months of costs | −0.23 | 33 | 11 | 30 |
| Fringe growth over the network's (CHK-11) | 0.27 | 0.28 | 0.45 | 0.42 |
| Orders a year (carried, arrived untaken) | — | — | 219 (135, 57) | 192 (96, 65) |
| Deliveries a year (units, price over base) | — | — | 2,746 (1.72M, 4.0) | 1,804 (0.80M, 2.0) |
| Errands a year | 2,359 | 603 | 189 | 276 |
| Checks passing on every seed | 28 of 44 | 27 | 28 | 32 |

- **The wage alone barely moves the fringe** (0.20 to 0.26): planets can buy food now,
  but no one brings the goods they don't make.
- **Orders double what the fringe keeps.** At full price the fringe paid about 4 times
  base a unit and its treasury fell from 33 months of costs to 11; at half price it pays
  about 2 times base, keeps 30 months, and still keeps most of the gain (0.54). Fewer
  units move (merchants take fewer deliveries at the lower price), so it reads as the
  planet getting the goods it needs rather than a windfall for the producers.
- **Couriers carry about half of the orders at half price** (about two thirds at full),
  and errands fall, since the courier legs are paid work for the bottom rung.
- **Checks:** 32 of 44 at half price, against 27 for the wage alone. Several of the
  differences are one seed either way (CHK-4, CHK-6b, CHK-26c, CHK-27); CHK-7c, CHK-8 and
  CHK-8b pass on every seed in all three wage-120 runs. Pirates stay under the CHK-23
  floor at wage 120 (about 0.9% of characters, floor 1%), as at part 6's wage sweep.
- **Still open:** the fringe still halves; the wage is a flat income that doesn't come
  from what a planet makes or sells, and most of it goes unspent; and whether there are
  enough merchants overall (the human's question).

**The money round** *(built, ECO-1, ECO-2, SYS-1).* The population eats from the stock,
and new money is an even grant of 40,000 a planet a day, with a life-support sell floor
of half a reserve. 8 seeds × 5 years against part 7 (`init120-on`):

- **Money in matches part 7's** (4.1M a day against 4.2M), but sinks fall to 3.0M:
  making goods costs less as fewer are made. Treasuries together end at 2.1B, against
  0.3B, and the money supply grows 28% a year (CHK-7).
- **The brake on size works, hard.** World population ends at 0.76 of its start,
  against 1.21. It falls over the first two to three years and then holds steady
  (CHK-12b cv 0.014). The 90th-percentile planet is 1,400 thousand against 2,900, and
  the largest is 2,300 against 7,400. A quarter of the biggest third are in debt; almost
  none of the smallest third are.
- **The small planets' spare money doesn't buy growth.** The smallest third sit on a
  median 480 months of running costs and hold 64% of all money. A planet's bid reads its
  stock against its reserve and never its treasury, and growth comes only from stock
  above reserve. So a rich small planet bids no more than a broke one, and its money
  sits. The balance the design counts on ("small planets will use their excess money to
  buy more food and luxuries and grow") has no channel yet.
- **Trade halves:** merchant profit per trip falls from 184 to 57 (CHK-14b), and goods
  traded halve. An importer's income is now the grant plus its exports, so its credit
  limit (two months of income) is far smaller than its old population spending gave it.
- **Pirates rise** to 1.9% of characters (CHK-23 floor 1%; part 7 0.99%), with battles up
  79%. Pirate planets get the grant too.
- **The floor does nothing measurable:** a run with `price.sell_floor` 0 lands within
  noise of it.

**A rich planet pays more** *(built, PRD-10).* Spare money raises every price a planet
posts (`price.wealth` 0.25, up to `price.wealth_max` 2). Against the grant alone:

- World population ends at 0.85 of its start (0.76), the fringe grows 0.70 as fast as
  the network (0.49), and merchant profit per trip is 80 (57). Prices end 21% higher.
- Small planets still hold a median 350 months of costs, at the 2× ceiling. A small
  planet costs about 40,000 a month to run and gets 40,000 a day.

**The grant sets the size of the world** (with the wealth bid). Population, end over
start: 0.85 at 40,000, 0.47 at 20,000, and 0.14 at 10,000, where the world collapses into
piracy (369 planets turned). A thousand people cost their planet about 23 a day: 14.5
to make what they eat (PRD-7, a sink) and 8 to run (ECO-4). Nothing pays that back now
that people don't buy, so world population settles near (grant × planets − fixed sinks)
/ 23. That gives about 90,000 at 40,000 and 50,000 at 20,000, as run. Geography only
divides the total: size still follows the Lattice, the fringe and big neighbours at
every grant. More output per head doesn't help, since each unit costs the same to make
however many people make it. That fits high-tier slots giving no edge in size.

**Making goods is free in money** *(built, PRD-7).* The unit cost still decides whether
a slot works, but the treasury pays nothing. With the wealth bid, against the grant at
40,000, 20,000 and 10,000:

- **The world grows again at 40,000:** population ends at 1.35 of its start (part 7:
  1.21; the paid cost: 0.85). The 90th-percentile planet is 2,700 thousand and the
  largest 5,600. Merchant profit per trip is back to 149, and pirates are 1.2% of
  characters, just over the CHK-23 floor.
- **The grant still sets the size:** 1.35, 0.89 and 0.43 of start. Total money isn't the
  limit, since money in beats money out at every grant (supply +21% a year). Where it
  sits is the limit. The smallest third hold a median 390 months of costs at 40,000, and
  a fifth to a third of the biggest planets are in debt. Their credit limit, two months
  of an income that is mostly the grant, stops them importing.
- **Why it pools:** a small planet's buying is bounded by its own population, and the
  grant isn't. At 2× the wealth bid is at its ceiling and still can't spend it. Trade
  doesn't return the money either: a planet sells where goods are cheap and buys where
  they're dear, so planets as a whole lose the merchants' margin, which ends in ships and
  fuel.
- **Geography:** size still follows the Lattice (−0.39), the fringe (−0.28) and big
  neighbours (+0.18); treasury still runs the other way. High-tier slots still give no
  edge in size (−0.14).
- **A steeper wealth bid** (`price.wealth` 0.5 up to 4×; 1 up to 5×) barely helps. The
  smallest third's hoard falls from 390 to 350 and 255 months, and world population
  holds at 1.35. Merchants pocket the difference: profit per trip rises from 149 to 222
  and 313, and pirates fall under the CHK-23 floor at the steepest (0.8%). Higher prices
  move money to whoever carries the goods, not into a small planet's growth. The
  defaults stay at 0.25 and 2.

*(Money round, part 4: life support before luxuries.)* The human: "Planets are not
reacting to their internal conditions well enough … hoarding money should not be
acceptable." One variable at a time from the defaults, 8 seeds × 5 years:

| Change | World pop, end/start | Smallest third, months of costs | Biggest third in debt | Growth/yr, small/big |
|---|---|---|---|---|
| Defaults | 1.36 | 390 | 22% | 14% / 14% |
| 3 faction merchants per planet | 1.37 | 396 | 13% | 16% / 15% |
| `happy.g_up` 1 | 2.2 | 169 | 32% | 19% / 31% |
| Life support yield 2 | 1.35 | 396 | 35% | 17% / 18% |
| `prod.labour_per_capita` 10 | 1.37 | 375 | 28% | 15% / 18% |

- **Every planet grows at the same capped rate, whatever its money**, so money buys no
  growth. Only `happy.g_up` (luxuries speed growth) moved the hoard, and it grew the
  world 2.2×.
- **More output makes it worse:** each planet is more self-sufficient, goods are cheaper,
  and big planets earn less from their exports.
- **More faction merchants only relabel** characters: `char.target` caps them, and trips
  and cargo stay where they were. 3,000 characters left the hoard where it was.
- **Osmosis carries about three quarters of all goods** (320,000 a day against
  merchants' 95,000), and its margin is the biggest sink after running costs. Fuel is
  4,000 a day against a grant of 4.1 million.
- **Life support before luxuries** (`econ.luxury_credit` 0) and **merchants seeing a
  market that won't buy** (SYS-7): profit a trip 142 to 453, supply deliveries doubled,
  failed missions 7.9% to 3.7%, the hoard 424 to 350 months, pirates 1.3% to 0.4% of
  characters (under CHK-23). The rule without the second change cut merchants'
  life-support trade by two thirds.
- **Fixed:** a planet turning pirate left its supply deliveries takeable past expiry,
  which could run the clock backwards in release builds; fixing it alone moved little.

**Later rounds from the roles design,** one area each:
- **The Authority's board and contract merchants** (MSN-7), as the response to the
  merchant squeeze (round 4: 23% of characters idle, 15% pirates).
- **Spies:** EXP-4 first; then spy missions and per-trip suspicion against stealth and
  detection (MSN-11, INF-14), if spies earn their place.
- **Spawned bounties** (MSN-6, MSN-10), with the hunt counting as patrol time.
- ~~Delivery top-ups (MSN-8)~~: replaced by supply orders (MSN-15, part 7).

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
