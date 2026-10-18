# Phase A checks (CHK)

What "stable, with every mechanic working roughly as intended" means, as things a
headless run can measure. These merge the headless-check lists of the four drafts and
the review rounds. Phase A is done when every check passes across many seeds at v1 scale
(SIM-2). The pass bands are starting proposals and are themselves tunable judgments.

Each check names the metric (ARC-8) and the rules it tests.

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
changing hands per year. *Tests:* MIL-5, MIL-10.

## Economy

### CHK-7 The money supply stays in a band `v1`

No runaway inflation or deflation over long runs, in peace and in war. *Metric:* total
money (treasuries + character wealth) and mean price index per month. *Tests:* ECO-1,
ECO-5.

### CHK-8 Peace and war cycle `v1`

Treasuries fill in peace and drain in war, and factions do go back to war. *Metric:*
treasury trend by diplomatic state; time spent at war. *Tests:* ECO-7, DIP-3, DIP-4.

### CHK-9 Collapse is individual and concentrated `v1`

Rebellion pressure comes from margin, not planet size. One disrupted route doesn't set
off a map-wide wave. Repeated collapse concentrates at overstretched factions; if whole
factions spiral, rulers value their planets too cheaply. *Metric:* pirate flips by
planet size, clustering in time and space, repeat flips per planet, faction war count at
the time. *Tests:* ECO-13, ECO-15.

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

### CHK-13 Labour doesn't oscillate `v1`

Labour shares settle, and shift when trade is disrupted, without a cobweb cycle.
*Metric:* labour share changes per planet. *Tests:* PRD-9.

### CHK-14 Price gaps narrow without closing `v1`

Osmosis and merchants narrow gaps; events keep reopening them; merchants still find
profitable trades in peacetime cores. *Metric:* price dispersion across planets by good;
mean merchant trip profit. *Tests:* TRD-13, PRD-11.

### CHK-15 The merchant-kill test `v1`

With raiders killing one faction's merchants on a route, that faction's wealth falls,
with the size of the effect depending on its place in the trade network, without
collapsing the map. *Metric:* experiment run (EXP-1 in reverse: a target faction).
*Tests:* TRD-7.

## Characters and trade

### CHK-16 Careers happen `v1`

A meaningful share of scouts become escorts, some become commanders and a few become
governors; a meaningful share of couriers become merchants. *Metric:* role transitions
per character over their life. *Tests:* CHR-10, CHR-11.

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

## Performance

### CHK-25 It runs fast enough `v1`

A full v1 world runs a game year in under a minute on one core (TIM-8). *Metric:*
wall-clock per game day.
