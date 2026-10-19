# Economy: money (ECO)

Where money enters and leaves the world, what planets earn and spend, how spending
decisions are made, and why planets turn pirate. Goods, population and prices are in
`production.md`; who moves goods is in `trade.md`.

## Principles

- The simulation is authoritative: goods exist on real ships, and markets move because
  real cargo arrives or doesn't.
- Wealth is the root of military power (FAC-4), so the economy decides who can fight.
- Economic warfare is real warfare: killing a planet's trade hurts it through the value
  lost, not an artificial penalty.
- Every playstyle works on its own; a merchant who never fires a shot can grow powerful.
- The player never has to memorise the supply chain. They see its effects: prices,
  shortages, missions.

## Money in

### ECO-1 Population is the only source of new money `v1`

Every planet's population has **purchasing power**, scaled by its size, productivity and
happiness. That is the only place new money enters the world, and it becomes money only
when goods are delivered and bought. A population with nothing to buy generates nothing.

*Why:* a blockaded planet must lose income (REJ-16, REJ-17).

### ECO-2 Every planet is a state trader `v1`

The governor decides what the planet produces (PRD-9) and owns the output. Sales of the
planet's own production go to its treasury. There are no private producers or shop
owners. **Every other trader (merchants and the player) buys from and sells to the
state;** contraband, sold by smugglers straight to the population, is the exception
(PRD-6). To the player this is the planet's trade goods screen.

*See:* REJ-18.

### ECO-3 The transaction tax `v1`

The governor taxes other traders' transactions at the planet's market, sales and
purchases alike. A planet's income is its own sales, plus that tax, plus tribute from its
vassals (FAC-6).

*Consequence:* a planet whose merchants stop coming loses three ways at once: its exports
pile up and their price falls, the goods its population wants stop arriving, and its tax
base shrinks.

## Money out

### ECO-4 Planets have operating costs `v1`

Planets pay to exist: base operating costs and patrol and policing upkeep (MIL-5,
PIR-15). These are **larger than Authority dues**. Their purpose is pressure on
governors: a governor has to keep making money, or the planet drifts toward piracy
(ECO-13). *(proposed)* Operating costs scale with population, so the pressure comes from
margin, not size; flat costs would make small planets always go pirate and big ones never.

### ECO-5 Sinks `v1`

The economy needs sinks or money piles up. **Operating costs are the baseline sink,** in
peace and war alike; **war is the amplifier**, raising patrol costs and destroying fleets
that must be replaced; **raids are a small sink of their own** (MIL-15).

| True sinks (paid to nobody) | Circulates (moves money, removes none) |
|---|---|
| Planet operating costs and patrol and policing upkeep | Tribute, patronage, ceasefire tribute |
| Fleet running costs, fuel (ECO-11) | Stipends and merchant salaries |
| Munitions refills, recruited ships, equipment (ECO-6) | Trade between characters and the state |
| Authority dues spent on network maintenance | Pirate hire pay and loot sales |
| Raid losses; the osmosis margin (TRD-13) | Contraband sales (lawful population to smuggler to pirate planet) |

Destroying goods isn't a money sink: the money that paid for them has already moved on
(REJ-21).

*See:* CHK-7.

### ECO-6 What the economy didn't make is paid to nobody `v1`

In v1, munitions refills, recruited ships, equipment and fuel aren't produced by the
economy, so the money spent on them is a sink. If a planet were paid, it would earn money
for something that cost it nothing, and border planets, which ought to be poorest, would
grow rich refilling the fleets fighting there. When these come into the economy
(`later`), the payment goes to the planet that made them.

### ECO-7 The peace and war cycle `v1`

In peace, ships, weapons and munitions accumulate and treasuries fill, which makes war
attractive. War consumes them, treasuries drain through stipends, and peace starts to
look better. This is an outcome to check for, not a rule.

*See:* DIP-4, CHK-8.

## Spending decisions

### ECO-8 Every spending decision is a budget decision `v1`

Whether to retake a planet, subsidise one, defend one or launch a raid is decided on
**believed value**, **full cost** and **what's available**, by **one decision model for
every spender** (governors, lieges, roots, commanders, merchants).

- **Full cost includes the aftermath:** retaking a planet costs the campaign plus
  rebuilding it.
- **The winner can't take everything.** An explicit spread rule stops the top
  opportunity starving all others; risk tolerance sets how concentrated spending is (a
  bold ruler goes all-in, a cautious one spreads thin). *(open)* Diminishing returns per
  opportunity, or a cap on any one item's share (OPN-18).
- **Speculative spending is allowed,** so **treasuries can go negative**, with pressure
  that grows the longer they stay there (OPN-18).
