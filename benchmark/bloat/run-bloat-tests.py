#!/usr/bin/env python3

import subprocess
import sys
import os
import pprint


exec_dir = sys.argv[1]
pp = pprint.PrettyPrinter(indent=4)

tests = [f for f
         in os.listdir(exec_dir)
         if os.path.isfile(os.path.join(exec_dir, f))
         and f.startswith('bloat-')
         and not f.endswith('stripped')
         and not f.endswith('.py')]

test_script = str(os.path.join(exec_dir, 'bloat-test.py'))
for test in tests:
    cp = subprocess.run([test_script, str(os.path.join(exec_dir, test))],
                        stdout=subprocess.PIPE, text=True)
    print('{}\n{}-----'.format(test, cp.stdout))
