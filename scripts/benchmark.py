#!/usr/bin/env python3

# Copyright 2017 Elias Kosunen
# SPDX-License-Identifier: Apache-2.0

"""Build, run and render the scnlib benchmarks.

The workflow has three stages, each runnable on its own:

  build    configure and build the benchmark binaries, once for each compiler
           (build/bench-<compiler>, and build/bench-<compiler>-<build type> for
           the binary size and build time benchmarks)
  run      run the benchmarks and save the raw results as one JSON file, with
           metadata (commit, compilers, flags, CPU, kernel, governor, date).
           The metadata is stored for each benchmark separately, so running
           only some of them (--what) doesn't relabel the results of the others
  render   read a saved JSON file, print Markdown tables to stdout, and write the
           charts (needs matplotlib, and nothing else)
  all      build, run and render

Typical usage (from the repository root):

  # Quick check, a few seconds of measurement per benchmark binary
  scripts/benchmark.py all --compilers clang --repetitions 2 --min-time 0.01 \\
      --filter 'scan_int_.*<int>' --what runtime --output-dir /tmp/charts

  # The full measurement, on a quiet machine
  scripts/benchmark.py all --cpu 3

  # Re-render from the saved results without building or running anything
  scripts/benchmark.py render --json build/benchmark-results.json

Without --output-dir, render overwrites the charts that the README links to:
benchmark/runtime/results/{int,float,string}.png and
benchmark/binarysize/graph-{debug,minsizerel,release}.png.

Requirements: CMake, Ninja, taskset (optional), git (optional, for the
metadata), GNU /usr/bin/time, size (binutils), and strip. Rendering, and thus
`all`, also needs matplotlib
(`pip install -r scripts/benchmark-requirements.txt`).

Run-time benchmarks are pinned to one CPU with taskset. The CPU frequency
governor is not changed: set it to "performance" and disable boost before a
real measurement.
"""

import argparse
import datetime
import json
import math
import os
import platform
import re
import shutil
import statistics
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
RESULTS_SCHEMA = 2

COMPILERS = {"clang": "clang++", "gcc": "g++"}
COMPILER_NAMES = {"clang": "Clang", "gcc": "GCC"}

RUNTIME_BINARIES = {
    "basic": "basic/scn_basic_bench",
    "int": "integer/scn_int_bench",
    "float": "float/scn_float_bench",
    "string": "string/scn_string_bench",
}

BINARYSIZE_BUILD_TYPES = {
    "debug": "Debug",
    "release": "Release",
    "minsizerel": "MinSizeRel",
}
BINARYSIZE_TESTS = ("control", "scanf", "iostream", "scnlib")
BINARYSIZE_TEST_NAMES = {
    "control": "empty",
    "scanf": "`std::scanf`",
    "iostream": "`std::istream`",
    "scnlib": "`scn::input`",
}
BINARYSIZE_BUILD_TYPE_DESCRIPTIONS = {
    "release": "Release build (`-O3 -DNDEBUG` + LTO)",
    "minsizerel": "Minimized (MinSizeRel) build (`-Os -DNDEBUG` + LTO)",
    "debug": "Debug build (`-g -O0`)",
}

BUILDTIME_METHODS = (
    ("empty", "empty.cpp", []),
    ("scanf", "cstdio.cpp", []),
    ("iostream", "iostream.cpp", []),
    ("scnlib", "scnlib.cpp", ["-lscn"]),
)
BUILDTIME_METHOD_NAMES = {
    "empty": "empty",
    "scanf": "`scanf`",
    "iostream": "`<iostream>`",
    "scnlib": "`scn::input`",
}
BUILDTIME_FLAGS = {"debug": ["-g"], "release": ["-O3", "-DNDEBUG"]}

STAGES = ("runtime", "binarysize", "buildtime")

# Each library has the same color in every chart.
# Slots 1 to 6 of the validated categorical palette, in order.
LIBRARY_COLORS = {
    "scnlib": "#2a78d6",
    "scanf": "#eb6834",
    "iostream": "#1baf7a",
    "strtol": "#eda100",
    "charconv": "#e87ba4",
    "fastfloat": "#008300",
    "empty": "#898781",
}
LIBRARY_NAMES = {
    "scnlib": "scnlib",
    "scanf": "sscanf",
    "iostream": "std::stringstream",
    "strtol": "strtol, strtod",
    "charconv": "std::from_chars",
    "fastfloat": "fast_float::from_chars",
}

SURFACE = "#fcfcfb"
INK_PRIMARY = "#0b0b0b"
INK_SECONDARY = "#52514e"
INK_MUTED = "#898781"
GRID = "#e1e0d9"
AXIS = "#c3c2b7"


def log(message):
    print(message, file=sys.stderr, flush=True)


def fail(message):
    log(f"error: {message}")
    sys.exit(1)


def split_list(text, allowed, what):
    items = [item for item in text.split(",") if item]
    for item in items:
        if item not in allowed:
            fail(f"unknown {what} '{item}', expected one of: {', '.join(allowed)}")
    return items


# Builds


def runtime_build_dir(args, compiler):
    return REPO / args.build_root / f"bench-{compiler}"


def binarysize_build_dir(args, compiler, build_type):
    return REPO / args.build_root / f"bench-{compiler}-{build_type}"


def run_command(command, **kwargs):
    log("+ " + " ".join(str(part) for part in command))
    return subprocess.run(command, check=True, **kwargs)


