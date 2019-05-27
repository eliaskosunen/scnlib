#!/usr/bin/env python3

import subprocess
import sys
import os
import math
import shutil


def convert_size(size_bytes):
    if size_bytes == 0:
        return '0 B'
    size_name = ('B', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB')
    i = int(math.floor(math.log(size_bytes, 1024)))
    p = math.pow(1024, i)
    s = round(size_bytes / p, 2)
    return '{} {}'.format(s, size_name[i])


out = sys.argv[1]
stripped = out + '.stripped'

size_out = os.path.getsize(out)
hsize_out = convert_size(size_out)
print('{} {}'.format(size_out, hsize_out))

shutil.copyfile(out, stripped)
subprocess.run(['strip', stripped])

size_stripped = os.path.getsize(stripped)
hsize_stripped = convert_size(size_stripped)
print('{} {}'.format(size_stripped, hsize_stripped))

os.remove(stripped)
