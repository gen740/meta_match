#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUT_DIR = ROOT / "artifacts" / "bench_report"
APPROACH_ORDER = ["meta_match", "if_match", "glaze"]
APPROACH_LABELS = {
    "meta_match": "meta_match",
    "if_match": "if_match",
    "glaze": "glaze",
}
APPROACH_COLORS = {
    "meta_match": "#0f766e",
    "if_match": "#2563eb",
    "glaze": "#7c3aed",
}
MODE_LABELS = {
    0: "hits_only_randomized",
    1: "hit_miss_randomized",
    2: "miss_only",
}
COMPILE_GROUPS = [
    (
        "syntax",
        "Compile-time: -fsyntax-only",
        "-std=c++23 -O3 -march=native -I. {nix_flags} -fsyntax-only",
        {
            "meta_match": "_ct_meta.cc",
            "if_match": "_ct_if.cc",
            "glaze": "_ct_glaze.cc",
        },
    ),
    (
        "object_o1",
        "Compile-time: -c -O1",
        "-std=c++23 -O1 -march=native -I. {nix_flags} -c",
        {
            "meta_match": "_ct_meta.cc -o /tmp/_mm_meta.o",
            "if_match": "_ct_if.cc -o /tmp/_mm_if.o",
            "glaze": "_ct_glaze.cc -o /tmp/_mm_glaze.o",
        },
    ),
    (
        "object_o3",
        "Compile-time: -c -O3",
        "-std=c++23 -O3 -march=native -I. {nix_flags} -c",
        {
            "meta_match": "_ct_meta.cc -o /tmp/_mm_meta.o",
            "if_match": "_ct_if.cc -o /tmp/_mm_if.o",
            "glaze": "_ct_glaze.cc -o /tmp/_mm_glaze.o",
        },
    ),
]


@dataclass
class CompileResult:
    approach: str
    mean_s: float
    stddev_s: float


@dataclass
class RuntimeResult:
    dataset: str
    approach: str
    mode: int
    cpu_ns: float


def run(
    cmd: list[str],
    *,
    cwd: Path = ROOT,
    env: dict[str, str] | None = None,
    capture_output: bool = True,
    announce: str | None = None,
) -> subprocess.CompletedProcess[str]:
    if announce is None:
        print("+", shlex.join(cmd))
    else:
        print("+", announce)
    return subprocess.run(
        cmd,
        cwd=cwd,
        env=env,
        check=True,
        text=True,
        capture_output=capture_output,
    )


def require_env(name: str) -> str:
    value = os.environ.get(name, "").strip()
    if not value:
        raise SystemExit(f"ERROR: {name} is empty")
    return value


def write_json(path: Path, data: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def build_binaries() -> None:
    run(["make", "check-deps"], capture_output=False, announce="make check-deps")
    run(["make", "benchmark"], capture_output=False, announce="make benchmark")


def run_compile_hyperfine(out_dir: Path, nix_flags: str) -> dict[str, list[CompileResult]]:
    all_results: dict[str, list[CompileResult]] = {}
    compiler = os.environ.get("CXX", "clang++")

    for group_key, title, base_flags, sources in COMPILE_GROUPS:
        export_path = out_dir / f"compile_{group_key}.hyperfine.json"
        commands = []
        for approach in APPROACH_ORDER:
            command = f"{compiler} {base_flags.format(nix_flags=nix_flags)} {sources[approach]}"
            commands.append(command)

        cmd = [
            "hyperfine",
            "--warmup",
            "1",
            "--runs",
            "10",
            "--style",
            "basic",
            "--time-unit",
            "second",
            "--export-json",
            str(export_path),
            *commands,
        ]
        run(cmd, announce=f"hyperfine {group_key}")

        payload = json.loads(export_path.read_text(encoding="utf-8"))
        results: list[CompileResult] = []
        for approach, item in zip(APPROACH_ORDER, payload["results"], strict=True):
            results.append(
                CompileResult(
                    approach=approach,
                    mean_s=float(item["mean"]),
                    stddev_s=float(item["stddev"]),
                )
            )
        all_results[group_key] = results

        print(f"\n{title}")
        for result in results:
            print(
                f"  {APPROACH_LABELS[result.approach]:<12}"
                f" {result.mean_s:.3f} s ± {result.stddev_s:.3f} s"
            )

    return all_results


def run_runtime_benchmark(out_dir: Path) -> list[RuntimeResult]:
    export_path = out_dir / "runtime_benchmark.json"
    completed = run(
        [
            "./benchmark",
            "--benchmark_format=json",
            f"--benchmark_out={export_path}",
            "--benchmark_out_format=json",
        ],
        announce="./benchmark --benchmark_format=json",
    )
    if completed.stderr:
        print(completed.stderr.strip())

    payload = json.loads(export_path.read_text(encoding="utf-8"))
    results: list[RuntimeResult] = []
    pattern = re.compile(r"^bench_(.+)_(meta_match|if_match|glaze)/([0-2])$")

    for bench in payload["benchmarks"]:
        name = bench["name"]
        match = pattern.match(name)
        if not match:
            continue
        dataset, approach, mode_raw = match.groups()
        results.append(
            RuntimeResult(
                dataset=dataset,
                approach=approach,
                mode=int(mode_raw),
                cpu_ns=float(bench["cpu_time"]),
            )
        )

    print("\nRuntime benchmark entries:", len(results))
    return results


def escape_xml(text: str) -> str:
    return (
        text.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )


def format_seconds(value: float) -> str:
    return f"{value:.3f} s"


def format_ns(value: float) -> str:
    return f"{value:.2f} ns"


def svg_root(width: int, height: int, body: Iterable[str]) -> str:
    joined = "\n".join(body)
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" font-family="Helvetica, Arial, sans-serif">\n'
        f"{joined}\n"
        "</svg>\n"
    )


