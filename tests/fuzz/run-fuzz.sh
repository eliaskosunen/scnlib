#!/usr/bin/env bash

fuzzer=$1
if [ "$fuzzer" = "string_impl" ]; then
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

mkdir -p "$corpus_dir"
echo "Running $fuzzer_bin, with dictionary at $dict_file and seed corpus at $seed_dir, generating corpus at $corpus_dir"
echo Running command: $fuzzer_bin -dict="$dict_file" "$corpus_dir" "$seed_dir" -rss_limit_mb=4096
$fuzzer_bin -dict="$dict_file" "$corpus_dir" "$seed_dir" -rss_limit_mb=4096
