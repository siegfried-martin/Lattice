#!/usr/bin/env python3
"""Phase A checks (docs/spec/sim/checks.md) over a group of runs, or two groups compared.

    checks.py runs/baseline                   the checks, each seed and their median
    checks.py runs/baseline runs/no-raids     every check side by side (Phase B, EXP)
    checks.py runs/baseline --json out.json   also write the numbers for a report

Each check reduces a run's series to one number and holds it to a band. The bands are
starting proposals (CHK: "the pass bands are themselves tunable judgments") and live in
CHECKS below. Only the standard library is used.
"""

import argparse
import csv
import json
import math
import re
import statistics
import sys
from collections import defaultdict
from pathlib import Path


def load(run_dir):
    """series[name][id] = [(day, value)], plus the run's planets and timing."""
    series = defaultdict(lambda: defaultdict(list))
    with open(run_dir / "series.csv", newline="") as f:
        for row in csv.DictReader(f):
            if row["value"] is None:
                continue  # a run still being written
            series[row["series"]][int(row["id"])].append((float(row["day"]), float(row["value"])))
    planets = list(csv.DictReader(open(run_dir / "planets.csv", newline="")))
    log = (run_dir / "run.log").read_text() if (run_dir / "run.log").exists() else ""
    m = re.search(r"([\d.]+) days/s", log)
    return {"series": series, "planets": planets, "days_per_s": float(m.group(1)) if m else float("nan"),
            "wars": war_lengths(run_dir / "events.csv")}


def war_lengths(path):
    """Days from each declaration to its ceasefire, for the wars that ended (CHK-8c)."""
    if not path.exists():
        return []
    started, lengths = {}, []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            kind = row["kind"] or ""
            if not kind.startswith("war.") or row["detail"] is None:
                continue
            pair = tuple(sorted((int(row["id"]), int(row["detail"].split()[1]))))
            if kind == "war.declared":
                started.setdefault(pair, float(row["day"]))
            elif kind == "war.ceasefire" and pair in started:
                lengths.append(float(row["day"]) - started.pop(pair))
    return lengths


def world(run, name):
    return run["series"][name][-1]


def values(run, name):
    return [v for _, v in world(run, name)]


def years(run):
    days = [d for d, _ in world(run, "world.population")]
    return max(1e-9, (days[-1] - days[0]) / 365)


def per_year(run, name):
    v = values(run, name)
    return (v[-1] - v[0]) / years(run)


def growth(run, name):
    v = values(run, name)
    return (v[-1] / v[0]) ** (1 / years(run)) if v[0] > 0 and v[-1] > 0 else 0.0


def cv(xs):
    m = statistics.fmean(xs)
    return statistics.pstdev(xs) / m if m else 0.0


