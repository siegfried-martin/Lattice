# Phase A checks (CHK)

What "stable, with every mechanic working roughly as intended" means, as things a
headless run can measure. These merge the headless-check lists of the four drafts and
the review rounds. Phase A is done when every check passes across many seeds at v1 scale
(SIM-2). The pass bands are starting proposals and are themselves tunable judgments.

Each check names the metric (ARC-8) and the rules it tests.

*As built (MS-8):*
- **The tools:**
  - `sim/tools/checks.py` reduces each run to one number per check and holds it to a
    band, across seeds.
  - `sim/tools/experiment.py` runs groups of seeds and config variants in parallel.
  - `sim/tools/report.py` writes an HTML page with the table and charts.
- **The bands are proposed** and live in `checks.py`, with the measure for each check.
- **What isn't measured yet:**
  - checks that need role transitions (CHK-16), mission delays by depth (CHK-22), escort
    choices against risk (CHK-21), or experiment runs (CHK-15);
  - CHK-2 and CHK-5, which need defections and reliefs broken down further.
- **Metrics added:** CHK-8b, war declarations per faction per year, because "any war
  in the world" is nearly always true with many factions; and CHK-17, characters at end
  over start. Round 2 made CHK-8 each faction's share of time at war, and added CHK-8c
  (median war length), CHK-6b (blockades) and CHK-9b (secessions). CHK-8b's band moved
  from 0.05–1 to 2–5 a year with DIP-4's pacing.

## Politics

### CHK-1 Faction count stays in a band `v1`

The number of independent factions (roots with more than one planet) never collapses to
one and never fragments into dust. *Metric:* roots with ≥ 2 planets per month. *Tests:*
FAC-11, WLD-19, REL-4.

### CHK-2 Culturally mixed factions fracture more, but not always `v1`

Defection rate is higher in multicultural factions than monocultural ones, but crossing a
cultural border still pays off often enough to happen. *Metric:* defections per planet-year
by the faction's culture count; conquests across culture lines. *Tests:* WLD-19, FAC-11.

### CHK-3 The Authority's income stays steady `v1`

*Metric:* monthly dues income; variance, and its drops matching pirate flips. *Tests:*
FAC-14, ECO-10.

### CHK-4 Pirates live on the fringe `v1`

Pirate planets near the Lattice are retaken in reasonable time; fringe havens persist
without growing indefinitely. Pirates holding a system on a major route means conquest
incentives are mistuned. *Metric:* pirate planet count and lifetime by distance from the
network. *Tests:* PIR-2, ECO-14, WLD-21.

### CHK-5 Defence works sometimes `v1`

Invasions against well-connected planets usually meet reinforcements; invasions against
poorly informed ones sometimes don't. *Metric:* share of sieges where relief arrives
before capture, by courier coverage and scouting. *Tests:* MIL-11, INF-12.

### CHK-6 Defence numbers aren't frozen or a free-for-all `v1`

Too low and everyone conquers everyone; too high and the map freezes. *Metric:* planets
changing hands per year. *(proposed, round 2)* Also blockades a year per faction, at
least 0.5: blockading is a normal part of war (MIL-17). *(proposed, round 3)* Border
planets change hands often in war, so the band is 5% to 40% a year; and the
most-traded planet changes hands at most twice a year (before round 2's garrisoning,
one planet changed hands 19 times in three years). *As built:* CHK-6c counts each
planet's `planet.captured` events and takes the largest, per year.
*Tests:* MIL-5, MIL-10, MIL-12, MIL-17.

## Economy

### CHK-7 The money supply stays in a band `v1`

No runaway inflation or deflation over long runs, in peace and in war. *Metric:* total
money (treasuries + character wealth) and mean price index per month. *Tests:* ECO-1,
ECO-5.

*(proposed, round 3)* **Reserves level off rather than compound.** Deep treasuries are
intended, especially at wealthy core planets and capitals: with no loans, cash is a
faction's only buffer against war and losses. What fails is endless growth. *Metric:*
total money growth over the second half of a run at most 1.1 a year, and the median
planet's treasury in months of costs no more than 1.5 times higher at the end than at
the halfway point. *As built:* CHK-7 is the yearly growth from the halfway sample to the
last, band 0.8 to 1.1; CHK-7c reads months of costs as treasury ÷ (`econ.op_base` +
`econ.op_per_capita` × population), ignoring pirate planets' lower costs.

*(proposed, tuning pass)* **Planets stay out of debt.** CHK-7c's band starts at 0, so a
median treasury that ends in debt fails; before, its band had no floor and passed it.
CHK-7d is the sum of the planets' treasuries at the end, which must be above 0.

### CHK-8 Peace and war cycle `v1`

