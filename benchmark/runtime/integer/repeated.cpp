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
static void scan_int_repeated_scn(benchmark::State& state)
{
    auto data = get_integer_string<Int>();
    auto range = scn::scan_map_input_range(data);
    for (auto _ : state) {
        auto [result, i] = scn::scan<Int>(range, "{}");

        if (!result) {
            if (result.error() == scn::scan_error::end_of_range) {
                range = scn::scan_map_input_range(data);
            }
            else {
                state.SkipWithError("Scan error");
                break;
            }
        }
        else {
            benchmark::DoNotOptimize(i);
            range = SCN_MOVE(result.range());
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_repeated_scn, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scn, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scn, unsigned);

template <typename Int>
static void scan_int_repeated_scn_value(benchmark::State& state)
{
    auto data = get_integer_string<Int>();
    auto range = scn::scan_map_input_range(data);
    for (auto _ : state) {
        auto [result, i] = scn::scan_value<Int>(range);

        if (!result) {
            if (result.error() == scn::scan_error::end_of_range) {
                range = scn::scan_map_input_range(data);
            }
            else {
                state.SkipWithError("Scan error");
                break;
            }
        }
        else {
            benchmark::DoNotOptimize(i);
            range = SCN_MOVE(result.range());
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, unsigned);

template <typename Int>
static void scan_int_repeated_sstream(benchmark::State& state)
{
    auto data = get_integer_string<Int>();
    auto stream = std::istringstream(data);
    for (auto _ : state) {
        Int i{};
        stream >> i;

        if (stream.eof()) {
            stream = std::istringstream(data);
        }
        else if (stream.fail()) {
            state.SkipWithError("Scan error");
            break;
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, int);
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, unsigned);

template <typename Int>
static void scan_int_repeated_scanf(benchmark::State& state)
{
    auto data = get_integer_string<Int>();
    const char* ptr = data.data();

    for (auto _ : state) {
        Int i{};

        auto ret = sscanf_integral_n(ptr, i);
        if (ret != 1) {
            if (ret == EOF) {
                ptr = data.data();
                continue;
            }

            state.SkipWithError("Scan error");
            break;
        }
        benchmark::DoNotOptimize(i);
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, unsigned);

#if SCN_HAS_INTEGER_CHARCONV

template <typename Int>
static void scan_int_repeated_charconv(benchmark::State& state)
{
    auto data = get_integer_string<Int>();
    const char* ptr = data.data();

    for (auto _ : state) {
        Int i{};

        for (; std::isspace(*ptr) != 0; ++ptr) {}
        if (ptr == data.data() + data.size()) {
            ptr = data.data();
        }

        auto ret = std::from_chars(ptr, data.data() + data.size(), i);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Scan error");
            break;
        }
        ptr = ret.ptr;
        benchmark::DoNotOptimize(i);
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_repeated_charconv, int);
BENCHMARK_TEMPLATE(scan_int_repeated_charconv, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_charconv, unsigned);

#endif  // SCN_HAS_INTEGER_CHARCONV
