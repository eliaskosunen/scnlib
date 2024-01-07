#!/usr/bin/env python

import fnmatch
import os
import pathlib
import subprocess


def find_file(pattern, path):
    for root, dirs, files in os.walk(path):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                return os.path.join(root, name)
    return None


script_dir = os.path.abspath(os.path.dirname(__file__))
examples_bin = find_file('scn_example_*', script_dir)
examples_dir = os.path.dirname(examples_bin)
examples_file_extension = ''.join(pathlib.Path(examples_bin).suffixes)


def check(i, input, expected_output):
    result = subprocess.run([os.path.join(examples_dir, f"scn_example_{i}{examples_file_extension}")],
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
check(1, "foo", "What's your favorite number? Well, nevermind then.\n")

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
