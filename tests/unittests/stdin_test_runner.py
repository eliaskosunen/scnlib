#!/usr/bin/env python3

import fnmatch
import os
import subprocess


def find_file(pattern, path):
    for root, dirs, files in os.walk(path):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                return os.path.join(root, name)
    return None


script_dir = os.path.abspath(os.path.dirname(__file__))
stdin_test = find_file('scn_stdin_test*', script_dir)

with open(os.path.join(script_dir, 'stdin_test_input.txt'), 'r') as input_file:
    result = subprocess.run([stdin_test], shell=True, stdin=input_file, text=True, capture_output=True)
    if result.returncode != 0:
        print(f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        result.check_returncode()
    print(f"Output:\n{result.stdout}")