def configure_and_build(args, compiler, build_dir, build_type, options, targets):
    command = [
        "cmake",
        "-S",
        REPO,
        "-B",
        build_dir,
        "-G",
        "Ninja",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_CXX_COMPILER={COMPILERS[compiler]}",
        f"-DCMAKE_CXX_FLAGS=-march={args.march}",
        "-DSCN_TESTS=OFF",
        "-DSCN_EXAMPLES=OFF",
        "-DSCN_DOCS=OFF",
        *(f"-D{name}={value}" for name, value in options.items()),
        *(f"-D{definition}" for definition in args.cmake_arg),
    ]
    # The log goes to stderr, so that stdout only has the Markdown tables
    run_command(command, stdout=sys.stderr)
    run_command(
        ["cmake", "--build", build_dir, "--target", *targets], stdout=sys.stderr
    )


def build_runtime(args, compiler):
    configure_and_build(
        args,
        compiler,
        runtime_build_dir(args, compiler),
        "Release",
        {
            "CMAKE_INTERPROCEDURAL_OPTIMIZATION": "ON",
            "SCN_BENCHMARKS": "ON",
            "SCN_BENCHMARKS_BINARYSIZE": "OFF",
        },
        [Path(binary).name for binary in RUNTIME_BINARIES.values()],
    )


def build_binarysize_and_buildtime(args, compiler, build_type, stages):
    # The build time benchmark only needs the library, so it shares the
    # binary size build directories (shared library, Debug and Release)
    if "binarysize" in stages:
        targets = [f"scn_benchmark_binarysize_{test}" for test in BINARYSIZE_TESTS]
    else:
        targets = ["scn"]
    configure_and_build(
        args,
        compiler,
        binarysize_build_dir(args, compiler, build_type),
        BINARYSIZE_BUILD_TYPES[build_type],
        {
            "BUILD_SHARED_LIBS": "ON",
            "CMAKE_INTERPROCEDURAL_OPTIMIZATION": (
                "OFF" if build_type == "debug" else "ON"
            ),
            "SCN_BENCHMARKS": "OFF",
            "SCN_BENCHMARKS_BINARYSIZE": "ON",
        },
        targets,
    )


def stage_build(args):
    stages = split_list(args.what, STAGES, "stage")
    for compiler in split_list(args.compilers, COMPILERS, "compiler"):
        if "runtime" in stages:
            build_runtime(args, compiler)
        for build_type in binarysize_build_types(args, stages):
            build_binarysize_and_buildtime(args, compiler, build_type, stages)


def binarysize_build_types(args, stages):
    wanted = set()
    if "binarysize" in stages:
        wanted.update(split_list(args.build_types, BINARYSIZE_BUILD_TYPES, "type"))
    if "buildtime" in stages:
        wanted.update(BUILDTIME_FLAGS)
    return [build_type for build_type in BINARYSIZE_BUILD_TYPES if build_type in wanted]


# Metadata


def command_output(command, **kwargs):
    try:
        result = subprocess.run(
            command, capture_output=True, text=True, check=True, **kwargs
        )
    except (OSError, subprocess.CalledProcessError):
        return None
    return result.stdout.strip()


def read_text(path):
    try:
        return Path(path).read_text().strip()
    except OSError:
        return None


def read_cmake_cache(build_dir):
    cache = {}
    try:
        lines = (build_dir / "CMakeCache.txt").read_text().splitlines()
    except OSError:
        return cache
    for line in lines:
        match = re.match(r"([A-Za-z0-9_\-]+):[A-Z]+=(.*)", line)
        if match:
            cache[match.group(1)] = match.group(2)
    return cache


def build_flags(build_dir):
    cache = read_cmake_cache(build_dir)
    build_type = cache.get("CMAKE_BUILD_TYPE", "")
    flags = " ".join(
        flag
        for flag in (
            cache.get("CMAKE_CXX_FLAGS", ""),
            cache.get(f"CMAKE_CXX_FLAGS_{build_type.upper()}", ""),
        )
        if flag
    )
    return {
        "build_type": build_type,
        "cxx_flags": flags,
        "lto": cache.get("CMAKE_INTERPROCEDURAL_OPTIMIZATION", "OFF"),
        "shared": cache.get("BUILD_SHARED_LIBS", "OFF"),
    }


def compiler_info(compiler):
    executable = COMPILERS[compiler]
    version_output = command_output([executable, "--version"]) or ""
    first_line = version_output.splitlines()[0] if version_output else ""
    match = re.search(r"\d+\.\d+\.\d+", first_line)
    return {
        "executable": executable,
        "version_line": first_line,
        "version": match.group(0) if match else "",
    }


def cpu_model():
    for line in (read_text("/proc/cpuinfo") or "").splitlines():
        if line.startswith("model name"):
            return line.split(":", 1)[1].strip()
    return platform.processor()


def operating_system():
    for line in (read_text("/etc/os-release") or "").splitlines():
        if line.startswith("PRETTY_NAME="):
            return line.split("=", 1)[1].strip('"')
    return platform.system()


def boost_enabled():
    boost = read_text("/sys/devices/system/cpu/cpufreq/boost")
    if boost is not None:
        return boost == "1"
    no_turbo = read_text("/sys/devices/system/cpu/intel_pstate/no_turbo")
    if no_turbo is not None:
        return no_turbo == "0"
    return None


def pinned_cpu(args):
    if args.cpu == "none":
        return None
    if args.cpu is not None:
        return int(args.cpu)
    if hasattr(os, "sched_getaffinity"):
        return max(os.sched_getaffinity(0))
    return None


