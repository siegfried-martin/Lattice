# Systems (SYS)

Each system's state, step and starting formulas, citing the design rules it implements.
Every constant is a tunable (`tunables.md`); names in `code` are config keys. Where the
design leaves a mechanism open, the formula here is a working default and is tagged
`proposed`.

Notation: `N` population, `H` happiness (0–1), `P` productivity, `g` a good, `p` a
planet, `c` a character. Rates are per game day unless noted.

## Economy

### SYS-1 Population and consumption `v1`

*Implements:* PRD-2, PRD-3, PRD-4, PRD-5, ECO-1. *Cadence:* daily.

```
budget     = N × econ.wage × P                       purchasing power, new money (ECO-1)
need_g     = N × good.consumption[g]                  for food, water, oxygen
buy_g      = min(stock_g, need_g), limited by budget   life support first, at ask price
shortfall  = max over life support of (1 − buy_g / need_g)

if shortfall > 0:   N −= N × pop.shrink_rate × shortfall
else:               excess_g = (stock_g − reserve_g) / reserve_g    after buying
                    N += N × growth_factor × pop.growth_rate × max(0, min_g excess_g)

lux_total  = N × lux.consumption × clamp(Σ_l stock_l / Σ_l reserve_l, 0, lux.max_fill)
buy_l      = lux_total split by each luxury's share of luxury stock, limited by budget left
variety    = number of luxuries with buy_l ≥ lux.variety_min × N
H         += Σ_l buy_l × good.happiness[l] / N + lux.variety_bonus[variety]
             + contraband_bought × good.happiness[contraband] / N − pop.happiness_drain
H          = clamp(H, 0, 1)
C          = EMA over pop.contraband_window days of contraband bought / N
P          = (pop.p_base + pop.p_happy × H) × max(0, 1 − pop.contraband_penalty × C)
```

- Money the population pays goes to the planet's treasury (ECO-2). Contraband is bought
  from smugglers, not the market (SYS-16).
- `growth_factor` is `pop.growth_factor_min` + risk tolerance × (`max` − `min`) for the
  governor (PRD-3).
- *Interpretation:* "consumed in proportion to combined excess" is read as consumption
  scaling with how full the luxury stocks are against their reserves.
- *Tuning constraint* (PRD-6): at steady state, the productivity a population loses to
  contraband must exceed what its extra happiness gains, so
  `pop.contraband_penalty` > `pop.p_happy` × `good.happiness[contraband]` per unit.

### SYS-2 Production and labour `v1`

*Implements:* PRD-7, PRD-9. *Cadence:* output daily, labour weekly.

```
eff_g      = prod.efficiency[tier_g + event_shift_g]       tiers: high, medium, low
output_g   = N × prod.labour_per_capita × share_g × eff_g × P    added to stock_g

weekly:    score_g = bid_price_g × eff_g                    at the planet's own market
           move prod.labour_shift of total labour from the lowest-scoring good
           to the highest-scoring good (shares stay ≥ 0 and sum to 1)
```

Local price is the right signal: if merchants buy a good away, its stock falls and its
price rises; if it gluts, the price falls and labour moves off it.

### SYS-3 Markets and prices `v1`

*Implements:* PRD-10, ECO-3. *Cadence:* recomputed on every trade and daily.

```
reserve_g  = good.reserve_days[g] × N × good.consumption[g]
             (luxuries: reference consumption at full fill; contraband: pirate markets only)
price_g    = good.base_price[g] × clamp((reserve_g / max(stock_g, ε))^good.k[g],
                                         price.min, price.max)
ask_g      = price_g × (1 + price.spread / 2)     the planet sells
bid_g      = price_g × (1 − price.spread / 2)     the planet buys
trader pays ask × (1 + tax) and receives bid × (1 − tax); tax goes to the treasury
```

- **Slippage:** a trade executes in chunks of `price.chunk` × reserve, recomputing the
  price after each, so a big delivery walks the price.
- **The planet buys with its treasury.** A planet whose treasury is below its credit
  limit (`econ.credit_limit` × monthly income) stops buying.
- Lawful markets don't trade contraband (PIR-14).
- **Bands for the player** (`price.bands`) are a view of `price_g / base_price`.

### SYS-4 World events `v1`

*Implements:* PRD-11. *Cadence:* daily roll.

