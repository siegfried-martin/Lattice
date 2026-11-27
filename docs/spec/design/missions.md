# Missions (MSN)

How anyone who wants something done gets it done: one mission primitive, who issues
missions, what they pay, what happens on success and failure, and the missions of each
path. How a character chooses among them is CHR-10. Its career is CHR-11.

*Created 2026-10-22 from the roles round (planning draft `ROLES.md`, answering OPN-42).
Everything here is `proposed`.*

## The market for wants

### MSN-1 Missions are how an entity says what it wants `v1` `proposed`

An entity that wants something done posts a **mission**. Characters free to choose
among missions take the ones worth it to them. That is a market: the issuer prices the
want, and whoever can do it most cheaply does it.

- **Price still runs ordinary trade** (TRD-2). A planet that wants a good lets its price
  rise. Missions carry the wants a price can't express: kill that fleet, scout that
  sector, carry this message, get food to my blockaded planet.
- **Most wants are already missions:** objectives (MIL-2), loans and relief (MIL-3,
  MIL-11), couriers and scouts (INF-8, INF-9), security patrols (MIL-5), the Authority's
  clearing contracts (FAC-14), smuggling runs (PIR-13), and escort offers (TRD-10).
- **The issuer pays from what the result is worth to it,** out of its own budget (ECO-8).
  Otherwise a mission is spending with nothing behind it.

*Why:* every character chooses its work (CHR-1, CHR-10). A posted want, chosen freely,
is the one way of getting work done that works for NPCs and the player alike.

### MSN-2 One mission primitive `v1` `proposed`

A mission is an offer posted by an **issuer**: a governor, a root, the Trade Authority, a
pirate planet or a merchant. It carries:

- **a type and a target:** cargo and route, a fleet power and sector, a named target, or
  a place to scout;
- **a payout,** visible up front. It grows while the mission sits unfilled and is fixed
  when it's taken (CHR-10);
- **a deadline,** and a **gate** (standing, reputation, loadout) on who may take it;
- **the issuer's funds:** the payout comes from the issuer's budget.

NPCs and the player take the same missions. NPCs pick by fit and pay (CHR-10), and work
away from the player resolves by MIL-13. Missions between governors use the same
primitive (MSN-12). There's no separate type system for each path (CHR-11).

### MSN-3 Success, failure and tiers `v1` `proposed`

- **Completing a mission is a good deed** with its issuer (REL-3). Skills grow with the
  missions taken (CHR-10), and reputation with missions completed (REL-7).
- **Failing a mission is a bad deed** with its issuer: running out of time, losing the
  cargo, or walking off with it. Failure doesn't touch reputation.
- **Abandoning a one-job contract counts as failing it.** This narrows REL-8: a standing
  contract (fealty, service, salary) can still be ended without a penalty, but quitting
  a mission doesn't escape the consequences of failing it.
- **Tiers:** better-paying, riskier missions are gated on the issuer's opinion (REL-5)
  and on reputation. Work for one issuer and its opinion rises, which opens its higher
  tiers. Progression is access and standing, not experience points.
- Non-rush missions have loose deadlines.

*Why:* without a cost to failing, a character about to fail would quit first, and an
issuer couldn't trust anyone with work that matters.

### MSN-4 Issuers decide on what they believe `v1` `proposed`

An issuer posts on its own information (INF-2), as stale as anyone's. Faction missions
reach a planet at courier speed (INF-11). The Authority's offers appear at a planet from
that planet's Authority data store (MSN-7), as old as the store.

What every issuer spends on missions, how many are completed and what it got back are
tracked separately for each issuer (CHK-28).

### MSN-5 Free money, named and measured `v1` `proposed`

If a mission costs its issuer more than it returns, the difference may be created as
**free money**: an exception to ECO-1. Bannerlord's caravan escorts, which pay more than
a merchant could afford, are the model. Each exception must be:

1. a named tunable;
2. tracked as its own flow in the metrics;
3. listed in `DECISIONS.md` as a deliberate exception.

Try without it first. For a small problem, free money is better than reworking the
model to make a mission feel worth taking.

*(proposed, tuning pass part 3)* The exceptions so far: **insurance payouts** (CHR-21,
`ship.insurance_payout`) and **passenger fares** (MSN-13, `passenger.fare`). Both are in
`free_money` and in their own flows.

### MSN-6 Invented targets change nothing `v1` `proposed`

A target that exists only for a mission (a spawned pirate, a bugged package) is
**spawned**, as ambient ships are (TRD-15). It's never persisted, killing it never
touches the economy, and it becomes a character only if something lasting happens (an
escaped pilot can become a rival with a grudge).