def collect_metadata(args, cpu):
    cpufreq = f"/sys/devices/system/cpu/cpu{cpu if cpu is not None else 0}/cpufreq"
    governor = read_text(f"{cpufreq}/scaling_governor")
    boost = boost_enabled()
    status = command_output(["git", "-C", REPO, "status", "--porcelain"])
    warnings = []
    if governor is None:
        warnings.append("Could not read the CPU frequency governor")
    elif governor != "performance":
        warnings.append(f"CPU frequency governor is '{governor}', not 'performance'")
    if boost:
        warnings.append("CPU frequency boost is enabled")
    if cpu is None:
        warnings.append("Not pinned to a CPU")
    return {
        "commit": command_output(["git", "-C", REPO, "rev-parse", "--short=8", "HEAD"]),
        "dirty": bool(status),
        "date": datetime.datetime.now().astimezone().isoformat(timespec="seconds"),
        "march": args.march,
        "cpu_model": cpu_model(),
        "pinned_cpu": cpu,
        "kernel": platform.release(),
        "os": operating_system(),
        "governor": governor,
        "scaling_driver": read_text(f"{cpufreq}/scaling_driver"),
        "boost": boost,
        "load_average": os.getloadavg()[0] if hasattr(os, "getloadavg") else None,
        "warnings": warnings,
    }


# Runs


def pin_command(cpu):
    if cpu is None:
        return []
    if shutil.which("taskset") is None:
        log("warning: taskset not found, not pinning to a CPU")
        return []
    return ["taskset", "-c", str(cpu)]


TIME_UNITS_NS = {"ns": 1.0, "us": 1e3, "ms": 1e6, "s": 1e9}


def run_runtime_binary(args, build_dir, binary, pin):
    executable = build_dir / "benchmark" / "runtime" / binary
    if not executable.exists():
        fail(f"{executable} does not exist, run the build stage first")
    command = [
        *pin,
        executable,
        "--benchmark_format=json",
        f"--benchmark_repetitions={args.repetitions}",
        "--benchmark_enable_random_interleaving=true",
        f"--benchmark_min_time={args.min_time}s",
    ]
    if args.filter:
        command.append(f"--benchmark_filter={args.filter}")
    # The string benchmarks read their input files from the working directory
    log("+ " + " ".join(str(part) for part in command))
    process = subprocess.run(
        command, cwd=executable.parent, stdout=subprocess.PIPE, text=True, check=True
    )
    if not process.stdout.strip():
        return {}, {}
    output = json.loads(process.stdout)
    benchmarks = {}
    for entry in output["benchmarks"]:
        if entry.get("run_type") != "iteration" or entry.get("error_occurred"):
            continue
        scale = TIME_UNITS_NS[entry["time_unit"]]
        samples = benchmarks.setdefault(entry["name"], {"cpu_ns": [], "real_ns": []})
        samples["cpu_ns"].append(entry["cpu_time"] * scale)
        samples["real_ns"].append(entry["real_time"] * scale)
    return benchmarks, output["context"]


def run_runtime(args, metadata, compiler):
    build_dir = runtime_build_dir(args, compiler)
    pin = pin_command(metadata["pinned_cpu"])
    benchmarks = {}
    library_build_types = set()
    for binary in RUNTIME_BINARIES.values():
        results, context = run_runtime_binary(args, build_dir, binary, pin)
        benchmarks.update(results)
        if context:
            library_build_types.add(context.get("library_build_type"))
    if library_build_types - {"release"}:
        log("warning: Google Benchmark was not built in release mode")
    return {
        "compiler": compiler_info(compiler),
        "build": build_flags(build_dir),
        "repetitions": args.repetitions,
        "min_time_seconds": args.min_time,
        "filter": args.filter,
        "benchmarks": benchmarks,
    }


def strip_size(path):
    with tempfile.TemporaryDirectory() as temporary:
        stripped = Path(temporary) / "stripped"
        shutil.copyfile(path, stripped)
        subprocess.run(["strip", stripped], check=True)
        return stripped.stat().st_size


def section_sizes(path):
    """Section sizes in bytes from `size --format=berkeley`: text (code and
    read-only data), data (initialized data) and bss (zero-initialized data)"""
    output = command_output(["size", "--format=berkeley", path])
    if output is None:
        fail("Could not run `size` (binutils)")
    text, data, bss = output.splitlines()[1].split()[:3]
    return {"text": int(text), "data": int(data), "bss": int(bss)}


def run_binarysize(args, compiler, build_type):
    build_dir = binarysize_build_dir(args, compiler, build_type)
    directory = build_dir / "benchmark" / "binarysize"
    tests = {}
    for test in BINARYSIZE_TESTS:
        executable = directory / f"scn_benchmark_binarysize_{test}"
        if not executable.exists():
            fail(f"{executable} does not exist, run the build stage first")
        tests[test] = {
            "size": executable.stat().st_size,
            "stripped": strip_size(executable),
            **section_sizes(executable),
        }
    libraries = sorted(build_dir.glob("libscn.so*"))
    return {
        "compiler": compiler_info(compiler),
        "build": build_flags(build_dir),
        "library_bytes": libraries[0].stat().st_size if libraries else None,
        "tests": tests,
    }


def run_buildtime_once(args, compiler, build_type, method, pin, output):
    _, source, libraries = next(
        entry for entry in BUILDTIME_METHODS if entry[0] == method
    )
    build_dir = binarysize_build_dir(args, compiler, build_type)
    command = [
        *pin,
        "/usr/bin/time",
        "-f",
        "%U %S %M",
        COMPILERS[compiler],
        "-x",
        "c++",
        f"-I{REPO / 'include'}",
        f"-I{build_dir / 'include'}",
        f"-L{build_dir}",
        REPO / "benchmark" / "buildtime" / source,
        "-w",
        *BUILDTIME_FLAGS[build_type],
        "-o",
        output,
        *libraries,
    ]
    # ccache would turn the compilation into a cache lookup
    environment = {**os.environ, "CCACHE_DISABLE": "1"}
    process = subprocess.run(
        command, env=environment, capture_output=True, text=True, check=False
    )
    lines = process.stderr.strip().splitlines()
    if process.returncode != 0 or not lines:
        fail(f"Compilation failed: {' '.join(map(str, command))}\n{process.stderr}")
    user, system, max_rss_kib = lines[-1].split()
    return float(user), float(system), int(max_rss_kib)


