#!/usr/bin/env bash

examples_bin_dir=$(find "$(dirname "$0")" -executable -type f -name "scn_example_*" -print0 | xargs -0 dirname | head -1)

check() {
  output=$(echo "$2" | "$examples_bin_dir/scn_example_$1" 2>&1)
  error_code=$?
  if [ "$error_code" -ne 0 ]; then
    printf "scn_example_$1 failed with exit code %d:\n%s" $error_code "$output"
    return;
  fi
  if [ "$output" != "$3" ]; then
    printf "scn_example_$1 failed with incorrect output:\n%s" "$output"
    exit 1
  else
    echo "scn_example_$1 successful"
  fi
}

check 1 "42" "What's your favorite number? 42, interesting"
check 1 "foo" "What's your favorite number? Well, nevermind then."

check 2 "John Doe" $'Input your full name:\nHello, John Doe'

check 3 "" '1
2
3
4
5
6
7
8
9
11
22
33
44
55
66
77
88
99
111
222
333
444
555
666
777
888
999'

check 4 "" "[{1: 2, 3: 4}, {5: 6}]"

check 5 "123 456" $'Write two integers:\nTwo integers: 123 456'
check 5 "123 abc" $'Write two integers:\nFirst integer: 123, rest of the line:  abc'
check 5 "abc def" $'Write two integers:\nEntire line: abc def'
