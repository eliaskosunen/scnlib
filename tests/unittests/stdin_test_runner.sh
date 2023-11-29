#!/usr/bin/env bash

find "$(dirname "$0")" -executable -type f -name "scn_stdin_test*" -print0 | \
  xargs -0 < "$(dirname "$0")/stdin_test_input.txt"
