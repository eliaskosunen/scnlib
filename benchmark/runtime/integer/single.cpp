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
static void scan_int_single_scn(benchmark::State& state)
{
    auto source = stringified_integers_list<Int>();
    auto it = source.begin();
    Int i;
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        auto result = scn::scan(*it, "{}", i);

        if (!result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_single_scn, int);
BENCHMARK_TEMPLATE(scan_int_single_scn, long long);
BENCHMARK_TEMPLATE(scan_int_single_scn, unsigned);

template <typename Int>
static void scan_int_single_scn_default(benchmark::State& state)
{
    auto source = stringified_integers_list<Int>();
    auto it = source.begin();
    Int i;
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        auto result = scn::scan_default(*it, i);

        if (!result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_single_scn_default, int);
BENCHMARK_TEMPLATE(scan_int_single_scn_default, long long);
BENCHMARK_TEMPLATE(scan_int_single_scn_default, unsigned);

template <typename Int>
static void scan_int_single_scn_value(benchmark::State& state)
{
    auto source = stringified_integers_list<Int>();
    auto it = source.begin();
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        auto result = scn::scan_value<Int>(*it);

        if (!result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_single_scn_value, int);
BENCHMARK_TEMPLATE(scan_int_single_scn_value, long long);
BENCHMARK_TEMPLATE(scan_int_single_scn_value, unsigned);

template <typename Int>
static void scan_int_single_scn_parse(benchmark::State& state)
{
    auto source = stringified_integers_list<Int>();
    auto it = source.begin();
    Int i{};
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        auto result = scn::parse_integer<Int>(
            scn::string_view{it->data(), it->size()}, i);

        if (!result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_single_scn_parse, int);
BENCHMARK_TEMPLATE(scan_int_single_scn_parse, long long);
BENCHMARK_TEMPLATE(scan_int_single_scn_parse, unsigned);

template <typename Int>
static void scan_int_single_sstream(benchmark::State& state)
{
    auto source = stringified_integers_list<Int>();
    auto it = source.begin();
    Int i;
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        std::istringstream iss{*it};
        iss >> i;

        if (iss.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_single_sstream, int);
BENCHMARK_TEMPLATE(scan_int_single_sstream, long long);
BENCHMARK_TEMPLATE(scan_int_single_sstream, unsigned);

template <typename Int>
static void scan_int_single_scanf(benchmark::State& state)
{
    auto source = stringified_integers_list<Int>();
    auto it = source.begin();
    Int i;
    for (auto _ : state) {
        if (it == source.end()) {
            it = source.begin();
        }

        auto ret = scanf_integral(it->c_str(), i);
        if (ret != 1) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scan_int_single_scanf, int);
BENCHMARK_TEMPLATE(scan_int_single_scanf, long long);
BENCHMARK_TEMPLATE(scan_int_single_scanf, unsigned);
