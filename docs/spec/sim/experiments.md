# Phase B experiments (EXP)

Once Phase A passes (SIM-2), test whether each mechanic earns its cost. For each
mechanic, the question is: **does a faction that skips it do worse?** If not, the
mechanic isn't pulling its weight. Tweak it, or remove it if it can't be made to matter.
Removing a mechanic is a spec change (`DECISIONS.md`).

## Method

### EXP-0 How every experiment runs `v1`

1. **The switch:** a per-faction policy flag in config (ARC-7), applied to one faction
   (the treatment) on a world otherwise unchanged.
2. **The control:** the same seed with the flag off. Paired seeds remove most of the
   world-to-world noise.
3. **Seeds:** at least 30 pairs at v1 scale (WLD-3), with the treated faction rotated
   across starting seats so geography doesn't decide it.
4. **Outcomes, per faction:** planets held, total treasury, population, prosperity, wars
   won and lost, years survived, character count, and the mechanic's own direct cost.
5. **Verdict:** a mechanic is worth keeping when skipping it makes the treated faction
   clearly worse on the outcomes it's supposed to affect (a paired difference outside the
   noise), and its costs are of the same order as the benefit. Write the result into
   `DECISIONS.md` either way.

Each experiment also records whether the world still passes Phase A with the switch on,
because some mechanics matter to the world more than to the faction using them.

## The experiments

| ID | Switch on the treated faction | Mechanic tested | Expect if it matters |
|---|---|---|---|
| EXP-1 | Never raids enemy merchants in war | Economic warfare (TRD-7) | Enemies of a non-raider stay richer and win more |
| EXP-2 | No scouting (planet or faction) | Scouts (INF-9) | More surprise sieges against it, fewer relief arrivals |
| EXP-3 | No couriers: information spreads only by contact | Couriers (INF-8) | Slower relief, staler prices, worse merchant profits, more defection at depth |
| EXP-4 | No spies | Spies (INF-13) | Musters go unseen until armies cross the border |
| EXP-5 | Merchants never hire escorts | Escorts (TRD-8) | Higher merchant losses, fewer merchants on risky routes |
| EXP-6 | No patronage | Patronage (FAC-7) | Conquered planets recover slower, flip pirate or defect more |
| EXP-7 | No policing | Policing (PIR-15) | More contraband, lower productivity and income |
| EXP-8 | No faction merchant salaries | Faction merchants (TRD-3) | Less in-faction trade, worse information |
| EXP-9 | World switch: every broke commander turns pirate, against the risk-tolerance split | PIR-4 (experimental) | Which gives a healthier pirate population (CHK-23) |
| EXP-10 | World switch: osmosis off | Osmosis (TRD-13) | Market deserts, more starvation in remote planets |
| EXP-11 | World switch: no efficiency events | Events (PRD-11) | Peacetime prices flatten and merchant profits vanish |
| EXP-12 | World switch: tribute cascade on and off | FAC-6 (proposed) | Which keeps faction size in band (CHK-1) and lieges granting planets |
| EXP-13 | Appoints no stewards: its governors run everything and stay home | Stewards (FAC-16) and governors' fleets (MIL-20) *(proposed, roles round; built as a world switch, `steward.enabled`)* | Its governors fight less far from home; whether the steward's trait changes outcomes |
| EXP-14 | World switch: the Authority's board off | The board (MSN-7) *(proposed, roles round)* | Whether osmosis alone holds prices, and how independents fare |

World switches (EXP-9 to EXP-12, EXP-14) compare whole worlds rather than one faction, using the
Phase A checks as the outcome.

New experiments are added here as mechanics are questioned.