def run_buildtime(args, metadata, compiler, build_type):
    pin = pin_command(metadata["pinned_cpu"])
    build_dir = binarysize_build_dir(args, compiler, build_type)
    if not list(build_dir.glob("libscn.so*")):
        fail(f"No libscn.so in {build_dir}, run the build stage first")
    methods = {}
    with tempfile.TemporaryDirectory() as temporary:
        output = Path(temporary) / "a.out"
        for method, _, _ in BUILDTIME_METHODS:
            log(f"build time: {compiler} {build_type} {method}")
            samples = {"user_s": [], "system_s": [], "max_rss_kib": []}
            for _ in range(args.buildtime_repetitions):
                user, system, rss = run_buildtime_once(
                    args, compiler, build_type, method, pin, output
                )
                samples["user_s"].append(user)
                samples["system_s"].append(system)
                samples["max_rss_kib"].append(rss)
            methods[method] = samples
    return {
        "compiler": compiler_info(compiler),
        "flags": " ".join(BUILDTIME_FLAGS[build_type]),
        "repetitions": args.buildtime_repetitions,
        "methods": methods,
    }


def load_results(path):
    try:
        results = json.loads(Path(path).read_text())
    except FileNotFoundError:
        return None
    except json.JSONDecodeError as error:
        fail(f"{path} is not valid JSON: {error}")
    schema = results.get("schema", results.get("metadata", {}).get("schema"))
    if schema == 1:
        # The metadata used to be shared by all benchmarks
        shared = results.pop("metadata")
        shared.pop("schema")
        results["schema"] = RESULTS_SCHEMA
        results["metadata"] = {stage: shared for stage in STAGES if stage in results}
    elif schema != RESULTS_SCHEMA:
        fail(f"{path} has an unsupported schema")
    return results


def stage_run(args):
    stages = split_list(args.what, STAGES, "stage")
    compilers = split_list(args.compilers, COMPILERS, "compiler")
    path = REPO / args.json
    # Running a subset of the stages keeps the other results in the file
    results = load_results(path) or {"schema": RESULTS_SCHEMA, "metadata": {}}
    cpu = pinned_cpu(args)
    metadata = collect_metadata(args, cpu)
    for warning in metadata["warnings"]:
        log(f"warning: {warning}")
    for stage in stages:
        results[stage] = {}
        results["metadata"][stage] = metadata
    for compiler in compilers:
        if "runtime" in stages:
            results["runtime"][compiler] = run_runtime(args, metadata, compiler)
        if "binarysize" in stages:
            results["binarysize"][compiler] = {
                build_type: run_binarysize(args, compiler, build_type)
                for build_type in split_list(
                    args.build_types, BINARYSIZE_BUILD_TYPES, "type"
                )
            }
        if "buildtime" in stages:
            results["buildtime"][compiler] = {
                build_type: run_buildtime(args, metadata, compiler, build_type)
                for build_type in BUILDTIME_FLAGS
            }
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(results, indent=2) + "\n")
    log(f"Wrote {path}")


# Rendering: tables


def format_number(value):
    if value is None:
        return "-"
    if value >= 100:
        return f"{value:.0f}"
    return f"{value:#.3g}"


def format_kib(size_bytes):
    kib = round(size_bytes / 1024, 1)
    return str(int(kib)) if kib >= 100 else f"{kib:.1f}"


def format_human_size(size_bytes):
    if size_bytes is None:
        return "-"
    for suffix, scale in (("G", 1024**3), ("M", 1024**2), ("K", 1024)):
        if size_bytes >= scale:
            value = size_bytes / scale
            return f"{value:.1f}{suffix}" if value < 10 else f"{value:.0f}{suffix}"
    return str(size_bytes)


def markdown_table(headers, rows):
    """The first column is left-aligned, the others right-aligned"""
    table = [headers, *rows]
    widths = [max(len(row[i]) for row in table) for i in range(len(headers))]
    widths = [max(width, 4) for width in widths]

    def line(row):
        cells = [row[0].ljust(widths[0])]
        cells += [cell.rjust(width) for cell, width in zip(row[1:], widths[1:])]
        return "| " + " | ".join(cells) + " |"

    separator = [":" + "-" * (widths[0] + 1)]
    separator += ["-" * (width + 1) + ":" for width in widths[1:]]
    return "\n".join([line(headers), "|" + "|".join(separator) + "|", *map(line, rows)])


def compiler_title(results, compiler, stage):
    data = results[stage][compiler]
    info = (
        data["compiler"]
        if stage == "runtime"
        else next(iter(data.values()))["compiler"]
    )
    return f"{COMPILER_NAMES[compiler]} {info['version']}".strip()


class RuntimeRow:
    def __init__(self, label, library, pattern, kinds=("single", "repeated")):
        self.label = label
        self.library = library
        self.pattern = pattern
        self.kinds = kinds

    def name(self, kind):
        return self.pattern.format(kind=kind) if kind in self.kinds else None


