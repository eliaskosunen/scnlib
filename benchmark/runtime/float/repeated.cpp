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

#include "float_bench.h"

#if SCN_HAS_FLOAT_CHARCONV
#include <charconv>
#endif

template <typename Float>
static void scan_float_repeated_scn(benchmark::State& state)
{
    auto data = get_float_string<Float>();
    auto range = scn::scan_map_input_range(data);
    for (auto _ : state) {
        auto [result, f] = scn::scan<Float>(range, "{}");

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
            benchmark::DoNotOptimize(f);
            range = SCN_MOVE(result.range());
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_repeated_scn, float);
BENCHMARK_TEMPLATE(scan_float_repeated_scn, double);
BENCHMARK_TEMPLATE(scan_float_repeated_scn, long double);

template <typename Float>
static void scan_float_repeated_scn_value(benchmark::State& state)
{
    auto data = get_float_string<Float>();
    auto range = scn::scan_map_input_range(data);
    for (auto _ : state) {
        auto [result, f] = scn::scan_value<Float>(range);

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
            benchmark::DoNotOptimize(f);
            range = SCN_MOVE(result.range());
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_repeated_scn_value, float);
BENCHMARK_TEMPLATE(scan_float_repeated_scn_value, double);
BENCHMARK_TEMPLATE(scan_float_repeated_scn_value, long double);

template <typename Float>
static void scan_float_repeated_sstream(benchmark::State& state)
{
    auto data = get_float_string<Float>();
    auto stream = std::istringstream(data);
    for (auto _ : state) {
        Float f{};
        stream >> f;

        if (stream.eof()) {
            stream = std::istringstream(data);
        }
        else if (stream.fail()) {
            state.SkipWithError("Scan error");
            break;
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_repeated_sstream, float);
BENCHMARK_TEMPLATE(scan_float_repeated_sstream, double);
BENCHMARK_TEMPLATE(scan_float_repeated_sstream, long double);

template <typename Float>
static void scan_float_repeated_scanf(benchmark::State& state)
{
    auto data = get_float_string<Float>();
    const char* ptr = data.data();

    for (auto _ : state) {
        Float f{};

        auto ret = sscanf_float_n(ptr, f);
        if (ret != 1) {
            if (ret == EOF) {
                ptr = data.data();
                continue;
            }

            state.SkipWithError("Scan error");
            break;
        }
        benchmark::DoNotOptimize(f);
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_repeated_scanf, float);
BENCHMARK_TEMPLATE(scan_float_repeated_scanf, double);
BENCHMARK_TEMPLATE(scan_float_repeated_scanf, long double);

#if SCN_HAS_FLOAT_CHARCONV

template <typename Float>
static void scan_float_repeated_charconv(benchmark::State& state)
{
    auto data = get_float_string<Float>();
    const char* ptr = data.data();

    for (auto _ : state) {
        Float f{};

        for (; std::isspace(*ptr) != 0; ++ptr) {}
        if (ptr == data.data() + data.size()) {
            ptr = data.data();
        }

        auto ret = std::from_chars(ptr, data.data() + data.size(), f);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Scan error");
            break;
        }
        ptr = ret.ptr;
        benchmark::DoNotOptimize(f);
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_repeated_charconv, float);
BENCHMARK_TEMPLATE(scan_float_repeated_charconv, double);
BENCHMARK_TEMPLATE(scan_float_repeated_charconv, long double);

#endif  // SCN_HAS_FLOAT_CHARCONV