def last_half(xs):
    return xs[len(xs) // 2:]


def labour_churn(run):
    """Mean absolute change in labour share per sample over the second half (CHK-13)."""
    changes = []
    for name, by_id in run["series"].items():
        if not name.startswith("planet.labour."):
            continue
        for points in by_id.values():
            vs = last_half([v for _, v in points])
            changes += [abs(b - a) for a, b in zip(vs, vs[1:])]
    return statistics.fmean(changes) if changes else 0.0


def fringe_ratio(run):
    """Fringe planets' population growth against on-network planets' (CHK-11)."""
    fringe = {int(p["id"]) for p in run["planets"] if p["fringe"] == "1"}
    by = run["series"]["planet.population"]
    def total(ids, index):
        return sum(by[i][index][1] for i in ids if i in by)
    on = set(by) - fringe
    if not fringe or not on:
        return 1.0
    g_f = total(fringe, -1) / max(1e-9, total(fringe, 0))
    g_o = total(on, -1) / max(1e-9, total(on, 0))
    return g_f / g_o if g_o > 0 else 0.0


def war_share(run):
    """Each faction's share of time at war: lawful roots at war over lawful roots."""
    at_war, roots = values(run, "world.roots_at_war"), values(run, "world.roots")
    return statistics.fmean(a / r for a, r in zip(at_war, roots) if r > 0)


def war_length(run):
    return statistics.median(run["wars"]) if run["wars"] else 0.0


def deaths_per_battle(run):
    b = per_year(run, "world.battles")
    return per_year(run, "world.deaths") / b if b > 0 else 0.0


def captures_share(run):
    return per_year(run, "world.captures") / max(1, len(run["planets"]))


def pirate_share(run):
    return statistics.fmean(values(run, "world.pirates")) / max(1, statistics.fmean(values(run, "world.characters")))


def idle_share(run):
    idle, total = values(run, "world.chars_idle"), values(run, "world.characters")
    return statistics.fmean(i / t for i, t in zip(idle, total) if t > 0)


def price_cv(run):
    return statistics.fmean(v for by in [run["series"]["good.price_cv"]] for pts in by.values() for _, v in last_half(pts))


def profit_per_trip(run):
    trips = per_year(run, "world.trips")
    return per_year(run, "world.trade_profit") / trips if trips > 0 else 0.0


# (id, what, value function, low, high). A band edge of None is open.
CHECKS = [
    ("CHK-1", "factions with 2+ planets, fewest seen", lambda r: min(values(r, "world.factions")), 2, None),
    ("CHK-3", "Authority dues, monthly variation (cv)", lambda r: cv([b - a for a, b in zip(values(r, "world.dues"), values(r, "world.dues")[3:])][::3] or [1]), None, 0.5),
    ("CHK-4", "pirate planets on the fringe, share of pirate-planet time", lambda r: (sum(values(r, "world.fringe_pirate_planets")) / max(1e-9, sum(values(r, "world.pirate_planets")))) if sum(values(r, "world.pirate_planets")) else 1.0, 0.5, None),
    ("CHK-6", "planets changing hands a year, share of planets", captures_share, 0.01, 0.2),
    ("CHK-7", "money supply growth a year", lambda r: growth(r, "world.money_total"), 0.8, 1.25),
    ("CHK-7b", "price index, end over start", lambda r: values(r, "world.price_index")[-1] / values(r, "world.price_index")[0], 0.67, 1.5),
    ("CHK-8", "share of time each faction is at war", war_share, 0.5, 0.8),
    ("CHK-8c", "war length in days, median", war_length, 30, 90),
    ("CHK-8b", "war declarations a year per faction", lambda r: per_year(r, "world.wars") / max(1, statistics.fmean(values(r, "world.roots"))), 0.05, 1),
    ("CHK-9", "pirate flips a year, share of planets", lambda r: per_year(r, "world.flips") / max(1, len(r["planets"])), None, 0.05),
    ("CHK-10", "contraband reaching lawful populations a year", lambda r: per_year(r, "world.smuggled"), 1, None),
    ("CHK-11", "fringe population growth over on-network growth", fringe_ratio, 0.5, None),
    ("CHK-12", "world population, end over start", lambda r: values(r, "world.population")[-1] / values(r, "world.population")[0], 0.7, 1.5),
    ("CHK-12b", "world population, variation over the second half (cv)", lambda r: cv(last_half(values(r, "world.population"))), None, 0.1),
    ("CHK-13", "labour share change per sample, second half", labour_churn, None, 0.02),
    ("CHK-14", "price dispersion across planets (cv), second half", price_cv, 0.05, 0.6),
    ("CHK-14b", "merchant profit per trip", profit_per_trip, 0, None),
    ("CHK-17", "characters over target", lambda r: values(r, "world.characters")[-1] / max(1, values(r, "world.characters")[0]), 0.95, 1.05),
    ("CHK-17b", "share of characters idle", idle_share, None, 0.2),
    ("CHK-20", "battles a year", lambda r: per_year(r, "world.battles"), 12, None),
    ("CHK-20b", "deaths per battle", deaths_per_battle, None, 0.05),
    ("CHK-23", "pirates, share of characters", pirate_share, 0.01, 0.1),
    ("CHK-24", "smuggling runs a year", lambda r: per_year(r, "world.smuggled_runs"), 1, None),
    ("CHK-25", "game days a second", lambda r: r["days_per_s"], 6, None),
]


def passes(value, low, high):
    if value is None or (isinstance(value, float) and math.isnan(value)):
        return False
    return (low is None or value >= low) and (high is None or value <= high)


def band(low, high):
    if low is None:
        return f"<= {high:g}"
    if high is None:
        return f">= {low:g}"
    return f"{low:g} to {high:g}"


def evaluate(group):
    runs = sorted(p for p in Path(group).glob("seed-*") if (p / "series.csv").exists())
    if not runs:
        raise SystemExit(f"no runs in {group}")
    loaded = [load(r) for r in runs]
    out = []
    for cid, what, fn, low, high in CHECKS:
        vals = []
        for run in loaded:
            try:
                vals.append(fn(run))
            except (KeyError, IndexError, ZeroDivisionError, statistics.StatisticsError):
                vals.append(float("nan"))
        finite = [v for v in vals if not math.isnan(v)]
        median = statistics.median(finite) if finite else float("nan")
        share = sum(passes(v, low, high) for v in vals) / len(vals)
        out.append({"id": cid, "what": what, "band": band(low, high), "low": low, "high": high, "values": vals,
                    "median": median, "pass_share": share})
    return [r.name for r in runs], out


def fmt(v):
    if isinstance(v, float) and math.isnan(v):
        return "n/a"
    return f"{v:.3g}"


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("groups", nargs="+")
    p.add_argument("--json", help="write the numbers here")
    a = p.parse_args()
    results = {g: evaluate(g) for g in a.groups}
    if len(a.groups) == 1:
        seeds, checks = results[a.groups[0]]
        print(f"{'check':8} {'what':58} {'band':>14} {'median':>9} {'pass':>5}")
        for c in checks:
            print(f"{c['id']:8} {c['what']:58} {c['band']:>14} {fmt(c['median']):>9} {c['pass_share']:>5.0%}")
        passed = sum(c["pass_share"] == 1 for c in checks)
        print(f"\n{passed} of {len(checks)} checks pass on every seed ({len(seeds)} seeds)")
    else:
        names = [Path(g).name for g in a.groups]
        print(f"{'check':8} {'what':58} " + " ".join(f"{n[:12]:>12}" for n in names))
        for i, c in enumerate(results[a.groups[0]][1]):
            row = [results[g][1][i]["median"] for g in a.groups]
            print(f"{c['id']:8} {c['what']:58} " + " ".join(f"{fmt(v):>12}" for v in row))
    if a.json:
        Path(a.json).write_text(json.dumps({g: {"seeds": s, "checks": c} for g, (s, c) in results.items()}, indent=1))


if __name__ == "__main__":
    sys.exit(main())
