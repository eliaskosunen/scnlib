#!/usr/bin/env bash

set -xeu

mkdir build
cd build

cmake .. \
  -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=17 \
  -DSCN_DISABLE_TOP_PROJECT=ON -DSCN_FUZZING=ON \
  -DSCN_FUZZING_LDFLAGS="$LIB_FUZZING_ENGINE" \
  -DSCN_REGEX_BACKEND=re2

cmake --build .

# Binary targets
cp tests/fuzz/scn_fuzz_* "$OUT"

fuzz_src_dir="$(pwd)/../tests/fuzz"

# Dictionaries and seed corpora
copy_data() {
  cp "$fuzz_src_dir/dictionaries/$1.txt" "$OUT/scn_fuzz_$2.dict"

  zip "$OUT/scn_fuzz_$2_seed_corpus.zip" "$fuzz_src_dir"/seed-corpora/"$1"/*
}
copy_data float float
copy_data format format
copy_data int int
copy_data string string
copy_data string string_impl
