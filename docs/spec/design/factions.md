# Factions and fealty (FAC)

Who holds planets, how factions form and hold together, the deal between a liege and a
vassal, what happens to a conquered planet, and the Trade Authority.

## Governors and factions

### FAC-1 Every planet is a state with its own governor `v1`

Every planet is its own state, run by a **governor**. It has an economy (ECO-2), its own
defences (MIL-5), and the commanders who hold contracts with its governor. **Every planet
always has its own governor,** and no character governs two planets.

### FAC-2 A faction is a fealty tree `v1`

A governor gains power by getting other governors to swear fealty to them.

- Every governor has at most one **liege**. The structure is a forest of fealty trees.
- **A faction is a root governor and everything under them.** It isn't stored
  separately. When a vassal breaks fealty, their own vassals stay sworn to them, so a new
  faction exists: the rebel and their branch.
- Standing with a faction is standing with its root (REL-5).
- The game starts with a few **seats of power**: powerful planets far apart, each the
  root of a large tree. That is an initial condition, not an invariant; factions split,
  merge and form.

*See:* GEN-8, OPN-5 (how many seats).

### FAC-3 What governors want `v1`

Governors want military power: to defend themselves, to gain standing with their liege,
and to launch invasions. So they hire the most powerful commanders they can afford,
through the budget model (ECO-8).

### FAC-4 Wealth is the root of military power `v1`

Force comes from commanders, and commanders go to whoever pays. A typical planet can't
support enough commanders to conquer a defended neighbour. A very rich or lucky one can,
which is one way a new faction starts. Starving a planet's trade starves its ability to
keep commanders: economic warfare is real warfare. A rich rival can lure commanders
away.

## The fealty deal

### FAC-5 Services down, obligations up `v1`

**The liege provides:**
- **protection:** reinforcements along the Lattice when the vassal is attacked (MIL-11);
- **patronage:** investment when the vassal needs it (FAC-7);
- **intelligence:** the faction's shared information (INF-4);
- **trade:** the faction's merchants, who trade at the vassal's markets (TRD-3).

**The vassal owes:**
- **tribute:** a share of its income passed up to its liege (FAC-6);
- **military service:** its commanders join when the liege raises an army, with the liege
  paying their stipends while they serve (MIL-3);
- **the liege's wars:** a vassal is at war when its faction is, which cuts its trade with
  the enemy and exposes its merchants. Border planets pay most of this cost.

*Why:* without a deal there's nothing to be loyal or disloyal about.

### FAC-6 Tribute `v1`

A vassal pays its liege a share of its income on a regular schedule. **The liege sets
each vassal's tribute rate,** a budget lever: high tribute funds wars and raises the
chance of defection (raising it is a bad deed, REL-3); low tribute keeps vassals loyal
and leaves the liege poorer. Tribute is how the root gets the money to pay faction
merchants (TRD-3) and fund wars.

- **Cascade** *(proposed)*: each governor pays tribute on what it receives from its own
  vassals as well as its own income, so the root's share of a planet shrinks with depth.
  Without a cascade, granting a planet gives away all its income and lieges would rarely
  grant.

*See:* OPN-7, ECO-9.

### FAC-7 Patronage `v1`

Patronage is a liege's **upfront investment in a vassal planet**, repaid later by higher
tribute. Its main use is rebuilding a newly conquered planet whose economy is wrecked.

- It's risky: the planet can be retaken before it pays off, or its governor can defect
  with the rebuilt planet.
- It's also a service, so it raises the vassal's opinion of the liege (REL-3), which
  partly offsets the defection risk.
- A liege can invest by paying money, or by setting a low tribute rate for a while (a
  grace period).
- The liege's opinion of each vassal (favor) decides who gets patronage.

## Conquest

### FAC-8 Keep or grant `v1`

When a governor's army takes a planet, that governor either:

- **keeps it:** installs a new governor directly under themselves, who pays tribute to
  them; or
- **grants it** to one of their vassals, who faces the same choice.

The choice moves down the tree until someone keeps it. A governor with no vassals has to
keep it.

- **Keeping** brings tribute straight to you, and the planet's loyalty is to you, but
  it's one more direct vassal to protect.
- **Granting** rewards a vassal and strengthens them, but a stronger vassal could defect
  with a bigger branch, and you give up some or all of the income (FAC-6).
- **Geography decides a lot on its own.** Protection is worth less from far away, so a
  distant conquest next to one of your vassals is more loyal under that vassal. Trees
  grow along geographic lines without a rule for it.
- A vassal next to the planet who sees it kept or given elsewhere may feel passed over
  (REL-3).

### FAC-9 Choosing the new governor `v1`

Mount & Blade style: **the governor who keeps the planet chooses its new governor** from
existing characters with good standing and reputation, with **extra weight for the
commander who led the capture**. So a commander's chance of becoming governor depends on
how far down the tree the planet travels. There are no stewards and no pool of standing
politicians.

*(open)* Whether "governing skill" weighs in. Skills come from missions, and a candidate
has never governed; it could be generated with the character, or reputation and
relationship could carry the choice alone (leaning that way).

*See:* REJ-8, REJ-9, OPN-10.

### FAC-10 Capture costs prosperity `v1` `proposed`

Taking a planet **drops its prosperity**, so every conquest starts with a rebuilding
decision. A long siege does more damage on top. Prosperity recovers slowly on its own
and faster with patronage, and tribute follows prosperity, so the payback is visible.

*See:* OPN-8, SYS-13.

## Holding together

### FAC-11 Defection `v1`

A governor breaks fealty when **their opinion of their liege has been low for a
sustained period and a better alternative exists**: independence (when nothing nearby
threatens them) or a rival liege. The vassals sworn to the defector stay with them, and
each can defect in turn by the same rule.

- Shared culture keeps opinion up (WLD-19). *(proposed)* The share of the faction's
  planets that are the vassal's culture raises the resting point of the vassal's opinion
  of its liege.
- Leaving costs the protection, patronage and place in the tree that the deal gave. Its
  old liege owes it nothing now; whether the liege then attacks is ordinary war
  behaviour, not a punishment rule.

*Why:* the damping (low for a while, plus a real alternative) keeps defection an
individual event, not a cascade.

*See:* CHK-1.

### FAC-12 The liege's tools `v1`

To keep a wavering vassal, the liege can send help, grant patronage or lower tribute.
Risk tolerance and the budget model decide how a ruler handles it.

## The Trade Authority

### FAC-13 A limited faction with a fixed ruleset `v1`

The **Trade Authority** runs the Lattice. It isn't a fealty tree, holds no planets and
doesn't play politics. It's never at war with any faction (DIP-7). It cares about dues and
pirates.

### FAC-14 What the Authority does `v1`

- It collects **two-tier dues** from every lawful planet (ECO-10).
- It spends them on **maintaining the network** (a sink, ECO-5), and uses the surplus to
  **hire mercenaries** to hunt specific pirates and keep stretches of the network clear.
  Those contracts are a main source of player missions and lower regional piracy
  (TRD-16).
- It holds the network key of every gate (WLD-12) and keeps its own opinion of each
  character, which is that character's Authority standing.
- Its income stays close to steady and drops only when planets go pirate.

*Why:* the one fixed point in a moving world.

*See:* CHK-3.