- **What does count is the hunter's time.** A character hunting spawned pirates in a
  sector is an armed ship really there, so its time counts toward the sector's patrol
  time, as a security patrol's does, and lowers the risk (TRD-16). The kills don't.
  It can meet real pirates while it's out.
- **Anything on a faction's war board is real** (MSN-10).

*Why:* the simulation's numbers are the truth characters' beliefs approximate (INF-6).
Lowering risk for invented kills would let belief and truth drift apart (REJ-43).

## Who posts missions

### MSN-7 The Trade Authority's board `v1` `proposed`

The Authority keeps a **mission computer** at every planet.

- **The data:** each planet holds a store of market facts (INF-2). **Only merchants
  under an Authority contract share into it,** on landing, keeping the newest facts. This
  makes the Authority a second sharing group beside a faction (INF-4). Other merchants
  can't read it.
- **The offer** reads "deliver N units of X to planet Y for $Z". The payout is the
  believed price spread less the Authority's **cut**.
- **The Authority owns the cargo.** It buys at the origin and sells at the destination,
  in real trades, so goods move on real ships and pay the transaction tax (ECO-3). The
  carrier is paid the fixed fee. **The Authority takes the price drift,** up or down,
  between quote and delivery. The carrier takes the transit risk: pirates, fuel and time.
- **Rush missions** have a shorter deadline and a smaller cut, which reads as a bonus.
  They favour fast ships and tide a short planet over until bulk merchants arrive. A
  chronic failure still fails on schedule (ECO-13).
- **The cut is the Authority's income,** beside dues (ECO-10). It funds the network and
  the mercenary contracts that lower risk (FAC-14). The cut circulates (ECO-5).
- **NPC merchants under an Authority contract** use the pooled data as their
  information, the way a faction merchant uses its faction's (TRD-3). For the player the
  board is a view of the same thing.

*Built after the war and fealty round, as the response to the merchant squeeze
(MS-8).*

### MSN-8 Delivery top-ups from governors and roots `v1` `proposed`

*(tuning pass part 7)* **Replaced by supply orders (MSN-15),** which reach the same
planets with the issuer's own price instead of a top-up.


A governor or root may post a delivery mission that **adds to a merchant's margin** on a
route, when a planet in its tree has stock below reserve and **can't bid its price up
enough to pull supply**: it's poor, starved or blockaded (MIL-17).