INT_ROWS = [
    RuntimeRow("`scn::scan`", "scnlib", "scan_int_{kind}_scn<int>"),
    RuntimeRow("`scn::scan_value`", "scnlib", "scan_int_{kind}_scn_value<int>"),
    RuntimeRow("`scn::scan_int`", "scnlib", "scan_int_{kind}_scn_int<int>"),
    RuntimeRow(
        "`scn::scan_int_exhaustive_valid`",
        "scnlib",
        "scan_int_{kind}_scn_int_exhaustive_valid<int>",
        ("single",),
    ),
    RuntimeRow("`scn::scan` with `{:d}`", "scnlib", "scan_int_{kind}_scn_decimal<int>"),
    RuntimeRow("`std::stringstream`", "iostream", "scan_int_{kind}_sstream<int>"),
    RuntimeRow("`sscanf`", "scanf", "scan_int_{kind}_scanf<int>"),
    RuntimeRow("`strtol`", "strtol", "scan_int_{kind}_strtol<int>"),
    RuntimeRow("`std::from_chars`", "charconv", "scan_int_{kind}_charconv<int>"),
    RuntimeRow(
        "`fast_float::from_chars`", "fastfloat", "scan_int_{kind}_fastfloat<int>"
    ),
]
FLOAT_ROWS = [
    RuntimeRow("`scn::scan`", "scnlib", "scan_float_{kind}_scn<double>"),
    RuntimeRow("`scn::scan_value`", "scnlib", "scan_float_{kind}_scn_value<double>"),
    RuntimeRow("`std::stringstream`", "iostream", "scan_float_{kind}_sstream<double>"),
    RuntimeRow("`sscanf`", "scanf", "scan_float_{kind}_scanf<double>"),
    RuntimeRow("`strtod`", "strtol", "scan_float_{kind}_strtod<double>"),
    RuntimeRow("`std::from_chars`", "charconv", "scan_float_{kind}_charconv<double>"),
    RuntimeRow(
        "`fast_float::from_chars`", "fastfloat", "scan_float_{kind}_fastfloat<double>"
    ),
]
# For strings, "kind" is the input text
STRING_ROWS = [
    RuntimeRow(
        "`scn::scan<string>`",
        "scnlib",
        "bench_string_scn<char, std::string, {kind}_tag>",
        ("lipsum", "unicode"),
    ),
    RuntimeRow(
        "`scn::scan<string_view>`",
        "scnlib",
        "bench_string_scn<char, std::string_view, {kind}_tag>",
        ("lipsum", "unicode"),
    ),
    RuntimeRow(
        "`scn::scan_value<string>`",
        "scnlib",
        "bench_string_scn_value<char, std::string, {kind}_tag>",
        ("lipsum", "unicode"),
    ),
    RuntimeRow(
        "`scn::scan_value<string_view>`",
        "scnlib",
        "bench_string_scn_value<char, std::string_view, {kind}_tag>",
        ("lipsum", "unicode"),
    ),
    RuntimeRow(
        "`std::stringstream`",
        "iostream",
        "bench_string_sstream<char, {kind}_tag>",
        ("lipsum", "unicode"),
    ),
    RuntimeRow(
        "`sscanf`",
        "scanf",
        "bench_string_scanf<char, {kind}_tag>",
        ("lipsum", "unicode"),
    ),
]
BASIC_ROWS = [
    RuntimeRow("`scn::scan`", "scnlib", "bench_basic_scn", ("",)),
    RuntimeRow(
        "`scn::scan` with options", "scnlib", "bench_basic_scn_withoptions", ("",)
    ),
    RuntimeRow(
        "`scn::scan` with locale", "scnlib", "bench_basic_scn_withlocale", ("",)
    ),
    RuntimeRow("`scn::scan` localized", "scnlib", "bench_basic_scn_localized", ("",)),
    RuntimeRow("`scn::scan_value`", "scnlib", "bench_basic_scn_value", ("",)),
    RuntimeRow("`std::stringstream`", "iostream", "bench_basic_sstream", ("",)),
    RuntimeRow("`sscanf`", "scanf", "bench_basic_scanf", ("",)),
    RuntimeRow("`strtol`", "strtol", "bench_basic_strtol", ("",)),
]


def median_ns(benchmarks, name):
    if name is None or name not in benchmarks:
        return None
    return statistics.median(benchmarks[name]["cpu_ns"])


def row_values(benchmarks, row, kinds):
    return [median_ns(benchmarks, row.name(kind)) for kind in kinds]


def has_values(benchmarks, rows, kinds):
    return any(
        value is not None
        for row in rows
        for value in row_values(benchmarks, row, kinds)
    )


def average(values):
    present = [value for value in values if value is not None]
    return sum(present) / len(present) if present else None


def runtime_table(benchmarks, rows, kinds, headers):
    """Medians of the CPU time, in nanoseconds. If kinds has two entries, adds a
    column for their average, as the README does."""
    body = []
    for row in rows:
        values = row_values(benchmarks, row, kinds)
        cells = [format_number(value) for value in values]
        if len(kinds) == 2:
            cells.append(format_number(average(values)))
        body.append([row.label, *cells])
    return markdown_table(headers, body)


RUNTIME_SECTIONS = [
    {
        "key": "int",
        "title": "Integer parsing (`int`)",
        "rows": INT_ROWS,
        "kinds": ("single", "repeated"),
        "kind_titles": ('Test 1 `"single"`', 'Test 2 `"repeated"`'),
        "chart": "benchmark/runtime/results/int.png",
        "log": True,
    },
    {
        "key": "float",
        "title": "Floating-point number parsing (`double`)",
        "rows": FLOAT_ROWS,
        "kinds": ("single", "repeated"),
        "kind_titles": ('Test 1 `"single"`', 'Test 2 `"repeated"`'),
        "chart": "benchmark/runtime/results/float.png",
        "log": True,
    },
    {
        "key": "string",
        "title": 'String "word" (whitespace-separated character sequence) parsing'
        " (`string` and `string_view`)",
        "rows": STRING_ROWS,
        "kinds": ("lipsum", "unicode"),
        "kind_titles": ("Lorem Ipsum text", "Unicode text"),
        "chart": "benchmark/runtime/results/string.png",
        "log": False,
    },
]