| Event | Chance | Effect | Length |
|---|---|---|---|
| Drought (planet) | `event.drought_p` per food producer | food efficiency one tier down | `event.drought_days` |
| Bumper harvest (planet) | `event.bumper_p` per producer, any good | one tier up (high stays high, ×`event.bumper_mult`) | `event.bumper_days` |
| Festival (planet) | `event.festival_p` | luxury consumption × `event.festival_mult` | `event.festival_days` |
| Regional drought | `event.region_drought_p` per region | drought on every food producer in the region | `event.drought_days` |

### SYS-5 Treasury and the governor's budget `v1`

*Implements:* ECO-3, ECO-4, ECO-8, MIL-5, PIR-15. *Cadence:* income daily, payments
monthly, decisions weekly.

```
income     = sales to population + sales and purchases taxed + tribute received
costs      = econ.op_base + econ.op_per_capita × N + patrol_upkeep + policing
             + stipends + tribute paid + dues (+ merchant salaries, roots only)
             + patronage given + mission pay + automated courier runs
treasury  += income − costs     (may go negative, ECO-8)
```

**The budget model** *(proposed)*, one routine for every spender (ECO-8):

1. List options, each with a believed value per period and a cost: hire or keep a
   commander, raise or cut patrols, policing, patronage to a vassal, post missions,
   change a vassal's tribute rate, commission an invasion (SYS-13).
2. Available = treasury + credit line, scaled by risk tolerance
   (`budget.credit_risk`).
3. Fund options in order of value ÷ cost, with each further unit in the same option
   worth `budget.diminishing` times the last, until money runs out. Risk tolerance
   lowers the diminishing rate (bold rulers concentrate).
4. A negative treasury adds a pressure term to every cost, growing with days in the red
   (`budget.debt_pressure`).

Starting value heuristics for options are in `tunables.md` under `value.*`. They're the
first thing Phase A tunes.

### SYS-6 Osmosis `v1`

*Implements:* TRD-13. *Cadence:* daily.

For each pair of markets within `osmosis.range_days` of each other, not at war, and each
lawful good:

```
fill_p     = stock_g / reserve_g at each market
gap        = fill_i − fill_j
flow       = osmosis.rate × gap × min(reserve_i, reserve_j) × exp(−travel_days / osmosis.falloff)
             capped at osmosis.max_close × the amount that would equalise the two fills
```

The flow executes as a buy at the source and a sell at the destination at market
prices with tax; the trader's margin is removed (a sink, ECO-5). Lattice travel time
uses the Lattice graph, so connected markets exchange faster.

## Characters and trade

### SYS-7 Merchants `v1`

*Implements:* TRD-3, TRD-4, TRD-8, TRD-9, TRD-10. *Cadence:* on arrival or when idle.

**Choosing a trip.** From its own facts, a merchant considers the `merchant.candidates`
nearest known planets by travel time:

```
leg_profit(A→B, g) = q × (believed bid_B,g × (1 − tax_B) − ask_A,g × (1 + tax_A))
                     q = min(cargo capacity, amount before slippage eats the margin)
trip_value         = Σ legs of best leg_profit − fuel − running cost × days − risk_cost
risk_cost          = cargo value × Σ open-space sector believed risk × merchant.loss_rate
score              = trip_value / days
```

- **Independent:** single trips from where it is. If no trip scores above
  `merchant.min_score`, move to a nearby system and look again.
- **Faction merchant:** the best trip among the three shapes (TRD-3), all starting and
  ending at a faction planet, outside planets not at war with the faction. Triangles are
  searched over the candidate set only.
- **Plans use believed prices** and can disappoint when they land.

**Escort decision** (per trip):

```
expected_loss(e) = cargo value × route risk × merchant.loss_rate × (1 − protection(e))
hire the escort e that maximises expected_loss(none) − expected_loss(e) − price(e),
if positive
protection(e)    = escort combat / (escort combat + merchant.threat_ref × route risk)
```

The offer is broadcast to characters in detection range (TIM-6) along the route; NPCs
accept by SYS-8. Paid at trip end (REL-9).

### SYS-8 Missions and careers `v1`

*Implements:* CHR-10, CHR-11, INF-10, INF-11. *Cadence:* on posting; on idle.

