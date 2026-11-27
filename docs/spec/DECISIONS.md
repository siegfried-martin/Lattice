# Decisions

A dated log of every change to the spec, newest first, and the ideas that were rejected so
they aren't reintroduced. One line per decision; the rule holds the detail.

## Log

### 2026-11-26 · The money round: an even grant, and the population eats from stock

The human's design after part 7's wage finding (ECO-1, ECO-2, PRD-2, PRD-5, PRD-7,
SYS-1; proposed, built):

- **No wage, no buying population** (ECO-2): the population eats from the state's stock,
  the pool merchants buy from. "If there's a reason we need to represent the citizen as a
  separate consumer outside of the planet market let me know but I don't see it."
- **New money is an even grant to every planet** (ECO-1, `econ.grant` 40,000 a day): a
  central bank "would overcomplicate for little gain". Even, not per person: "Right now we
  have no mechanic preventing rich planets from growing infinitely, this adds some
  inefficiency at scale … Small planets will use their excess money to buy more food and
  luxuries and grow more, which provides our balancing mechanic." Money per person gave
  a starving planet no relief, as its money fell as fast as its need.
- **A floor on selling life support** (`price.sell_floor` 0.5 of reserve), for merchants
  and osmosis alike.
- **Removed:** `econ.wage` and the starting income estimate from spending at reserve;
  a planet starts with its grant as its income.
- **Found on the way:** with spending capped by a wage, people paid any price under the
  cap, so a shortage created more money; the wage bound only where life support was
  dear (spend 32 a thousand a day at wage 40, 34.5 at 120).
- REJ-11 still stands (no wages from production); REJ-16's worry, a blockaded planet
  printing income, comes back for the grant (OPN-47).

### 2026-11-26 · Tuning pass, part 7: supply orders

The human's design after the fringe finding (MSN-15, TRD-2, MSN-8, REJ-26):

- **Supply orders** (MSN-15, `order.*`): a lawful planet short of a life-support good
  sends an order to the nearest surplus it has heard of, as a one-way courier mission it
  pays for; the producer posts the delivery at the short planet's price, and merchants
  there weigh it as a trip. Why: merchants plan among their 12 nearest planets, and no
  Lattice planet has a fringe planet among them.
- **The courier leg is base work for new characters:** "The mission to deliver
  information (a mission) to the other planet serves as a good base level mission for
  new characters."
- **Untaken, an order arrives anyway** after `order.courier_wait_days` past its trip:
  "We still want these planets to get their goods."
- **It replaces MSN-8's top-ups and narrows TRD-2 and REJ-26:** the price is still the
  planet's own; the order carries it.
- **Not Bannerlord's caravans:** buying anything under a known average ignores who knows
  what, a core mechanic here (INF-2); an order goes where the planet's own facts point.
- **A fixed price at half the planet's own** (`order.price_share` 0.5): "They shouldn't
  advertise the full price they will pay at that moment, but reduced significantly. The
  planet is taking the risk of market conditions changing."
- **Wage 120** (`econ.wage`, from 40): "Sure 120 is good, easy change to make later."
  Life support falls from 37% to 13% of a Lattice planet's budget.
- **Starting income is what a population would spend at reserve,** not its whole wage:
  at wage 120 the old estimate tripled the starting military (a fix, MS-8 part 7).

### 2026-11-26 · Tuning pass, part 6: bankruptcy and errands

The human's direction after the bankruptcy diagnosis (MSN-14, CHR-16, TRD-4, PIR-4):

- **Errands** (MSN-14, `passenger.wait_min_days` 0.5, `passenger.wait_max_days` 2): a
  lawful bottom-rung character with nothing waits a random 12 to 48 hours, then its
  planet posts it a free-money passenger run to a nearby planet. "It basically pays them
  to explore their next planet." It replaces wandering for the bottom rung.
- **The player's board is never empty** *(later, with the player)*: "there should always
  be at least a couple missions in pretty much any planet for the player."
- **Fixes found on the way,** built with it: the bottom rung explores only with money in
  hand; the broke clock runs at landing too; errands reach planets open space doesn't
  get out of (by the Lattice, the fare covering fuel, or the nearest reachable lawful
  planet) and pirate planets post them for the lawful; a free hull can always leave.
- **Measured as characters too:** `chars.bankrupt_chars`, so the same character's
  repeats don't read as ruin.

### 2026-11-25 · Tuning pass, part 5: ships without a Lattice Drive, and standing

The human's direction (CHR-16, TRD-18, REL-5, MSN-13):

- **The free hull follows who it was; standing wins** over a merchant record: courier
  with a drive for the trusted, small freighter without one for a merchant, else a
  shuttle; a pirate's scout (CHR-16). "We can change it easily later."
- **Every class with and without a drive** (TRD-18, `ship.no_drive_*`). Where a ship
  goes comes from what it can reach; no rule steers it to or from the Lattice.
- **Passenger runs are open-space hops,** the only missions a hull without a drive can
  take (MSN-13).
- **Standing is seeded and grows from faction work** (REL-5): born with
  `char.start_standing`; a planet's job or a passenger run raises the root's opinion
  (`deed.faction_job` +10, after +3 proved too thin); courier runs need
  `mission.courier_standing`. Slower opinion fade was tried and rejected: it moves every
  opinion in the game.
- **A character with nothing to do for a day moves on** (TRD-4,
  `merchant.wander_after_days`, `merchant.wander_days`): to a lawful planet within a few
  days' travel, likelier the busier it is, so long as the fuel adds no debt. "This will
  help make things feel more alive." Idle stayed near 11%: almost all of it is the wait
  in port between jobs, not characters stuck.
- **Open:** the shuttle rung is capped by the passenger supply; scaling it with demand is
  the next lever.

### 2026-11-25 · Tuning pass, part 4: pirates and bankruptcy

The human's direction after part 3's idle characters turned out to be stranded pirates
(PIR-4, PIR-16, CHR-16, SYS-16):

- **Piracy is terminal, and rare enough to be a story arc** (PIR-4): weighed once a broke
  spell, only by the bold (`pirate.turn_min_risk`, `pirate.turn_p`). Many characters
  never turn. Debts are left behind on renouncing.
