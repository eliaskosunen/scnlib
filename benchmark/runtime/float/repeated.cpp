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

#include <fast_float/fast_float.h>

template <typename Float>
static void scan_float_repeated_scn(benchmark::State& state)
{
    repeated_state<Float> s{get_float_string<Float>()};

    for (auto _ : state) {
        auto result = scn::scan<Float>(s.view(), "{}");

        if (!result) {
            if (result.error() == scn::scan_error::end_of_input) {
                s.reset();
            }
            else {
                state.SkipWithError("Scan error");
                break;
            }
        }
        else {
            s.push(result->value());
            s.it = scn::detail::to_address(result->range().begin());
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_repeated_scn, float);
BENCHMARK_TEMPLATE(scan_float_repeated_scn, double);
BENCHMARK_TEMPLATE(scan_float_repeated_scn, long double);

template <typename Float>
static void scan_float_repeated_scn_value(benchmark::State& state)
{
    repeated_state<Float> s{get_float_string<Float>()};

    for (auto _ : state) {
        auto result = scn::scan_value<Float>(s.view());

        if (!result) {
            if (result.error() == scn::scan_error::end_of_input) {
                s.reset();
            }
            else {
                state.SkipWithError("Scan error");
                break;
            }
        }
        else {
            s.push(result->value());
            s.it = scn::detail::to_address(result->range().begin());
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_repeated_scn_value, float);
BENCHMARK_TEMPLATE(scan_float_repeated_scn_value, double);
BENCHMARK_TEMPLATE(scan_float_repeated_scn_value, long double);

template <typename Float>
static void scan_float_repeated_sstream(benchmark::State& state)
{
    repeated_state<Float> s{get_float_string<Float>()};
    std::istringstream stream{s.source};

    for (auto _ : state) {
        Float f{};
        stream >> f;

        if (stream.eof()) {
            stream = std::istringstream(s.source);
            s.reset();
        }
        else if (stream.fail()) {
            state.SkipWithError("Scan error");
            break;
        }
        else {
            s.push(f);
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_repeated_sstream, float);
BENCHMARK_TEMPLATE(scan_float_repeated_sstream, double);
BENCHMARK_TEMPLATE(scan_float_repeated_sstream, long double);

template <typename Float>
static void scan_float_repeated_scanf(benchmark::State& state)
{
    repeated_state<Float> s{get_float_string<Float>()};

    for (auto _ : state) {
        Float f{};

        auto ret = sscanf_float_n(s.it, f);
        if (ret != 1) {
            if (ret == EOF) {
                s.reset();
                continue;
            }

            state.SkipWithError("Scan error");
            break;
        }
        s.push(f);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_repeated_scanf, float);
BENCHMARK_TEMPLATE(scan_float_repeated_scanf, double);
BENCHMARK_TEMPLATE(scan_float_repeated_scanf, long double);

template <typename Float>
static void scan_float_repeated_strtod(benchmark::State& state)
{
    repeated_state<Float> s{get_float_string<Float>()};

    for (auto _ : state) {
        Float f{};
        s.skip_classic_ascii_space();

        auto ret = strtod_float_n(s.it, f);
        if (ret != 0) {
            if (ret == EOF) {
                s.reset();
                continue;
            }

            state.SkipWithError("Scan error");
            break;
        }
        s.push(f);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_repeated_strtod, float);
BENCHMARK_TEMPLATE(scan_float_repeated_strtod, double);
BENCHMARK_TEMPLATE(scan_float_repeated_strtod, long double);

#if SCN_HAS_FLOAT_CHARCONV

template <typename Float>
static void scan_float_repeated_charconv(benchmark::State& state)
{
    repeated_state<Float> s{get_float_string<Float>()};

    for (auto _ : state) {
        Float f{};
        s.skip_classic_ascii_space();

        auto ret = std::from_chars(s.view().begin(), s.view().end(), f);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Scan error");
            break;
        }
        s.it = ret.ptr;
        s.push(f);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_repeated_charconv, float);
BENCHMARK_TEMPLATE(scan_float_repeated_charconv, double);
BENCHMARK_TEMPLATE(scan_float_repeated_charconv, long double);

#endif  // SCN_HAS_FLOAT_CHARCONV

template <typename Float>
static void scan_float_repeated_fastfloat(benchmark::State& state)
{
    repeated_state<Float> s{get_float_string<Float>()};

    for (auto _ : state) {
        Float f{};
        s.skip_classic_ascii_space();

        auto ret = fast_float::from_chars(s.view().begin(), s.view().end(), f);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Scan error");
            break;
        }
        s.it = ret.ptr;
        s.push(f);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_repeated_fastfloat, float);
BENCHMARK_TEMPLATE(scan_float_repeated_fastfloat, double);
BENCHMARK_TEMPLATE(scan_float_repeated_fastfloat, long double);
