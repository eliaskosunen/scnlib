#!/usr/bin/env bash

set -xeu

mkdir build
cd build

cmake .. \
  -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=23 \
  -DSCN_DISABLE_TOP_PROJECT=ON -DSCN_FUZZING=ON \
  -DSCN_FUZZING_LDFLAGS="$LIB_FUZZING_ENGINE" \
  -DSCN_REGEX_BACKEND=re2

cmake --build .

fuzz_src_dir="$(pwd)/../tests/fuzz"

# Dictionaries and seed corpora
copy_target() {
  target="$1"
  data_to_use="$2"

  cp "$(pwd)/tests/fuzz/scn_fuzz_$target" "$OUT"

  cp "$fuzz_src_dir/dictionaries/$data_to_use.txt" "$OUT/scn_fuzz_$target.dict"

  zip "$OUT/scn_fuzz_${target}_seed_corpus.zip" "$fuzz_src_dir"/seed-corpora/"$data_to_use"/*
}
copy_target chrono chrono
copy_target float float
copy_target format format
copy_target int int
copy_target string string
copy_target string_impl string

# TODO: Currently fails in CI, with "Failed to sync with underlying source",
# possibly a read-only filesystem?
#copy_target file string
