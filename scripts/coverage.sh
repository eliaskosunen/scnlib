#!/usr/bin/env bash

# Clean up
rm -rf ./coverage-*.info ./coverage-html
lcov --zerocounters --directory .

# Build
cmake --build . --target scn_tests --parallel
cmake --build . --target scn_impl_tests --parallel

# Baseline lcov
lcov --capture --initial --directory . --output-file coverage-base.info --ignore-errors inconsistent

# Run tests
ctest --output-on-failure

# Capture and combine lcov data
lcov --capture --directory . --output-file coverage-test.info --ignore-errors inconsistent
lcov --add-tracefile coverage-base.info --add-tracefile coverage-test.info --output-file coverage-total.info --ignore-errors inconsistent

# Filter lcov data
lcov --remove coverage-total.info \
  '/usr/*' '*/tests/*' '*/examples/*' '*/benchmark/*' '*/src/scn/impl/external/*' '*/include/scn/util/expected_impl.h' '*/_deps/*' \
  --output-file coverage-filtered.info --ignore-errors inconsistent

# Display summary
lcov --list coverage-filtered.info --ignore-errors inconsistent

# Generate html
mkdir coverage-html
genhtml --prefix $(dirname $(pwd)) coverage-filtered.info --legend --output-directory=coverage-html

# See results:
# firefox ./coverage-html/index.html
