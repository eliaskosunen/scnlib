#!/usr/bin/env python

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
    result = subprocess.run([stdin_test], check=True, stdin=input_file, text=True, capture_output=True)
    print(f"Output:\n{result.stdout}")
