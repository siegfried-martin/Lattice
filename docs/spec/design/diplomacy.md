# Diplomacy (DIP)

How factions go to war and make peace. Modelled on Mount & Blade.

### DIP-1 Three states between factions `v1`

Every pair of **neighbouring factions** is in one of three states: **war**, **peace** or
**ceasefire**. The state is held between the factions' leadership (their roots, FAC-2)
and applies to every member. Factions that don't border each other are at peace unless
something puts them in contact. *(proposed)* "Neighbouring" means some planet of one is
within a set Lattice or sector distance of a planet of the other.

### DIP-2 Ceasefire `v1`

A ceasefire follows a war. **The loser of the last war pays tribute to the winner** for a
period (about three months to start), and **no war can be declared between them** while
it lasts. When it ends the factions are at peace.

- *(proposed)* The tribute goes to the winner's root, and its size follows how badly the
  war was going for the loser.

*See:* OPN-2.

### DIP-3 Suing for peace `v1`

A faction that sees it is doing poorly **sues for peace and offers tribute**. Being in
several wars at once is a common reason, but it isn't hardcoded: several wars just make
each one go worse.

- *(proposed)* "Doing poorly" is judged from the root's own beliefs: planets and fleets
  lost, the treasury draining, and **weariness**, which grows with every month at war.
  The number of wars isn't a term of its own.
- *(proposed)* The winner accepts when the tribute offered is worth more than its
  believed value of continuing: what it has been taking, or what it hoped to take when
  the war began, whichever is more. That hope fades as the war drags on without
  results.
- *(proposed)* The winner hears no offer until its armies could have reached the front
  (`dip.min_war_days`), so a war is fought before it's settled.

### DIP-4 Declaring war `v1` `proposed`

Either faction can declare war during peace. What triggers it is not settled. The
working proposal is **a vote among the faction's governors, weighted by height in the
tree** (a governor's weight grows with the size of the branch under them). Each
governor's vote comes from what they know and feel:

- **for war:** idle commanders wanting campaigns (that's how commanders win planets), rich
  enemy planets near their own border, believed strength over the target, prosperity
  built up during peace, and *(proposed)* money piled up beyond a reserve, which a war
  spends;
- **against:** wars already being fought, recent losses;
- scaled by the governor's risk tolerance (CHR-4).

Border governors have the most at stake both ways: they gain most from a war and are hit
first.

*(proposed)* **Pacing:** with several neighbours each, a faction is at war a little more
than half the time, and a war lasts one to three game months (one to three real hours,
WLD-15). CHK-8 holds the simulation to it.

*See:* OPN-1.

### DIP-5 Declarations travel physically `v1` `proposed`

News of a war or a ceasefire reaches each planet by courier (INF-8), like any other fact.
A remote planet can be attacked before it hears war was declared, and a fleet can fight a
battle after a ceasefire it hasn't heard of.

### DIP-6 What war changes `v1`

Between factions at war:

- **trade stops:** faction merchants don't trade with the enemy's planets, merchants in
  transit there turn for home, and osmosis stops (TRD-6, TRD-13);
- **gates are closed** to members of the enemy faction (WLD-12);
- **ships are hostile:** scouts, merchants and fleets of the enemy faction are fair
  targets, and attacking them isn't piracy (PIR-11);
- **invasions are possible** (MIL-9).

### DIP-7 Outside diplomacy `v1`

The **pirate faction** is permanently hostile to everyone outside it (PIR-1). The **Trade
Authority** is never at war with anyone (FAC-13). Neither takes part in war, peace or
ceasefire.
