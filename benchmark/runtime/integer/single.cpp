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

#include "benchmark_common.h"

#include "int_bench.h"

#if SCN_HAS_INTEGER_CHARCONV
#include <charconv>
#endif

template <typename Int>
static void scan_int_single_scn(benchmark::State& state)
{
    const auto& source = get_integer_list<Int>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        if (auto [result, i] = scn::scan<Int>(*it, "{}"); !result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        else {
            benchmark::DoNotOptimize(i);
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_single_scn, int);
BENCHMARK_TEMPLATE(scan_int_single_scn, long long);
BENCHMARK_TEMPLATE(scan_int_single_scn, unsigned);

template <typename Int>
static void scan_int_single_scn_value(benchmark::State& state)
{
    const auto& source = get_integer_list<Int>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        if (auto [result, i] = scn::scan_value<Int>(*it); !result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        else {
            benchmark::DoNotOptimize(i);
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_single_scn_value, int);
BENCHMARK_TEMPLATE(scan_int_single_scn_value, long long);
BENCHMARK_TEMPLATE(scan_int_single_scn_value, unsigned);

template <typename Int>
static void scan_int_single_sstream(benchmark::State& state)
{
    const auto& source = get_integer_list<Int>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        Int i;
        std::istringstream iss{*it};
        iss >> i;

        if (iss.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        benchmark::DoNotOptimize(i);
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_single_sstream, int);
BENCHMARK_TEMPLATE(scan_int_single_sstream, long long);
BENCHMARK_TEMPLATE(scan_int_single_sstream, unsigned);

template <typename Int>
static void scan_int_single_scanf(benchmark::State& state)
{
    const auto& source = get_integer_list<Int>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        Int i;
        auto ret = sscanf_integral(it->c_str(), i);
        if (ret != 1) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        benchmark::DoNotOptimize(i);
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_single_scanf, int);
BENCHMARK_TEMPLATE(scan_int_single_scanf, long long);
BENCHMARK_TEMPLATE(scan_int_single_scanf, unsigned);

#if SCN_HAS_INTEGER_CHARCONV

template <typename Int>
static void scan_int_single_charconv(benchmark::State& state)
{
    const auto& source = get_integer_list<Int>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        Int i;
        auto ret = std::from_chars(it->data(), it->data() + it->size(), i);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        benchmark::DoNotOptimize(i);
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_single_charconv, int);
BENCHMARK_TEMPLATE(scan_int_single_charconv, long long);
BENCHMARK_TEMPLATE(scan_int_single_charconv, unsigned);

#endif  // SCN_HAS_INTEGER_CHARCONV
