#!/usr/bin/env python3

import fnmatch
import os
import sys
import subprocess


def find_file(pattern, path):
    for root, dirs, files in os.walk(path):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                return os.path.join(root, name)
    raise RuntimeError(f"Couldn't find pattern '{pattern}' in {path}")


if os.name == 'nt':
    examples_file_extension = '.exe'
else:
    examples_file_extension = ''

cmd_args = sys.argv[1:]
if len(cmd_args) != 1:
    raise RuntimeError(
        f"Expected a single command-line argument, containing path to example binaries, got {cmd_args} instead")
examples_root_dir = os.path.normpath(cmd_args[0])
examples_bin = find_file(f"scn_example*{examples_file_extension}", examples_root_dir)
examples_dir = os.path.dirname(examples_bin)


def check(i, input, expected_output):
    path = os.path.join(examples_dir, f"scn_example_{i}{examples_file_extension}")
    print(f"Invoking {path}")
    result = subprocess.run([path],
                            shell=True,
                            input=input,
                            capture_output=True,
                            text=True)
    if result.returncode != 0:
        print(f"scn_example_{i} stdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        result.check_returncode()
    if result.stdout != expected_output:
        raise RuntimeError(
            f"scn_example_{i} failed with incorrect output:\n{result.stdout}\nExpected:\n{expected_output}")
    print(f"scn_example_{i} successful")


check(1, "42", "What's your favorite number? 42, interesting\n")
check(1, "foo", "What's your favorite number? Well, never mind then.\n")

check(2, "John Doe", "Input your full name:\nHello, John Doe")

check(3, "", """1
2
3
4
5
6
7
8
9
11
22
33
44
55
66
77
88
99
111
222
333
444
555
666
777
888
999
""")

check(4, "", "[{1: 2, 3: 4}, {5: 6}]")

check(5, "123 456", "Write two integers:\nTwo integers: 123 456\n")
check(5, "123 abc", "Write two integers:\nFirst integer: 123, rest of the line:  abc")
check(5, "abc def", "Write two integers:\nEntire line: abc def")
