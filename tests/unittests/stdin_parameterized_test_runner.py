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
    return None


exe_suffix = '.exe' if os.name == 'nt' else ''
script_dir = os.path.abspath(os.path.dirname(__file__))
stdin_parameterized_test = find_file(f'scn_stdin_parameterized_test*{exe_suffix}', script_dir)


def parse_output_line(line):
    bytes = []
    for byte in line.split(' '):
        if len(byte) == 0:
            continue
        bytes.append(chr(int(byte, 16)))
    return ''.join(bytes)


def compare_output(idx, output, expected_parsed, expected_leftovers):
    lines = output.splitlines()
    if len(lines) != 2:
        print(f'Test idx {idx} failed:\nMalformed output:\n"{output}"')
        return False
    parsed = parse_output_line(lines[0])
    leftovers = parse_output_line(lines[1])
    if parsed != expected_parsed:
        print(f'Test idx {idx} failed:\nExpected output:\n"{expected_parsed}"\nGot:\n"{parsed}"')
        return False
    if leftovers != expected_leftovers:
        print(f'Test idx {idx} failed:\nExpected leftovers:\n"{expected_leftovers}"\nGot:\n"{leftovers}"')
        return False
    print(f'Test idx {idx} succeeded')
    return True


def do_test_with_method(method_param, idx, type_param, format_string, input, expected_success, expected_parsed,
                        expected_leftovers):
    command_line = [stdin_parameterized_test, type_param, method_param, format_string]
    print('Running:', command_line)
    result = subprocess.run(command_line, shell=False,
                            input=input, text=True,
                            capture_output=True)

    if result.returncode != 0 and result.returncode != 1:
        print(
            f'Test idx {idx} (method #{method_param}) failed:\nexit code: {result.returncode}\nstderr:\n{result.stderr}')
        return False

    if expected_success:
        if result.returncode == 0:
            return compare_output(idx, result.stdout, expected_parsed, expected_leftovers)
        else:
            print(
                f'Test idx {idx} (method #{method_param}) failed:\nExpected success, got exit code {result.returncode}\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}')
            return False
    else:
        if result.returncode == 1:
            return compare_output(idx, result.stdout, expected_parsed, expected_leftovers)
        else:
            print(
                f'Test idx {idx} (method #{method_param}) failed:\nExpected failure, got success\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}')
            return False


def do_test(idx, type, format_string, input, expected_success, expected_parsed, expected_leftovers):
    if type == 'string':
        type_param = '0'
    elif type == 'int':
        type_param = '1'
    else:
        raise ValueError(f'Unexpected value for `type`: {type}')

    success = True
    for method in range(4):
        if not do_test_with_method(str(method), idx, type_param, format_string, input, expected_success, expected_parsed,
                                   expected_leftovers):
            success = False
    return success


tests = [
    {'type': 'string', 'format_string': '{}', 'input': 'Hello!', 'expected_success': True, 'expected_parsed': 'Hello!',
     'expected_leftovers': ''},
    {'type': 'int', 'format_string': '{}', 'input': '123', 'expected_success': True, 'expected_parsed': '123',
     'expected_leftovers': ''},
    {'type': 'int', 'format_string': '{}', 'input': 'foo', 'expected_success': False, 'expected_parsed': '',
     'expected_leftovers': 'foo'},
    {'type': 'int', 'format_string': '{}', 'input': '123foo', 'expected_success': True, 'expected_parsed': '123',
     'expected_leftovers': 'foo'},
]

success = True
for idx, test in enumerate(tests):
    if not do_test(idx, **test):
        success = False
if not success:
    sys.exit(1)