Treasuries fill in peace and drain in war, and factions do go back to war. *Metric:*
treasury trend by diplomatic state; each faction's share of time at war, and the median
war's length. *(proposed bands)* At war 0.5 to 0.8 of the time; median war 30 to 90 days
(DIP-4's pacing). *Tests:* ECO-7, DIP-3, DIP-4.

### CHK-9 Collapse is individual and concentrated `v1`

Rebellion pressure comes from margin, not planet size. One disrupted route doesn't set
off a map-wide wave. Repeated collapse concentrates at overstretched factions; if whole
factions spiral, rulers value their planets too cheaply. *Metric:* pirate flips by
planet size, clustering in time and space, repeat flips per planet, faction war count at
the time. *(proposed, round 2)* Secessions count as collapse too, with the same band as
flips (at most 5% of planets a year). *Tests:* ECO-13, ECO-15.

### CHK-10 Contraband tension holds `v1`

Some governors police hard and some don't; smuggling stays profitable without taking
over. Contraband is net positive for a population's happiness and net negative for its
governor's income. *Metric:* policing spend distribution; smuggler profit; contraband
share of consumption. *Tests:* PRD-6, PIR-15.

### CHK-11 Fringe worlds survive `v1`

Off-network planets aren't simply poorer forever. *Metric:* population and treasury of
fringe planets over time against on-network ones. *Tests:* WLD-6.

### CHK-12 Populations settle `v1`

Populations reach a size their production and imports can feed, without oscillating or
dying off; a blockade shrinks a planet slowly. *Metric:* population per planet; its
variance after warm-up. *Tests:* PRD-2.

*(proposed, round 3)* **Happiness spreads out rather than piling at the ends:** at most
20% of planet-days have happiness's productivity effect within 5% of its floor or
ceiling, and the fringe's median happiness is below the network's but not at the floor.
*As built:* over the second half's samples, CHK-12c is the share with u(H) ≥ 0.95
(SYS-1), CHK-12d the fringe median less the network median (at most 0), CHK-12e u of the
fringe median when it's below neutral (at most 0.95).
*Tests:* PRD-4, PRD-5.

### CHK-13 Labour doesn't oscillate `v1`

Labour shares settle, and shift when trade is disrupted, without a cobweb cycle.
*Metric:* labour share changes per planet. *Tests:* PRD-9.

### CHK-14 Price gaps narrow without closing `v1`

Osmosis and merchants narrow gaps; events keep reopening them; merchants still find
profitable trades in peacetime cores. *Metric:* price dispersion across planets by good;
mean merchant trip profit. *Tests:* TRD-13, PRD-11.

*(proposed, round 4: built)* **Prices rest on costs, not the clamp:** at most 10% of planet-good
samples at `price.min`, second half (CHK-14c; round 3: 11%, life support near the floor everywhere; round 4: under 0.01%).
*Tests:* PRD-7, PRD-10.

### CHK-15 The merchant-kill test `v1`

With raiders killing one faction's merchants on a route, that faction's wealth falls,
with the size of the effect depending on its place in the trade network, without
collapsing the map. *Metric:* experiment run (EXP-1 in reverse: a target faction).
*Tests:* TRD-7.

## Characters and trade

### CHK-16 Careers happen `v1`

A meaningful share of scouts become escorts, some become commanders and a few become
governors; a meaningful share of couriers become merchants. *Metric:* role transitions
per character over their life. *Tests:* CHR-10, CHR-11. *(proposed, roles round)*
Stewards are a destination too: some merchants and spies become stewards (CHK-27).

### CHK-17 The population holds and new characters find work `v1`

Character count stays near its target; new characters find tier-one work at once; no
character sits idle for long. *Metric:* character count; days idle per character.
*Tests:* CHR-5, CHR-6.

### CHK-18 Nobody goes bankrupt waiting `v1`

A character with no work loses time, not much money; bankruptcies come from losses,
not lulls. *Metric:* cause of each bankruptcy. *Tests:* CHR-9.

### CHK-19 Losing a fleet is recoverable `v1`

Commanders who lose a fleet are rehired in a plausible time; reckless ones recover
slower than conservative ones. *Metric:* days from fleet loss to new service contract,
by risk tolerance. *Tests:* CHR-16.

### CHK-20 Deaths stay rare while fights stay frequent `v1`

*Metric:* battles per month; deaths per battle. *Tests:* CHR-15.

### CHK-21 Escorts are hired when it makes sense `v1`

Merchants hire escorts when route risk and cargo value justify it, and not when they
don't; hiring doesn't herd every merchant onto the same few routes; escort contracts
end with their trips. *Metric:* escort rate by route risk × cargo value; route
concentration. *Tests:* TRD-8, TRD-9.

### CHK-22 Missions spread and stale pay works `v1`

Faction-level missions reach fringe planets late but reach them; stale mission pay
pulls characters toward unpopular work without starving popular work. *Metric:* mission
delay by fealty depth; fill time by mission kind. *Tests:* INF-11, CHR-10.

### CHK-23 There are enough pirates, not too many `v1`

Broke commanders supply a pirate population that keeps fringe danger real without
overrunning lawful space. *Metric:* pirate commanders over time; share from broke
commanders against top-up. *Tests:* PIR-4, PIR-8.

### CHK-24 Smuggling runs on more than the player `v1`

NPC smugglers carry a meaningful share of contraband. *Metric:* contraband volume by
carrier. *Tests:* PIR-13.

### CHK-26 War and fealty money are where they're meant to be `v1` `proposed`

War and fleets are expensive, tribute is high, and patronage is rare (MIL-18, FAC-6,
FAC-7). Measured per faction, as shares of the faction's net income (SYS-5), split by
diplomatic state. *(proposed bands, placeholders until the runs say otherwise)*

| | Peace | War |
|---|---|---|
| Tribute paid up, median vassal, share of its own net income | 20% to 30% | 20% to 30% |
| Military: stipends + security + bounties + loan fees | 10% to 20% | 40% to 60%, with the root's treasury falling month on month |
| Patronage | under 5% | under 5% |

*Metric:* monthly flows from the ledger per faction, by its state that month. *Tests:*
MIL-2, MIL-18, FAC-6, FAC-7, FAC-15, ECO-7. *Found in round 4:* military spending of
every kind was about 4% of the money created, tribute 3% to 6%, and patronage 1.3 to 2.7
times tribute.

*In `checks.py` (war and fealty round):* CHK-26 and CHK-26b, tribute at peace and at war;
CHK-26c and CHK-26d, military at peace and at war; CHK-26e, patronage over both. Each is
the measured run's total of the monthly flows of lawful planets, bucketed by their
faction's state that month. That's an aggregate share, not the median vassal, and
tribute counts the cascade over the vassal's own net income. The root's treasury falling
at war isn't computed.

### CHK-27 Paths cross `v1` `proposed`

*(roles round)* Posts aren't filled from one path. Some governors were merchants, spies
or stewards, and some stewards were commanders. If every governor is a warrior and every
steward a merchant, the fitness weights are too narrow (CHR-20). *Metric:* each
governor's, steward's and commander's main earnings before the post (trade, missions,
stipends). *Tests:* CHR-11, CHR-20, FAC-16.