- **Posting:** governors post courier, scout and security missions from the budget
  (SYS-5); pirate planets post smuggling and legal-goods runs (SYS-16); merchants post
  escort offers (SYS-7). Faction-level missions are posted by being carried on a courier
  run (SYS-15).
- **Pay grows while unfilled:** `pay = base × (1 + mission.pay_growth × days_waiting)`,
  expiring after `mission.expiry_days`.
- **Choosing:** an idle character takes the available mission it meets the requirements
  for with the best `pay / days × (1 + skill.bonus × skill_kind)`.
- **Skill:** +`skill.gain` per mission of a kind completed, capped at `skill.max`.
- **Spending:** when `wealth > spend.threshold(risk) × price of next ship`, buy it or
  upgrade. Conservative characters keep `spend.reserve_months` of running costs back.
  What to buy follows the missions that have paid best recently: more combat or more
  cargo.

### SYS-9 Contracts and hiring `v1` `proposed`

*Implements:* REL-8, MIL-2, MIL-6, TRD-3. *Cadence:* weekly with the budget.

- **Commander service:** a governor hiring considers commanders it knows of without a
  service contract, within `hire.range_days`:
  `score = hire.w_power × combat power + hire.w_opinion × opinion + hire.w_culture × same culture`,
  against the stipend asked (`hire.stipend_per_power` × combat power). The best
  affordable candidate gets an offer, and accepts if it beats their current income.
- **Faction merchant salary:** the root sets a salary; an independent merchant in the
  faction's territory accepts when salary + `merchant.info_value` exceeds its estimate of
  what the trip-shape constraint costs it. It quits when that flips.
- **Ending:** either side ends a contract when it stops paying for them (REL-8). Losing
  the fleet ends service contracts automatically (CHR-16).

### SYS-10 Relationships `v1`

*Implements:* REL-2, REL-3. *Cadence:* deeds on events; fade weekly.

```
opinion in [−100, 100]
on a deed:  opinion += deed.<kind>          values in tunables.md
weekly:     opinion += (rest − opinion) × rel.fade
rest        = 0, or for a vassal's opinion of its liege:
              rel.culture_rest × (share of faction planets of the vassal's culture − 0.5)
```

Each character stores its last few deeds per relationship for the UI (REL-12).

### SYS-11 Fealty, tribute, patronage and defection `v1`

*Implements:* FAC-5 to FAC-12. *Cadence:* tribute monthly; checks weekly.

```
tribute_paid = tribute_rate × (own income + tribute received if fealty.cascade)
```

- **Setting tribute** is a budget option for the liege (SYS-5): raising it is a deed
  (`deed.tribute_raised`).
- **Patronage** is money paid into the vassal's treasury. It needs no special rebuilding
  rule: with money the planet buys life support and luxuries, keeps patrols and recovers
  (FAC-7). It's a deed (`deed.patronage`).
- **Defection** *(proposed)*: weekly, if a governor's opinion of its liege has been below
  `fealty.defect_opinion` for `fealty.defect_weeks` weeks and an alternative exists, it
  defects. Alternatives: **independence** if it believes no hostile army within
  `fealty.threat_days`; **a rival** if a neighbouring root's opinion of it is above
  `fealty.rival_opinion`. It picks the better.
- **Keep or grant** *(proposed)*, at the governor G whose army took the planet:
  `keep = income share kept − protection cost(G's travel time to the planet)`;
  `grant to V = fealty.w_favor × opinion of V + protection value(V's travel time) − income given up`.
  The highest wins; a grant recurses at V.
- **Choosing the governor** *(proposed)*: candidates the keeper knows (its commanders and
  characters with a contract in its tree), scored
  `rep + opinion + fealty.capture_bonus if they led the capture`.

### SYS-12 Diplomacy `v1` `proposed`

*Implements:* DIP-1 to DIP-5. *Cadence:* weekly per root.

**War vote**, between peaceful neighbours, using each governor's own beliefs:

```
weight      = 1 + planets in the governor's branch
vote        = (dip.w_idle × idle combat power under them
             + dip.w_targets × believed wealth of target planets within dip.border_days
             + dip.w_strength × (believed own strength / believed target strength − 1)
             + dip.w_peace × prosperity growth since the last war
             − dip.w_wars × wars ongoing − dip.w_losses × recent losses)
             × (0.5 + risk tolerance)
declare if Σ weight × vote / Σ weight > dip.declare_threshold
```

