#!/usr/bin/env python3

import sys
import os
import pprint
import matplotlib.pyplot as plt

import bloat_test


def rounded_kb(bytes):
    x = round(bytes / 1024, 1)
    if x >= 100:
        return int(x)
    return x


def get_test_prettyname(test):
    names = {
        'bloat-control': 'empty',
        'bloat-scanf': '`std::scanf`',
        'bloat-iostream': '`std::istream`',
        'bloat-scnlib': '`scn::input`',
        'bloat-scnlib-header-only': '`scn::input` (header-only)',
        'bloat-scnlib-value': '`scn::scan_value`',
        'bloat-scnlib-value-header-only': '`scn::scan_value` (header-only)',
    }
    return names[test]


def process_test(exec_dir, test):
    size, stripped = bloat_test.run(os.path.join(exec_dir, test))
    return {'test': test, 'testPretty': get_test_prettyname(test).replace("`", ""),
            'testPrettyMD': get_test_prettyname(test),
            'size': size, 'stripped': stripped,
            'sizeH': bloat_test.convert_size(size), 'strippedH': bloat_test.convert_size(stripped),
            'sizeKiB': rounded_kb(size), 'strippedKiB': rounded_kb(stripped)}


def make_plot(results, title):
    results = list(reversed(results))

    names = list(map(lambda x: x['testPretty'].replace(' ', '\n'), results))
    plt.figure(figsize=(10, 10.5))
    plt.suptitle(f'Executable size benchmarks: {title}')

    a = plt.subplot(211)
    a.barh(names, list(map(lambda x: x['size'], results)))
    a.set_title('Executable size')
    plt.xlabel('Size in KiB')

    b = plt.subplot(212)
    b.barh(names, list(map(lambda x: x['stripped'], results)))
    b.set_title('Stripped size')
    plt.xlabel('Size in KiB')

    plt.show()


def main():
    exec_dir = sys.argv[1]
    title = sys.argv[2]
    pp = pprint.PrettyPrinter(indent=4)

    tests = [f for f
             in os.listdir(exec_dir)
             if os.path.isfile(os.path.join(exec_dir, f))
             and f.startswith('bloat-')
             and not f.endswith('stripped')
             and not f.endswith('.py')]
    presort_results = {item['test']: item for item in list(map(lambda test: process_test(exec_dir, test), tests))}
    results = [
        presort_results['bloat-control'],
        presort_results['bloat-scanf'],
        presort_results['bloat-iostream'],
        presort_results['bloat-scnlib'],
        presort_results['bloat-scnlib-header-only'],
        presort_results['bloat-scnlib-value'],
        presort_results['bloat-scnlib-value-header-only']
    ]

    first_column_width = len(max(results, key=lambda x: len(x['testPrettyMD']))['testPrettyMD'])
    execsize_txt = 'Executable size'
    stripsize_txt = 'Stripped size'

    print('Full results pretty-printed')
    pp.pprint(results)
    print('\n')

    print('Formatted as markdown table (sorted by size, asc)')
    print(f'| {"Method":{first_column_width}} | {execsize_txt} | {stripsize_txt} |')
    print(f'| {":":-<{first_column_width}} | {":":->{len(execsize_txt)}} | {":":->{len(stripsize_txt)}} |')
    for result in results:
        print(
            f'| {result["testPrettyMD"]:{first_column_width}} | {result["sizeKiB"]:>{len(execsize_txt)}} | {result["strippedKiB"]:>{len(stripsize_txt)}} |')
    print('\n')

    make_plot(results, title)


if __name__ == '__main__':
    main()
