#!/usr/bin/env python3
"""Runs a group of worlds in parallel, one per core (ARC-5, MS-8).

    experiment.py --name baseline --config sim/configs/v1.toml --seeds 1-8 --days 1825
    experiment.py --name no-raids --config sim/configs/v1.toml --set raid.enabled=false --seeds 1-8

Each run's output lands in OUT/NAME/seed-N (config.toml, run.toml, the map, series.csv,
events.csv and run.log). OUT/NAME/config.toml is the group's flattened config. Only the
standard library is used.
"""

import argparse
import concurrent.futures
import os
import subprocess
import sys
import tomllib
from pathlib import Path

HERE = Path(__file__).resolve().parent
RUNNER = HERE.parent / "build" / "release" / "lattice-run"


def flatten(table, prefix=""):
    out = {}
    for key, value in table.items():
        path = f"{prefix}{key}"
        if isinstance(value, dict):
            out.update(flatten(value, path + "."))
        else:
            out[path] = value
    return out


def toml_value(value):
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, (int, float)):
        return repr(value)
    if isinstance(value, list):
        return "[" + ", ".join(toml_value(v) for v in value) + "]"
    return '"' + str(value).replace('"', '\\"') + '"'


def parse_override(text):
    key, _, raw = text.partition("=")
    if not key or not raw:
        raise SystemExit(f"--set needs key=value, got {text!r}")
    return key.strip(), tomllib.loads(f"v = {raw.strip()}")["v"]


def parse_seeds(text):
    seeds = []
    for part in text.split(","):
        if "-" in part:
            lo, hi = part.split("-")
            seeds.extend(range(int(lo), int(hi) + 1))
        else:
            seeds.append(int(part))
    return seeds


def run_one(args):
    config, seed, days, every, out = args
    out.mkdir(parents=True, exist_ok=True)
    cmd = [str(RUNNER), "--config", str(config), "--seed", str(seed), "--days", str(days), "--every", str(every),
           "--out", str(out)]
    result = subprocess.run(cmd, capture_output=True, text=True)
    (out / "run.log").write_text(result.stdout + result.stderr)
    return seed, result.returncode, result.stdout.strip() or result.stderr.strip()


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--name", required=True)
    p.add_argument("--config", help="base config (TOML)")
    p.add_argument("--set", action="append", default=[], metavar="KEY=VALUE", help="override a key (repeatable)")
    p.add_argument("--seeds", default="1-8")
    p.add_argument("--days", type=int, default=1825, help="measured days after the warm-up")
    p.add_argument("--every", type=int, default=10, help="keep series every N days")
    p.add_argument("--jobs", type=int, default=os.cpu_count())
    p.add_argument("--out", default="runs")
    a = p.parse_args()

    if not RUNNER.exists():
        raise SystemExit(f"{RUNNER} not found: run `make sim-release` first")
    flat = {}
    if a.config:
        with open(a.config, "rb") as f:
            flat = flatten(tomllib.load(f))
    for text in a.set:
        key, value = parse_override(text)
        flat[key] = value

    group = Path(a.out) / a.name
    group.mkdir(parents=True, exist_ok=True)
    config = group / "config.toml"
    config.write_text("".join(f"{k} = {toml_value(v)}\n" for k, v in sorted(flat.items())))
    (group / "experiment.toml").write_text(
        f'name = "{a.name}"\nbase = "{a.config or ""}"\nseeds = "{a.seeds}"\ndays = {a.days}\nevery = {a.every}\n'
        + "overrides = [" + ", ".join(toml_value(s) for s in a.set) + "]\n")

    jobs = [(config.resolve(), seed, a.days, a.every, (group / f"seed-{seed}").resolve()) for seed in parse_seeds(a.seeds)]
    failed = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:
        for seed, code, line in pool.map(run_one, jobs):
            print(f"seed {seed}: {line}" if code == 0 else f"seed {seed}: FAILED ({code}) {line}")
            failed += code != 0
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