def render_runtime_tables(results):
    print("### Run-time performance\n")
    print("All times are in nanoseconds of CPU time (median). Lower is better.\n")
    for compiler, data in results["runtime"].items():
        benchmarks = data["benchmarks"]
        title = compiler_title(results, compiler, "runtime")
        for section in RUNTIME_SECTIONS:
            if not has_values(benchmarks, section["rows"], section["kinds"]):
                continue
            print(f"#### {section['title']}: {title}\n")
            kinds = section["kinds"]
            headers = ["Test", *section["kind_titles"]]
            if section["key"] != "string":
                headers.append("Test average")
            print(runtime_table(benchmarks, section["rows"], kinds, headers))
            print()
        if has_values(benchmarks, BASIC_ROWS, ("",)):
            print(f"#### Basic `scn::scan` options: {title}\n")
            print(runtime_table(benchmarks, BASIC_ROWS, ("",), ["Test", "Time"]))
            print()


def code_and_data_bytes(test_sizes):
    return test_sizes["text"] + test_sizes["data"]


def has_section_sizes(results):
    return all(
        "text" in sizes
        for by_type in results["binarysize"].values()
        for data in by_type.values()
        for sizes in data["tests"].values()
    )


def render_binarysize_tables(results):
    print("### Executable size\n")
    print(
        'All sizes are in kibibytes (KiB). "Code + data" is the size of the '
        "`text` and `data` sections of the executable, as reported by `size`: "
        "the machine code and the read-only and initialized data. Debug "
        "information and symbol tables are not included, and neither is `bss`, "
        "which takes no space in the file. Lower is better.\n"
    )
    for build_type in ("release", "minsizerel", "debug"):
        for compiler, by_type in results["binarysize"].items():
            if build_type not in by_type:
                continue
            data = by_type[build_type]
            title = compiler_title(results, compiler, "binarysize")
            description = BINARYSIZE_BUILD_TYPE_DESCRIPTIONS[build_type]
            print(f"#### {description}: {title}\n")
            print(
                "Size of `scnlib` shared library (`.so`): "
                f"{format_human_size(data['library_bytes'])}\n"
            )
            rows = [
                [
                    BINARYSIZE_TEST_NAMES[test],
                    format_kib(code_and_data_bytes(data["tests"][test])),
                ]
                for test in BINARYSIZE_TESTS
            ]
            print(markdown_table(["Method", "Code + data"], rows))
            print()


def buildtime_value(samples, key):
    if key == "cpu":
        totals = [
            user + system
            for user, system in zip(samples["user_s"], samples["system_s"])
        ]
        return statistics.median(totals)
    return statistics.median(samples["max_rss_kib"]) / 1024


def render_buildtime_tables(results):
    print("### Build time\n")
    for compiler, by_type in results["buildtime"].items():
        title = compiler_title(results, compiler, "buildtime")
        for key, heading, note in (
            ("cpu", "CPU time", "seconds of CPU time (user time + sys/kernel time)"),
            ("memory", "Memory consumption", "mebibytes (MiB) used while compiling"),
        ):
            print(f"#### {heading}: {title}\n")
            print(f"Median of {by_type['debug']['repetitions']} runs, in {note}.\n")
            rows = []
            for method, _, _ in BUILDTIME_METHODS:
                cells = []
                for build_type in BUILDTIME_FLAGS:
                    value = buildtime_value(by_type[build_type]["methods"][method], key)
                    cells.append(
                        f"{value:.2f}" if key == "cpu" else format_number(value)
                    )
                rows.append([BUILDTIME_METHOD_NAMES[method], *cells])
            print(markdown_table(["Method", "Debug", "Release"], rows))
            print()


def describe_run(metadata):
    dirty = " (with uncommitted changes)" if metadata["dirty"] else ""
    return f"{metadata['date']}, commit {metadata['commit']}{dirty}"


def render_metadata(results):
    print("### About these results\n")
    for stage, name in (
        ("runtime", "Run-time"),
        ("binarysize", "Binary size"),
        ("buildtime", "Build time"),
    ):
        if not results.get(stage):
            continue
        metadata = results["metadata"][stage]
        print(f"* {name}: {describe_run(metadata)}")
        if stage != "binarysize":
            print(
                f"  * Machine: {metadata['cpu_model']}, {metadata['os']}, "
                f"kernel {metadata['kernel']}"
            )
            print(
                f"  * CPU frequency governor: {metadata['governor']}, "
                f"boost: {metadata['boost']}, pinned to CPU {metadata['pinned_cpu']}"
            )
        for data in results[stage].values():
            if stage == "runtime":
                build = data["build"]
                print(
                    f"  * {data['compiler']['version_line']}: "
                    f"`{build['build_type']}`, `{build['cxx_flags']}`, "
                    f"LTO {build['lto']}"
                )
            elif stage == "binarysize":
                for build_type, entry in data.items():
                    build = entry["build"]
                    print(
                        f"  * {entry['compiler']['version_line']}, "
                        f"{build_type}: `{build['cxx_flags']}`, LTO {build['lto']}, "
                        f"shared library: {build['shared']}"
                    )
        if stage != "binarysize":
            for warning in metadata["warnings"]:
                print(f"  * Warning at run time: {warning}")
    print()


# Rendering: charts


def apply_style(plt):
    plt.rcParams.update(
        {
            "figure.facecolor": SURFACE,
            "axes.facecolor": SURFACE,
            "savefig.facecolor": SURFACE,
            "font.family": "sans-serif",
            "font.size": 9,
            "text.color": INK_SECONDARY,
            "axes.labelcolor": INK_SECONDARY,
            "axes.edgecolor": AXIS,
            "xtick.color": INK_MUTED,
            "ytick.color": INK_SECONDARY,
            "axes.spines.top": False,
            "axes.spines.right": False,
            "axes.spines.left": False,
            "axes.axisbelow": True,
            "axes.grid": True,
            "axes.grid.axis": "x",
            "grid.color": GRID,
            "grid.linewidth": 0.8,
            "grid.linestyle": "-",
            "figure.dpi": 110,
        }
    )