- **Raid until destroyed, then again until dead** (new PIR-16): a pirate takes a mission
  if offered, else raids on its own. **No extra death chance for pirates:** they die
  young because they lose often, at the ordinary pod roll.
- **A pirate's bottom rung is a fighter**, a free tier-1 scout, so it can raid at once; a
  pirate can still save up and build a bigger fleet.
- **Bankruptcy** (CHR-16): still in debt with one hull, a character is swapped onto the
  bottom rung's hull with its debt written off, rather than sitting in debt for good.
- **The bottom rung looks for work without keeping a reserve** (CHR-16, TRD-4).
- **Fixed:** a legal-goods run taken at the haven with nowhere known to buy the good.
- **Open:** pirates sit at CHK-23's 1% floor; the band or the pirates may move.

### 2026-11-24 · Tuning pass, part 3: insurance and the bottom rung

The human's direction on the shipless and ruined characters (CHR-6, CHR-16, CHR-21,
MSN-5, MSN-13, MIL-20, SYS-8, SYS-13):

- **Insurance is built into a ship's price** (new CHR-21): a premium on top
  (`ship.insurance_premium`, 0.3), and part of the price paid out when the ship is
  destroyed (`ship.insurance_payout`, 0.5). No insurer is simulated. The premium is sunk,
  and the payout is free money, a deliberate MSN-5 exception: what's good for the game
  decides the numbers, not what a real insurer could offer. Cover is one sum per
  character. Plans chosen by risk tolerance are `later`.
- **A fall from the top is a peg at a time** (CHR-16): a lost fleet is rebuilt from the
  payout and savings, about a tier down.
- **A ruined character starts over** in a free tier-1 courier, uninsured, keeping its
  standing and skills (CHR-16, CHR-6). Ideally a ship without a Lattice Drive, once
  those exist (TRD-18).
- **Passenger runs** (new MSN-13): one-way, free-money work for a character with one
  tier-1 hull (`passenger.*`), a second deliberate MSN-5 exception.
- **Insurance counts in a commander's expected loss** (CHR-10, MIL-2), so it lowers what
  fighting costs and keeps combat frequent.
- **Governors buy and rebuild their own fleets** (MIL-20), gunships.
- **Fixes the run turned up:** a character with no ship waits rather than flying off
  exploring; no ships are sold at a besieged planet; only armed fleets defend; a siege
  fights defenders who come home strong enough to stand, and a weaker one can't block
  the capture (SYS-13). Sieges had been running over 1,000 days.
- **Not built:** courier work gated on the faction's opinion of the character.

### 2026-10-25 · Tuning pass, part 2: what a fleet costs, and held purses

The human's direction after part 1 showed war money piling up with commanders (CHR-9,
MIL-2, MIL-4, SYS-21):

- **Keeping a fleet is the larger spend.** Every ship is maintained, crewed and refuelled.
  Crews are averaged by class and tier, not tracked person by person. Upkeep is
  maintenance on the ship's price (`fleet.maintenance`) plus its class's crew
  (`ship.<class>.crew` × `fleet.crew_tier[tier]` × `fleet.crew_wage`); it replaces
  `fleet.run_cost[tier]`. Fuel stays per transit.
- **Munitions are a small spend,** now charged: about a tenth of a month's stipend a
  battle, paid at the next planet that isn't hostile.
- **A purse is held when it's offered** and goes back if the campaign fails, so a planet
  can't offer more than it holds.
- **Fixed: a character who lost its last ship never bought another** (CHR-16). About half
  of all characters ended runs with no ship.
- **CHK-7c's band gets a floor of 0,** and CHK-7d checks that the planets' treasuries
  together end above 0.
- **Ceasefire tribute is paid only from what the payer holds** (DIP-2, SYS-12), for
  sign-off. The run found it was the main cause of planets' debt: the root pays it alone,
  sized at up to 30% of its whole faction's gross monthly income, and paid it into debt.

### 2026-10-24 · Tuning pass, part 1: rewarded objectives and a higher tribute base

Agreed with the human after the stage 2 results (MIL-2, SYS-9, FAC-6):

- **Objectives carry a purse** sized from the commissioner's own valuation, paid on
  success, rising at the muster until it clears, and counted in the budget at its ceiling
  (`objective.reward_*`, 0.02, 0.02, 0.3).
- **Declining is free;** `deed.objective_declined` is no longer applied.
- **An abandoned campaign waits** `objective.retry_days` (28) before the next.
- **The loan fee is paid only** for the lent commanders who take the objective.
- **Tribute stays static** except in its special circumstances (a liege in the red,
  disloyalty, relief). Its base is raised from 0.15 to 0.25 (`fealty.tribute_rate`).

### 2026-10-24 · War and fealty money built (MS-8 stage 2)

Stage 2 of the roles and war round, built as designed on 2026-10-19; results in MS-8.
Choices made in building it, for sign-off (SYS-5, SYS-9, SYS-11, SYS-13, SYS-14, SYS-15
"as built"):

- **A loan moves the service contract's employer** to the borrower, and back to the
  lender planet's governor at the end; the contract ends instead if the lender planet
  has left the borrower's faction. Borrowed commanders aren't in the borrower's budget
  options, but their stipends come off its money first.
- **Loan requests go only to vassals with someone at their posts.** A vassal's own
  fleet comes with the loan and chooses like its commanders; MSN-12's missions between
  governors aren't built. The `exposed` term a fee must cover is counted over
  `loan.max_days`.
- **Answered or refused is recorded** when the army marches or at the deadline, by
  whether any of that vassal's fleets arrived (INF-16's "the fleet is the reply"). An
  army marches early once every request is answered and everyone called has arrived.
- **Objectives are offered directly,** with no board record: once, to the commanders at
  the muster and to each lent commander as its loan starts. The commissioner's own fleet
  always joins, and relief goes as ordered. A lent commander that declines serves at
  the borrower's planet until the loan ends. New keys `objective.choose` (EXP-15's
  switch) and `objective.stake_months` (3), the months of stipend counted at stake when
  staying.
