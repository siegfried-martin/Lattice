# Tunables (TUN)

Every config key the simulation reads (ARC-7), with its meaning and a starting value.
Starting values are guesses to be replaced by Phase A tuning; when one changes, update it
here. Units: money is credits, time is game days unless noted, population is in
thousands.

## Clock and cadence (TIM)

| Key | Meaning | Start |
|---|---|---|
| `time.real_seconds_per_day` | Rendered clock (WLD-15) | 60 |
| `time.space_step_minutes` | Open-space kinematic step (TIM-6) | 10 |
| `time.battle_round_hours` | Battle round length (SYS-14) | 1 |
| `time.labour_days`, `time.budget_days`, `time.diplomacy_days` | Cadences (TIM-3) | 7 |
| `time.payment_days` | Monthly payments | 30 |

## Debug

| Key | Meaning | Start |
|---|---|---|
| `debug.pings` | Records that fire at random intervals, to exercise the queue, RNG and saves (scaffolding, MS-1) | 0 |

## Goods and prices (PRD, SYS-1 to SYS-4)

Per-good keys are written `good.<name>.<key>` in config, for example
`good.food.base_price`. The goods are food, water and oxygen (life support); wine,
spice, silk, tea, perfume and glassware (regional luxuries) and crystal and amber
(fringe luxuries), all placeholder names (OPN-35); and stims (contraband). Luxuries
have no `consumption` key (they use `happy.need`). Keys with a tier in brackets are
written as separate keys (`prod.cost_high`).

*Round 3 (SYS-1, SYS-3, PRD-4, PRD-5), proposed and built:* the `happy.*` keys replace
`pop.happiness_drain`, `pop.starve_happiness`, `pop.p_base`, `pop.p_happy`,
`lux.consumption`, `lux.variety_min`, `lux.variety_bonus` and `good.happiness`, which
are gone.

| Key | Meaning | Start |
|---|---|---|
| `good.base_price[g]` | Price at reserve | life support 10, luxuries 30, contraband 60 |
| `good.reserve_days[g]` | Reserve target in days of consumption | life support 30, luxuries 15, contraband 15 |
| `good.k[g]` | Price exponent (steeper is more sensitive) | life support 1.5, luxuries 1.0, contraband 1.0 |
| `good.consumption[g]` | Per thousand people per day | life support 1.0 each; contraband 0.2 |
| `good.yield[g]` | Output per unit of labour, against life support's (SYS-2) *(proposed, round 3: at 1, then 1/3, luxuries glutted)* | life support and contraband 1, regional luxuries 1/6, fringe luxuries 1/3 |
| `price.min`, `price.max` | Clamp on price ÷ base | 0.2, 5 |
| `price.spread` | Ask–bid spread | 0.06 |
| `price.chunk` | Trade chunk as share of reserve | 0.05 |
| `price.bands` | Player bands of price ÷ base | <0.6 very low, <0.9 low, <1.2 normal, <2 high, else very high |
| `econ.tax` | Transaction tax on traders | 0.05 |
| `econ.wage` | Purchasing power per thousand per day | 40 |
| `econ.credit_limit` | Months of income a planet may owe before it stops buying | 2 |
| `pop.growth_rate` | Daily growth at full excess | 0.0005 |
| `pop.shrink_rate` | Daily shrink at total shortfall | 0.002 |
| `pop.full_excess` | The scarcest life-support good's stock over reserve, as a share of the reserve, at which growth is full (SYS-1) *(proposed, round 4; was 1)* | 0.5 |
| `pop.growth_factor_min`, `_max` | Governor growth factor range (PRD-3) | 0.5, 1.5 |
| `pop.contraband_penalty` | Productivity lost per unit of recent contraband per thousand | 4 |
| `pop.contraband_window` | Days in the contraband average | 30 |
| `lux.max_fill` | Fill above which luxury use stops rising | 1.5 |
| `happy.need` | Each luxury's full use, per thousand per day | 0.1 |
| `happy.sated` | How much a happy population cuts its luxury use | 1 |
| `happy.base` | Target happiness with no luxuries | −0.3 |
| `happy.lux` | Target happiness per luxury, × ln(1 + use ÷ need) | 0.25 |
| `happy.contraband` | The same for contraband, more than a legal luxury (PRD-6) | 0.5 |
| `happy.hunger` | Target happiness lost at total life-support shortfall | 1 |
| `happy.settle_days` | Days for happiness to close most of the gap to its target | 30 |
| `happy.scale` | Happiness at which effects are about three quarters of their full size | 0.5 |
| `happy.p_up`, `happy.p_down` | Productivity's ceiling and floor: 1 + up, 1 − down | 0.3, 0.2 |
| `happy.g_up`, `happy.g_down` | Growth's boost when happy, and its cut when unhappy (1 stops it) | 0, 1 |
| `happy.want` | An unhappy planet's luxury reserves grow by up to this share | 1 |
| `prod.cost[tier]`, `prod.cost_contraband` | The cost of making a unit, × the good's base price, by tier; contraband's (PRD-7, SYS-2) *(proposed, round 4; replaced `prod.efficiency`, high 1.0, medium 0.6, low 0.3)* | high 0.3, medium 0.5, low 0.8; 0.1 |
| `prod.labour_per_capita` | Output per thousand at share 1, P 1, every tier alike *(Phase A round 1: 8, was 3.5; round 4: 5)* | 5 |
| `prod.labour_shift` | Share of labour moved per week | 0.05 |
| `event.drought_p`, `event.drought_days` | Drought chance per producer per day, length | 0.0005, 30 |
| `event.bumper_p`, `event.bumper_days`, `event.bumper_mult` | Bumper harvest | 0.0005, 15, 1.3 |
| `event.festival_p`, `event.festival_days`, `event.festival_mult` | Festival | 0.0005, 14, 1.5 |
| `event.region_drought_p` | Regional drought per region per day | 0.0002 |

