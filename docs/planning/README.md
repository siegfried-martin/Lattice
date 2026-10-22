# Planning

Ideation drafts for the next design round. **Nothing here is authoritative:** the game's
rules are in `docs/spec/`. Between rounds this folder holds only this README.

## For the planning agent

1. **Read `docs/spec/README.md` first**, then `docs/spec/OPEN.md`. The open questions
   tagged *design* are the natural starting points for a round, and every rule you'd
   change already has an ID.
2. **Write drafts here,** one file per topic, as rough as the ideas are.
3. **Name the rules a draft changes or adds to:** "replaces FAC-8", "answers OPN-1",
   "new rule for `trade.md`". That's what lets the review be quick.
4. **Mark what's settled and what isn't** with the spec's tags (`proposed`, `open`,
   `later`, `experimental`), so the review knows what's being asked.
5. Don't edit `docs/spec/` directly.

## The round

1. The planning agent writes drafts here.
2. The human reviews them with the implementation agent. Within a round, a newer draft
   wins over an older one.
3. The implementation agent writes the settled decisions into `docs/spec/`, logs them in
   `docs/spec/DECISIONS.md`, and deletes the drafts. Git keeps them.

The first drafts (`FACTIONS.md`, `ECONOMY.md`, `TRADE.md`, `CHARACTERS.md`) were
formalised on 2026-10-18 and are preserved at the git tag `planning-drafts-1`. The roles draft (`ROLES.md`) and its review were formalised on
2026-10-22; they're at commit 3145d4b.