- **Gate attacks pay no spoils,** so with commanders choosing, a gate's objective pays
  only the bounties for its guards.
- **A relief's loan starts when it sets out,** with its fee and `deed.relief_used`, for
  warnings' relief too.
- **Kills are credited** by power at the round's start, and retreat losses count as
  kills. Bounties are paid in part when the treasury runs short. Tallies move with the
  root and are dropped when the faction is gone.
- **Spoils count siege days** from the daily siege rounds, among members at the capture.
- **Tribute relief** is decided when the request arrives, with "money allows" meaning
  the liege's treasury plus its credit line. The liege answers either way, a vacant
  seat refuses, and swearing elsewhere drops relief.
- **CHK-26 in `checks.py`** is an aggregate share over lawful planets per state, not the
  median vassal.

### 2026-10-24 · Roles model built (MS-8 stage 1)

Stage 1 of the roles and war round, built as designed on 2026-10-22; results in MS-8.
Choices made in building it, for sign-off (SYS-22, SYS-8 and SYS-5 "as built"):

- **Steward offers go down the fitness order** until someone accepts, not to the best fit
  only. Candidates must be between jobs with an empty hold.
- **A dead governor holds its seat until a successor comes.** Its commanders, steward,
  spies, merchants, posted missions and armies pass to the successor. With no steward,
  the vacant planet's budget doesn't run, and it never adjusts tribute, defects, raids or
  posts missions while vacant.
- **A governor's pod goes home**, and a governor that lost its fleet governs on without
  one; it doesn't buy ships.
- **A pirate run's deadline** is `mission.deadline_mult` × `mission.expiry_days`, since its
  route isn't known when it's taken; a failed smuggling run dumps its contraband.
- **The self-stipend** is an amount on the planet, not a contract.
- **Seeded and generated governors** also get `char.start_wealth`.
- **A capture winner from the keeper's own commanders** now travels to its planet instead of
  appearing there.
- **`hire.w_*` are removed;** `fealty.capture_bonus` is 0.3; new keys `steward.enabled`
  (EXP-13's switch, world-wide for now), `fit.*`, `steward.salary_share`,
  `mission.deadline_mult`, `hire.rep_half`, `gen.governor_ships`, `deed.mission_failed`.
- **Not yet:** escort contracts aren't mission records, so failing one isn't a bad deed;
  the hunt as patrol time waits for spawned bounties.
- **New checks in `checks.py`:** CHK-27 to CHK-29, with placeholder bands.

### 2026-10-22 · Roles, progression and missions, design

The planning agent's draft `ROLES.md` (answering OPN-42), reviewed with the human and
written into the spec as `proposed`; not built. The draft and its review are in git at
commit 3145d4b.

- **Missions are the market for wants:** an entity posts what it wants, and characters
  free to choose take what's worth it to them. Price still runs ordinary trade. One
  mission primitive for every path (new `design/missions.md`, MSN-1, MSN-2).
- **Failing a mission is a bad deed,** and abandoning a one-job contract counts as
  failing it. This narrows REL-8 and REJ-15 for missions only. Completion is a good deed,
  and tiers open with the issuer's opinion. Reputation isn't hit by failure (MSN-3,
  REL-3, REL-7).
- **Paths, not a ladder:** merchant, warrior and spy describe tendencies, and nothing is
  gated behind anything (CHR-11, rewritten).
- **One fitness function** fills every post (governor, steward, commander), with weights
  per post. Culture stays an input. Answers OPN-15 and OPN-40 (CHR-20, MIL-6, FAC-9,
  SYS-22).
- **Stewards return** (reverses REJ-8): an NPC-only post that runs a planet's economy and
  holds the planet while its governor sails. It's a way up for merchants and spies
  (FAC-16, CHR-17).
- **One budget, two deciders:** the governor decides war and defence against the rest,
  and how that share is spent; the steward decides everything else. Each line reads its
  decider's risk tolerance (CHR-4, ECO-8, SYS-5).
- **Governors command their own fleets** (admiral; grand admiral with vassals). They
  defend at home, sail with a steward in charge, pay themselves a stipend, and can die
  (MIL-1, MIL-20). The human chose to build this now rather than balance a model it would
  overturn.
- **Succession:** a liege chooses a dead governor's successor, with the steward among the
  candidates. A dead root's steward becomes the root. Capture keeps its own rule (FAC-17).
- **Free money is allowed for missions,** named, measured and logged: try without first,
  and prefer it to reworking the model for a small issue (MSN-5, ECO-1).
- **Invented targets change nothing,** but the time spent hunting them counts as patrol
  time in the sector's risk (MSN-6, TRD-16, SYS-19).
- **Designed for later rounds:** the Authority's board, data store and cut (MSN-7,
  FAC-14, ECO-10), as the response to the merchant squeeze; spy missions and per-trip
  suspicion against stealth and detection, replacing salaried spies and the per-faction
  meter (MSN-11, INF-13, INF-14, PIR-9, PIR-12, PIR-15); delivery top-ups, a narrow
  exception to TRD-2 held until tribute relief is measured (MSN-8); war margins
  (MSN-9); merchant ship profiles (TRD-18); missions between governors, with refusal
  sized by the ask (MSN-12, REL-10).
- **The next round** builds the roles model (fitness, stewards, governors' fleets,
  succession, mission rules), then the war and fealty money, and then tunes once (MS-8).
- **Retired:** OPN-15, OPN-40 and OPN-42 (answered). **New questions:** the start and the
  Lattice Drive (OPN-43), the ship's effect on stealth (OPN-44), the spy and pirate
  overlap (OPN-45), and the fitness weights (OPN-46).

### 2026-10-19 · War and fealty money, design

Agreed with the human in conversation, written into the spec as `proposed`; not built.
Placeholders for the targets, to be tuned when something isn't working.

- **A target before tuning:** CHK-26's bands for tribute, military spending and
  patronage as shares of net income, in peace and war. Round 4 measured military
  spending at about 4% of money created and patronage above tribute (MS-8).
