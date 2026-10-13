#!/usr/bin/env bash

fuzzer=$1
if [ "$fuzzer" = "string_impl" ]; then
  data="string"
elif [ "$fuzzer" = "file" ]; then
  data="string"
else
  data=$fuzzer
fi

repo_dir=$(git rev-parse --show-toplevel)
data_dir=$repo_dir/tests/fuzz
script_dir=$(dirname "$(readlink -f $0)")

fuzzer_bin=$script_dir/scn_fuzz_$fuzzer
dict_file=$data_dir/dictionaries/$data.txt
corpus_dir=$script_dir/corpora/$fuzzer
seed_dir=$data_dir/seed-corpora/$data

fuzzer_args=()
if [ -f "$dict_file" ]; then
  fuzzer_args+=("-dict=$dict_file")
fi
corpus_args=("$corpus_dir")
if [ -d "$seed_dir" ]; then
  corpus_args+=("$seed_dir")
fi

mkdir -p "$corpus_dir"
echo "Running $fuzzer_bin, with dictionary at $dict_file and seed corpus at $seed_dir, generating corpus at $corpus_dir"
echo Running command: $fuzzer_bin "${fuzzer_args[@]}" "${corpus_args[@]}" -rss_limit_mb=4096
$fuzzer_bin "${fuzzer_args[@]}" "${corpus_args[@]}" -rss_limit_mb=4096
