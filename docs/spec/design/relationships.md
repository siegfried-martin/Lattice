# Relationships and contracts (REL)

How characters feel about each other, what standing and reputation are, and how every
job and allegiance works as a contract.

## Two systems

### REL-1 Money and relationships, nothing else `v1`

The political and social game runs on exactly two systems: **money** and
**relationships between characters**. There is no favor currency, no separate loyalty
value and no standing currency.

- **Money** pays for what costs money: tribute, patronage, stipends, salaries, ships.
- **Relationships** are memory: how one character feels about another because of what
  has happened.

Money is never traded for opinion directly. Spending money on someone is a deed
(REL-3), so it moves opinion, but there's no exchange rate.

*Why:* every extra currency is one more thing for the AI to price and the player to
manage. The human wanted elegance; one relationship read from both sides does what
loyalty, favor and standing currencies were going to do (REJ-14).

### REL-2 Opinion, read from both sides `v1`

A relationship is each character's **opinion** of another, and it's directional. Between
a liege and a vassal:

- **the vassal's opinion of the liege is loyalty.** It decides whether the vassal stays
  or defects (FAC-11);
- **the liege's opinion of the vassal is favor.** It decides who gets aid first, who is
  granted conquered planets and who gets patronage (FAC-7, FAC-8, MIL-11).

The same opinion runs between any two characters: a governor and a commander, a merchant
and an escort, the player and anyone.

### REL-3 Deeds move opinion; opinion fades `v1`

Opinion changes only because something happened. Examples:

| Deed | Whose opinion moves |
|---|---|
| Liege sends help that was asked for | Vassal's of liege, up |
| Asked-for help never comes | Vassal's of liege, down |
| Liege grants a planet or patronage | Vassal's of liege, up |
| Liege raises tribute | Vassal's of liege, down |
| Passed over when a conquered planet is handed out | Vassal's of liege, down |
| Vassal answers a call to arms | Liege's of vassal, up |
| Vassal refuses a call to arms | Liege's of vassal, down (REL-10) |
| Vassal holds a dangerous border | Liege's of vassal, up |
| A job done well under contract | Employer's of employee, up |
| Attacking someone's ships or merchants | Victim's (and their governor's) of attacker, down |
| *(proposed)* Vassal lends commanders its liege asked for (MIL-3) | Liege's of vassal, up |
| *(proposed)* Vassal refuses a loan its liege asked for | Liege's of vassal, down (REL-10) |
| *(proposed)* Vassal borrows its liege's commanders for relief | Liege's of vassal, down a little (favour spent) |
| *(proposed)* Liege grants tribute relief (FAC-15) | Vassal's of liege, up |
| *(proposed)* Liege refuses tribute relief to a vassal that qualifies | Vassal's of liege, down |
| *(proposed)* Commander declines every objective its employer posts (MIL-2) | Employer's of commander, down a little |
| *(proposed)* Root can't pay the bounties it owes (MIL-18) | Commander's of root, down |

**Opinion drifts slowly back toward a resting point,** neutral by default, so old deeds
fade and loyalty needs renewing. Grievances are bad deeds that haven't faded yet.

*See:* OPN-9 (deed sizes and fade rate are tuning), SYS-10.

### REL-4 What emerges, with no extra rules `v1`

- **Distant vassals** get help late or not at all, so they collect fewer good deeds and
  drift away. There is no separate fracture-distance rule.
- **Large factions** stretch help thinner and leave more calls unanswered, so they
  fracture without a size rule.
- **A vassal sends its fleet** because refusing lowers the liege's opinion, and that
  opinion decides aid and grants later.

## Standing and reputation

### REL-5 Standing is a relationship `v1`

- **Standing with a planet** is your relationship with its governor: their opinion of
  you.
- **Standing with a faction** is the opinion of its root governor. It's the one number
  the player tracks for that faction.

Landing permission (WLD-14), gate access (WLD-12), mission gates and hiring all read
these. The **pirate faction** is the exception, with one shared opinion (PIR-9). The
**Trade Authority** keeps its own opinion of each character (FAC-14).

### REL-6 Local opinion matters locally `experimental`

A mid-level governor's opinion of you matters at their own planet only: landing
permission, their missions and their prices. That way a lord who likes you reads as
"this planet treats me well", not as a second faction standing. Whether this confuses
players is for play to decide.

*See:* OPN-27.

### REL-7 Reputation is how widely known you are `v1` `proposed`

Reputation is separate from opinion: how well known and accomplished a character is,
not how any one character feels about them. It grows with missions completed and fleet
power, and it unlocks better missions (CHR-10), shortens landing waits (WLD-14) and
weighs in choosing governors (FAC-9). *(proposed, war and fealty round)* It also raises
a commander's stipend, on a curve that levels off (MIL-2).

*See:* OPN-11 (how it's computed).

## Contracts

### REL-8 Every job and allegiance is a contract `v1`

Fealty, a commander's service, a merchant's salary, an escort job, a courier or scout
run, a mercenary task and a pirate hire are all **contracts** between characters.

- **Either side can end any contract at any time.** The only cost is losing what the
  contract gave: pay, protection, information. There is **no reputation or relationship
  penalty** for ending one, for NPCs or the player.
- **Duration depends on the job.** Fealty and service contracts are standing until
  someone ends them. Escort, courier, scout and mercenary tasks are for one job: the
  contract starts with it and ends with it. Nobody is on retainer.
- **The player is in the same system.** Swearing fealty is signing a contract; leaving is
  ending one.
- *(proposed, war and fealty round)* **A loan** is a contract between two governors that
  moves a service contract's employer for a duration (MIL-3). It ends on its own terms
  (INF-16).

*Why:* players mustn't feel locked in. Losing the benefits is enough to hold most
contracts together.

*See:* REJ-15.

### REL-9 Escorts are paid when the trip ends `v1` `proposed`

So an escort who quits mid-trip forfeits the pay, which is losing the contract's
benefit, not a penalty.

### REL-10 Refusing a call is a deed `v1` `proposed`

Refusing a call to arms doesn't end a contract, so it can lower the liege's opinion
(REL-3). Drop this if it makes the player feel locked in.

*(proposed, war and fealty round)* The call to arms is now a loan request (MIL-3), and
refusing it is this deed. The same holds for a liege refusing tribute relief a vassal
qualifies for (FAC-15), and, as a smaller deed, for a commander declining every
objective its employer posts (MIL-2). None of these ends a contract.

*See:* OPN-16.

### REL-11 Sworn and mercenary are contract states `v1`

There's no caste line between a sworn commander and a mercenary. A **sworn** commander
holds a standing contract with a governor; a **mercenary** holds none and works specific
tasks for whoever pays (the Authority, governors, merchants). Mercenaries lead smaller
fleets and are less prestigious, which is reputation, not a type. A mercenary without
work can swear fealty, downsize or turn pirate (PIR-4).

### REL-12 What the player sees `proposed`

One thing to read per character: how they feel about you and the main reasons why, like
Crusader Kings' opinion breakdown. There's no favor balance to manage.
