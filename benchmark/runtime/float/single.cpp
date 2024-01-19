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
static void scan_float_single_scn(benchmark::State& state)
{
    single_state<Float> s{get_float_list<Float>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        if (auto result = scn::scan<Float>(*s.it, "{}"); !result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        else {
            s.push(result->value());
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_single_scn, float);
BENCHMARK_TEMPLATE(scan_float_single_scn, double);
BENCHMARK_TEMPLATE(scan_float_single_scn, long double);

template <typename Float>
static void scan_float_single_scn_value(benchmark::State& state)
{
    single_state<Float> s{get_float_list<Float>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        if (auto result = scn::scan_value<Float>(*s.it); !result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        else {
            s.push(result->value());
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_single_scn_value, float);
BENCHMARK_TEMPLATE(scan_float_single_scn_value, double);
BENCHMARK_TEMPLATE(scan_float_single_scn_value, long double);

template <typename Float>
static void scan_float_single_sstream(benchmark::State& state)
{
    single_state<Float> s{get_float_list<Float>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        Float f{};
        std::istringstream iss{*s.it};
        iss >> f;

        if (iss.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        s.push(f);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_single_sstream, float);
BENCHMARK_TEMPLATE(scan_float_single_sstream, double);
BENCHMARK_TEMPLATE(scan_float_single_sstream, long double);

template <typename Float>
static void scan_float_single_scanf(benchmark::State& state)
{
    single_state<Float> s{get_float_list<Float>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        Float f{};
        auto ret = sscanf_float(s.it->c_str(), f);
        if (ret != 1) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        s.push(f);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_single_scanf, float);
BENCHMARK_TEMPLATE(scan_float_single_scanf, double);
BENCHMARK_TEMPLATE(scan_float_single_scanf, long double);

template <typename Float>
static void scan_float_single_strtod(benchmark::State& state)
{
    single_state<Float> s{get_float_list<Float>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        Float f{};
        auto ret = strtod_float(s.it->c_str(), f);
        if (!ret) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        s.push(f);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_single_strtod, float);
BENCHMARK_TEMPLATE(scan_float_single_strtod, double);
BENCHMARK_TEMPLATE(scan_float_single_strtod, long double);

#if SCN_HAS_FLOAT_CHARCONV

template <typename Float>
static void scan_float_single_charconv(benchmark::State& state)
{
    single_state<Float> s{get_float_list<Float>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        Float f{};
        auto ret =
            std::from_chars(s.it->data(), s.it->data() + s.it->size(), f);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        s.push(f);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_single_charconv, float);
BENCHMARK_TEMPLATE(scan_float_single_charconv, double);
BENCHMARK_TEMPLATE(scan_float_single_charconv, long double);

#endif  // SCN_HAS_FLOAT_CHARCONV

template <typename Float>
static void scan_float_single_fastfloat(benchmark::State& state)
{
    single_state<Float> s{get_float_list<Float>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        Float f{};
        auto ret = fast_float::from_chars(s.it->data(),
                                          s.it->data() + s.it->size(), f);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        s.push(f);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_float_single_fastfloat, float);
BENCHMARK_TEMPLATE(scan_float_single_fastfloat, double);
BENCHMARK_TEMPLATE(scan_float_single_fastfloat, long double);