## Money (ECO, SYS-5)

| Key | Meaning | Start |
|---|---|---|
| `econ.op_base`, `econ.op_per_capita` | Planet operating cost per month *(round 4: per capita was 300; making goods now sinks money too)* | 2,000, 250 |
| `dues.access`, `dues.gate` | Authority dues per month (ECO-10) | 500, 1,500 |
| `authority.maintenance` | Share of dues spent on the network (a sink) | 0.6 |
| `authority.clear_days` | Length of a clearing contract | 30 |
| `authority.risk_post`, `authority.pay`, `authority.contracts`, `authority.opinion_per_job` | A sector this risky near the network gets a clearing contract, at this pay a day, at most this many a week; the Authority's opinion of a mercenary who completes one rises by this *(proposed, MS-6)* | 0.3, 300, 3, 5 |
| `budget.credit_risk` | How far into the red risk tolerance lets a spender plan | 1 month of income × risk |
| `budget.diminishing` | Value multiplier per further unit of one option | 0.6 |
| `budget.debt_pressure` | Cost multiplier per month in the red | 0.1 |
| `fuel.base`, `fuel.fringe`, `fuel.burn` | Fuel price and use (GEN-7, SYS-21); `fuel.burn` is per sector width of Lattice travel per ship (a sector width is about a Lattice km at the prototype's scale) | 2, 1.0, 1 |
| `fuel.fringe_mult` | Fuel price multiplier off the network (GEN-7) *(proposed, MS-2)* | 1.5 |
| `value.defence`, `value.power_ref` | A fleet of `power_ref` combat power is worth `defence` of the governor's monthly income per month (SYS-5) *(proposed, MS-4)* | 0.01, 10 |
| `value.patronage_months` | Patronage is worth this many months of the tribute it keeps flowing (SYS-5) *(proposed, MS-4)* | 12 |

## Travel (TIM-4)

Reference speeds for planning and osmosis, in sector widths per game day (neighbouring
sector centres are 1 apart). Ship variants bring their own open-space speeds (MS-3).
*(proposed, MS-2)*

| Key | Meaning | Start |
|---|---|---|
| `travel.lattice_speed` | On the Lattice | 2 |
| `travel.space_speed` | In open space (a freighter) | 0.15 |
| `travel.hop_days` | One hop lane | 0.25 |

## Trade (TRD, SYS-6, SYS-7)

| Key | Meaning | Start |
|---|---|---|
| `osmosis.range_days` | Markets this close exchange | 3 |
| `osmosis.rate` | Daily share of the fill gap moved *(proposed, round 4; was 0.01, SYS-6)* | 0.2 |
| `osmosis.falloff` | Travel-time falloff | 2 |
| `osmosis.max_close` | Most of a gap osmosis may close | 0.5 |
| `merchant.candidates` | Known planets considered per trip | 12 |
| `merchant.min_score` | Profit per day below which a merchant moves on | 50 |
| `merchant.loss_rate` | Expected cargo loss per unit of risk | 0.1 |
| `merchant.threat_ref` | Threat scale for escort protection | 5 |
| `merchant.info_value` | Value of faction information per month | 300 |
| `merchant.salary` | Monthly salary every root offers its merchants until the budget model sets it (MS-4) *(proposed, MS-3)* | 600 |
| `merchant.per_planet` | Salaried merchants a root wants per faction planet *(proposed, MS-3)* | 1 |
| `merchant.stale_days` | Prices older than this send an idle merchant to look again (TRD-4) *(proposed, MS-3)* | 30 |
| `merchant.dock_hours` | Time at each stop, loading and unloading (TRD-12) *(proposed, MS-3)* | 6 |

## Characters and contracts (CHR, REL, SYS-8 to SYS-11, SYS-18)

| Key | Meaning | Start |
|---|---|---|
| `char.target` | Target character count | dev 500, v1 2,000 |
| `char.spawn_per_day` | Most new characters a day | 3 |
| `char.risk_dist` | Risk tolerance distribution (built in, not a key) | uniform 0–1 |
| `char.start_wealth` | New character's money | 2,000 |
| `char.start_armed_p` | Chance a new character gets an armed hull | 0.5 |
| `char.broke_days` | Days below zero before broke | 14 |
| `fleet.run_cost[tier]` | Running cost per ship per day | 5, 15, 40 |
| `fleet.munitions_per_power` | Refill cost per combat power used | 100 |
| `fleet.max_ships` | Most ships in one character's fleet; past it a purchase replaces the cheapest *(proposed, MS-3)* | 4 |
| `ship.<class>.combat`, `.cargo`, `.speed`, `.price` | Tier-1 stats of each class: courier, scout, freighter, gunship (MOD-1). Content, to be designed (OPN-35) *(proposed, MS-3)* | courier 0, 60, 0.3, 8,000; scout 2, 20, 0.25, 10,000; freighter 0.5, 400, 0.12, 20,000; gunship 5, 30, 0.18, 30,000 |
| `ship.tier_combat`, `_cargo`, `_speed`, `_price` | Multiplier per tier above 1 *(proposed, MS-3)* | 1.8, 2, 1.05, 2.5 |
| `ship.culture_spread` | Each culture's variant of a class within ± this *(proposed, MS-3)* | 0.15 |
| `ship.resale` | Share of the price a sold ship returns *(proposed, MS-3)* | 0.5 |
| `mission.pay_growth` | Pay growth per day unfilled | 0.05 |
| `mission.expiry_days` | Mission lifetime | 30 |
| `mission.courier_pay`, `mission.scout_pay` | Base pay per day of the expected round trip *(proposed, MS-3)* | 150, 300 |
| `skill.gain`, `skill.max`, `skill.bonus` | Skill per mission, cap, pay weighting | 1, 20, 0.02 |
| `spend.threshold_base`, `spend.threshold_risk` | Wealth over ship price before buying: `base − risk × this` | 2, 1 |
| `spend.reserve_months` | Running costs kept back by the cautious | 3 × (1 − risk) |
| `hire.range_days`, `hire.stipend_per_power` | Hiring reach, stipend per combat power per month | 5, 300 |
| `hire.w_power`, `hire.w_opinion`, `hire.w_culture` | Hiring weights | 1, 0.05, 0.5 |
| `hire.rep_mult`, `hire.rep_half` | A stipend rises by up to `rep_mult` with reputation, half of it at `rep_half` (MIL-2) *(proposed, war and fealty round)* | 1, 10 |
| `loan.fee_months` | A liege borrowing a vassal's commanders pays the vassal this many months of their stipends (MIL-3) *(proposed, war and fealty round)* | 1 |
| `loan.relief_fee` | A vassal borrowing its liege's commanders for relief pays this many months of their stipends on top (MIL-11) *(proposed, war and fealty round)* | 0 |
| `loan.max_days` | Longest a loan runs if its purpose doesn't end it first (MIL-3) *(proposed, war and fealty round)* | 60 |
| `relief.shortfall`, `relief.income_per_head`, `relief.capture_months` | Need tests for asking tribute relief: life-support shortfall at least this; net income per thousand a month below this; captured within this many months (FAC-15) *(proposed, war and fealty round)* | 0.1, 200, 6 |
| `relief.rate`, `relief.clear_weeks` | The tribute rate granted on relief, and the weeks passing no test before it ends (FAC-15) *(proposed, war and fealty round)* | 0.05, 8 |
| `rel.fade` | Weekly share of opinion lost toward rest | 0.03 |
| `rel.culture_rest` | Rest shift from culture share | 20 |
| `rel.recent_deeds` | Deeds kept per relationship for the UI (REL-12) *(proposed, MS-4)* | 3 |
| `deed.<kind>` | Opinion change per deed (REL-3), as `deed.help_sent` and so on | help sent +15, help missed −20, planet granted +25, patronage +10, tribute raised −10, passed over −10, call answered +10, call refused −15, border held +5, job done +5, attacked −30, tribute lowered +5 *(proposed, MS-4)*. *(proposed, war and fealty round)* Call answered and call refused apply to loan requests (MIL-3); relief used −5, relief granted +10, relief refused −15, objective declined −3, bounty unpaid −10 |
| `fealty.cascade` | Tribute cascades (FAC-6) | true |
| `fealty.tribute_rate` | Starting tribute rate | 0.15 |
| `fealty.rate_step`, `fealty.rate_min`, `fealty.rate_max` | How far a liege moves a tribute rate at once, and its bounds (FAC-12) *(proposed, MS-4)* | 0.05, 0, 0.5 |
| `fealty.raise_opinion` | A liege in the red raises tribute only on vassals at least this loyal (FAC-12) *(proposed, MS-4)* | 20 |
| `fealty.defect_opinion`, `fealty.defect_weeks` | Defection threshold and duration | −30, 8 |
| `fealty.threat_days`, `fealty.rival_opinion` | Independence safety, rival offer threshold | 10, 20 |
| `fealty.w_favor`, `fealty.capture_bonus` | Keep-or-grant and governor choice weights | 1, 30 |
| `pod.death_p` | Out-of-range pod death chance | 0.05 |

## War (DIP, MIL, SYS-12 to SYS-14)

| Key | Meaning | Start |
|---|---|---|
| `dip.neighbour_days` | Lattice days within which factions border | 3 |
| `dip.w_idle`, `dip.w_targets`, `dip.w_strength`, `dip.w_peace`, `dip.w_wars`, `dip.w_losses` | War vote weights *(proposed; Phase A round 1, from 0.2, 0.3, 1, 0.5, 1.5, 0.5)* | 0.1, 0.05, 0.1, 0.5, 1.5, 0.5 |
| `dip.border_days`, `dip.declare_threshold` | War vote reach and threshold *(threshold proposed; Phase A round 1, from 1)* | 5, 1.5 |
| `dip.w_treasury` | Money beyond `budget.reserve_months` of operating costs votes for war, up to `budget.hoard_cap` (DIP-4) *(proposed, round 2; the reserve and cap moved to `budget.*` in round 3)* | 0.3 |
| `dip.s_planets`, `dip.s_power`, `dip.s_treasury`, `dip.s_months`, `dip.sue_threshold` | Suing-for-peace score; `s_months` is weariness a month at war *(proposed; round 2 replaced `s_wars` 1)* | 3, 2, 1, 0.4, 1 |
| `dip.min_war_days` | No offer is heard before this (DIP-3) *(proposed, round 2)* | 30 |
| `dip.hope_share`, `dip.hope_halflife_months` | The winner's hope: this share of the loser's income near its border, by power share, halving this often (DIP-3) *(proposed, round 2)* | 0.5, 2 |
| `dip.tribute_rate`, `dip.ceasefire_days` | Ceasefire tribute and length (DIP-2) | 0.1, 90 |
| `army.muster_days`, `army.detour_ratio` | Muster timeout, detour budget | 14, 0.15 |
| `combat.lethality` | Power lost per round per enemy power | 0.05 |
| `combat.noise` | Randomness per round | 0.3 |
| `combat.retreat_base`, `combat.retreat_risk` | Retreat when own ÷ enemy falls below `base − risk × this` | 0.6, 0.4 |
| `combat.retreat_loss`, `combat.cargo_destroyed` | Retreat losses, cargo destroyed | 0.2, 0.3 |
| `defence.per_spend` | Static defence per credit of security spend per month *(proposed; round 2 0.01, from 0.005; round 3 0.0015, with spending sized to income)* | 0.0015 |
| `defence.regen` | Share of the maximum static defence rebuilt a day, when not besieged *(proposed, MS-5)* | 0.05 |
| `value.security_share`, `value.security_ratio` | A security unit is this share of monthly income, replacing `value.security_unit` for defence; each is worth ratio × its cost × threat × hoard (SYS-5) *(proposed, round 3)* | 0.02, 2 |
| `budget.hoard`, `budget.reserve_months`, `budget.hoard_cap` | Money beyond the reserve raises defence values by up to hoard × cap; the war vote's reserve and cap (SYS-5, DIP-4) *(proposed, round 3)* | 1, 6, 3 |
| `value.blockade_months`, `value.retake_months` | Blockading an enemy's gate, and retaking one's own, are worth this many months of the income of the planets it serves (MIL-17) *(proposed, round 2)* | 8, 8 |
| `blockade.enabled`, `blockade.defence_share`, `blockade.hold_days` | Gate attacks on or off; a gate's static defence against its holder's; how long the captor's army garrisons it (MIL-17) *(proposed, round 2)* | true, 0.7, 30 |
| `value.war_mult`, `value.border_mult` | Defence is worth `1 + war_mult` at war, `+ border_mult` with an enemy planet within `dip.border_days` *(proposed, MS-5)* | 2, 1 |
| `value.security_unit` | Policing is bought in units of this much a month *(proposed, MS-5; static defence used it until round 3)* | 1,000 |
| `value.invade_months`, `value.gate_months`, `value.aftermath_months` | A planet taken is worth this many months of its income, more for a gate holder, less rebuilding *(proposed; round 2 raised aftermath from 2)* | 12, 6, 8 |
| `value.protect` | Keep or grant: protection cost per day of travel from the keeper *(proposed, MS-5)* | 0.1 |
| `capture.happiness_drop`, `capture.stock_loss`, `capture.pop_loss` | Capture effects (FAC-10): happiness lost (round 3: subtracted, was a share), stock and population shares lost | 0.4, 0.5, 0.05 |
| `capture.hold_days` | A conquering army that chooses to hold the planet garrisons it this long (SYS-13) *(proposed, round 2; a choice from round 3)* | 30 |
| `bounty.share` | The root pays this share of an enemy ship's price to the commander whose fleet destroyed it (MIL-18) *(proposed, war and fealty round)* | 0.5 |
| `spoils.per_prosperity` | A captured planet yields this × population × productivity from its treasury, split by power × days in the siege (MIL-19). About half a month of a typical planet's gross income *(proposed, war and fealty round)* | 400 |

## Information, piracy, suspicion (INF, PIR, SYS-15 to SYS-17, SYS-19)

| Key | Meaning | Start |
|---|---|---|
| `courier.interval_days`, `courier.run_cost` | Automated courier cadence and cost | 7, 50 |
| `courier.mission_share` | Share of runs posted as courier missions instead (INF-8) *(proposed, MS-3)* | 0.25 |
| `scout.min_combat`, `scout.stale_days` | Combat power a scout mission needs; a root scouts outside markets it hasn't heard of for this long *(proposed, MS-3)* | 1, 60 |
| `space.detect_range` | Detection range in open space, sector widths (the rendered 4 km) *(proposed, MS-3)* | 0.25 |
| `space.meet_cooldown_hours` | A character shares in open space at most this often *(proposed, MS-3)* | 6 |
| `scout.threat_days` | Threat range that triggers scouting | 7 |
| `risk.window`, `risk.presence`, `risk.clear` | Risk average and weights (SYS-19); presence is per hostile ship-day, clear per patrol day | 30, 0.2, 0.5 |
| `space.engage_p`, `space.evade` | Chance per step a hostile in range is engaged; a faster quarry's chance to slip away, × its speed edge *(proposed, MS-5)* | 0.5, 0.5 |
| `raid.enabled`, `raid.share`, `raid.days`, `raid.standoff` | Raiding at war: on (EXP-1 switches it off), the weekly chance a spare commander is sent, days held, distance short of the enemy planet in sector widths *(proposed, MS-5)* | true, 0.3, 10, 0.15 |
| `escort.rate` | What an escort asks per combat power per day of the trip *(proposed, MS-5)* | 20 |
| `security.risk_post`, `security.days`, `security.pay` | A planet whose sector's believed risk passes this posts a patrol of this many days, at this pay a day *(proposed, MS-5)* | 0.5, 10, 200 |
| `pirate.flip_debt` | Debt, in months of gross income, at which debt's share of the rebellion chance is full (ECO-13) *(proposed, round 4; was 3 months of operating costs)* | 1 |
| `rebel.scale`, `rebel.power`, `rebel.w_debt` | Weekly failure chance = scale × (u(H)^power below neutral + w_debt × debt share), replacing `pirate.flip_days`, `rebel.happiness`, `rebel.shortfall` and `rebel.days` (SYS-16) *(proposed, round 3: a scale of 0.01 flipped 9% of planets a year)* | 0.007, 2, 1 |
| `pirate.cost_mult` | Pirate planet operating cost multiplier | 0.3 |
| `pirate.turn_all` | EXP-9's switch: every broke commander who knows a pirate planet turns pirate, instead of with chance equal to its risk tolerance | false |
| `pirate.range_days` | How far pirates go raiding, in days of open space at the reference speed *(proposed, MS-6)* | 20 |
| `pirate.commanders` | Pirate commanders at each starting pirate planet (GEN-9) *(proposed, MS-6)* | 2 |
| `space.engage_cooldown_hours` | After a fight in open space, neither side is engaged again for this long *(proposed, MS-6)* | 12 |
| `pirate.salary` | Pirate stipend ÷ lawful stipend | 0.6 |
| `pirate.need_fill`, `pirate.surplus_fill` | Fills that trigger pirate missions | 0.5, 1.2 |
| `pirate.smuggler_risk` | Risk tolerance above which NPCs smuggle | 0.7 |
| `pirate.contraband_price`, `pirate.demand_cap` | Lawful contraband price and demand cap | base × 1.5, 0.3 per thousand |
| `pirel.accept`, `pirel.success`, `pirel.fail`, `pirel.rest`, `pirel.fade` | Pirate relation (PIR-9) | to 0, +15, to −50, −50, 0.02 |
| `pirate.legal_bonus` | A legal-goods run's pay per unit delivered, on top of the sale *(proposed, MS-6)* | 5 |
| `sus.inspect_base`, `sus.war`, `sus.police_per_unit` | P(inspect) = base × (1 + police_per_unit per `value.security_unit` of monthly policing + war when at war) *(proposed, MS-6)* | 0.05, 0.5, 0.5 |
| `sus.find_base`, `sus.scale` | An inspection catches with min(1, find_base + suspicion ÷ scale) *(proposed, MS-6)* | 0.3, 100 |
| `sus.contraband`, `sus.pirate`, `sus.trade`, `sus.caught`, `sus.fade` | Suspicion per landing with contraband, per point of pirate relation above neutral per landing, removed per genuine trade, added when caught; weekly fade *(proposed, MS-6)* | 10, 0.2, 2, 30, 0.05 |
| `sus.police_effect` | Budget value of policing: × the share of income contraband costs *(proposed, MS-6)* | 2 |
| `sus.no_trade`, `sus.muster` | Suspicion for a visit without trading, and for lingering where an army is forming (INF-14) *(proposed, MS-7)* | 15, 10 |
| `spy.enabled` | EXP-4's switch | true |
| `spy.salary`, `spy.range_days`, `spy.value` | A spy's secret monthly pay; how far from home it works; its budget value as a share of monthly income × threat *(proposed, MS-7)* | 500, 10, 0.005 |
| `deed.caught` | Opinion change when a governor's inspection catches someone (INF-14) *(proposed, MS-6)* | −20 |

## World generation (GEN)

| Key | Meaning | Start |
|---|---|---|
| `gen.width`, `gen.height` | Map size | dev 6×8, v1 10×15 |
| `gen.planets` | Target planet count | dev 30, v1 100 |
| `gen.regions`, `gen.seats` | Culture regions, seats of power | dev 3 and 3 |
| `gen.fringe_share` | Share of systems off the network | 0.15 |
| `gen.lattice_loops` | Extra edges beyond the spanning tree | 0.3 |
| `gen.min_producers` | Producers per good per region | 2 |
| `gen.independent_share`, `gen.pirate_planets` | Starting independents and pirate planets | 0.1, 2 |
| `gen.commanders_per_income` | Starting combat power sworn to a governor per credit of its planet's monthly income (GEN-9) *(proposed, MS-4)* | 0.00001 |
| `gen.start_treasury_months`, `gen.warmup_days` | Starting treasury, warm-up | 3, 180 |
| `gen.moon_share` | Share of planets with one to three moons (WLD-4) | 0.25 |
| `gen.split_planets` | Planets in a system that make it two groups (GEN-3) *(proposed, MS-2)* | 4 |
| `gen.pop_median` | Median starting population, thousands (GEN-3) *(proposed, MS-2)* | 1,000 |
| `gen.pop_spread`, `gen.region_pop_spread` | Log-normal spread per planet and per region *(proposed, MS-2)* | 0.8, 0.3 |
| `gen.moon_pop` | A moon's population against a planet's *(proposed, MS-2)* | 0.3 |
| `gen.start_happiness` | Starting happiness (GEN-10) *(proposed, MS-2; round 3: 0, neutral, was 0.5)* | 0 |
| `gen.weight_spread`, `gen.favour_mult` | Region production weights `1 ± spread`, and the multiplier for a region's favoured life-support good and its home luxuries (GEN-6) *(proposed, MS-2)* | 0.5, 3 |
| `gen.lux_home_regions`, `gen.lux_away`, `gen.fringe_lux_mult`, `gen.fringe_ls_mult` | Home regions per regional luxury; a luxury's weight away from home; fringe planets' weight for the fringe luxuries, and for life support (GEN-6) *(proposed, round 3: a fringe luxury weight of 5, with life support at 1, starved most fringe planets to a tenth)* | 2, 0.1, 2, 3 |
