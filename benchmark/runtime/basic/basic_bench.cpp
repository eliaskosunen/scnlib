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

#if SCN_HAS_INTEGER_CHARCONV
#include <charconv>
#endif

static void bench_basic_scn(benchmark::State& state)
{
    std::string_view input{"123"};
    for (auto _ : state) {
        if (auto result = scn::scan<int>(input, "{}")) {
            benchmark::DoNotOptimize(SCN_MOVE(result->value()));
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}
BENCHMARK(bench_basic_scn);

static void bench_basic_scn_withoptions(benchmark::State& state)
{
    std::string_view input{"123"};
    for (auto _ : state) {
        if (auto result = scn::scan<int>(input, "{:i}")) {
            benchmark::DoNotOptimize(SCN_MOVE(result->value()));
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}
BENCHMARK(bench_basic_scn_withoptions);

#if !SCN_DISABLE_LOCALE
static void bench_basic_scn_withlocale(benchmark::State& state)
{
    std::string_view input{"123"};
    auto loc = std::locale{};
    for (auto _ : state) {
        if (auto result = scn::scan<int>(loc, input, "{}")) {
            benchmark::DoNotOptimize(SCN_MOVE(result->value()));
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}
BENCHMARK(bench_basic_scn_withlocale);

static void bench_basic_scn_localized(benchmark::State& state)
{
    std::string_view input{"123"};
    auto loc = std::locale{};
    for (auto _ : state) {
        if (auto result = scn::scan<int>(loc, input, "{:L}")) {
            benchmark::DoNotOptimize(SCN_MOVE(result->value()));
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}
BENCHMARK(bench_basic_scn_localized);
#endif

static void bench_basic_scn_value(benchmark::State& state)
{
    std::string_view input{"123"};
    for (auto _ : state) {
        if (auto result = scn::scan_value<int>(input)) {
            benchmark::DoNotOptimize(SCN_MOVE(result->value()));
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}
BENCHMARK(bench_basic_scn_value);

#if SCN_HAS_INTEGER_CHARCONV

static void bench_basic_from_chars(benchmark::State& state)
{
    std::string_view input{"123"};
    for (auto _ : state) {
        int i{};
        if (auto res =
                std::from_chars(input.data(), input.data() + input.size(), i);
            res.ec == std::errc{}) {
            benchmark::DoNotOptimize(SCN_MOVE(i));
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}
BENCHMARK(bench_basic_from_chars);

#endif  // SCN_HAS_INTEGER_CHARCONV

static void bench_basic_scanf(benchmark::State& state)
{
    std::string input{"123"};
    for (auto _ : state) {
        int i{};
        if (auto res = std::sscanf(input.c_str(), "%i", &i); res != 0) {
            benchmark::DoNotOptimize(SCN_MOVE(i));
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}
BENCHMARK(bench_basic_scanf);

static void bench_basic_strtol(benchmark::State& state)
{
    std::string input{"123"};
    for (auto _ : state) {
        auto prev_errno = errno;
        errno = 0;

        int i{};
        char* end{};
        auto res = std::strtol(input.c_str(), &end, 10);
        if (errno != 0 || end != input.data() + input.size()) {
            state.SkipWithError("Failed scan");
            errno = prev_errno;
            break;
        }
        benchmark::DoNotOptimize(SCN_MOVE(i));

        errno = prev_errno;
    }
}
BENCHMARK(bench_basic_strtol);

static void bench_basic_sstream(benchmark::State& state)
{
    std::string input{"123"};
    for (auto _ : state) {
        std::istringstream ss{input};
        int i{};
        if (ss >> i) {
            benchmark::DoNotOptimize(SCN_MOVE(i));
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}
BENCHMARK(bench_basic_sstream);

BENCHMARK_MAIN();
