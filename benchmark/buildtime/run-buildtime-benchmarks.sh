#!/usr/bin/env bash

run () {
  file=$1
  shift

  echo "$file"
  /usr/bin/time c++ -x c++ -I../include -L. "./benchmark/buildtime/${file}" -w "$@"
}

run_benchmarks () {
  run empty.cpp "$@"
  run cstdio.cpp "$@"
  run iostream.cpp "$@"
  run scnlib.cpp -lscn "$@"
}

echo "Debug"
run_benchmarks -g

echo "Release"
run_benchmarks -O3 -DNDEBUG