def render_compile_svg(results: dict[str, list[CompileResult]], out_path: Path) -> None:
    width = 980
    panel_height = 240
    margin_left = 180
    margin_right = 40
    chart_width = width - margin_left - margin_right
    body: list[str] = [
        '<rect width="100%" height="100%" fill="#fffdf8"/>',
        '<text x="24" y="34" font-size="22" font-weight="700" fill="#111827">Compile-time</text>',
    ]

    panels = list(COMPILE_GROUPS)
    for panel_index, (group_key, title, _, _) in enumerate(panels):
        top = 60 + panel_index * panel_height
        chart_top = top + 30
        row_gap = 36
        bar_height = 18
        panel_results = results[group_key]
        max_value = max(r.mean_s + r.stddev_s for r in panel_results)

        body.append(
            f'<text x="24" y="{top}" font-size="16" font-weight="700" fill="#1f2937">{escape_xml(title)}</text>'
        )

        for tick in range(5):
            ratio = tick / 4 if max_value else 0.0
            value = max_value * ratio
            x = margin_left + chart_width * ratio
            body.append(
                f'<line x1="{x:.1f}" y1="{chart_top - 10}" x2="{x:.1f}" y2="{chart_top + row_gap * len(panel_results)}" '
                'stroke="#e5e7eb" stroke-width="1"/>'
            )
            body.append(
                f'<text x="{x:.1f}" y="{chart_top - 16}" font-size="11" text-anchor="middle" fill="#6b7280">{format_seconds(value)}</text>'
            )

        for row, result in enumerate(panel_results):
            y = chart_top + row * row_gap
            bar_width = 0.0 if max_value == 0 else chart_width * (result.mean_s / max_value)
            err = 0.0 if max_value == 0 else chart_width * (result.stddev_s / max_value)
            body.append(
                f'<text x="{margin_left - 12}" y="{y + 13}" font-size="13" text-anchor="end" fill="#111827">'
                f"{escape_xml(APPROACH_LABELS[result.approach])}</text>"
            )
            body.append(
                f'<rect x="{margin_left}" y="{y}" width="{bar_width:.1f}" height="{bar_height}" '
                f'fill="{APPROACH_COLORS[result.approach]}" rx="4"/>'
            )
            err_x0 = margin_left + max(bar_width - err, 0)
            err_x1 = margin_left + min(bar_width + err, chart_width)
            body.append(
                f'<line x1="{err_x0:.1f}" y1="{y + bar_height / 2:.1f}" x2="{err_x1:.1f}" y2="{y + bar_height / 2:.1f}" '
                'stroke="#111827" stroke-width="1.5"/>'
            )
            body.append(
                f'<line x1="{err_x0:.1f}" y1="{y + 4:.1f}" x2="{err_x0:.1f}" y2="{y + bar_height - 4:.1f}" '
                'stroke="#111827" stroke-width="1.5"/>'
            )
            body.append(
                f'<line x1="{err_x1:.1f}" y1="{y + 4:.1f}" x2="{err_x1:.1f}" y2="{y + bar_height - 4:.1f}" '
                'stroke="#111827" stroke-width="1.5"/>'
            )
            body.append(
                f'<text x="{margin_left + chart_width + 8}" y="{y + 13}" font-size="12" fill="#374151">'
                f"{format_seconds(result.mean_s)} ± {format_seconds(result.stddev_s)}</text>"
            )

    total_height = 60 + panel_height * len(panels)
    out_path.write_text(svg_root(width, total_height, body), encoding="utf-8")


