# Production, population and prices (PRD)

What goods exist in v1, what populations need, how planets make goods, and how prices
form. The formulas are in `sim/systems.md` (SYS-1 to SYS-4).

## Goods

### PRD-1 The v1 goods `v1`

- **Life support:** food, water, oxygen.
- **Luxuries:** three goods. What they are is flavour.
- **Contraband:** one good, a luxury that gives more happiness and carries a
  productivity penalty (PRD-6).

Industrial and military goods have no buyer in v1 (munitions, ships and equipment are
sinks, ECO-6; fuel has a fixed price, ECO-11), so they wait for the full supply chain
(PRD-13).

*Why:* the smallest list that exercises shortages, comparative advantage, luxuries and
smuggling.

## Population

### PRD-2 Life support holds and grows a population `v1`

A planet's population has a **size** and a **happiness**. Size sets how much it consumes
and how much labour it has.

- A population needs **all three life-support goods to stay** and all three to grow.
- **Growth follows the excess of whichever life-support good is lowest,** relative to its
  reserve (PRD-10).
- **A population missing any of the three slowly shrinks.** Never a sudden die-off.

*Consequence:* each planet settles at the size its own production plus imports can feed.
Merchants literally keep populations alive, a blockade shrinks a planet slowly, and a
peaceful trade network grows everyone. Since population is the only source of new money
(ECO-1), a planet that loses trade loses future income as well as goods.

### PRD-3 The governor's growth factor `v1`

Each governor sets a **growth factor** that scales growth. Fast growth eats reserves, so
it makes shortages, overshoot and shrinkage likelier: the factor trades growth against
safety. It comes from the governor's risk tolerance (CHR-4).

### PRD-4 Luxuries and happiness `v1`

Populations consume luxuries **in proportion to their combined excess** of luxury stock.
Consuming luxuries raises happiness, with a **bonus for variety** (more of the three
available). **Happiness drains slowly all the time.**

### PRD-5 Productivity `v1`

Happiness raises productivity, which scales both output (PRD-7) and purchasing power
(ECO-1).

### PRD-6 Contraband `v1`

Contraband raises happiness more than legal luxuries but **lowers productivity by more
than that happiness would have raised it**.

- **The population wants it:** it's the cheapest happiness there is. An unhappy
  population wants it more.
- **The governor hates it:** the point of a happy population is productivity.
- **It's bought from smugglers, untaxed,** so the treasury gets nothing, and the money
  leaves lawful space for the pirates who made it (PIR-6).
- *(proposed)* Contraband demand saturates, so a planet doesn't reach the bottom too
  fast.

The spiral (unhappy → more contraband → lower productivity → lower income → fewer legal
luxuries → more unhappy) has **piracy as its exit,** not a hard floor (ECO-13). Pirate
planets pay the same productivity cost for their own use.

*Tuning target:* net positive for a population, net negative for its governor.

*See:* PIR-14, PIR-15, CHK-10, REJ-22.

## Production

### PRD-7 Each planet makes three goods `v1`

Each planet can make **three goods, at high, medium and low efficiency**. Output is
population × labour share × efficiency × productivity; the planet owns it and it goes
into the planet's market.

- **Drawn partly regionally:** neighbouring planets tend to share strengths (a farming
  region, a water region). That's what makes long hauls pay and gives regions identity.
- A planet in peacetime drifts toward its best good; it's fine if an efficient faction
  has planets producing only that.

*Why:* comparative advantage from geography. Opportunity comes from breadth and spatial
separation, not chain length.

### PRD-8 Redundancy comes from geography `v1`

At world build, every good has **at least two or three producers per culture region**.
The guarantee is per region, not per faction, because factions move and regions don't. A
faction can still lose a good by losing territory; that's the cost of losing it.

### PRD-9 Labour shifts toward what pays `v1`

A planet splits its labour across its three goods and **shifts it slowly toward
whichever pays best** (local price × efficiency). That's v1's role switching: already
damped, driven by price, and in both directions. When a good stays scarce, planets that
can make it at medium or low efficiency shift toward it; when it gluts, they shift away.