def import_matplotlib():
    try:
        import matplotlib

        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        from matplotlib.patches import Patch
        from matplotlib.ticker import FuncFormatter, NullLocator
    except ImportError:
        fail(
            "matplotlib is needed to render the charts: "
            "pip install -r scripts/benchmark-requirements.txt"
        )
    apply_style(plt)
    return plt, Patch, FuncFormatter, NullLocator


def plain_label(label):
    return label.replace("`", "")


def draw_bars(
    axis,
    labels,
    values,
    colors,
    limits,
    log_scale,
    formatter,
    null_locator,
    value_format,
):
    positions = list(range(len(labels)))
    axis.set_yticks(positions)
    axis.set_yticklabels([plain_label(label) for label in labels])
    if not axis.yaxis_inverted():
        axis.invert_yaxis()
    axis.tick_params(axis="y", length=0)
    axis.tick_params(axis="x", length=0)
    low, high = limits
    if log_scale:
        axis.set_xscale("log")
        axis.xaxis.set_minor_locator(null_locator())
        axis.xaxis.set_major_formatter(formatter(lambda tick, _: f"{tick:g}"))
        axis.set_xlim(10 ** math.floor(math.log10(low)), high * 2.4)
    else:
        axis.set_xlim(0, high * 1.2)
    for position, value, color in zip(positions, values, colors):
        if value is None:
            continue
        axis.barh(position, value, height=0.62, color=color)
        pad = 1.12 if log_scale else 1.0
        offset = 0 if log_scale else high * 0.012
        axis.text(
            value * pad + offset,
            position,
            value_format(value),
            va="center",
            ha="left",
            fontsize=8,
            color=INK_SECONDARY,
        )


def value_limits(nested):
    present = [
        value
        for compilers in nested
        for values in compilers
        for value in values
        if value is not None
    ]
    return min(present), max(present)


def finish_figure(figure, title, footer, legend_handles, legend_labels):
    figure.suptitle(title, x=0.01, ha="left", fontsize=13, color=INK_PRIMARY)
    figure.legend(
        legend_handles,
        legend_labels,
        loc="outside right upper",
        ncol=1,
        frameon=False,
        fontsize=9,
    )
    figure.supxlabel(footer, x=0.01, ha="left", fontsize=7, color=INK_MUTED)


def footer_text(metadata, build_flags_text):
    commit = metadata["commit"] + ("+dirty" if metadata["dirty"] else "")
    return (
        f"{metadata['cpu_model']}, {metadata['kernel']}, {build_flags_text}; "
        f"commit {commit}, {metadata['date'][:10]}"
    )


def render_runtime_charts(results, output_dir, modules):
    plt, patch, formatter, null_locator = modules
    metadata = results["metadata"]["runtime"]
    for section in RUNTIME_SECTIONS:
        rows = section["rows"]
        kinds = section["kinds"]
        sources = {
            compiler: data
            for compiler, data in results["runtime"].items()
            if has_values(data["benchmarks"], rows, kinds)
        }
        if not sources:
            continue
        figure, axes = plt.subplots(
            len(sources),
            len(kinds),
            figsize=(
                5.4 * max(len(kinds), 2) + 3.9,
                0.3 * len(rows) * len(sources) + 2.6,
            ),
            sharex=True,
            sharey=True,
            squeeze=False,
            layout="constrained",
        )
        all_values = [
            [
                [median_ns(data["benchmarks"], row.name(kind)) for row in rows]
                for kind in kinds
            ]
            for data in sources.values()
        ]
        limits = value_limits(all_values)
        for row_index, (compiler, data) in enumerate(sources.items()):
            for column, kind in enumerate(kinds):
                axis = axes[row_index][column]
                values = all_values[row_index][column]
                draw_bars(
                    axis,
                    [row.label for row in rows],
                    values,
                    [LIBRARY_COLORS[row.library] for row in rows],
                    limits,
                    section["log"],
                    formatter,
                    null_locator,
                    format_number,
                )
                axis.set_title(
                    compiler_title(results, compiler, "runtime")
                    + (
                        f": {plain_label(section['kind_titles'][column])}"
                        if len(kinds) > 1
                        else ""
                    ),
                    loc="left",
                    fontsize=10,
                    color=INK_PRIMARY,
                )
                if column > 0:
                    axis.tick_params(axis="y", labelleft=False)
        scale = "log scale" if section["log"] else "linear scale"
        for axis in axes[-1]:
            axis.set_xlabel(
                f"Nanoseconds of CPU time, median ({scale}); lower is better"
            )
        libraries = []
        for row in rows:
            if row.library not in libraries:
                libraries.append(row.library)
        flags = next(iter(sources.values()))["build"]["cxx_flags"]
        finish_figure(
            figure,
            plain_label(section["title"]),
            footer_text(
                metadata,
                flags
                + (
                    " + LTO"
                    if next(iter(sources.values()))["build"]["lto"] == "ON"
                    else ""
                ),
            ),
            [patch(color=LIBRARY_COLORS[library]) for library in libraries],
            [LIBRARY_NAMES[library] for library in libraries],
        )
        save_figure(plt, figure, output_dir, section["chart"])