def render_runtime_svg(results: list[RuntimeResult], out_path: Path) -> None:
    datasets = sorted({result.dataset for result in results})
    width = 1180
    panel_height = 340
    margin_left = 220
    margin_right = 60
    chart_width = width - margin_left - margin_right
    body: list[str] = [
        '<rect width="100%" height="100%" fill="#fffdf8"/>',
        '<text x="24" y="34" font-size="22" font-weight="700" fill="#111827">Runtime benchmark (CPU time)</text>',
    ]

    runtime_map = {
        (result.dataset, result.approach, result.mode): result.cpu_ns for result in results
    }
    mode_keys = sorted(MODE_LABELS)
    total_height = 60 + panel_height * len(mode_keys)

    for panel_index, mode in enumerate(mode_keys):
        top = 60 + panel_index * panel_height
        chart_top = top + 30
        row_group = 42
        body.append(
            f'<text x="24" y="{top}" font-size="16" font-weight="700" fill="#1f2937">{escape_xml(MODE_LABELS[mode])}</text>'
        )

        max_value = max(runtime_map[(dataset, approach, mode)] for dataset in datasets for approach in APPROACH_ORDER)
        for tick in range(6):
            ratio = tick / 5 if max_value else 0.0
            value = max_value * ratio
            x = margin_left + chart_width * ratio
            body.append(
                f'<line x1="{x:.1f}" y1="{chart_top - 10}" x2="{x:.1f}" y2="{chart_top + row_group * len(datasets)}" '
                'stroke="#e5e7eb" stroke-width="1"/>'
            )
            body.append(
                f'<text x="{x:.1f}" y="{chart_top - 16}" font-size="11" text-anchor="middle" fill="#6b7280">{format_ns(value)}</text>'
            )

        for dataset_index, dataset in enumerate(datasets):
            group_y = chart_top + dataset_index * row_group
            body.append(
                f'<text x="{margin_left - 12}" y="{group_y + 18}" font-size="12" text-anchor="end" fill="#111827">{escape_xml(dataset)}</text>'
            )
            for approach_index, approach in enumerate(APPROACH_ORDER):
                value = runtime_map[(dataset, approach, mode)]
                y = group_y + approach_index * 8
                bar_height = 6
                bar_width = 0.0 if max_value == 0 else chart_width * (value / max_value)
                body.append(
                    f'<rect x="{margin_left}" y="{y}" width="{bar_width:.1f}" height="{bar_height}" '
                    f'fill="{APPROACH_COLORS[approach]}" rx="3"/>'
                )
                if approach_index == 0:
                    pass
            legend_y = chart_top + row_group * len(datasets) + 18
        for index, approach in enumerate(APPROACH_ORDER):
            lx = 24 + index * 170
            ly = chart_top + row_group * len(datasets) + 20
            body.append(
                f'<rect x="{lx}" y="{ly - 10}" width="18" height="10" fill="{APPROACH_COLORS[approach]}" rx="2"/>'
            )
            body.append(
                f'<text x="{lx + 26}" y="{ly - 1}" font-size="12" fill="#374151">{escape_xml(APPROACH_LABELS[approach])}</text>'
            )

    out_path.write_text(svg_root(width, total_height, body), encoding="utf-8")


def write_summary(
    out_path: Path,
    compile_results: dict[str, list[CompileResult]],
    runtime_results: list[RuntimeResult],
) -> None:
    lines = ["# Benchmark Report", ""]
    lines.append("## Compile-time")
    lines.append("")
    for group_key, title, _, _ in COMPILE_GROUPS:
        lines.append(f"### {title}")
        lines.append("")
        lines.append("| Approach | Mean | Stddev |")
        lines.append("| --- | ---: | ---: |")
        for result in compile_results[group_key]:
            lines.append(
                f"| {APPROACH_LABELS[result.approach]} | {result.mean_s:.3f} s | {result.stddev_s:.3f} s |"
            )
        lines.append("")

    lines.append("## Runtime")
    lines.append("")
    lines.append("| Dataset | Mode | Approach | CPU time |")
    lines.append("| --- | --- | --- | ---: |")
    for result in sorted(
        runtime_results, key=lambda item: (item.dataset, item.mode, APPROACH_ORDER.index(item.approach))
    ):
        lines.append(
            f"| {result.dataset} | {MODE_LABELS[result.mode]} | {APPROACH_LABELS[result.approach]} | {result.cpu_ns:.2f} ns |"
        )
    lines.append("")
    out_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT_DIR)
    args = parser.parse_args()

    out_dir = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    nix_flags = require_env("NIX_CFLAGS_COMPILE")

    build_binaries()
    compile_results = run_compile_hyperfine(out_dir, nix_flags)
    runtime_results = run_runtime_benchmark(out_dir)

    write_json(
        out_dir / "compile_summary.json",
        {
            group: [
                {
                    "approach": result.approach,
                    "mean_s": result.mean_s,
                    "stddev_s": result.stddev_s,
                }
                for result in values
            ]
            for group, values in compile_results.items()
        },
    )
    write_json(
        out_dir / "runtime_summary.json",
        [
            {
                "dataset": result.dataset,
                "approach": result.approach,
                "mode": result.mode,
                "cpu_ns": result.cpu_ns,
            }
            for result in runtime_results
        ],
    )

    render_compile_svg(compile_results, out_dir / "compile_time.svg")
    render_runtime_svg(runtime_results, out_dir / "runtime_benchmark.svg")
    write_summary(out_dir / "summary.md", compile_results, runtime_results)

    print(f"\nWrote report to {out_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
