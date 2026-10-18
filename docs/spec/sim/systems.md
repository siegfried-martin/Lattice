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

*(proposed, round 3; not built yet: the block after it is what's built)*

```
budget     = N × econ.wage × P
need_g     = N × good.consumption[g]                  life support, as now
shortfall  = max over life support of (1 − buy_g / need_g)
growth     as now, × G(H); shrinking as now

lux_need   = happy.need × N                           per luxury
fill_l     = stock_l / reserve_l
use_l      = lux_need × min(fill_l, lux.max_fill) / (1 + happy.sated × max(0, H))
buy_l      = use_l, limited by the budget left after life support and contraband

H*         = happy.base
             + Σ_l happy.lux × ln(1 + buy_l / lux_need)            each luxury its own curve
             + happy.contraband × ln(1 + contraband_bought / lux_need)
             − happy.hunger × shortfall
H         += (H* − H) / happy.settle_days                          daily, no clamp
capture    H −= capture.happiness_drop                             a shock, then recovery

u(H)       = tanh(|H| / happy.scale)                               0 to 1, diminishing
F(H)       = 1 + happy.p_up × u(H) for H ≥ 0;  1 − happy.p_down × u(H) for H < 0
G(H)       = 1 + happy.g_up × u(H) for H ≥ 0;  max(0, 1 − happy.g_down × u(H)) for H < 0
P          = F(H) × max(0, 1 − pop.contraband_penalty × C)
```

- **The floor:** at the bottom, F = 1 − `happy.p_down`. Start it where a planet can just
  buy its life support at reserve prices: `econ.wage` × F ≥ Σ consumption × base price
  (40 × 0.8 = 32 against 30).
- **Starting values** put a planet with no luxuries a little unhappy (`happy.base`
  −0.3, F ≈ 0.89) and one with full use of three luxuries a little happy (about +0.2).
  The budget after life support buys roughly three luxuries' full use, so variety
  costs.
- Contraband demand stays as SYS-16 has it, with `(1 − H)` read as `u` of unhappiness
  (more the unhappier, nothing when content).

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
             − pop.starve_happiness × min(shortfall, 1)                 (proposed, round 2)
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
- *As built (MS-2):*
  - A planet's day runs production (SYS-2) first, then buying, then growth, happiness
    and productivity.
  - The population buys at the ask computed before it buys, without slippage. When the
    budget can't cover all three life-support goods, every purchase is scaled down by
    the same factor, and the shortfall counts.
  - Growth uses the excess clamped to 1, so `pop.growth_rate` is the most a day can add
    ("daily growth at full excess").
  - *Correction:* the variety threshold is `lux.variety_min` × a luxury's full use,
    `N × lux.consumption / 3`, as the tunable describes. The formula's `lux.variety_min × N`
    is more than a luxury's full use, so variety could never count.
  - Contraband bought is 0 until smugglers exist (MS-6); `C` still averages it.

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

*As built:* ties go to the first good in the planet's list, and no labour moves when
all three score the same. `eff_g` above high is high × `event.bumper_mult` (SYS-4); below
low it is low × low ÷ medium *(proposed)*.

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
- *(proposed, round 3)* **A luxury's reserve** is `good.reserve_days` × `happy.need` × N
  × (1 + `happy.want` × u(H) when H < 0): an unhappy planet stocks more, so it bids more
  at the same stock (PRD-10).
- *As built:* a luxury's reserve uses its third of `lux.consumption` (its use at a fill
  of 1). Monthly income for the credit limit is a running average of daily income over
  `time.payment_days`, started at what the population would spend with its whole
  budget. `price.bands` waits for the binding to be connected.

### SYS-4 World events `v1`

*Implements:* PRD-11. *Cadence:* daily roll.

| Event | Chance | Effect | Length |
|---|---|---|---|
| Drought (planet) | `event.drought_p` per food producer | food efficiency one tier down | `event.drought_days` |
| Bumper harvest (planet) | `event.bumper_p` per producer, any good | one tier up (high stays high, ×`event.bumper_mult`) | `event.bumper_days` |
| Festival (planet) | `event.festival_p` | luxury consumption × `event.festival_mult` | `event.festival_days` |
| Regional drought | `event.region_drought_p` per region | drought on every food producer in the region | `event.drought_days` |

