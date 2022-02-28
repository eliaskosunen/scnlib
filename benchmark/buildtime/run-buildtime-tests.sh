#!/usr/bin/env bash

run () {
  file=$1
  shift

  echo "$file"
  /usr/bin/time c++ -x c++ -I../include -I../src -I../src/deps/fast_float/single_include -L$(pwd) "./benchmark/buildtime/${file}" -w "$@"
}

run_tests() {
  run empty.cpp "$@"
  run cstdio.cpp "$@"
  run iostream.cpp "$@"
  run scnlib.cpp -lscn "$@"
  run scnlib.cpp -DSCN_HEADER_ONLY=1 "$@"
}

echo "Debug"
run_tests -g

echo "Release"
run_tests -O3 -DNDEBUG
