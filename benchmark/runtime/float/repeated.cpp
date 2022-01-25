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
static void scan_float_repeated_scn(benchmark::State& state)
{
    auto data = stringified_float_list<Float>();
    Float f{};
    auto result = scn::make_result(data);
    for (auto _ : state) {
        result = scn::scan(result.range(), "{}", f);

        if (!result) {
            if (result.error() == scn::error::end_of_range) {
                result = scn::make_result(data);
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_repeated_scn, float);
BENCHMARK_TEMPLATE(scan_float_repeated_scn, double);

template <typename Float>
static void scan_float_repeated_scn_default(benchmark::State& state)
{
    auto data = stringified_float_list<Float>();
    Float f{};
    auto result = scn::make_result(data);
    for (auto _ : state) {
        result = scn::scan_default(result.range(), f);

        if (!result) {
            if (result.error() == scn::error::end_of_range) {
                result = scn::make_result(data);
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_repeated_scn_default, float);
BENCHMARK_TEMPLATE(scan_float_repeated_scn_default, double);
BENCHMARK_TEMPLATE(scan_float_repeated_scn_default, long double);

template <typename Float>
static void scan_float_repeated_scn_value(benchmark::State& state)
{
    auto data = stringified_float_list<Float>();
    auto result = scn::make_result<scn::expected<Float>>(data);
    for (auto _ : state) {
        result = scn::scan_value<Float>(result.range());

        if (!result) {
            if (result.error() == scn::error::end_of_range) {
                result = scn::make_result<scn::expected<Float>>(data);
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_repeated_scn_value, float);
BENCHMARK_TEMPLATE(scan_float_repeated_scn_value, double);
BENCHMARK_TEMPLATE(scan_float_repeated_scn_value, long double);

template <typename Float>
static void scan_float_repeated_sstream(benchmark::State& state)
{
    auto data = stringified_float_list<Float>();
    auto stream = std::istringstream(data);
    Float f{};
    for (auto _ : state) {
        stream >> f;

        if (stream.eof()) {
            stream = std::istringstream(data);
        }
        if (stream.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_repeated_sstream, float);
BENCHMARK_TEMPLATE(scan_float_repeated_sstream, double);
BENCHMARK_TEMPLATE(scan_float_repeated_sstream, long double);

template <typename Float>
static void scan_float_repeated_scanf(benchmark::State& state)
{
    auto data = stringified_float_list<Float>();
    auto ptr = &data[0];
    Float f{};
    for (auto _ : state) {
        auto ret = scanf_float_n(ptr, f);

        if (ret != 1) {
            if (ret == EOF) {
                ptr = &data[0];
                continue;
            }
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scan_float_repeated_scanf, float);
BENCHMARK_TEMPLATE(scan_float_repeated_scanf, double);
BENCHMARK_TEMPLATE(scan_float_repeated_scanf, long double);
