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
`good.food.base_price`. The goods are food, water and oxygen (life support), wine, spice
and silk (luxuries) and stims (contraband). Luxuries have no `consumption` key (they use
`lux.consumption`) and life support no `happiness` key. Keys with a tier or count in
brackets are written as separate keys (`prod.efficiency_high`) or a list
(`lux.variety_bonus = [0, 0, 0.003, 0.006]`).

| Key | Meaning | Start |
|---|---|---|
| `good.base_price[g]` | Price at reserve | life support 10, luxuries 30, contraband 60 |
| `good.reserve_days[g]` | Reserve target in days of consumption | life support 30, luxuries 15, contraband 15 |
| `good.k[g]` | Price exponent (steeper is more sensitive) | life support 1.5, luxuries 1.0, contraband 1.0 |
| `good.consumption[g]` | Per thousand people per day | life support 1.0 each; contraband 0.2 |
| `good.happiness[g]` | Happiness per unit per thousand | luxuries 0.02, contraband 0.06 |
| `price.min`, `price.max` | Clamp on price ÷ base | 0.2, 5 |
| `price.spread` | Ask–bid spread | 0.06 |
| `price.chunk` | Trade chunk as share of reserve | 0.05 |
| `price.bands` | Player bands of price ÷ base | <0.6 very low, <0.9 low, <1.2 normal, <2 high, else very high |
| `econ.tax` | Transaction tax on traders | 0.05 |
| `econ.wage` | Purchasing power per thousand per day | 40 |
| `econ.credit_limit` | Months of income a planet may owe before it stops buying | 2 |
| `pop.growth_rate` | Daily growth at full excess | 0.0005 |
| `pop.shrink_rate` | Daily shrink at total shortfall | 0.002 |
| `pop.growth_factor_min`, `_max` | Governor growth factor range (PRD-3) | 0.5, 1.5 |
| `pop.happiness_drain` | Daily drain | 0.01 |
| `pop.p_base`, `pop.p_happy` | Productivity = base + happy × H | 0.6, 0.6 |
| `pop.contraband_penalty` | Productivity lost per unit of recent contraband per thousand | 4 |
| `pop.contraband_window` | Days in the contraband average | 30 |
| `lux.consumption` | Luxury use per thousand at full fill | 0.5 |
| `lux.max_fill` | Fill above which luxury use stops rising | 1.5 |
| `lux.variety_min` | Share of full use that counts toward variety | 0.2 |
| `lux.variety_bonus[n]` | Daily bonus for n luxuries | 0, 0, 0.003, 0.006 |
| `prod.efficiency[tier]` | Output multiplier by tier | high 1.0, medium 0.6, low 0.3 |
| `prod.labour_per_capita` | Output per thousand at share 1, efficiency 1, P 1 | 3.5 |
| `prod.labour_shift` | Share of labour moved per week | 0.05 |
| `event.drought_p`, `event.drought_days` | Drought chance per producer per day, length | 0.0005, 30 |
| `event.bumper_p`, `event.bumper_days`, `event.bumper_mult` | Bumper harvest | 0.0005, 15, 1.3 |
| `event.festival_p`, `event.festival_days`, `event.festival_mult` | Festival | 0.0005, 14, 1.5 |
| `event.region_drought_p` | Regional drought per region per day | 0.0002 |

## Money (ECO, SYS-5)

| Key | Meaning | Start |
|---|---|---|
| `econ.op_base`, `econ.op_per_capita` | Planet operating cost per month | 2,000, 300 |
| `dues.access`, `dues.gate` | Authority dues per month (ECO-10) | 500, 1,500 |
| `authority.maintenance` | Share of dues spent on the network (a sink) | 0.6 |
| `authority.clear_days` | Length of a clearing contract | 30 |
| `budget.credit_risk` | How far into the red risk tolerance lets a spender plan | 1 month of income × risk |
| `budget.diminishing` | Value multiplier per further unit of one option | 0.6 |
| `budget.debt_pressure` | Cost multiplier per month in the red | 0.1 |
| `fuel.base`, `fuel.fringe`, `fuel.burn` | Fuel price and use (GEN-7, SYS-21) | 2, 1.0, 1 per Lattice km per ship |
| `fuel.fringe_mult` | Fuel price multiplier off the network (GEN-7) *(proposed, MS-2)* | 1.5 |
| `value.*` | Option value heuristics for the budget model (SYS-5) | set in MS-4 |

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
| `osmosis.rate` | Daily share of the fill gap moved | 0.01 |
| `osmosis.falloff` | Travel-time falloff | 2 |
| `osmosis.max_close` | Most of a gap osmosis may close | 0.5 |
| `merchant.candidates` | Known planets considered per trip | 12 |
| `merchant.min_score` | Profit per day below which a merchant moves on | 50 |
| `merchant.loss_rate` | Expected cargo loss per unit of risk | 0.1 |
| `merchant.threat_ref` | Threat scale for escort protection | 5 |
| `merchant.info_value` | Value of faction information per month | 300 |

## Characters and contracts (CHR, REL, SYS-8 to SYS-11, SYS-18)

