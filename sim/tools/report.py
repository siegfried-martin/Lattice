#!/usr/bin/env python3
"""An HTML report for one or more experiment groups: the Phase A checks and charts of the
world series, every seed drawn faintly and the median bold (ARC-8).

    report.py runs/baseline runs/no-raids --out report.html

Charts are inline SVG, so the page needs nothing else. Only the standard library is used.
"""

import argparse
import html
import statistics
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import checks  # noqa: E402

SERIES = [
    ("world.population", "Population (thousands)"),
    ("world.money_total", "Money held"),
    ("world.price_index", "Price index"),
    ("world.factions", "Factions (2+ planets)"),
    ("world.wars_now", "Wars now"),
    ("world.captures", "Planets captured (cumulative)"),
    ("world.pirate_planets", "Pirate planets"),
    ("world.pirates", "Pirates"),
    ("world.commanders", "Sworn commanders"),
    ("world.chars_trading", "Characters trading"),
    ("world.max_risk", "Highest sector risk"),
    ("world.smuggled", "Contraband smuggled (cumulative)"),
]
COLOURS = ["#3366cc", "#dc3912", "#109618", "#ff9900", "#990099", "#0099c6"]


def svg_chart(title, groups, width=420, height=180):
    """groups: [(name, colour, [[(day, value)] per seed])]."""
    pad_l, pad_r, pad_t, pad_b = 54, 10, 22, 22
    xs = [d for _, _, runs in groups for run in runs for d, _ in run]
    ys = [v for _, _, runs in groups for run in runs for _, v in run]
    if not xs:
        return ""
    x0, x1 = min(xs), max(xs)
    y0, y1 = min(0.0, min(ys)), max(ys)
    if y1 <= y0:
        y1 = y0 + 1
    sx = lambda d: pad_l + (d - x0) / max(1e-9, x1 - x0) * (width - pad_l - pad_r)
    sy = lambda v: height - pad_b - (v - y0) / (y1 - y0) * (height - pad_t - pad_b)
    out = [f'<svg viewBox="0 0 {width} {height}" class="chart" role="img" aria-label="{html.escape(title)}">',
           f'<text x="{pad_l}" y="14" class="title">{html.escape(title)}</text>',
           f'<line x1="{pad_l}" y1="{height - pad_b}" x2="{width - pad_r}" y2="{height - pad_b}" class="axis"/>',
           f'<text x="{pad_l - 4}" y="{sy(y1) + 4:.0f}" class="tick" text-anchor="end">{checks.fmt(y1)}</text>',
           f'<text x="{pad_l - 4}" y="{sy(y0) + 4:.0f}" class="tick" text-anchor="end">{checks.fmt(y0)}</text>',
           f'<text x="{pad_l}" y="{height - 6}" class="tick">day {x0:.0f}</text>',
           f'<text x="{width - pad_r}" y="{height - 6}" class="tick" text-anchor="end">day {x1:.0f}</text>']
    for name, colour, runs in groups:
        for run in runs:
            pts = " ".join(f"{sx(d):.1f},{sy(v):.1f}" for d, v in run)
            out.append(f'<polyline points="{pts}" fill="none" stroke="{colour}" stroke-opacity="0.25" stroke-width="1"/>')
        days = sorted({d for run in runs for d, _ in run})
        median = []
        for d in days:
            vals = [v for run in runs for dd, v in run if dd == d]
            if vals:
                median.append((d, statistics.median(vals)))
        pts = " ".join(f"{sx(d):.1f},{sy(v):.1f}" for d, v in median)
        out.append(f'<polyline points="{pts}" fill="none" stroke="{colour}" stroke-width="2"/>')
    out.append("</svg>")
    return "\n".join(out)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("groups", nargs="+")
    p.add_argument("--out", default="report.html")
    a = p.parse_args()

    names = [Path(g).name for g in a.groups]
    results = {g: checks.evaluate(g) for g in a.groups}
    loaded = {g: [checks.load(r) for r in sorted(Path(g).glob("seed-*")) if (r / "series.csv").exists()] for g in a.groups}

    rows = []
    for i, c in enumerate(results[a.groups[0]][1]):
        cells = []
        for g in a.groups:
            r = results[g][1][i]
            ok = r["pass_share"] == 1
            cells.append(f'<td class="{"pass" if ok else "fail"}">{checks.fmt(r["median"])}'
                         f'<span class="share">{r["pass_share"]:.0%}</span></td>')
        rows.append(f'<tr><th scope="row">{c["id"]}</th><td>{html.escape(c["what"])}</td>'
                    f'<td class="band">{html.escape(c["band"])}</td>{"".join(cells)}</tr>')
    legend = "".join(f'<span class="key" style="--c:{COLOURS[i % len(COLOURS)]}">{html.escape(n)}</span>' for i, n in enumerate(names))
    charts = []
    for series, title in SERIES:
        groups = [(names[i], COLOURS[i % len(COLOURS)], [checks.world(run, series) for run in loaded[g] if series in run["series"]])
                  for i, g in enumerate(a.groups)]
        charts.append(svg_chart(title, groups))

    page = f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Lattice Phase A</title>