def render_binarysize_charts(results, output_dir, modules):
    plt, patch, formatter, null_locator = modules
    metadata = results["metadata"]["binarysize"]
    compilers = list(results["binarysize"])
    for build_type, build_type_name in BINARYSIZE_BUILD_TYPES.items():
        present = [c for c in compilers if build_type in results["binarysize"][c]]
        if not present:
            continue
        figure, axes = plt.subplots(
            1,
            len(present),
            figsize=(4.8 * len(present) + 3.9, 3.4),
            sharex=True,
            sharey=True,
            squeeze=False,
            layout="constrained",
        )
        limits = value_limits(
            [
                [
                    [
                        code_and_data_bytes(
                            results["binarysize"][c][build_type]["tests"][t]
                        )
                        / 1024
                        for t in BINARYSIZE_TESTS
                    ]
                ]
                for c in present
            ]
        )
        for column, compiler in enumerate(present):
            data = results["binarysize"][compiler][build_type]
            axis = axes[0][column]
            values = [
                code_and_data_bytes(data["tests"][test]) / 1024
                for test in BINARYSIZE_TESTS
            ]
            draw_bars(
                axis,
                [BINARYSIZE_TEST_NAMES[test] for test in BINARYSIZE_TESTS],
                values,
                [LIBRARY_COLORS[test_library(test)] for test in BINARYSIZE_TESTS],
                limits,
                False,
                formatter,
                null_locator,
                lambda kib: format_kib(kib * 1024),
            )
            axis.set_title(
                f"{compiler_title(results, compiler, 'binarysize')}"
                f" (libscn.so {format_human_size(data['library_bytes'])})",
                loc="left",
                fontsize=10,
                color=INK_PRIMARY,
            )
            if column > 0:
                axis.tick_params(axis="y", labelleft=False)
        for axis in axes[-1]:
            axis.set_xlabel(
                "Code + data (text + data sections) in KiB; lower is better"
            )
        sample = results["binarysize"][present[0]][build_type]["build"]
        flags = sample["cxx_flags"] + (" + LTO" if sample["lto"] == "ON" else "")
        finish_figure(
            figure,
            f"Executable size, code + data: {build_type_name} build",
            footer_text(metadata, flags),
            [patch(color=LIBRARY_COLORS[test_library(t)]) for t in BINARYSIZE_TESTS],
            [plain_label(BINARYSIZE_TEST_NAMES[test]) for test in BINARYSIZE_TESTS],
        )
        save_figure(
            plt, figure, output_dir, f"benchmark/binarysize/graph-{build_type}.png"
        )


def test_library(test):
    return {"control": "empty", "scnlib": "scnlib"}.get(test, test)


def save_figure(plt, figure, output_dir, relative_path):
    if output_dir is None:
        path = REPO / relative_path
    else:
        path = Path(output_dir) / relative_path
    path.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(path)
    plt.close(figure)
    log(f"Wrote {path}")


def stage_render(args):
    results = load_results(REPO / args.json)
    if results is None:
        fail(f"{REPO / args.json} does not exist, run the run stage first")
    modules = import_matplotlib()
    render_metadata(results)
    if results.get("runtime"):
        render_runtime_tables(results)
        render_runtime_charts(results, args.output_dir, modules)
    if results.get("binarysize") and not has_section_sizes(results):
        log(
            "warning: the binary size results have no section sizes, "
            "run the binarysize stage again; skipping them"
        )
    elif results.get("binarysize"):
        render_binarysize_tables(results)
        render_binarysize_charts(results, args.output_dir, modules)
    if results.get("buildtime"):
        render_buildtime_tables(results)


def stage_all(args):
    import_matplotlib()
    stage_build(args)
    stage_run(args)
    stage_render(args)


def make_parser():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument(
        "--compilers",
        default=",".join(COMPILERS),
        help="comma-separated list of: clang, gcc (default: %(default)s)",
    )
    common.add_argument(
        "--what",
        default=",".join(STAGES),
        help="comma-separated benchmarks to build and run: runtime, binarysize, "
        "buildtime (default: all)",
    )
    common.add_argument(
        "--build-types",
        default=",".join(BINARYSIZE_BUILD_TYPES),
        help="binary size build types (default: %(default)s)",
    )
    common.add_argument("--build-root", default="build", help="default: %(default)s")
    common.add_argument(
        "--json",
        default="build/benchmark-results.json",
        help="the results file, relative to the repository root (default: %(default)s)",
    )

    build = argparse.ArgumentParser(add_help=False)
    build.add_argument(
        "--march", default="x86-64-v3", help="-march value (default: %(default)s)"
    )
    build.add_argument(
        "--cmake-arg",
        action="append",
        default=[],
        metavar="NAME=VALUE",
        help="extra CMake definition, passed as -DNAME=VALUE (repeatable)",
    )

    run = argparse.ArgumentParser(add_help=False)
    run.add_argument(
        "--cpu",
        help="CPU to pin the run-time and build time benchmarks to, or 'none' "
        "(default: the highest-numbered allowed CPU)",
    )
    run.add_argument(
        "--repetitions",
        type=int,
        default=3,
        help="run-time benchmark repetitions (default: %(default)s)",
    )
    run.add_argument(
        "--min-time",
        type=float,
        default=0.5,
        help="minimum seconds per run-time benchmark repetition (default: %(default)s)",
    )
    run.add_argument(
        "--filter",
        help="regex, passed to --benchmark_filter, to run only some run-time benchmarks",
    )
    run.add_argument(
        "--buildtime-repetitions",
        type=int,
        default=3,
        help="build time benchmark repetitions (default: %(default)s)",
    )

    render = argparse.ArgumentParser(add_help=False)
    render.add_argument(
        "--output-dir",
        help="write the charts under this directory, with the same relative "
        "paths as in the repository (default: overwrite the charts in the repository)",
    )

    subparsers = parser.add_subparsers(dest="command", required=True)
    for name, parents, function, text in (
        ("build", [common, build], stage_build, "configure and build"),
        ("run", [common, build, run], stage_run, "run, and save the results"),
        ("render", [common, render], stage_render, "print tables, write charts"),
        ("all", [common, build, run, render], stage_all, "build, run and render"),
    ):
        subparser = subparsers.add_parser(name, parents=parents, help=text)
        subparser.set_defaults(function=function)
    return parser


def main():
    args = make_parser().parse_args()
    args.function(args)


if __name__ == "__main__":
    main()