- **Bounties for kills,** paid monthly by the root at `bounty.share` 0.5 of each enemy
  ship's price; for kills, not participation (MIL-18, SYS-14).
- **Spoils of capture:** `spoils.per_prosperity` × N × P from the captured planet's
  treasury, split by combat power × days in the siege (MIL-19, SYS-13).
- **Fleet loans:** serving another governor is a loan of the service contract for a
  duration. A liege borrowing pays the stipends plus `loan.fee_months`; a vassal
  borrowing for relief pays the stipends and spends favour, with `loan.relief_fee` 0.
  What's lent is the employer, never the ships (MIL-3, MIL-11, REL-8; REJ-12 stands).
- **Commanders choose their objectives** as missions, by expected kills, spoils, losses
  and the stipend at stake if their employer falls; declining every one is a small bad
  deed. No forced missions. Armies are whoever joins by the deadline (MIL-2, MIL-9,
  CHR-10, SYS-9, SYS-13).
- **Tribute is meant to be high;** a vassal meeting a fixed need test can ask for
  relief, granted or refused at a cost in opinion, ending when the need clears
  (FAC-6, FAC-15, SYS-11). **Patronage** is narrowed to rebuilding captured planets
  (FAC-7, SYS-5).
- **Reputation raises a stipend,** up to double on a levelling curve (`hire.rep_mult`,
  `hire.rep_half`) (MIL-2, REL-7); what it's worth to an employer is OPN-40.
- **Messages go one way:** contracts end on their own terms, requests carry a deadline
  and a default, and the fleet is the reply (INF-16).
- **Goals and decisions are separate;** a decision never replans a goal on the spot
  (CHR-2).