- The issuer pays from its budget (the steward's lines, FAC-16) and owns the cargo,
  bought from one of its own planets (ECO-2), so the money circulates inside the tree.
- To merchants a top-up is a margin like any other, so NPCs need no separate mission
  object.
- **It narrows TRD-2 and REJ-26 for this case only.** Everywhere else, price is the
  motivator.

*Held until tribute relief (FAC-15) has been measured,* since both answer a planet too
poor to buy what it needs.

### MSN-9 War margins `v1` `proposed`

Faction merchants and osmosis stop at a war line (TRD-6), so prices drift apart, and a
long war opens a gap. An issuer posts a delivery from enemy space when its believed price
there is well below its own, or to supply a planet cut off by a blockade:

- **payout** = the believed spread, plus a premium for the detection risk (INF-14), less
  the cut;
- **what the issuer will pay is capped by the value of what it lacks,** not by the
  spread, so it can't overpay for a cheap good it doesn't need.

## Missions by path

### MSN-10 Warrior missions `v1` `proposed`

| Mission | Issuer | Target |
|---|---|---|
| Kill N pirates in a sector that has gained power | Authority (neutral and fringe space); a root (in its territory) | spawned |
| Kill a named pirate | Authority or root, by territory | spawned |
| Escort a merchant | the merchant (TRD-10) | real. Needs an armed ship, a Lattice Drive for a Lattice merchant, and a minimum combat power for an armoured one |
| Scout a neutral or friendly planet | Authority (neutral); any governor (friendly) | real facts |
| Scout a dangerous pirate sector | a root or the Authority | real facts |
| Scout an enemy sector | any governor, usually border and fringe | real facts |
| Kill a named enemy ship | a root | real |
| Destroy a caravan | a root | real (TRD-17's raid) |
| Escort a fleet | any governor | real (a loan, MIL-3, seen from the player's side) |
| Destroy a fleet | a governor with vassals, through the root | real |
| Capture a planet | a governor with vassals, through the root | real; how it plays is OPN-4 |

- **Spawned bounties** carry a power rating and a sector, not a ship record. A bounty is
  completed by destroying the ship; a pod that escapes is the pilot surviving (CHR-15).
  Kill-count bounties pay by the sector's risk and disappear when it falls, so farming
  them limits itself.
- **Faction missions are the objective board (MIL-2) seen from the player's side.** The
  same bounties and spoils pay the player (MIL-18, MIL-19); they aren't written
  separately.
- **The Authority and the roots split by territory:** the Authority posts where no
  faction holds the sector, and a faction posts in its own. No double payment for the
  same risk.
- **Scouting follows information:** a governor posts a scout mission when its belief
  about a large nearby threat is stale, and pays by both. A scout reports what's visible
  from outside and risks combat (INF-9). Scouting an enemy commander or governor is a spy
  mission (MSN-11).

### MSN-11 Spy missions `v1` `proposed`

**A scout reports what it can see from outside. A spy works under cover.** A scout
risks combat; a spy risks detection (INF-14). The spy is the smuggler turned good: the
same small, fast, low-profile hull (CHR-3), working for a different customer.

| Mission | Notes |
|---|---|
| Land on an enemy planet | the basic exposure |
| Track an enemy ship | find it and stay within sensor range. Also gives the player intercept targets (TRD-17) |
| Covert delivery | an ordinary delivery the enemy faction offers, with a "bugged" package that has no real effect; the risk is detection. The spy path's first job, as the spawned named pirate is the warrior's |
| Smuggle | contraband for a pirate planet (PIR-3), or goods to or from a blockaded or cut-off planet (MSN-9) |
| Scout an enemy commander or governor | the hardest, and the best paid |
| Copy a planet's data | land, copy the planet's information, deliver it to the issuer |
| Sabotage a ship; meet an enemy leader | `later`: no political effect in v1. Sabotage is safe only on a spawned target |

- **Issuers:** any governor or root, posting when its belief about a threat is stale and
  the threat large, as for scouting; the Authority; pirate planets, for smuggling.
- **Pay is per job** (REL-8). A spy is a freelancer. Completing jobs raises the issuer's
  opinion, which opens higher tiers (MSN-3).
- **NPCs take these missions too.** Spying is a real tool for factions. Independents and
  characters on spy missions may cross a war line (TRD-6).
- `open` **Spies and pirates overlap:** a faction's spy running contraband for a pirate
  planet taints its own record. Until that's decided, runs under a faction contract
  don't move the pirate relation (OPN-45).

### MSN-12 Missions between governors `v1` `proposed`

A liege may offer a vassal missions beyond a loan (MIL-3), with the same board and the
same rules. Every request has a deadline and a default, no answer is a refusal, and the
fleet arriving is the answer (INF-16). A governor with a fleet takes these as any
commander takes an objective (MIL-20).

**Refusing is a deed sized by the ask** (REL-10), so a liege can't push a vassal away by
spamming small requests.

### MSN-13 Passenger runs `v1` `proposed`

*(tuning pass part 3)* Work for the bottom rung. Every lawful planet keeps a few
**passenger runs** posted (`passenger.per_planet`, 2): one way, to a lawful planet within
`passenger.max_days` (4), paying `passenger.fare` (50) a day of the trip.

- **Only a character flying a single tier-1 hull may take one,** so they're a way back up
  for the new and the ruined (CHR-6, CHR-16), not a living for anyone else.
- **The passengers aren't simulated** (MSN-6) and no one issues the run, so the fare is
  free money (MSN-5). Completing one grows the courier skill but not reputation, and
  failing one is no one's bad deed. *(tuning pass part 5)* It does count as a small job
  for the faction it was posted in (REL-5).
- The fare is sized so a ruined courier earns back a bought hull in under a year
  between courier jobs, at about a third of a courier run's pay.
- *(proposed, tuning pass part 5)* **They're short hops in open space:** destinations and
  fares go by open-space travel time, so a shuttle without a Lattice Drive (TRD-18) can
  fly every one. They're the only missions such a hull can take; the rest need a drive.


### MSN-14 Errands: no one waits long for work `v1` `proposed`

*(tuning pass part 6, the human's rule)* A lawful character on the bottom rung (a single
tier-1 hull, MSN-13) that finds nothing worth doing waits a random
`passenger.wait_min_days` to `passenger.wait_max_days` (12 to 48 hours). Then its planet
posts it an **errand**: a passenger run (MSN-13) to a nearby lawful planet, which it
takes. In effect it's paid to look at its next planet.

- **It's free money** (MSN-5), counted with passenger fares and in its own count
  (`chars.errands`), and it counts as faction work (REL-5), like any passenger run.
- **It replaces wandering for the bottom rung** (TRD-4): it moves on paid, not on its
  own. Characters above the bottom rung still wander.
- **No one is stranded.** Where open space doesn't get out (no lawful planet within
  `passenger.max_days` of open space), a hull with a Lattice Drive is posted its errand
  by the Lattice, within the same days, and the fare also covers the fuel. Failing that,
  the errand goes to the nearest lawful planet the hull can reach at all. A pirate planet
  posts them too, for a lawful character whose pod landed there. Without this, a
  character in debt where nothing nearby could be reached couldn't buy fuel, and stayed
  there, going bankrupt every two weeks.
- **None at a planet under siege;** it looks again the next day.
- **For the player** *(later, once the player is in the sim)*: a lawful planet's board
  is never empty when the player docks. It always shows at least a couple of missions,
  topped up with errands if it must. The player doesn't wait 12 to 48 hours; the wait
  is for characters.

*Why:* the bottom rung had more characters than paid work, so the jobless fell into
debt on upkeep and went bankrupt over and over, every two or three weeks. Paying them
to move keeps them solvent and moving, so the galaxy looks alive. It also mirrors what
the player should find: "I don't want a player going to a new planet and finding a
mission computer that has nothing."

### MSN-15 Supply orders `v1` `proposed`

*(tuning pass part 7, the human's design)* A planet short of life support sends for it.
Merchants plan among the planets nearest them (TRD-4), so a short planet far off the
Lattice never enters their sums, however high its price. An order carries the want to
where the merchants are.

1. **The order.** Each day a lawful planet that can buy (ECO-8), not under siege, looks
   at each life-support good. If its stock plus what it has on order is below
   `order.trigger_fill` (0.6) of its reserve, it orders the shortfall to the reserve.
   The order goes to the nearest planet it has heard has a surplus of the good (a price
   below base, by its own facts, INF-2): lawful, not under siege, not at war with it,
   and reachable by some hull. If it knows of none, it orders nothing. The price a unit
   is fixed when the order is made, at `order.price_share` (0.5) of its own price then:
   well under it, because the planet takes the risk that the market moves. "They
   shouldn't advertise the full price they will pay at that moment, but reduced
   significantly. The planet is taking the risk of market conditions changing since
   they are giving a fixed reward."
2. **The courier leg.** The order is a one-way courier mission at the short planet, to
   the producer, paying `order.courier_pay` (50) a day of the trip from the issuer's
   treasury. Any hull may carry it, including one without a Lattice Drive if open space
   gets there (TRD-18), so it's real, paid work for the bottom rung beside errands
   (MSN-14). It's a job for the issuer (REL-3, REL-7) and grows the courier skill.
3. **Untaken, it arrives anyway.** If no one has taken the courier leg
   `order.courier_wait_days` (3) past the trip's own days, the order reaches the
   producer without a carrier and without a fee. "We still want these planets to get
   their goods." A courier who takes it gets it there sooner.
4. **The delivery.** At the producer the order is posted on the board as a delivery:
   the good, the amount, the price. A merchant there with an empty hold weighs it as a
   trip beside its own (TRD-3, TRD-4): buy the good at the producer's market for no more
   than the order's price less its believed route risk, fly to the issuer, and sell it
   there at the order's price. It takes as much as its hold carries and the rest stays
   posted. A faction merchant delivering outside its faction comes back with a leg, as
   in TRD-3.
5. **The money is real.** The issuer pays the courier and the delivery from its
   treasury (ECO-2), so this is no exception to ECO-1 (MSN-5). A planet too broke to buy
   orders nothing, and starves, which is meant: "having broke planets that can't buy
   food is actually correct for the simulation."
6. **What's on order is counted** (`on_order` on the planet), so a planet doesn't send
   for the same shortfall twice. An order that's lost (its courier fails, MSN-3) or a
   delivery no one takes within `order.supply_days` (30) comes off the count, and the
   planet may order again. A delivery taken and not completed is a failed mission
   (MSN-3).
7. **Measured:** orders posted, carried and arrived untaken; deliveries made and
   failed; units delivered, what the issuers paid for them, and the couriers' fees
   (`orders.*`).

- **It narrows TRD-2 and replaces MSN-8.** The planet still sets its price; the order
  carries that price to merchants who would never have looked, and commits to it.
  Unlike REJ-26's matchmaker it adds no waiting and no subsidy.
- **Once taken, a delivery is no one else's,** so ten merchants don't race on the same
  news and arrive to a glut.
- **The incentives the human named fall out of it.** An interior producer gets steady
  sales; a far planet gets its food at a price that pays the haul; a faction's merchants
  ship it inside the faction.
