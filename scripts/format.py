#!/usr/bin/env python3

# Copyright 2017 Elias Kosunen
# SPDX-License-Identifier: Apache-2.0

import difflib
import fnmatch
import os
import re
import shutil
import subprocess
import sys
from argparse import ArgumentParser
from concurrent.futures import ThreadPoolExecutor

# Keep the versions in sync with scripts/format-requirements.txt
EXPECTED_VERSIONS = {
    "clang-format": "22.1.8",
    "ruff": "0.17.0",
    "gersemi": "0.29.2",
    "shfmt": "v3.14.1",
}

TOOL_ENVIRONMENT_VARIABLES = {
    "clang-format": "CLANG_FORMAT",
    "ruff": "RUFF",
    "gersemi": "GERSEMI",
    "shfmt": "SHFMT",
}

# Vendored third-party files, kept as-is to stay diffable against upstream
EXCLUDED_PATTERNS = ("cmake/icm_*.cmake",)

INSTALL_HINT = (
    "Install the pinned formatters into a virtual environment with:\n"
    "  pip install -r scripts/format-requirements.txt"
)


def formatter_command(tool, executable, path):
    if tool == "clang-format":
        return [executable, f"--assume-filename={path}"]
    if tool == "ruff":
        return [executable, "format", "--stdin-filename", path]
    if tool == "gersemi":
        return [executable, "--stdin-filepath", path, "--warnings-as-errors", "-"]
    return [executable, "--filename", path]


def tool_for_path(path):
    if any(fnmatch.fnmatchcase(path, pattern) for pattern in EXCLUDED_PATTERNS):
        return None
    name = path.rsplit("/", 1)[-1]
    if name == "CMakeLists.txt" or name.endswith(".cmake"):
        return "gersemi"
    if name.endswith((".cpp", ".h", ".cppm")):
        return "clang-format"
    if name.endswith(".py"):
        return "ruff"
    if name.endswith(".sh"):
        return "shfmt"
    return None


def run_git(*arguments, text=False):
    result = subprocess.run(
        ["git", *arguments], capture_output=True, text=text, check=False
    )
    if result.returncode != 0:
        message = result.stderr if text else result.stderr.decode(errors="replace")
        sys.exit(f"git {' '.join(arguments)} failed:\n{message}")
    return result.stdout


def selected_paths(args):
    if args.all:
        output = run_git("ls-files", "-z")
    elif args.rev is None:
        output = run_git("diff", "--cached", "--name-only", "-z", "--diff-filter=ACMR")
    else:
        revision = args.rev if ".." in args.rev else f"{args.rev}^!"
        output = run_git("diff", "--name-only", "-z", "--diff-filter=ACMR", revision)
    paths = output.decode(errors="surrogateescape").split("\0")
    return sorted({path for path in paths if path and tool_for_path(path)})


def find_tool(tool):
    variable = TOOL_ENVIRONMENT_VARIABLES[tool]
    requested = os.environ.get(variable, tool)
    executable = shutil.which(requested)
    expected = EXPECTED_VERSIONS[tool]
    if executable is None:
        return None, (
            f"{tool}: '{requested}' not found (expected version {expected}). "
            f"Set {variable} to override.\n{INSTALL_HINT}"
        )
    try:
        output = subprocess.run(
            [executable, "--version"], capture_output=True, text=True, check=False
        ).stdout
    except OSError as error:
        return None, f"{tool}: failed to run {executable}: {error}"
    if not re.search(rf"(?<![\w.]){re.escape(expected)}(?![\w.])", output):
        return None, (
            f"{tool}: expected version {expected}, found "
            f"{output.strip() or 'unknown'} ({executable}). "
            f"Set {variable} to override.\n{INSTALL_HINT}"
        )
    return executable, None


def read_content(path, from_index):
    if from_index:
        return run_git("show", f":{path}")
    if not os.path.isfile(path):
        return None
    with open(path, "rb") as f:
        return f.read()


def format_content(command, content):
    result = subprocess.run(command, input=content, capture_output=True, check=False)
    if result.returncode != 0:
        return None, result.stderr.decode(errors="replace").strip()
    return result.stdout, None


def process_file(path, executables, from_index):
    content = read_content(path, from_index)
    if content is None:
        return path, None, None, None
    tool = tool_for_path(path)
    formatted, error = format_content(
        formatter_command(tool, executables[tool], path), content
    )
    return path, content, formatted, error


def unified_diff(path, before, after):
    return "".join(
        difflib.unified_diff(
            before.decode(errors="replace").splitlines(keepends=True),
            after.decode(errors="replace").splitlines(keepends=True),
            fromfile=f"a/{path}",
            tofile=f"b/{path}",
        )
    )


def main():
    parser = ArgumentParser(
        "format.py",
        description="Checks or fixes the formatting of C++, Python, CMake and shell files.",
    )
    parser.add_argument(
        "--check", action="store_true", help="print diffs instead of writing files"
    )
    parser.add_argument("--all", action="store_true", help="process all tracked files")
    parser.add_argument(
        "rev",
        nargs="?",
        help="process files changed by a commit, or in a range (A..B); "
        "default: staged files",
    )
    args = parser.parse_args()
    if args.all and args.rev:
        parser.error("--all and REV are mutually exclusive")

    os.chdir(run_git("rev-parse", "--show-toplevel", text=True).strip())

    paths = selected_paths(args)
    if not paths:
        return 0

    executables = {}
    failures = []
    for tool in sorted({tool_for_path(path) for path in paths}):
        executable, problem = find_tool(tool)
        if problem:
            failures.append(problem)
        else:
            executables[tool] = executable
    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1

    from_index = args.check and not args.all and args.rev is None
    with ThreadPoolExecutor() as pool:
        results = list(
            pool.map(lambda path: process_file(path, executables, from_index), paths)
        )

    unformatted = []
    errored = []
    for path, content, formatted, error in results:
        if content is None:
            continue
        if error is not None:
            errored.append(path)
            print(f"error: {path}\n{error}", file=sys.stderr)
        elif formatted != content:
            unformatted.append(path)
            if args.check:
                print(unified_diff(path, content, formatted), end="")
            else:
                with open(path, "wb") as f:
                    f.write(formatted)

    verb = "unformatted" if args.check else "reformatted"
    for path in unformatted:
        print(f"{'would reformat' if args.check else 'reformatted'}: {path}")
    print(
        f"{len(results)} files checked, {len(unformatted)} {verb}, "
        f"{len(errored)} errors"
    )
    if errored or (args.check and unformatted):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
