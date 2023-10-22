// Copyright 2017 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is a part of scnlib:
//     https://github.com/eliaskosunen/scnlib

#include <scn/scan.h>
#include "benchmark_common.h"

#include <sstream>

static void bench_string_scn_stringview(benchmark::State& state)
{
    std::string_view input{"qwertyuiopasdfghjklzxcvbnm foobar"};
    for (auto _ : state) {
        if (auto result = scn::scan<std::string_view>(input, "{}")) {
            benchmark::DoNotOptimize(result->value());
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}
BENCHMARK(bench_string_scn_stringview);

static void bench_string_scn_string(benchmark::State& state)
{
    std::string_view input{"qwertyuiopasdfghjklzxcvbnm foobar"};
    for (auto _ : state) {
        if (auto result = scn::scan<std::string>(input, "{}")) {
            benchmark::DoNotOptimize(SCN_MOVE(result->value()));
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}
BENCHMARK(bench_string_scn_string);

static void bench_string_scn_transcoding(benchmark::State& state)
{
    std::string_view input{"qwertyuiopasdfghjklzxcvbnm foobar"};
    for (auto _ : state) {
        if (auto result = scn::scan<std::wstring>(input, "{}")) {
            benchmark::DoNotOptimize(SCN_MOVE(result->value()));
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}
BENCHMARK(bench_string_scn_transcoding);

static void bench_string_scn_iostream(benchmark::State& state)
{
    std::string_view input{"qwertyuiopasdfghjklzxcvbnm foobar"};
    for (auto _ : state) {
        std::istringstream iss{std::string{input}};
        std::string val{};
        if (!(iss >> val)) {
            state.SkipWithError("Failed scan");
            break;
        }
        benchmark::DoNotOptimize(SCN_MOVE(val));
    }
}
BENCHMARK(bench_string_scn_iostream);