<style>
:root {{ --bg: #fbfbfa; --fg: #1d1d1f; --muted: #6b6b70; --line: #d8d8dc; --pass: #e6f4ea; --fail: #fde8e7; }}
@media (prefers-color-scheme: dark) {{
  :root:not([data-theme="light"]) {{ --bg: #17181b; --fg: #ececf0; --muted: #9a9aa2; --line: #34353a; --pass: #183a24; --fail: #45201e; }}
}}
:root[data-theme="dark"] {{ --bg: #17181b; --fg: #ececf0; --muted: #9a9aa2; --line: #34353a; --pass: #183a24; --fail: #45201e; }}
body {{ background: var(--bg); color: var(--fg); font: 15px/1.45 system-ui, sans-serif; margin: 0; padding: 24px 16px; }}
main {{ max-width: 1100px; margin: 0 auto; }}
h1 {{ font-size: 22px; margin: 0 0 4px; }} h2 {{ font-size: 17px; margin: 28px 0 8px; }}
p {{ color: var(--muted); margin: 0 0 12px; }}
.wrap {{ overflow-x: auto; }}
table {{ border-collapse: collapse; width: 100%; font-size: 13px; }}
th, td {{ border-bottom: 1px solid var(--line); padding: 5px 8px; text-align: left; vertical-align: top; }}
td.pass {{ background: var(--pass); }} td.fail {{ background: var(--fail); }}
td.band {{ color: var(--muted); white-space: nowrap; }}
.share {{ color: var(--muted); font-size: 11px; margin-left: 6px; }}
.charts {{ display: grid; grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); gap: 12px; }}
.chart {{ width: 100%; height: auto; }}
.chart .title {{ font-size: 12px; fill: var(--fg); }} .chart .tick {{ font-size: 10px; fill: var(--muted); }}
.chart .axis {{ stroke: var(--line); }}
.key {{ display: inline-flex; align-items: center; margin-right: 14px; font-size: 13px; }}
.key::before {{ content: ""; width: 14px; height: 3px; background: var(--c); margin-right: 6px; }}
</style></head>
<body><main>
<h1>Lattice Phase A</h1>
<p>{len(a.groups)} group(s); each cell is the median over seeds, with the share of seeds inside the band.</p>
<div class="wrap"><table><thead><tr><th>Check</th><th>What</th><th>Band</th>{"".join(f"<th>{html.escape(n)}</th>" for n in names)}</tr></thead>
<tbody>{"".join(rows)}</tbody></table></div>
<h2>World series</h2><p>{legend}</p>
<div class="charts">{"".join(charts)}</div>
</main></body></html>"""
    Path(a.out).write_text(page)
    print(f"wrote {a.out}")


if __name__ == "__main__":
    main()
