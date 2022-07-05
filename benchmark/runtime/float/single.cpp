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

#include <benchmark/benchmark.h>

#include "float_bench.h"

#include <charconv>

template <typename Float>
static void scan_float_single_scn(benchmark::State& state)
{
    const auto& source = get_float_list<float>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        if (auto [result, f] = scn::scan<Float>(*it, "{}"); !result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        else {
            benchmark::DoNotOptimize(f);
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_single_scn, float);
BENCHMARK_TEMPLATE(scan_float_single_scn, double);
BENCHMARK_TEMPLATE(scan_float_single_scn, long double);

template <typename Float>
static void scan_float_single_scn_value(benchmark::State& state)
{
    const auto& source = get_float_list<Float>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        if (auto [result, f] = scn::scan_value<Float>(*it); !result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        else {
            benchmark::DoNotOptimize(f);
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_single_scn_value, float);
BENCHMARK_TEMPLATE(scan_float_single_scn_value, double);
BENCHMARK_TEMPLATE(scan_float_single_scn_value, long double);

template <typename Float>
static void scan_float_single_sstream(benchmark::State& state)
{
    const auto& source = get_float_list<Float>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        Float f;
        std::istringstream iss{*it};
        iss >> f;

        if (iss.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        benchmark::DoNotOptimize(f);
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_single_sstream, float);
BENCHMARK_TEMPLATE(scan_float_single_sstream, double);
BENCHMARK_TEMPLATE(scan_float_single_sstream, long double);

template <typename Float>
static void scan_float_single_scanf(benchmark::State& state)
{
    const auto& source = get_float_list<Float>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        Float f;
        auto ret = sscanf_float(it->c_str(), f);
        if (ret != 1) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        benchmark::DoNotOptimize(f);
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_single_scanf, float);
BENCHMARK_TEMPLATE(scan_float_single_scanf, double);
BENCHMARK_TEMPLATE(scan_float_single_scanf, long double);

template <typename Float>
static void scan_float_single_charconv(benchmark::State& state)
{
    const auto& source = get_float_list<Float>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        Float f;
        auto ret = std::from_chars(it->data(), it->data() + it->size(), f);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        benchmark::DoNotOptimize(f);
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_single_charconv, float);
BENCHMARK_TEMPLATE(scan_float_single_charconv, double);
BENCHMARK_TEMPLATE(scan_float_single_charconv, long double);