- **Deferred:** munitions (MIL-4) and the liege paying for a whole campaign, until
  bounties show whether war is still too cheap; whether the war vote weighs bounty costs
  (OPN-41); the economic levers (cheaper life support, growth to a capacity, luxuries
  from what's left), discussed for a later round (MS-8).
- **Open:** character roles (OPN-42), which the human is taking to the planning agent
  and may change the commander rules before they're built.
- **Not taken:** parallelising decisions on a GPU or threads (sweeps already use every
  core; decisions are branchy and depend on each other; profile first, TIM-8).

### 2026-10-18 · Phase A round 4, built and run

The design below, built (MS-8, round 4), and what the runs needed. Proposed until the
human signs off.

- **Budgets and tribute read net income** (receipts less goods bought and the cost of
  making them); war, capture value, credit and rebellion keep gross (SYS-5, SYS-11;
  answers OPN-39). On gross, 55% to 80% of planets fell into debt. Net for everything
  was tried and not taken: the war vote divides by a planet's own income, which can be
  near zero net, and wars tripled.
- **Rebellion counts debt in months of gross income** (`pirate.flip_debt` 1; was 3
  months of operating costs) (SYS-16).
- **Growth is full at half a reserve's surplus** (`pop.full_excess` 0.5): prices resting
  on cost leave stocks only 15% to 40% over reserve (SYS-1).
- **Osmosis carries most life support** (`osmosis.rate` 0.2, was 0.01): with no glut,
  merchants carried 87% of it and war cut them off (SYS-6).
- `econ.op_per_capita` 250 (was 300); `prod.labour_per_capita` 5.
- **Found:** merchant profit per trip fell to a tenth; idle characters (23%) and pirates
  (15% of characters) are over their bands. Next to design (MS-8).

### 2026-10-18 · Phase A round 4, design changes

Agreed with the human in conversation, written into the spec as `proposed`.

- **Making goods costs money, and efficiency is that cost:** a share of base price per
  unit by tier, with every tier making the same per worker. A slot idles when its bid
  doesn't cover its cost, so prices rest on the cheapest producers' cost rather than
  the clamp. Contraband is cheap to make; its cost is risk (PRD-6, PRD-7, SYS-2).
- **Labour moves toward the best margin,** not the best price (PRD-9).
- **The cost is a sink,** paid even in debt (ECO-5, ECO-8). Paying it as wages was
  considered and not taken: wages are spent, which is new money again (ECO-1).
- CHK-14c: at most 10% of prices at the clamp. The build plan is in MS-8.

### 2026-10-18 · Phase A round 3, built and run

The design below, built (MS-8, round 3), with what the runs found. Proposed until the
human signs off.

- **Luxuries take more labour than life support** (`good.yield`: regional 1/6, fringe
  1/3). At a yield of 1 a luxury paid more per worker than food even at a glut, so
  every luxury market sat at the price floor (PRD-7, SYS-2).
- **A security unit is worth a fixed multiple of its cost** (× threat × reserves), so
  the number bought doesn't grow with income; a rich planet buys bigger units. Valued
  by income, the richest spent a third of their gross income on guns and dragged their
  lieges into debt through patronage (MIL-5, SYS-5).
- **Fringe planets weight life support × 3** (`gen.fringe_ls_mult`), and the fringe
  luxuries × 2, not 5: most fringe planets otherwise lost 90% of their people (PRD-7,
  GEN-6).
- **A luxury's producer guarantee holds across its home regions together,** not per
  home region: small worlds can't fit it per region (PRD-8, GEN-6).
- **Contraband demand** reads the old `(1 − H)` as `(1 − s(H)) ÷ 2`, s the signed u: ½
  at neutral, so content planets still buy some (SYS-16).
- **Tuning:** `rebel.scale` 0.01 → 0.007, `defence.per_spend` 0.01 → 0.0015.
- **Fixed:** an event's efficiency shift was left behind when a planet's goods changed
  (turning pirate, recapture); it's now moved with them (SYS-4).

### 2026-10-18 · Phase A round 3, design changes

Agreed with the human in conversation, written into the spec as `proposed`.

- **Happiness:** a signed scale with 0 neutral and no caps. It settles toward a target
  set by luxuries (each with diminishing returns, so variety pays), contraband and
  hunger, below neutral with no luxuries. A content population buys fewer luxuries
  (PRD-4, SYS-1).
- **Productivity and growth:** 1× at neutral, rising toward a ceiling and falling toward
  a floor with diminishing returns. The floor lets a miserable planet just feed itself:
  poverty is sticky, not a death spiral. Unhappiness slows growth (PRD-5, PRD-2, PRD-6).
- **Eight luxuries,** six regional and two from the fringe, so luxuries are carried and
  distant planets have something to sell (PRD-1, PRD-8, GEN-6). Names are placeholders
  (OPN-35).
- **One money supply per planet:** an unhappy planet raises its luxury reserves and so
  pays merchants more (PRD-10, SYS-3).
- **Rebellion** is a weekly chance from unhappiness and debt, under one scale factor
  (ECO-13, SYS-16).
- **Garrisoning a conquest** is a weighed choice, not fixed (MIL-12, SYS-13).
- **Defence** is bought in proportion to income, and reserves beyond a cushion raise its
  value; deep treasuries at wealthy core planets are intended (MIL-5, SYS-5, CHK-7).
- **Checks:** border planets may change hands often (CHK-6 to 40% a year, the
  most-traded planet at most twice a year); reserves must level off, not stop at a growth
  cap (CHK-7); happiness must spread out (CHK-12).
- Held in reserve: output per head falling with population (OPN-38).

### 2026-10-18 · Phase A round 2, design changes

Agreed with the human in conversation, written straight into the spec as `proposed`.

- **War pacing:** the number of wars is no longer a suing term; weariness a month at war
  is. The winner hears no offer before `dip.min_war_days`, and holds out for what it
  hoped to take, a hope that fades. Money beyond a reserve votes for war. Target: each
  faction at war a little more than half the time, wars of one to three months (DIP-3,
  DIP-4, SYS-12, CHK-8).
- **Rebellion:** hunger drains happiness (PRD-2). A planet fails on a long deficit or on
  long hunger and misery; with no vassals it turns pirate, with vassals it secedes and
  its old faction declares war (ECO-13, PIR-3, SYS-16). Rebellions continue through the
  warm-up (GEN-10). A failing root with vassals is open (OPN-37). World generation stays
  as it is: where piracy lives should come from the mechanics.
- **Blockades:** an army at war can attack an enemy's gate, defended at a share of its
  holder's static defence. Captured, it's a discrete blockade, shut to all but the
  captor's faction, which can exit there. Unheld, it's retaken by any force of the
  holder's faction; it lifts with peace. Blockades are facts; travellers route around
  known ones and find out at the gate otherwise (MIL-17, WLD-12, WLD-13, INF-2, SYS-13,
  CHK-6).
- **Capture:** the conquering army garrisons its prize for `capture.hold_days` (SYS-13).
- **Round 2 tuning:** `value.aftermath_months` 2 → 8, `defence.per_spend` 0.005 → 0.01,
  `value.blockade_months` 8, `rebel.shortfall` 0.5, `rebel.days` 90; CHK-8b's band 2 to 5
  (MS-8's note).

### 2026-10-18 · MS-8, the harness and Phase A round 1

- **Tools:** experiments, checks with proposed bands, and an HTML report, using only
  Python's standard library (ARC-8, CHK).
- **Fealty:** swearing into one's own branch is refused, and a siege ends without
  capture once the planet is no longer hostile (FAC-2, SYS-13).
- **Round 1** (tuning, not design):
  - `prod.labour_per_capita` 3.5 → 8;
  - war-vote weights w_strength 1 → 0.1, w_targets 0.3 → 0.05, w_idle 0.2 → 0.1;
  - `dip.declare_threshold` 1 → 1.5.
  Population now holds and conquest slows (MS-8's note).

### 2026-10-18 · MS-7, spies

- **Hiring:** spies are a budget option at war, hired from idle independent traders on a
  secret contract that doesn't count as membership (INF-13, SYS-17).
- **Rounds:** to the least recently seen enemy market in range. Cover trades lower
  suspicion; a visit without one raises it.
- **What they see:** armies forming or stationed there, recorded as sightings that reach
  home and the threat check.
- **Caught:** cover blown on inspection; the contract ends; the `caught` deed; an attack
  by the planet's commanders.
- **EXP-4:** `spy.enabled` turns spies off.

### 2026-10-18 · MS-6 part 2, smuggling and suspicion

- **Pirate runs:**
  - posted weekly at havens from need and surplus;
  - offered on contact to bold independents, which sets the relation to neutral;
  - taken when free;
  - no contract, so the runner isn't a full pirate (SYS-16, PIR-10, PIR-13).
- **Smuggling** sells to the most populous known lawful market at a daily wish capped
  by `pirate.demand_cap`, as untaxed population spending (PRD-6, PIR-6).
- **Legal-goods runs** sell into the haven's market with a per-unit bonus (PIR-3).
- **Suspicion:**
  - held per root;
  - raised by contraband aboard and a high pirate relation, lowered by genuine trade;
  - inspections on landing at lawful planets, with intensity from policing and war;
  - caught means confiscation, a new `caught` deed, and suspicion (INF-14, SYS-17).
- **Policing** is a budget option valued by the productivity contraband costs (PIR-15).

### 2026-10-18 · MS-6 part 1, pirates

- **The pirate faction:**
  - a faction id with no root;
  - no diplomacy and no osmosis;
  - full pirates are off the Lattice, and pirate planets' gates are shut to everyone
    (PIR-1, PIR-2, WLD-12).
- **Flips:**
  - weekly by ECO-13's rule;
  - the governor turns with the planet; its vassals go free;
  - production goes to contraband;
  - forgiven debt is money created;
  - neighbours see it (PIR-3, ECO-13).
- **Pirate planets:**
  - their population buys contraband before luxuries;
  - they keep every commander who joins and budget only static defence (proposed);
  - capture restores production (PIR-3, MIL-12).
- **Broke commanders** turn pirate by risk tolerance if they know a haven (PIR-4,
  EXP-9's switch).
- **Pirate raids** *(proposed)*: target lawful planets within `pirate.range_days` by
  population; hunger raises the rate (PIR-3, SYS-16).
- **Engagements:** a cooldown after any fight in open space (proposed).
- **The Authority** posts clearing contracts paid from its treasury (FAC-14).
- **Warm-up:** no invasions during it.

### 2026-10-18 · MS-5 part 2, risk, raids, escorts and news

- **Risk:**
  - per sector, fed by raids and raiders' time in open space, cleared by patrols;
  - beliefs are seen at leg ends and planets and shared (SYS-19, INF-6);
  - merchants charge believed route risk on their cargo (SYS-7).
- **Raids** *(proposed)*: spare commanders sortie toward the nearest believed-enemy
  planet, engage enemy members in range, take loot up to capacity, and sell it at home
  (MIL-15).
- **Escorts:** hired at a stop from armed characters idle there, by the expected-loss
  formula; they travel and fight with the merchant and are paid at the trip's end
  (TRD-8 to TRD-10).
- **Sorties:** out toward a planet without landing, held at a standoff, then home. They
  serve raids, war scouting and patrols.
- **Warnings and garrisons:**
  - sightings record intent and power;
  - a governor warns its liege of an army coming for it;
  - relief can garrison before a siege (MIL-11, INF-12).
- **Security missions** patrol a planet's own space when its believed risk is high
  (MIL-5).
- **DIP-5:** war and peace are facts. Governors, merchants, raiders and patrols act on
  them; osmosis, gates and the vote use the truth.

### 2026-10-18 · MS-5 part 1, war

- **Correction:** escorts come with risk in MS-5, not MS-6 as MS-3's entry said; MS-5's
  scope already lists them.
- **Diplomacy:**
  - weekly war vote and suing with first-guess weights;
  - no declarations in the warm-up;
  - relations between roots, forgotten by a new root and inherited by a captured root's
    heir;
  - declarations take effect at once until DIP-5 is built (SYS-12).
- **Combat:** damage carries between rounds; zero-power ships go first; the strongest
  fleet's risk tolerance sets the retreat point; pods land at the nearest planet not at
  war with the pilot (SYS-14).
- **Defence:**
  - static defence is a budget option bought in units of monthly security spend;
  - defence and invasions are worth more at war and on the border;
  - static defence rebuilds daily when not besieged (MIL-5, SYS-5).
- **Invasions:**
  - a one-unit budget option on P(win) = A² ÷ (A² + D²);
  - vassals answer by opinion;
  - armies move as one around enemy gates;
  - static defence is only damaged once the defending ships are gone;
  - relief goes at P(win) of at least a third, or the call goes up the tree;
  - keep or grant scores protection by travel days (SYS-13, FAC-8, FAC-9).
- **Capture of a root:** its most populous vassal becomes the root, with the wars.
- **Goods ledger:** capture losses and destroyed cargo are recorded. A merchant carries
  one good at a time.

### 2026-10-18 · MS-4, politics

- **Opinion** fades lazily (the same as a weekly pass) and keeps the last
  `rel.recent_deeds` deeds per relationship (SYS-10).
- **Tribute** is paid at the vassal's monthly payment on its income since the last one,
  cascading; the liege's tools are rules: raise on the loyal when in the red, lower for
  the wavering, each a deed, with `deed.tribute_lowered` new (SYS-11, FAC-12).
- **Defection** goes to the rival root that thinks best of the defector within
  `fealty.threat_days`, else independence; a "not set" time is now `kNever`, not −1
  (SYS-11).
- **Budget:** commanders and patronage as unit options valued in money per month, with
  the `value.*` heuristics; risk eases diminishing returns as
  `budget.diminishing ^ (1 − risk ÷ 2)`; the existing commanders are funded first
  (SYS-5, SYS-9). Every value is a first guess for Phase A.
- **Commanders** serve at their governor's planet in peacetime; starting commanders are
  one gunship each, up to `gen.commanders_per_income` × income (GEN-9).
- **Dues** at each planet's monthly payment; the Authority keeps its surplus until it
  can hire mercenaries (SYS-20).

### 2026-10-18 · MS-3, characters, movement, information and trade

- **Order:** fealty trees (GEN-8) and governors (GEN-9) are built in MS-3, ahead of MS-4,
  because faction merchants, sharing and couriers need them. Escorts move to MS-6 (no
  risk before pirates); sightings to MS-5.
- **Ships:** every culture has four classes (courier, scout, freighter, gunship) at three
  tiers, from per-class stats scaled per tier and varied per culture, as placeholders for
  the content round (OPN-35).
- **Movement:** journeys are routes of legs; open space is straight legs with known
  times, interpolated by the kinematic step, which hashes only ships something could
  happen to (TIM-5, TIM-6). Fuel is per Lattice transit, at the last landing's price.
- **Facts** are per-market price records; members are governors and holders of standing
  contracts; members share on landing at a faction planet and on meeting in open space
  (INF-4, SYS-15).
- **Merchants:** slice-by-slice leg sizing, cargo aboard sold first, docking time, a
  faction merchant heads home when away, idle merchants explore stale markets (SYS-7).
  Every root offers a fixed salary until the budget model exists (SYS-9).
- **Missions:** courier missions are a share of the liege-to-vassal runs; roots post
  trade scouting as planet-level missions open to anyone *(proposed)*; missions are round
  trips paid on return; reputation is missions completed (SYS-8, REL-7).
- **Spending:** a ship of the class that has paid (freighter or gunship), added or
  replacing the cheapest one tier up; broke characters sell ships (SYS-8, PIR-4).
- **Found, not acted on:** merchants can't fix the starting values' food shortage, and the
  happiness loop is bistable (MS-3's note).

### 2026-10-18 · MS-2, world generation and the economy

Choices made while building, all *(proposed)* where they go beyond a rule:

- **Scale:** the planet count includes moons, every body a settled market (GEN-1).
  Distances are in sector widths, and reference travel speeds follow the prototype's
  scale, a week to cross a sector in open space and half a day on the Lattice (TIM-4).
- **Generation:** at most one system per sector, placed by smoothed random density with
  regions taking turns so each has enough planets (GEN-3); large systems split into
  inner and outer groups (GEN-3); fringe chosen toward edges and region borders (GEN-3);
  junctions where three or more roads meet an interchange (GEN-4); the producer repair
  never creates a new shortfall (GEN-6); seats of power are chosen now, for fuel prices
  (GEN-7, GEN-8); fringe fuel × `fuel.fringe_mult` (GEN-7).
- **Economy:** production runs before buying; a short budget scales life-support
  purchases down evenly; growth's excess is capped at 1; the variety threshold is a share
  of a luxury's full use, correcting SYS-1's formula, which made variety impossible
  (SYS-1). A tier below low is low × low ÷ medium (SYS-2). Events don't stack (SYS-4).
- **Osmosis** stops when the sale no longer covers the purchase, so it can never create
  money (SYS-6, ECO-1).
- **Saves:** each record lists its fields once; loading checks every index (ARC-9).
- **Found, not acted on:** with the starting values the world can't feed itself
  (MS-2's note). Tuning waits for merchants and Phase A.

### 2026-10-18 · MS-1, the simulation skeleton

- **Build:** the Godot binding is built with godot-cpp's CMake support, not SCons, so
  one build system covers the core, tests, runner and binding (ARC-11, ARC-12).
- **Determinism:** the RNG distributions are written in the core, and FMA contraction
  is off, so builds at different optimisation levels agree (ARC-6).
- **Event order:** events at the same time run by phase (scheduled, cadence, kinematic),
  then sequence number; `advance(dt)` covers `[now, now + dt)`; cadences are recurring
  events in the one queue (TIM-2, MOD-11).
- **Saves:** magic, version, payload and hash; config stored by key; a build reads only
  its own version (ARC-9).
- **Binding:** `make gdext` registers the extension itself, working around a Godot 4.7
  crash on the first discovery of a godot-cpp extension (ARC-4).

### 2026-10-18 · The spec is created

The planning drafts `FACTIONS.md` (2026-09-27), `ECONOMY.md` (2026-09-28), `TRADE.md`
(2026-10-12) and `CHARACTERS.md` (2026-10-17) were consolidated into `docs/spec/` and
removed from `docs/planning/`. They're preserved at the git tag `planning-drafts-1`. The
PR that made this change carries a coverage table mapping every draft section to its
rule.

Decided in the review rounds of 2026-10-17 and 2026-10-18 and recorded here for the
first time:

- **Process:** the planning folder is for ideation; the spec is the source of truth; drafts
  are deleted once formalised (README).
- **Characters:** stewards removed (REJ-8); standing politicians removed (REJ-9); the
  governor who keeps a conquered planet chooses its governor, preferring the capturing
  commander (FAC-9); pod death means reload and the game autosaves on landing (CHR-18);
  running costs are low, not zero (CHR-9); most couriers are automated (INF-8); the
  merchant path ends at rich merchant (CHR-11); patrols are procedurally generated, with
  optional paid security missions (MIL-5).
- **Fealty:** the fealty deal (FAC-5); tribute (FAC-6); patronage as investment (FAC-7);
  keep or grant (FAC-8); money and relationships as the only two systems, loyalty and
  favor as opinion read from each side, deeds and fading (REL-1 to REL-4); defection
  from sustained low opinion plus an alternative (FAC-11).
- **Merchants:** only the root pays faction merchants, who get the information network
  and run three trip shapes (TRD-3); independents run single trips (TRD-4); planet
  merchant fleets dropped (REJ-25).
- **Information:** every character has its own facts; sharing keeps the newest; for now
  only within a faction (INF-2, INF-4).
- **Contracts:** either side can end any contract with no penalty (REL-8).
- **How far the player can rise:** a player governor who leaves fealty becomes a root,
  by the ordinary rules (CHR-17). This closes the drafts' open question.
- **Standing** is the governor's or root's opinion (REL-5); local opinion is experimental
  (REL-6). Independent single trips are open to all but pirates; faction missions need
  membership (TRD-5, INF-10).
- **Piracy:** one collective pirate faction, an anarchy (PIR-1); broke commanders turn
  pirate, with the risk-tolerance split experimental (PIR-4); loot sells only to pirate
  planets, whose prices steer raids (PIR-5); no redemption for ships (PIR-7); pirates
  raid, don't invade (PIR-8); pirate planets make only contraband and live on missions
  and raids (PIR-3); the pirate relation as shield and suspicion (PIR-9, PIR-10);
  smugglers are the disguised pirates, in v1 (PIR-12).
- **Fuel** is a fixed local price in v1, not a good (ECO-11).
- **Production v1:** three life-support goods, three luxuries, one contraband (PRD-1);
  growth from the scarcest life support (PRD-2); three goods per planet at high, medium
  and low efficiency, drawn partly regionally (PRD-7); labour shifts toward what pays
  (PRD-9); price from stock as a smooth curve shown in bands (PRD-10); efficiency events
  (PRD-11); the governor's growth factor from risk tolerance (PRD-3).
- **Diplomacy:** war, peace and ceasefire, with suing for peace (DIP-1 to DIP-3).
- **Sieges** in the simulation (MIL-10); a simple combat formula for now (MIL-13).
- **Simulation:** C++20 core with a thin godot-cpp binding (ARC-1, ARC-4); world
  generation in C++ (ARC-10); all milestones built before judging, small worlds for
  building, verdicts at v1 scale (SIM-2); the goals of stability then ablation (SIM-2);
  spies in v1, built last (INF-13, MS-7); academies and aliens out of v1 (SIM-3).

Proposed in the same rounds and not yet signed off (tagged `proposed`): the war vote
(DIP-4), declarations by courier (DIP-5), ceasefire terms (DIP-2), siege detail
(MIL-10), the tribute cascade (FAC-6), prosperity loss on capture (FAC-10), escort pay at
trip end (REL-9), refusing a call as a deed (REL-10), reputation (REL-7), full pirate
status (PIR-11), the pirate top-up (PIR-8), and every `proposed` formula in `sim/`.

### 2026-09-27 to 2026-10-17 · The planning drafts

Four rounds of drafts and reviews, merged as PRs #24–#34. Their content is in the spec;
their text is at the tag `planning-drafts-1`.

## Rejected

Ideas considered and dropped. Don't reintroduce one without a new reason.

| ID | Rejected | Why | Replaced by |
|---|---|---|---|
| REJ-1 | The Lattice as a natural wormhole | Breaks the built, trucker's-highway feel | WLD-1 |
| REJ-2 | A committee of three powers running the Lattice, with proxy wars and cores that never fall | Static | Emergent fealty trees and the Authority (FAC-2, FAC-13) |
| REJ-3 | A Trade Authority with planets and a large army | Makes it a player in wars | Hired mercenaries (FAC-14) |
| REJ-4 | Three fixed factions | Static | Seats of power in an emergent structure (FAC-2) |
| REJ-5 | Faction- or culture-specific Lattice technology | Variety belongs to hardware | WLD-20 |
| REJ-6 | "Threader" | A speech-to-text error | The Lattice Drive (WLD-8) |
| REJ-7 | Pirate range bounded by Lattice fuel | Pirates don't use the Lattice | Slow open-space travel (PIR-2) |
| REJ-8 | Stewards running planets for governors in the field | Characters rise organically instead | FAC-9. *Reversed 2026-10-22:* governors now sail with their own fleets, and stewarding is a way up for merchants and spies (FAC-16) |
| REJ-9 | A pool of standing politicians without planets | No job once governors are chosen from existing characters | FAC-9 |
| REJ-10 | Prisoners, ransom, exchange, captivity | Clunky, and from a historical setting Lattice isn't in. Never in the game. | Escape pods (CHR-15) |
| REJ-11 | A closed money loop (wages from production) | Money bunches up in one part of the economy | ECO-1, ECO-5 |
| REJ-12 | Governors owning and lending fleets | Breaks the player's ownership of their ships | CHR-8 |
| REJ-13 | No fleet running cost at all | Money piles up and fleet size isn't tied to income | Low running costs (CHR-9) |
| REJ-14 | Favor as a currency or ledger, alongside loyalty and money | Three systems where two do the job | REL-1 |
| REJ-15 | Reputation or opinion penalties for ending contracts | Players mustn't feel locked in | REL-8. *Narrowed 2026-10-22:* abandoning a mission counts as failing it (MSN-3) |
| REJ-16 | Money from population alone | A blockaded planet would keep printing income | ECO-1 |
| REJ-17 | Money from trade taxes alone | Doesn't explain where money enters | ECO-1 |
| REJ-18 | Private producers and shop owners | Accounts without decisions anyone sees | ECO-2 |
| REJ-19 | Dropping gate dues without going pirate | A closed exit the player can't explain | ECO-10 |
| REJ-20 | Resetting a new pirate planet's productivity | A special case | Low cost base (ECO-14) |
| REJ-21 | Goods destruction as a money sink; sinks only in combat and conquest | Money keeps circulating, and piles up in peace | ECO-5 |
| REJ-22 | Dependence, or lost tax base alone, as contraband's downside | Not sensible; too weak | The productivity penalty (PRD-6) |
| REJ-23 | Discrete price levels internally | Every merchant sees the same jump and herds | Smooth curve shown as bands (PRD-10) |
| REJ-24 | Agent auction pricing (BazaarBot style) | Hard to tune and read; beliefs already give emergence | PRD-10 |
| REJ-25 | Planet merchant fleets | No motivator once merchants are characters paid by price | TRD-1, TRD-3 |
| REJ-26 | The planet as matchmaker for convoys and escorts | Adds waiting, duplicates price | TRD-2, TRD-8. *Narrowed 2026-10-22:* top-ups for planets that can't bid (MSN-8). *Narrowed 2026-11-26:* supply orders, the planet's own price carried to a producer (MSN-15) |
| REJ-27 | Rust with gdext for the simulation | 0.x with breaking changes, few shipped games, weaker Windows story | C++ (ARC-1) |
| REJ-28 | Fuel and munitions competing with cargo space | Makes buying consumables feel bad | ECO-12 |
| REJ-29 | Separate legality lists for the Authority and each faction | Unreadable | PIR-14 |
| REJ-30 | Catalogue equipment through the supply chain | Too specific | PRD-14 |
| REJ-31 | A supply chain of pilots | Replaced by academy worlds, which were then dropped too (REJ-39) | CHR-6 |
| REJ-32 | Governors hiring procedurally generated escorts for planet convoys | Planet convoys are gone; escorts are characters | TRD-8 |
| REJ-33 | The invading commander automatically becoming governor | The keeper chooses, preferring them | FAC-9 |
| REJ-34 | A fracture-pressure rule by distance or size | Emerges from deeds | REL-4 |
| REJ-35 | Pirates as individuals, enemies of everyone including each other | Simpler as one anarchic faction | PIR-1 |
| REJ-36 | Purely random production draws | Every good ends up close to everywhere | PRD-7 |
| REJ-37 | Trading standing for money | Money is never traded for opinion | REL-1 |
| REJ-38 | A greed trait | Risk tolerance covers it | CHR-4 |
| REJ-39 | Academy worlds training crew of their culture | Not needed; new characters start at low level | CHR-6; crew hiring stays open (OPN-36) |
| REJ-40 | Bounties for taking part in a war action | Pays for turning up, not for fighting | Bounties for kills (MIL-18); spoils weighted by days in the siege (MIL-19) |
| REJ-41 | Governors ordering their commanders onto missions | The one forced choice in a design where everyone chooses | Objectives as missions (MIL-2) |
| REJ-42 | Acknowledgements and resent messages; couriers that nothing can stop, even a blockade | Protocol state for every character, or no fragility at all | One-way messages with deadlines and defaults (INF-16) |
| REJ-43 | Spawned mission kills lowering a sector's risk | Invented targets would change real decisions, and belief and truth would drift apart | The hunt counts as patrol time (MSN-6) |
| REJ-44 | A rule keeping a governor at home during a siege | Leaving is the governor's own risk call | The steward runs the planet (FAC-16) |

## Retired IDs

None yet.