| Key | Meaning | Start |
|---|---|---|
| `char.target` | Target character count | dev 500, v1 2,000 |
| `char.spawn_per_day` | Most new characters a day | 3 |
| `char.risk_dist` | Risk tolerance distribution | uniform 0–1 |
| `char.start_wealth` | New character's money | 2,000 |
| `char.start_armed_p` | Chance a new character gets an armed hull | 0.5 |
| `char.broke_days` | Days below zero before broke | 14 |
| `fleet.run_cost[tier]` | Running cost per ship per day | 5, 15, 40 |
| `fleet.munitions_per_power` | Refill cost per combat power used | 100 |
| `mission.pay_growth` | Pay growth per day unfilled | 0.05 |
| `mission.expiry_days` | Mission lifetime | 30 |
| `skill.gain`, `skill.max`, `skill.bonus` | Skill per mission, cap, pay weighting | 1, 20, 0.02 |
| `spend.threshold(risk)` | Wealth over ship price before buying | 2 − risk |
| `spend.reserve_months` | Running costs kept back by the cautious | 3 × (1 − risk) |
| `hire.range_days`, `hire.stipend_per_power` | Hiring reach, stipend per combat power per month | 5, 300 |
| `hire.w_power`, `hire.w_opinion`, `hire.w_culture` | Hiring weights | 1, 0.05, 0.5 |
| `rel.fade` | Weekly share of opinion lost toward rest | 0.03 |
| `rel.culture_rest` | Rest shift from culture share | 20 |
| `deed.*` | Opinion change per deed (REL-3) | help sent +15, help missed −20, planet granted +25, patronage +10, tribute raised −10, passed over −10, call answered +10, call refused −15, border held +5, job done +5, attacked −30 |
| `fealty.cascade` | Tribute cascades (FAC-6) | true |
| `fealty.tribute_rate` | Starting tribute rate | 0.15 |
| `fealty.defect_opinion`, `fealty.defect_weeks` | Defection threshold and duration | −30, 8 |
| `fealty.threat_days`, `fealty.rival_opinion` | Independence safety, rival offer threshold | 10, 20 |
| `fealty.w_favor`, `fealty.capture_bonus` | Keep-or-grant and governor choice weights | 1, 30 |
| `pod.death_p` | Out-of-range pod death chance | 0.05 |

## War (DIP, MIL, SYS-12 to SYS-14)

| Key | Meaning | Start |
|---|---|---|
| `dip.neighbour_days` | Lattice days within which factions border | 3 |
| `dip.w_idle`, `dip.w_targets`, `dip.w_strength`, `dip.w_peace`, `dip.w_wars`, `dip.w_losses` | War vote weights | set in MS-5 |
| `dip.border_days`, `dip.declare_threshold` | War vote reach and threshold | 5, set in MS-5 |
| `dip.s_*`, `dip.sue_threshold` | Suing-for-peace score | set in MS-5 |
| `dip.tribute_rate`, `dip.ceasefire_days` | Ceasefire tribute and length (DIP-2) | 0.1, 90 |
| `army.muster_days`, `army.detour_ratio` | Muster timeout, detour budget | 14, 0.15 |
| `combat.lethality` | Power lost per round per enemy power | 0.05 |
| `combat.noise` | Randomness per round | 0.3 |
| `combat.retreat_ratio(risk)` | Retreat when own ÷ enemy below | 0.6 − 0.4 × risk |
| `combat.retreat_loss`, `combat.cargo_destroyed` | Retreat losses, cargo destroyed | 0.2, 0.3 |
| `defence.per_spend` | Static defence per credit of security spend per month | set in MS-5 |
| `capture.happiness_loss`, `capture.stock_loss`, `capture.pop_loss` | Capture effects (FAC-10) | 0.4, 0.5, 0.05 |

## Information, piracy, suspicion (INF, PIR, SYS-15 to SYS-17, SYS-19)

| Key | Meaning | Start |
|---|---|---|
| `courier.interval_days`, `courier.run_cost` | Automated courier cadence and cost | 7, 50 |
| `scout.threat_days` | Threat range that triggers scouting | 7 |
| `risk.window`, `risk.presence`, `risk.clear` | Risk average and weights (SYS-19) | 30, 0.2, 0.5 |
| `pirate.flip_days`, `pirate.flip_debt` | Deficit days and debt before a planet turns | 60, 3 |
| `pirate.cost_mult` | Pirate planet operating cost multiplier | 0.3 |
| `pirate.turn_p(risk)` | Chance a broke commander turns pirate | risk (EXP-9: 1) |
| `pirate.salary` | Pirate stipend ÷ lawful stipend | 0.6 |
| `pirate.need_fill`, `pirate.surplus_fill` | Fills that trigger pirate missions | 0.5, 1.2 |
| `pirate.smuggler_risk` | Risk tolerance above which NPCs smuggle | 0.7 |
| `pirate.contraband_price`, `pirate.demand_cap` | Lawful contraband price and demand cap | base × 1.5, 0.3 per thousand |
| `pirel.accept`, `pirel.success`, `pirel.fail`, `pirel.rest`, `pirel.fade` | Pirate relation (PIR-9) | to 0, +15, to −50, −50, 0.02 |
| `sus.*` | Suspicion per behaviour, per trade, fade, inspection base | set in MS-6 |

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
| `gen.commanders_per_income` | Starting commanders | set in MS-4 |
| `gen.start_treasury_months`, `gen.warmup_days` | Starting treasury, warm-up | 3, 180 |
| `gen.moon_share` | Share of planets with one to three moons (WLD-4) | 0.25 |
| `gen.split_planets` | Planets in a system that make it two groups (GEN-3) *(proposed, MS-2)* | 4 |
| `gen.pop_median` | Median starting population, thousands (GEN-3) *(proposed, MS-2)* | 1,000 |
| `gen.pop_spread`, `gen.region_pop_spread` | Log-normal spread per planet and per region *(proposed, MS-2)* | 0.8, 0.3 |
| `gen.moon_pop` | A moon's population against a planet's *(proposed, MS-2)* | 0.3 |
| `gen.start_happiness` | Starting happiness (GEN-10) *(proposed, MS-2)* | 0.5 |
| `gen.weight_spread`, `gen.favour_mult` | Region production weights `1 ± spread`, and the favoured good's multiplier (GEN-6) *(proposed, MS-2)* | 0.5, 3 |
