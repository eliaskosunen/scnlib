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

#include "bench_float.h"

template <typename Float>
static void scan_float_single_scn(benchmark::State& state)
{
    auto source = stringified_floats_list<Float>();
    auto it = source.begin();
    Float f;
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        auto result = scn::scan(*it, "{}", f);

        if (!result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_single_scn, float);
BENCHMARK_TEMPLATE(scan_float_single_scn, double);
BENCHMARK_TEMPLATE(scan_float_single_scn, long double);

template <typename Float>
static void scan_float_single_scn_default(benchmark::State& state)
{
    auto source = stringified_floats_list<Float>();
    auto it = source.begin();
    Float f;
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        auto result = scn::scan_default(*it, f);

        if (!result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_single_scn_default, float);
BENCHMARK_TEMPLATE(scan_float_single_scn_default, double);
BENCHMARK_TEMPLATE(scan_float_single_scn_default, long double);

template <typename Float>
static void scan_float_single_scn_value(benchmark::State& state)
{
    auto source = stringified_floats_list<Float>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        auto result = scn::scan_value<Float>(*it);

        if (!result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_single_scn_value, float);
BENCHMARK_TEMPLATE(scan_float_single_scn_value, double);
BENCHMARK_TEMPLATE(scan_float_single_scn_value, long double);

template <typename Float>
static void scan_float_single_scn_parse(benchmark::State& state)
{
    auto source = stringified_floats_list<Float>();
    auto it = source.begin();
    Float f{};
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        auto result = scn::parse_float<Float>(
            scn::string_view{it->data(), it->size()}, f);

        if (!result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_single_scn_parse, float);
BENCHMARK_TEMPLATE(scan_float_single_scn_parse, double);
BENCHMARK_TEMPLATE(scan_float_single_scn_parse, long double);

template <typename Float>
static void scan_float_single_sstream(benchmark::State& state)
{
    auto source = stringified_floats_list<Float>();
    auto it = source.begin();
    Float f;
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        std::istringstream iss{*it};
        iss >> f;

        if (iss.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
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
    auto source = stringified_floats_list<Float>();
    auto it = source.begin();
    Float f;
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        auto ret = scanf_float(it->c_str(), f);
        if (ret != 1) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_single_scanf, float);
BENCHMARK_TEMPLATE(scan_float_single_scanf, double);
BENCHMARK_TEMPLATE(scan_float_single_scanf, long double);