*(later)* Building facilities for a fourth good, and governors investing to raise
efficiency (PRD-16).

## Prices

### PRD-10 Price comes from stock `v1`

Each good at each market has a **reserve target**: a number of days of the planet's own
consumption (longer for life support than for luxuries). **Price rises smoothly as stock
falls below the reserve and falls as stock rises above it.** Out of stock means a very
high price.

- **Life-support prices are more sensitive** to reserves than luxuries.
- **A spread:** the planet sells a little above the curve and buys a little below it,
  plus the transaction tax (ECO-3).
- **The price moves as you trade:** selling 200 tons walks the price down as you go.
- **Market depth follows from population:** the reserve scales with consumption, so a
  small colony floods quickly and a capital absorbs a lot.
- **The player sees bands** ("very high", "high", …) over a smooth curve underneath.

*Why:* the X-series model: simple, stable and readable, and the amount and the price are
the same fact, which is what the information economy needs. Discrete levels make every
merchant see the same jump at once and pile onto the same trade (REJ-23). Agent auctions
are harder to tune and read, and beliefs already give the emergence (REJ-24).

### PRD-11 Events keep prices moving `v1`

Price-from-stock settles into a steady state in peacetime, which would flatten the
information economy. **Events** stop that:

- **supply shocks:** a drought drops a planet's food efficiency a tier for 30 days; a
  bumper harvest raises a good a tier;
- **demand shocks:** a festival raises luxury consumption for a few weeks;
- **regional shocks:** a drought across a farming region, so long hauls from the next
  region suddenly pay.

*(later)* Events become news that travels as facts, so a merchant who hears first can
trade ahead of the price.

*See:* CHK-14.

### PRD-12 Consumption is the goods sink `v1`

What's consumed is gone, so demand never runs out. Life support holds and grows
populations, luxuries keep them happy, contraband makes them happy and unproductive.

## The full supply chain

### PRD-13 Supply chains `later`

The target once the v1 economy works:

- **At most three tiers:** raw → refined → finished. Wide, not deep: many goods per
  tier, finished goods taking two or three inputs.
- **Hard dependencies for life support, soft for production:** a population without
  oxygen declines; a refinery without an optional input runs slower.
- **Fuel cells** are the model soft dependency: an input to many refining processes that
  speeds them up.
- **Oxygen** ships as solid tablets, made from an oxygen-rich planet, a rare catalyst
  mined elsewhere and, optionally, fuel cells.
- **Resources are static** (fixed with the region, like culture); **facilities are
  mutable**; output is labour × facilities × inputs.
- **Substitution** (aluminum for steel at higher cost) sparingly, only where it tells a
  story.
- Munitions, ships, equipment and fuel join the economy, paid to the planet that made
  them (ECO-6). War consumes munitions, so fronts are visibly supplied or starved.
- **Pirate facilities** combine legal goods into contraband (oxide catalyst and medical
  supplies into a drug, say), and military contraband such as plutonium feeds pirate
  weapons (MIL-16).
- A first-pass list of about 4 life support, 3 luxury, 8 raw, 6 refined, a handful of
  military commodities and a few contraband goods, to be adjusted by the simulation.

### PRD-14 Commodities and the catalogue `v1`

Military goods exist at two scales. **Commodities** (guns, turrets, hulls in general) are
trade goods in the full supply chain (`later`). **The catalogue** (the specific blue
laser a player fits) is too specific for the supply chain: markets stock it **by random
chance, weighted by distance from the culture that makes it** (WLD-17). *(proposed,
later)* Weight the chance by local supply of the parent commodity, if catalogues feel
static.

### PRD-15 Specialist worlds `later`

Every culture has one or more planets expert at ships, at guns and at missiles. That's
where its hardware comes from, and they're prizes in war.

### PRD-16 Facilities and investment `later`

A planet with a suitable resource can build facilities to make a new good when it stays
scarce long enough, and governors can invest to raise efficiency. Native producers stay
better than converted ones. Damped against the cobweb cycle: the price signal must hold
for a while, switching is slow and costly, and a conversion in progress counts against
the signal others see.