**Suing for peace:**

```
war_score  = − dip.s_planets × planets lost share − dip.s_power × fleet power lost share
             − dip.s_treasury × treasury trend − dip.s_wars × (wars − 1)
if war_score < −dip.sue_threshold: offer tribute = dip.tribute_rate × severity × income
                                   per month, for dip.ceasefire_days
winner accepts if offer × months > believed value of continuing
             (expected planets taken × planet value − monthly war cost × months)
```

Declarations, acceptances and ceasefire ends are facts that travel by courier (DIP-5).

## War

### SYS-13 Armies, invasions, sieges and capture `v1`

*Implements:* MIL-7 to MIL-12, FAC-10. *Cadence:* events; siege rounds daily.

- **Commissioning** *(proposed)*: an invasion is a budget option (SYS-5) for governors at
  war or facing a pirate planet:
  `value = believed (target income + gate value + culture access) × P(win) − stipends × expected days − aftermath`,
  `P(win)` from believed strength against believed defence (static defence + known
  defenders).
- **Muster:** the commissioner calls its commanders and its vassals' commanders by
  courier. Each joins or refuses (a deed). The army departs when assembled or after
  `army.muster_days`.
- **Movement:** TIM-4, at the slowest member's speed, intent fixed, actions re-decided on
  events (fleeing a larger force, picking off a target within `army.detour_ratio` of the
  remaining trip).
- **Siege:** on arrival, battle with defenders present (SYS-14), with static defence
  adding power to the defender. Once no defending ships remain, daily rounds reduce
  static defence by attacker damage while it keeps firing back. **Static defence at zero
  and no defenders is capture.** A relief force that arrives joins as a new battle.
- **Capture** *(proposed)*: the planet's happiness × (1 − `capture.happiness_loss`),
  stocks × (1 − `capture.stock_loss`), population × (1 − `capture.pop_loss`); then keep
  or grant (SYS-11). Prosperity, `N × P`, falls with it and recovers through ordinary
  economics.

### SYS-14 Combat resolution `v1`

*Implements:* MIL-13, CHR-15. *Cadence:* battle rounds hourly.

```
S_side     = Σ combat power of the side's ships (+ static defence for a planet)
each round, each side loses combat power:
           loss = combat.lethality × S_enemy × uniform(1 − combat.noise, 1 + combat.noise)
           removed as whole ships, weakest first, the commander's flagship last
retreat when S_own < combat.retreat_ratio(risk) × S_enemy   (bolder holds longer)
           a retreating side loses combat.retreat_loss of what remains
```

- **Pods:** each destroyed character's pod dies with probability `pod.death_p` (CHR-15).
- **Cargo:** a destroyed merchant's cargo is destroyed with share `combat.cargo_destroyed`;
  a raiding winner takes up to its capacity from the rest; the remainder is lost (a sink,
  MIL-15).
- *(later)* Calibrate `combat.lethality` and `combat.noise` against rendered combat
  (OPN-23).

## Information

### SYS-15 Facts, sharing, couriers and scouts `v1`

*Implements:* INF-2 to INF-12. *Cadence:* events.

- **Observing:** landing at a market records its prices and stock; a detection (TIM-6)
  records a sighting; visiting a sector records its risk.
- **Sharing:** landing at a planet of your faction (as member or contract holder) merges
  facts with its governor; members who detect each other in open space merge too. Merge
  keeps the newer of each subject (INF-4).
- **Automated couriers:** every governor with vassals schedules a run to each direct
  vassal every `courier.interval_days` (shorter with more spend). The arrival event merges
  facts both ways and delivers orders, missions and requests. A run costs
  `courier.run_cost` (a sink).
- **Calls for help:** a governor that learns of a threat sends an immediate courier to its
  liege, who decides relief by the budget model, ordered by opinion (MIL-11). Relief
  orders travel by courier too.
- **Scouts:** a planet posts a scout mission when it believes a threat within
  `scout.threat_days` or is a border planet at war; a root posts faction scouting. The
  scout flies to the target, records sightings, and its facts merge with the poster's on
  return.

### SYS-16 Piracy `v1`

*Implements:* PIR-1 to PIR-13, ECO-13. *Cadence:* events and daily.

