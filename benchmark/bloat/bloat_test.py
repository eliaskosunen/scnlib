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
    return f'{s} {size_name[i]}'


def run(out):
    stripped = out + '.stripped'

    size_out = os.path.getsize(out)

    shutil.copyfile(out, stripped)
    subprocess.run(['strip', stripped])

    size_stripped = os.path.getsize(stripped)

    os.remove(stripped)

    return size_out, size_stripped

def main():
    a, b = run(sys.argv[1])
    print(f'{sys.argv[1]}:\nSize: {a} ({convert_size(a)})\nStripped: {b} ({convert_size(b)})')

if __name__ == '__main__':
    main()