*As built:* bumper harvests roll per good a planet makes. Events don't stack: a planet
under a drought (its own or its region's) gets no second drought, a good under a bumper
harvest no second one, and a planet in a festival no second festival. Each event is a
record (MOD-10) that schedules its own end, which undoes its effect.

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

*In MS-2:* income is sales to the population and to osmosis, plus the tax on osmosis
trades; costs are operating costs, paid monthly, staggered by planet. The rest arrive
with the systems that pay them.

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

*(proposed, round 3; not built yet)*
- **Security is bought in units of `value.security_share` × monthly income,** not a fixed
  `value.security_unit`, so defence spending scales with wealth (MIL-5). Static defence
  per credit (`defence.per_spend`) is retuned to match.
- **Reserves:** every defence unit's value (commanders and security) is ×
  (1 + `budget.hoard` × clamp(treasury months ÷ `budget.reserve_months` − 1, 0,
  `budget.hoard_cap`)). The war vote's treasury term (SYS-12) uses the same reserve and
  cap, which move from `dip.*` to `budget.*`.

*As built (MS-4):*
- **When:** weekly per governor.
- **Units:** costs and values are per month. Monthly income is the planet's daily income
  average × `time.payment_days`.
- **Options:**
  - **Commanders.** Each sworn or candidate commander is a unit, costing its stipend
    and worth `value.defence` × income × its combat power ÷ `value.power_ref`. Value and
    stipend both scale with power, so how many are funded depends on income, not fleet
    size. Sworn commanders come first, so they aren't churned.
  - **Patronage** to each vassal in the red. A unit is a month of the vassal's operating
    costs, worth the vassal's last tribute × `value.patronage_months` ×
    (1 + `fealty.w_favor` × favor ÷ 100), up to clearing the deficit.
- **Funding:** units are funded best value per cost first, while value covers cost and
  money lasts. Each further unit of an option is worth `budget.diminishing ^ (1 − risk
  ÷ 2)` times the last, so the boldest ruler's units fall off at the square root of the
  cautious one's rate.
- **Commanders not funded are let go.** A funded candidate gets an offer of
  `hire.stipend_per_power` × combat power, and takes it if that beats its running
  average of earnings.
- **Later options:** patrols, policing, missions and invasions join with MS-5 and MS-6.
  Merchant salaries stay a fixed offer for now.

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

*As built:*
- Travel days are the reference times of TIM-4, the slower direction of the pair.
- The trade runs in chunks of `price.chunk` × the smaller reserve and stops when the
  sale at the destination no longer covers the purchase at the source, or the
  destination is past its credit limit. *(proposed)* Without that stop, a flow across a
  small gap would pay out more than it took in and create money, against ECO-1.
- Pairs are visited in a fixed order (lower planet ID first), goods in good order.

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

*As built (MS-3):*
- Candidates are ranked by the reference travel times; trip days and fuel use the
  merchant's own fleet speed along the reference route.
- A leg's quantity is found in eight slices of the most it can carry or afford, adding
  slices while the next one still pays at the source's walked ask and the destination's
  walked bid. The destination's reserve is read back off the believed price and stock.
- Cargo already aboard (left unsold) is sold first, on a single leg.
- Each stop takes `merchant.dock_hours`. On arrival the merchant sells everything aboard
  at whatever the market pays: plans can disappoint.
- A faction merchant away from its faction plans a leg home. Triangles return to the
  planet they started from.
- No trip at `merchant.min_score` or better sends it to the nearest market it hasn't seen
  for `merchant.stale_days` (TRD-4), if it can afford the fuel and keep its reserve;
  otherwise it waits a day.
- **Risk** *(MS-5)*: route risk is the believed risk of the open-space sectors on the
  reference route (both ends of each open-space leg).
  - Each slice of a leg costs its purchase price × (1 + route risk × `merchant.loss_rate`).
  - Cargo carried to a sale is charged the same.
- **Escorts** *(MS-5)*:
  - At each stop, a merchant with cargo and no escort weighs the armed characters idle at
    that planet by the escort formula, over the rest of its trip.
  - The price asked is `escort.rate` × combat power × trip days. The chosen escort
    accepts, which stands in for fit and pay (CHR-10).
  - The escort travels with the merchant and fights beside it. It's paid at the trip's
    end (REL-9), with a skill point and a `job_done` deed, or let go unpaid if the
    merchant loses its fleet.
  - The broadcast along the route (TRD-10) is the planet where the merchant stands.

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

*As built (MS-3):*
- **Posting:**
  - Courier missions are runs from a liege to a vassal, which `courier.mission_share` of
    the time are posted instead of automated; an untaken one goes automated when it
    expires.
  - Scout missions are a root's weekly trade scouting of the nearest outside market it
    hasn't heard of for `scout.stale_days`. *(proposed)* These are posted as planet-level
    missions at the root's own planet, open to anyone with `scout.min_combat`. In
    peacetime there are no other members to take faction-level work.
  - Base pay is `mission.courier_pay` or `mission.scout_pay` per day of the reference
    round trip.
- **Missions are round trips:**
  - A courier carries the poster's facts out and the target governor's facts back.
  - A scout observes every market in the target's system and brings them back.
  - Pay comes from the poster's treasury on return, then the contract ends (REL-8).
- **Choosing:** a character sees the board where it is. Score = pay ÷ days ×
  (1 + `skill.bonus` × skill) − running cost − fuel per day, compared with its best trade
  score. Ties go to the mission.
- **Reputation** grows by 1 per mission completed (REL-7, OPN-11's working default).
- **Spending:**
  - The focus is cargo or combat, whichever of the running averages of trade and courier
    earnings against scout and combat earnings is higher. The next ship is a freighter
    or a gunship of that culture.
  - It's added at the best tier owned while the fleet has fewer than `fleet.max_ships`;
    past that it replaces the cheapest ship one tier up, which is sold at `ship.resale`.
- **Broke** (`char.broke_days` below zero): sell ships, cheapest first, until solvent,
  keeping the last hull. Turning pirate instead arrives in MS-6.

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
  *As built (MS-3):*
  - Every root offers `merchant.salary` until the budget model sets it (MS-4), to
    `merchant.per_planet` merchants per faction planet. It pays monthly from its
    treasury.
  - The cost of the constraint is the merchant's best independent score less its best
    faction score, from its own facts.
  - A character weighs a salary at most once a payment period.
- **Ending:** either side ends a contract when it stops paying for them (REL-8). Losing
  the fleet ends service contracts automatically (CHR-16).

*As built (MS-4):*
- **Candidates:** armed characters at planets within `hire.range_days` (reference
  travel), not on a mission, with no service or salary contract. They're known because
  they're near.
- **Sworn commanders** go to their governor's planet and serve there; in peacetime the
  objective is home defence. They're paid monthly, and their stipend follows their
  combat power each week.

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

*As built (MS-4):*
- **Storage:** opinions are a sorted list per holder. The weekly fade is applied lazily,
  as `(1 − rel.fade)` to the power of the weeks since the opinion last changed, whenever
  it's read or moved. The same thing as a weekly pass, without one.
- **Deeds in use so far:** patronage, tribute raised and lowered, and job done (a
  mission's poster about its taker). The rest arrive with war and piracy.

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

*As built (MS-4):*
- **Tribute:**
  - It's paid at the vassal's monthly payment, on its own income since the last payment
    (sales and tax) plus, with the cascade, the tribute it received.
  - Received tribute counts as the liege's income for its credit limit.
  - Starting rates are `fealty.tribute_rate`; a planet that swears fealty later starts
    there too.
- **The liege's tools** *(proposed)*: weekly, for each vassal:
  - in the red, it raises tribute by `fealty.rate_step` on vassals at least
    `fealty.raise_opinion` loyal (a bad deed);
  - it lowers tribute for vassals below half `fealty.defect_opinion` (a good deed,
    `deed.tribute_lowered`).
- **Defection:**
  - A rival is a root within `fealty.threat_days` (reference travel) whose governor
    thinks better of the defector than `fealty.rival_opinion`; the defector picks the
    one that thinks best of it.
  - Otherwise it goes independent. With no armies before MS-5, nothing threatens it.
- **Keep or grant and choosing a governor** arrive with capture (MS-5).

### SYS-12 Diplomacy `v1` `proposed`

*Implements:* DIP-1 to DIP-5. *Cadence:* weekly per root.

**War vote**, between peaceful neighbours, using each governor's own beliefs:

```
weight      = 1 + planets in the governor's branch
vote        = (dip.w_idle × idle combat power under them
             + dip.w_targets × believed wealth of target planets within dip.border_days
             + dip.w_strength × (believed own strength / believed target strength − 1)
             + dip.w_peace × prosperity growth since the last war
             + dip.w_treasury × clamp(treasury months / dip.reserve_months − 1, 0, dip.treasury_cap)
             − dip.w_wars × wars ongoing − dip.w_losses × recent losses)
             × (0.5 + risk tolerance)
declare if Σ weight × vote / Σ weight > dip.declare_threshold
```

**Suing for peace:**

```
war_score  = − dip.s_planets × planets lost share − dip.s_power × fleet power lost share
             − dip.s_treasury × treasury trend − dip.s_months × months at war
if war_score < −dip.sue_threshold: offer tribute = dip.tribute_rate × severity × income
                                   per month, for dip.ceasefire_days
no offer is heard before dip.min_war_days
winner accepts if offer ≥ max(taken, hope)
  taken = planets taken a month × the loser's average planet income × value.invade_months ÷ 12
  hope  = dip.hope_share × hoped × winner's share of the two sides' power
          × 0.5 ^ (months at war ÷ dip.hope_halflife_months)
  hoped = the loser's monthly income within dip.border_days of the winner, at the declaration
```

Declarations, acceptances and ceasefire ends are facts that travel by courier (DIP-5).

*As built (MS-5):*
- **When:** weekly per root, and no declarations during the warm-up (GEN-10).
- **News travels (DIP-5):**
  - War, ceasefire and peace are facts. Both roots know at once; everyone else hears by
    courier and sharing.
  - These act on what they've heard: governors' budgets (defence threat, invasions, raids,
    war scouting), faction merchants' plans, and raiders and patrols choosing whom to
    engage.
  - These use the truth: osmosis, gates, and the vote itself.
- **Neighbours:** two factions border when any planet of one is within
  `dip.neighbour_days` of a planet of the other.
- **The vote's terms** *(proposed)*:
  - idle = the governor's commanders at their posts ÷ `value.power_ref`;
  - targets = the enemy's income within `dip.border_days` ÷ the governor's own income;
  - strength = faction power (commanders and static defence) ÷ the target's, − 1;
  - peace = faction income ÷ its income when the last war with this target ended, − 1;
  - treasury months = the governor's treasury ÷ its monthly operating costs;
  - wars = wars now;
  - losses = planets lost in them.
  Strength and wealth are read from the world, not beliefs, until facts carry them.
- **Suing** *(proposed)*:
  - The score uses the shares of planets and power lost since the war began, the
    treasury below zero against monthly income, and months at war.
  - The offer is `dip.tribute_rate` × min(severity, 3) × income a month.
  - The winner accepts by the formula above. Power is read from the world.
  - *Phase A round 2:* before it, the score also counted wars at once (`dip.s_wars`) and
    the winner valued only what it had taken. A faction in three wars sued at once, and
    with nothing taken yet any offer was accepted, so the median war lasted 7 days.
- **A ceasefire** stands down armies sent between the two. When it ends, peace records
  each side's income for the next vote.
- **Roots changing:**
  - Relations are kept between roots.
  - A new root (defection, or an heir) forgets anything left from an earlier time as a
    root.
  - The heir of a captured root inherits its wars (MOD-5).


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

*As built (MS-5, part 1):*
- **Commissioning:** an invasion is a budget option for a governor whose faction is at
  war. It is one unit for the best enemy planet within `dip.border_days`:
  - value = P(win) × the target's monthly income × (`value.invade_months`, plus
    `value.gate_months` for a gate holder, less `value.aftermath_months`);
  - cost = the stipends of the commanders it would use over about two weeks plus travel;
  - P(win) = A² ÷ (A² + D²), where A is its own commanders at their posts plus half its
    vassals', and D is the target's static defence plus its sworn commanders at home.
  One campaign at a time.
- **Muster:** a vassal answers the call if its opinion of the liege is at least 0, a
  deed either way. Its commanders leave after the courier's travel time, go to the
  commissioner's planet and join. The army leaves when everyone called has arrived, or
  after `army.muster_days` without the stragglers.
- **Moving:** the army moves as one, on the route for its faction (gates of enemies
  shut, WLD-13), at its slowest member's speed. Each fleet pays its own fuel. Armies
  don't fight on the way yet.
- **Siege:**
  - On arrival, the defender's commanders at home fight with the static defence beside
    them; the static defence takes no damage while ships remain.
  - Then daily rounds trade damage between the army and the static defence. The army
    retreats by SYS-14's rule.
  - Static defence at zero with no defenders is capture. A battle at the planet pauses
    the siege.
- **Calls for help:**
  - A siege sends a call to the planet's liege, arriving after the reference travel time.
  - The liege sends its own commanders at home as relief when P(win) against the
    besiegers is at least a third (a deed, `help_sent`). Otherwise the call is a missed
    deed and goes on up the tree.
  - Relief fights the besiegers beside the planet's defence. A lifted siege is a
    `border_held` deed for the planet's governor.
  - A liege answers calls in the order they arrive, not by favour yet.
- **Warnings ahead of the siege:**
  - A governor that learns from a sighting that an army it believes hostile means to take
    its planet warns its liege by courier.
  - A liege that learns its vassal is threatened decides at once. Relief goes on the same
    test as a siege's call, and garrisons the planet until the threat passes or
    `army.muster_days` go by.
  - A garrison there when the siege starts fights beside the defenders.
- **Capture:**
  - The deposed governor stays as an ordinary character; contracts it paid end.
  - The planet's vassals stay with the old faction: under its liege, or, if it was the
    root, under its most populous vassal, which becomes the root and inherits the wars.
  - Keep or grant runs from the commissioner. The keeper chooses the new governor from
    the army and its own commanders.
  - Its security spending restarts at zero, so a fresh conquest is weakly held.
  - *(proposed, round 2)* The conquering army garrisons it for `capture.hold_days`, then
    goes home. Without it, a planet could change hands 19 times in three years.
  - *(proposed, round 3; not built yet)* **Garrisoning is a choice** (MIL-12). At
    capture the commissioner compares:
    - hold: value = P(loss) × the planet's capture value (SYS-13's gain), with
      P(loss) = E² ÷ (E² + A²), E the enemy power it believes is within
      `dip.border_days` and A the army's;
    - cost = the army's stipends over `capture.hold_days`, plus the defence value its
      home loses meanwhile (`value.defence` × home income × the army's power ÷
      `value.power_ref` × threat, for the same time).
    It holds if the value is larger, and goes home otherwise; moving on to a next target
    is the next budget week's decision.

*As built (round 2): gate attacks and blockades* (MIL-17) *(proposed)*:
- **State:** every Lattice node has a gate record: static defence, the attack it's under,
  its captor (the planet whose army took it; its faction holds the blockade), when, and
  the army garrisoning it. The holder's day rebuilds the defence toward
  `blockade.defence_share` × `defence.per_spend` × the holder's security spending, unless
  attacked or blockaded.
- **Commissioning:** the invasion option also scores gates within `dip.border_days`:
  - an enemy's open gate: value = P(win) × the monthly income of the planets it serves ×
    `value.blockade_months`, with D its static defence plus any of its faction's armies
    garrisoning it;
  - its own faction's gate blockaded by an enemy, as its governor has heard: value with
    `value.retake_months`, D the captor's garrison.
  The best of planets and gates is the one unit. The army's target is the gate's holder,
  for sightings and warnings, and its objective the gate.
- **Moving:** armies and characters plan around gates blockaded by another faction as
  their facts say (an army by its commissioner's facts); a blockade by their own
  faction opens the gate to them, even an enemy's. One that arrives on or off the
  Lattice at a gate blockaded against it learns it and replans from there: along the
  Lattice if it was riding it, through open space if it was about to board.
- **The attack:** any of the holder's faction's armies garrisoning the gate fight first,
  a battle at the gate. Then daily rounds against the gate's static defence, as for a
  planet. The holder calls for help an hour in: it answers first, with its own
  commanders, then the call goes up the tree. Relief that finds the gate under attack
  fights the besiegers; one that finds it untouched garrisons it for `army.muster_days`.
- **Capture:** static defence at zero is a blockade. The army garrisons the gate for
  `blockade.hold_days`, then goes home. The captor's army and commissioner, and everyone
  at the planets the gate serves, learn it at once; others by sharing and couriers.
- **Retaking:** an army of the holder's faction arriving at a blockaded gate retakes it at
  once if nobody holds it, or fights the garrison and retakes it by winning.
- **Lifting:** the holder's weekly check lifts a blockade once the captor's faction and the
  holder are no longer hostile (peace, a ceasefire, the holder now the captor's own), or
  the captor turned pirate.
- **Trade:** osmosis between a planet the blockaded gate serves and any partner its
  reference route reaches along the Lattice stops.
- **Not yet:** merchants don't weigh a blockade's detour when picking a destination,
  only on the way.

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
  *As built (MS-5):* a surviving pod lands at the nearest planet whose faction isn't at
  war with the pilot's, and every contract that needed the fleet ends (CHR-16).
- **Cargo:** a destroyed merchant's cargo is destroyed with share `combat.cargo_destroyed`;
  a raiding winner takes up to its capacity from the rest; the remainder is lost (a sink,
  MIL-15).
- *(later)* Calibrate `combat.lethality` and `combat.noise` against rendered combat
  (OPN-23).

*As built (MS-5):*
- **Damage:** damage below a ship's combat power carries over to the next round.
  Zero-power ships go at the first damage.
- **Retreat:** the side's risk tolerance is its strongest fleet's. A retreating side
  pays `combat.retreat_loss` of its remaining power as damage, then leaves.
- **Static defence** never retreats; the planet's defenders can.
- **Accounting:** losses go into the war ledger, and destroyed cargo into the goods
  ledger.

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

*As built (MS-5):*
- **Facts beyond prices:** risk per sector, seen at each leg's end in open space and at
  each planet; war and peace states; sightings of armies with their power and intent.
- **Sightings:** anyone who comes within range of an army records one (INF-9: sightings
  aren't private). A scout on a mission also puts it in the packet it carries home.
- **Threat scouting:** a border planet at war (a believed enemy planet within
  `dip.border_days`) posts a scout mission toward the nearest enemy planet. It's flown as a
  sortie (no landing), holding half a day off the target.
- **After every merge** into a governor (courier, mission return, a member landing), its
  sightings are checked for threats (SYS-13).

*As built (MS-3):*
- **Facts:** facts are prices only, one per market: every lawful good's price and stock
  at the time seen. Sightings, risk and diplomatic states arrive with the systems that
  use them (MS-5, MS-6).
- **Members:** a member is a governor in the faction, or the holder of a standing
  contract with one of its governors. One-job contracts don't count.
- **A governor** observes its own market whenever it shares.
- **Automated runs:** a run is due for every sworn planet every
  `courier.interval_days`, staggered. It takes the reference travel time each way and
  carries a copy of the facts.
- **Open space:** members merge at most every `space.meet_cooldown_hours`.

### SYS-16 Piracy `v1`

*Implements:* PIR-1 to PIR-13, ECO-13. *Cadence:* events and daily.

- **A planet fails** when it has been in deficit for `pirate.flip_days` and its
  treasury is below `−pirate.flip_debt` × monthly costs, or *(proposed, round 2)* when
  its happiness has been below `rebel.happiness` with a life-support shortfall above
  `rebel.shortfall` for `rebel.days` in a row. Checked weekly, warm-up included.
- *(proposed, round 3; not built yet)* **Failure becomes a weekly chance,** replacing
  the day counters:
  `p = rebel.scale × (u(H)^rebel.power for H < 0 + rebel.w_debt × clamp(debt ÷ (pirate.flip_debt × monthly costs), 0, 1))`,
  drawn from the planet's events stream. Hunger acts through H.
- **A failing planet with no vassals turns pirate.** It leaves its faction, its labour
  moves wholly to contraband, its costs drop to `pirate.cost_mult`, dues stop.
- *(proposed, round 2)* **A failing planet with vassals secedes:** it becomes a root with
  its branch, forgets any relations from an earlier time as a root, stops paying
  tribute, and its old root declares war on it. A failing root with vassals does
  nothing (OPN-37).
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

*As built (MS-6, part 1):*
- **The pirate faction:**
  - Pirate planets and full pirates form one faction with no root.
  - They take no part in diplomacy, are hostile to everyone outside them, and have no
    osmosis with lawful markets.
  - Full pirates route without the Lattice; a pirate planet's gates are shut to everyone.
- **Turning pirate:**
  - The check is weekly.
  - The flipped planet's governor turns with it, and contracts it paid end.
  - Its vassals become roots at peace. Its contraband takes its first slot at high
    efficiency, with all its labour, and labour doesn't shift.
  - Its forgiven debt counts as money created (ECO-13).
  - Governors within `dip.neighbour_days` see it happen; others hear. Landing shows every
    planet in that system.
- **A pirate planet:**
  - Its population buys contraband from its own market, right after life support.
  - Its budget buys only static defence. It keeps every commander who joins, at
    `pirate.salary` of the stipend (proposed).
  - It can be invaded by any governor who knows of it. Capture restores its old goods
    and destroys its contraband.
  - No invasions during the warm-up (GEN-10).
- **Broke commanders:**
  - They go to the nearest pirate planet they know of, with chance equal to their risk
    tolerance (all of them under `pirate.turn_all`).
  - They ride the Lattice as lawful until they arrive, renounce every contract, and are
    hired.
- **Pirate raids** *(proposed)*:
  - Weekly, each pirate commander at home sorties with chance `raid.share` × (1 + the
    planet's worst life-support shortfall).
  - The target is a lawful planet within `pirate.range_days` of open space, weighted by
    population, which stands in for believed traffic.
  - They attack anyone below neutral with them (PIR-9) and sell loot only at home
    (PIR-5).
*As built (MS-6, part 2):*
- **Runs:**
  - Weekly, a pirate planet posts a legal-goods run for each life-support good under
    `pirate.need_fill`, sized back to its reserve, and a smuggling run for its contraband
    surplus over `pirate.surplus_fill`.
  - On contact, a pirate offers its home's best run to an independent with risk
    tolerance above `pirate.smuggler_risk`. That sets the pirate relation to neutral at
    least, so it isn't attacked, and the run is taken when the character is next free
    with an empty hold.
- **Smuggling:**
  - At the haven, the smuggler buys contraband from its market (PIR-6). It takes it to
    the most populous lawful market it knows, discounted by distance.
  - It sells to the population at `pirate.contraband_price` × base, up to its daily wish
    `N × consumption × (1 − H)`, capped at `pirate.demand_cap` per thousand.
  - The money is population spending (ECO-1), untaxed. What doesn't sell goes on to the
    next market, or overboard.
- **Legal goods:** the runner buys at the cheapest lawful source it knows, sells into the
  haven's market, and gets `pirate.legal_bonus` a unit from the haven.
- **The relation:** success raises it by `pirel.success`; failure or being caught drops it
  to `pirel.fail`. No contract: a run isn't a commander's contract, so it doesn't make
  the runner a full pirate (PIR-11).

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

*As built (MS-6):*
- **Inspection** happens on every landing at a lawful planet, with suspicion held per
  root.
- **What an inspection can find in v1** is contraband aboard. A spy's cover joins in MS-7.
- **Raised by:** a landing with contraband aboard and not caught, and a pirate relation
  above neutral (`sus.pirate` per point, each landing).
- **Lowered by:** each genuine purchase at a planet of the faction.
- **Caught:** confiscated, the `caught` deed, and suspicion + `sus.caught`.
- **Policing** is a budget option for planets whose population buys contraband. Its
  units are worth the share of income contraband costs in productivity ×
  `sus.police_effect`; spend is a monthly sink.
- **Not yet:** the Authority's inspections on the Lattice.

*As built (MS-7), spies* *(proposed throughout)*:
- **Hiring:** a governor that believes it's at war buys spies through its budget, each
  worth `spy.value` × monthly income × threat for `spy.salary` a month. They're hired
  from independent traders idle within `hire.range_days`; at peace they're let go.
- **The secret contract:** it doesn't make the spy a member (`faction_of` ignores it).
  It shares with its own governor at home, which checks the news for threats (SYS-13).
- **The round:** from home, to the enemy market within `spy.range_days` it has seen least
  recently, and back.
  - Abroad it sells what it carries and buys for home as cover. A visit without a
    purchase adds `sus.no_trade`.
  - It records every army at that planet as a sighting with its intent. One forming
    there adds `sus.muster`.
- **Caught:** an inspection that catches a spy blows its cover. The contract ends, the
  governor there takes the `caught` deed, and the planet's commanders at home attack it.

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

*As built (MS-3):* the armed hull is a scout and the unarmed one a courier, of the
planet's culture. A new character knows its own planet's market. The starting world
fills `char.target` the same way (GEN-9).

### SYS-19 Risk `v1`

*Implements:* TRD-16, INF-6. *Cadence:* daily.

```
risk_s = EMA over risk.window days of (raids in s + risk.presence × pirate ship-days in s)
         − risk.clear × Authority mercenary days in s, floored at 0
```

The truth is exposed to the rendered game for ambient spawns (TRD-15). Characters observe
it on visit and share it as a fact.

*As built (MS-5):*
- **Inputs:** raids are engagements started by raiders. Hostile ship-days are raiders'
  time in open space (pirates join in MS-6). Patrol days stand in for the Authority's
  mercenaries (MS-6) with planets' security patrols.
- **Raids** *(proposed)*:
  - Each week at war, every commander at home but one is sent with chance `raid.share`
    toward the nearest believed-enemy planet within `dip.border_days`.
  - It holds `raid.standoff` short of the planet for `raid.days`, engaging members of
    believed-enemy factions that come in range, then goes home.
  - Loot: the winner takes cargo up to its capacity after `combat.cargo_destroyed`; the
    rest is lost. A raider sells its loot at home.
- **Security missions** (MIL-5) *(proposed)*:
  - A planet whose believed sector risk passes `security.risk_post` posts one: a patrol
    over its own space for `security.days`, at `security.pay` a day.
  - The patroller engages raiders of factions it believes hostile, and counts as clearing
    while out.

### SYS-20 The Trade Authority `v1`

*Implements:* FAC-13, FAC-14, ECO-10. *Cadence:* monthly.

Collects `dues.access` from every lawful planet and `dues.gate` from gate holders.
*As built (MS-4):* each planet pays at its monthly payment; gate holders are the holders
of on-network groups.

*As built (MS-6):*
- **Clearing contracts:** weekly, the Authority posts up to `authority.contracts` of
  them, for the riskiest sectors (above `authority.risk_post`) that hold an on-network
  lawful planet, while its treasury covers them.
- **A contract** is a patrol from that planet of `authority.clear_days`, paid from the
  Authority's treasury. Completing one raises the Authority's opinion of the mercenary.
- **Not yet:** hunting named pirate commanders.
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

*As built (MS-3):* running costs are charged daily. Fuel is charged per Lattice transit at
entry, at the fuel price of the planet where the fleet last landed, for every ship.