- **A planet turns pirate** when it has been in deficit for `pirate.flip_days` and its
  treasury is below `−pirate.flip_debt` × monthly costs. It leaves its faction (vassals
  become roots), its labour moves wholly to contraband, its costs drop to
  `pirate.cost_mult`, dues stop.
- **A commander goes broke** when wealth stays below zero for `char.broke_days`. Then,
  if it knows a pirate planet: with probability `pirate.turn_p(risk)` it travels there
  (running costs suspended), renounces and is hired at `pirate.salary`; otherwise it
  sells ships until solvent (EXP-9 compares).
- **Raiding:** a pirate commander picks the sector maximising
  `Σ believed merchant traffic × cargo value at its home pirate planet's prices × detection chance`
  within range, patrols it, attacks merchants it detects, and returns to sell loot.
- **Pirate missions:** a pirate planet posts legal-goods runs when a life-support fill
  is below `pirate.need_fill`, and smuggling runs when contraband fill is above
  `pirate.surplus_fill`. Pirates offer them to independents with risk tolerance above
  `pirate.smuggler_risk` on contact, and hail the player (PIR-10).
- **Selling contraband in lawful space:** a smuggler landing at a lawful planet faces an
  inspection roll (SYS-17). If not caught, it sells to the population at
  `pirate.contraband_price` × demand, where
  `demand = N × good.consumption[contraband] × (1 − H)`, saturating at
  `pirate.demand_cap`. Money comes from the population's budget, untaxed.
- **The pirate relation** moves by `pirel.accept`, `pirel.success`, `pirel.fail` and fades
  toward `pirel.rest` (hostile) at `pirel.fade` per week.

### SYS-17 Suspicion and spies `v1`

*Implements:* INF-13, INF-14, PIR-15. *Cadence:* events; fade weekly.

```
suspicion_f(c) += sus.<behaviour>   when observed by faction f: loitering near musters,
                                     military visits without trade, cargo off-route,
                                     repeat visits, pirate relation above neutral
suspicion_f(c) −= sus.trade          per genuine trade at a planet of f
weekly fade toward 0 at sus.fade
P(inspect)     = sus.inspect_base × intensity(policing spend, at war, paranoia)
P(caught)      = P(inspect) × f(suspicion)      contraband found or cover blown
```

- **Spies** *(proposed)*: a spy holds a **secret** contract with its planet, so to others
  it reads as an independent and keeps neutral standing (and Lattice access). It travels
  to enemy planets, records what it sees there (musters, prices), and returns to report.
  Caught: attacked, contraband confiscated, opinion down.

## Population of characters and the world

### SYS-18 Characters: generation and death `v1`

*Implements:* CHR-5, CHR-6, CHR-15. *Cadence:* daily.

If the living character count is below `char.target`, spawn up to `char.spawn_per_day`
at random lawful planets weighted by population, with the planet's culture, risk
tolerance drawn from `char.risk_dist`, wealth `char.start_wealth` and a first-tier hull
(armed or unarmed at `char.start_armed_p`). Characters die only by pod roll (SYS-14).

### SYS-19 Risk `v1`

*Implements:* TRD-16, INF-6. *Cadence:* daily.

```
risk_s = EMA over risk.window days of (raids in s + risk.presence × pirate ship-days in s)
         − risk.clear × Authority mercenary days in s, floored at 0
```

The truth is exposed to the rendered game for ambient spawns (TRD-15). Characters observe
it on visit and share it as a fact.

### SYS-20 The Trade Authority `v1`

*Implements:* FAC-13, FAC-14, ECO-10. *Cadence:* monthly.

Collects `dues.access` from every lawful planet and `dues.gate` from gate holders.
Spends `authority.maintenance` of income (a sink); the surplus funds mercenary contracts:
hunting known pirate commanders and clearing the highest-risk sectors near the Lattice
for `authority.clear_days`. Its opinion of a character rises with contracts completed and
falls with unprovoked attacks; full pirates are refused regardless.

### SYS-21 Fleet running costs `v1`

*Implements:* CHR-9, ECO-11. *Cadence:* daily; fuel at Lattice entry.

```
running cost/day = Σ ships fleet.run_cost[tier]       (suspended while travelling to turn pirate)
fuel             = Lattice distance × fuel.burn × fuel price at the last refuelling planet
```

Munitions after a battle cost `fleet.munitions_per_power` × combat power used, at the next
friendly or neutral landing (MIL-4).
