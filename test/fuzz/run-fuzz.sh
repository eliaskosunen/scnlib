#!/usr/bin/env bash

fuzzer=$1
if [[ $fuzzer = "roundtrip" ]]; then
  data="int"
elif [[ $fuzzer = "getline" ]]; then
  data="string"
else
  data=$fuzzer
fi

repo_dir=$(git rev-parse --show-toplevel)
data_dir=$repo_dir/test/fuzz
script_dir=$(dirname "$(readlink -f "$0")")

mkdir "$script_dir/corpus-$fuzzer"
"$script_dir/fuzz-$fuzzer" -dict="$data_dir/dict-$data.txt" "$script_dir/corpus-$fuzzer" "$data_dir/seed-corpora/$data"
