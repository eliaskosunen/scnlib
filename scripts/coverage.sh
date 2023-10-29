#!/usr/bin/env bash

# Clean up
rm -rf ./coverage-*.info ./coverage-html
lcov --zerocounters --directory .

# Build
cmake --build . --parallel

# Baseline lcov
lcov --capture --initial --directory . --output-file coverage-base.info

# Run tests
ctest --output-on-failure

# Capture and combine lcov data
lcov --capture --directory . --output-file coverage-test.info
lcov --add-tracefile coverage-base.info --add-tracefile coverage-test.info --output-file coverage-total.info

# Filter lcov data
lcov --remove coverage-total.info \
  '/usr/*' '*/tests/*' '*/examples/*' '*/benchmark/*' '*/include/scn/ranges/*' '*/src/deps/*' \
  --output-file coverage-filtered.info

# Display summary
lcov --list coverage-filtered.info

# Generate html
mkdir coverage-html
genhtml --prefix $(dirname $(pwd)) coverage-filtered.info --legend --output-directory=coverage-html

# See results:
# firefox ./coverage-html/index.html