- **Value is subjective:** shaped by risk tolerance, relationships and possibly stale
  information (INF-2).

## Money within a faction

### ECO-9 How money moves inside a faction `v1`

- **Tribute** flows up (FAC-6).
- **Patronage** flows down (FAC-7).
- **Stipends** for vassal commanders serving the liege flow down (MIL-3).
- **Merchant salaries** flow out from the root (TRD-3).
- *(later)* Loans with interest, only if the above and negative treasuries aren't enough.
- *(proposed)* Trade deals between planets for large standing flows, only if the
  simulation shows planets need them.

Well-run factions can reabsorb a struggling planet; badly run ones leave it failing,
which is intended.

## The Trade Authority's dues

### ECO-10 Two tiers, and it's binary `v1`

- **Network access:** a planet pays for its own ships to travel the Lattice.
- **Gate holding:** a planet that holds gates (WLD-5) also pays for the right to control
  entry and exit there.

A gate holder pays the higher tier or becomes a pirate planet. There's no lawful middle
state of an unpaid, closed interchange, because the player couldn't tell it from a pirate
one. *(open)* Tier amounts against the trade advantage of holding a gate (OPN-19).

*See:* REJ-19.

## Fuel

### ECO-11 Fuel is a fixed local price, not a good `v1`

In v1 fuel isn't cargo. **Every planet and station sells fuel at a fixed local price**
set from its traits at world build: cheap in the core, dear on the fringe. Nobody can
carry fuel as cargo, so price differences never become trade routes. Money spent on fuel
is a sink and part of a fleet's running cost (CHR-9). **Pirate planets sell fuel too.**

*Why:* everyone needs fuel, so as a good it would drive the whole trade network and every
fuel rule change would ripple through everything.

*(later)* Lattice fuel through its own supply chain, with producers at every scale (some
planets import, some export a little, a few are huge exporters and high-value targets),
and fuel cells as a separate production input (PRD-13).

### ECO-12 Capacity costs weight; contents don't `v1`

Fuel tank and missile magazine size are fitting choices that cost weight like any other
fitting. **Filling them costs nothing in weight or space:** fuel and munitions never
compete with cargo. Buying a missile must never mean dropping cargo.

*(open)* How weight affects a ship (OPN-28).

## Turning pirate

### ECO-13 Piracy is an economic outcome `v1`

A planet whose income can't cover its operating costs drifts toward piracy, and
eventually renounces its faction and the Authority to join the pirate faction (PIR-3).

*(proposed)* **Failure has two causes and two outcomes.** A planet fails when it has
been in deficit long enough, or when its population has been hungry and miserable long
enough (PRD-2); isolation from the Lattice shows up as hunger before it shows up in the
treasury. *(proposed, round 3)* Failure is **a chance each week, not a deadline:** it
grows as happiness goes further below neutral and as debt deepens, under one scale factor
for tuning. A failing planet **rebels**:

- **with no vassals, it turns pirate** (PIR-3);
- **with vassals, it secedes** as an independent faction, taking its branch, and its old
  faction **declares war on it** at once. A small rebel faction against its old root is
  meant to be unlikely to last.
- *(open)* A failing root with vassals has no one to rebel against (OPN-37).

Nothing at world generation decides where piracy lives: planets far from the Lattice
go hungry first, so they rebel first.

- *(proposed)* **Damped like defection:** a treasury buffer, plus a deficit that must
  persist for a while before a governor turns, keeps it an individual or regional event
  rather than a map-wide wave when a war cuts a trade route.
- **Levers short of rebellion:** buying legal luxuries, spending on policing, cutting
  patrols.
- **Debt a planet leaves behind when it turns pirate is ignored.**

### ECO-14 Pirate planets have a low cost base `v1`

A pirate planet has much lower operating costs and pays no dues, so a new one has a
decent chance to prosper despite a weak economy and no Lattice access. Strategic ones
usually get retaken; lasting ones gather where nobody cares to take them back, on the
fringe. **Flips near the Lattice are short.**

*See:* REJ-20.

### ECO-15 Repeated collapse is intended `v1`

A faction at war on two fronts prioritises one; a planet on the other border, drained by
raids, turns pirate; the faction retakes it but doesn't invest, and it falls back. Places
that collapsed recently are likeliest to collapse again. This is realistic and wanted.

*See:* CHK-9.

## The merchant playstyle

### ECO-16 No caravan cap `v1`

Bannerlord caps caravan play because a bigger caravan is slower. Lattice doesn't: a
player commanding large freighters can start to move markets, and that's intended. Size
costs speed in open space, but the Lattice is fast either way. Hiring other ships is open
to any character who can afford it.
