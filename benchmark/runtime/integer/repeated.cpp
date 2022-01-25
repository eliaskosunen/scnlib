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

#include "bench_int.h"

template <typename Int>
static void scan_int_repeated_scn(benchmark::State& state)
{
    auto data = stringified_integer_list<Int>();
    Int i{};
    auto result = scn::make_result(data);
    for (auto _ : state) {
        result = scn::scan(result.range(), "{}", i);

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
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_repeated_scn, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scn, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scn, unsigned);

template <typename Int>
static void scan_int_repeated_scn_default(benchmark::State& state)
{
    auto data = stringified_integer_list<Int>();
    Int i{};
    auto result = scn::make_result(data);
    for (auto _ : state) {
        result = scn::scan_default(result.range(), i);

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
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_repeated_scn_default, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_default, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_default, unsigned);

template <typename Int>
static void scan_int_repeated_scn_value(benchmark::State& state)
{
    auto data = stringified_integer_list<Int>();
    auto result = scn::make_result<scn::expected<Int>>(data);
    for (auto _ : state) {
        result = scn::scan_value<Int>(result.range());

        if (!result) {
            if (result.error() == scn::error::end_of_range) {
                result = scn::make_result<scn::expected<Int>>(data);
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, unsigned);

template <typename Int>
static void scan_int_repeated_sstream(benchmark::State& state)
{
    auto data = stringified_integer_list<Int>();
    auto stream = std::istringstream(data);
    Int i{};
    for (auto _ : state) {
        stream >> i;

        if (stream.eof()) {
            stream = std::istringstream(data);
        }
        if (stream.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, int);
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, unsigned);

template <typename Int>
static void scan_int_repeated_scanf(benchmark::State& state)
{
    auto data = stringified_integer_list<Int>();
    auto ptr = &data[0];
    Int i{};
    for (auto _ : state) {
        auto ret = scanf_integral_n(ptr, i);

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
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, unsigned);
