#!/usr/bin/env bash

# Run from the build directory (generated headers and libscn are looked up there)
script_dir="$(cd "$(dirname "$0")" && pwd)"
include_dir="${SCN_INCLUDE_DIR:-${script_dir}/../../include}"

run() {
  file=$1
  shift

  echo "$file"
  /usr/bin/time c++ -x c++ -I"${include_dir}" -Iinclude -L. "${script_dir}/${file}" -w "$@"
}

run_benchmarks() {
  run empty.cpp "$@"
  run cstdio.cpp "$@"
  run iostream.cpp "$@"
  run scnlib.cpp -lscn "$@"
}

echo "Debug"
run_benchmarks -g

echo "Release"
run_benchmarks -O3 -DNDEBUG
