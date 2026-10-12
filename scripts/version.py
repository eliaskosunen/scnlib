#!/usr/bin/env python3

# Copyright 2017 Elias Kosunen
# SPDX-License-Identifier: Apache-2.0

# This file is adapted from a similar script from toml++:
# https://github.com/marzer/tomlplusplus/blob/master/tools/version.py
# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
# toml++ is licensed under the MIT license

import sys
import re
from argparse import ArgumentParser
from pathlib import Path


def read_text_file(path):
    print(rf'Reading {path}')
    with open(path, r'r', encoding=r'utf-8') as f:
        return f.read()


def write_text_file(path, text):
    print(rf'Writing {path}')
    with open(path, r'w', encoding=r'utf-8', newline='\n') as f:
        f.write(text)


if __name__ == '__main__':

    args = ArgumentParser(r'version.py', description=r'Sets the project version in all the necessary places.')
    args.add_argument(r'version', type=str)
    args = args.parse_args()

    version = re.fullmatch(r'\s*[vV]?\s*([0-9]+)\s*[.,;]+\s*([0-9]+)\s*[.,;]+\s*([0-9]+)\s*', args.version)
    if not version:
        print(rf"Couldn't parse version triplet from '{args.version}'", file=sys.stderr)
        sys.exit(1)
    version = (int(version[1]), int(version[2]), int(version[3]))
    version_str = rf'{version[0]}.{version[1]}.{version[2]}'
    print(rf'version: {version_str}')

    root = Path(__file__).parent.parent.resolve()

    # (file, pattern, replacement); every pattern must match at least once
    substitutions = [
        (r'CMakeLists.txt',
         r'''(?<![\w])((?<!cmake_minimum_required\()VERSION\s+)[0-9]+[.][0-9]+[.][0-9]+\b''',
         rf'\g<1>{version_str}'),
        (r'CMakeLists.txt',
         r'''(?<![\w])(SOVERSION\s+)[0-9]+[.][0-9]+[.][0-9]+\b''',
         rf'\g<1>{version_str}'),
        (r'include/scn/macros.h',
         r'''(#\s*define\s+SCN_VERSION\s+)SCN_COMPILER\(\s*[0-9]+\s*,\s*[0-9]+\s*,\s*[0-9]+\s*\)''',
         rf'\g<1>SCN_COMPILER({version[0]}, {version[1]}, {version[2]})'),
        (r'docs/pages/mainpage.md',
         r'''(GIT_TAG\s+)(?:v\s*)?[0-9]+[.][0-9]+[.][0-9]+\b''',
         rf'\g<1>v{version_str}'),
    ]

    texts = {}
    failed = False
    for file, pattern, replacement in substitutions:
        path = root / file
        if path not in texts:
            texts[path] = read_text_file(path)
        texts[path], count = re.subn(pattern, replacement, texts[path])
        if count == 0:
            print(rf"error: {file}: pattern didn't match anything: {pattern}", file=sys.stderr)
            failed = True
    if failed:
        sys.exit(1)

    for path, text in texts.items():
        write_text_file(path, text)

    print('Remember to also:')
    print(' - Add an entry to CHANGELOG.md')
    print(' - Update definition of SCN_BEGIN_NAMESPACE, if necessary')