### CHK-28 Missions are worth what they cost `v1` `proposed`

*(roles round)* Each kind of issuer gets something for its mission spend, and missions
supplement price without replacing it (TRD-2). *Metrics and bands, placeholders:*

- spend, completions and failures per kind of issuer (MSN-4). An issuer that keeps losing
  on one kind of mission is a candidate for free money (MSN-5), decided with these
  numbers;
- failures are a minority of taken missions, but not zero;
- commander hiring doesn't run away in a long war (CHK-8);
- *(with the Authority board, MSN-7)* the Authority's cut against its drift losses stays
  in a band, not exactly balanced. Missions move under a ceiling of all goods moved, and
  the board doesn't widen the gap between member and independent merchants (the merchant
  squeeze, MS-8);
- *(with top-ups, MSN-8)* top-ups don't keep a chronic failure (ECO-13) alive forever;
- *(with spawned bounties, MSN-10)* pay falls as a sector's risk falls;
- *(with the spy round, INF-14)* spy failures sit in a band: not zero, not constant, and
  falling with stealth.

*Tests:* MSN-1 to MSN-11.

### CHK-29 Successions settle `v1` `proposed`

*(roles round)* Every planet always has a governor (FAC-1). Most governors who die are
succeeded inside a month. A root's death sometimes costs it a vassal, but rarely more
than one (FAC-11's damping). *Metric:* days without a governor; defections within
three months of a succession, against the base rate. *Tests:* FAC-16, FAC-17, MIL-20.

*In `checks.py` (roles round, placeholder bands):* CHK-27, governors who came from trade,
and CHK-27b, stewards who came from combat work, each at least 5% of those with any
earnings; CHK-28, missions failed, 0.1% to 50% of those taken; CHK-28b, the worst kind of
issuer's failed share, at most 50% (issuers with 20 or more taken); CHK-29, days a seat
stays empty per succession, at most 30; CHK-29b, defections from a planet within three
months of its succession, at most one each. The base-rate comparison isn't computed.

## Performance

### CHK-25 It runs fast enough `v1`

A full v1 world runs a game year in under a minute on one core (TIM-8). *Metric:*
wall-clock per game day.
