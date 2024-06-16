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

#include <fast_float/fast_float.h>

template <typename Int>
static void scan_int_single_scn(benchmark::State& state)
{
    single_state<Int> s{get_integer_list<Int>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        if (auto result = scn::scan<Int>(*s.it, "{}"); !result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        else {
            s.push(result->value());
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_single_scn, int);
BENCHMARK_TEMPLATE(scan_int_single_scn, long long);
BENCHMARK_TEMPLATE(scan_int_single_scn, unsigned);

template <typename Int>
static void scan_int_single_scn_value(benchmark::State& state)
{
    single_state<Int> s{get_integer_list<Int>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        if (auto result = scn::scan_value<Int>(*s.it); !result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        else {
            s.push(result->value());
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_single_scn_value, int);
BENCHMARK_TEMPLATE(scan_int_single_scn_value, long long);
BENCHMARK_TEMPLATE(scan_int_single_scn_value, unsigned);

template <typename Int>
static void scan_int_single_scn_decimal(benchmark::State& state)
{
    single_state<Int> s{get_integer_list<Int>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        if (auto result = scn::scan<Int>(*s.it, "{:d}"); !result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        else {
            s.push(result->value());
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_single_scn_decimal, int);
BENCHMARK_TEMPLATE(scan_int_single_scn_decimal, long long);
BENCHMARK_TEMPLATE(scan_int_single_scn_decimal, unsigned);

template <typename Int>
static void scan_int_single_scn_int(benchmark::State& state)
{
    single_state<Int> s{get_integer_list<Int>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        if (auto result = scn::scan_int<Int>(*s.it); !result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        else {
            s.push(result->value());
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_single_scn_int, int);
BENCHMARK_TEMPLATE(scan_int_single_scn_int, long long);
BENCHMARK_TEMPLATE(scan_int_single_scn_int, unsigned);

// scan_int_exhaustive_valid requires little-endian
#if !SCN_IS_BIG_ENDIAN
template <typename Int>
static void scan_int_single_scn_int_exhaustive_valid(benchmark::State& state)
{
    single_state<Int> s{get_integer_list<Int>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        auto val = scn::scan_int_exhaustive_valid<Int>(*s.it);
        s.push(val);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_single_scn_int_exhaustive_valid, int);
BENCHMARK_TEMPLATE(scan_int_single_scn_int_exhaustive_valid, long long);
BENCHMARK_TEMPLATE(scan_int_single_scn_int_exhaustive_valid, unsigned);
#endif

template <typename Int>
static void scan_int_single_sstream(benchmark::State& state)
{
    single_state<Int> s{get_integer_list<Int>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        Int i{};
        std::istringstream iss{*s.it};
        iss >> i;

        if (iss.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        s.push(i);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_single_sstream, int);
BENCHMARK_TEMPLATE(scan_int_single_sstream, long long);
BENCHMARK_TEMPLATE(scan_int_single_sstream, unsigned);

template <typename Int>
static void scan_int_single_scanf(benchmark::State& state)
{
    single_state<Int> s{get_integer_list<Int>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        Int i{};
        auto ret = sscanf_integral(s.it->c_str(), i);
        if (ret != 1) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        s.push(i);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_single_scanf, int);
BENCHMARK_TEMPLATE(scan_int_single_scanf, long long);
BENCHMARK_TEMPLATE(scan_int_single_scanf, unsigned);

template <typename Int>
static void scan_int_single_strtol(benchmark::State& state)
{
    single_state<Int> s{get_integer_list<Int>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        Int i{};
        auto ret = strtol_integral(s.it->c_str(), i);
        if (!ret) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        s.push(i);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_single_strtol, int);
BENCHMARK_TEMPLATE(scan_int_single_strtol, long long);
BENCHMARK_TEMPLATE(scan_int_single_strtol, unsigned);

#if SCN_HAS_INTEGER_CHARCONV

template <typename Int>
static void scan_int_single_charconv(benchmark::State& state)
{
    single_state<Int> s{get_integer_list<Int>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        Int i{};
        auto ret =
            std::from_chars(s.it->data(), s.it->data() + s.it->size(), i);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        s.push(i);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_single_charconv, int);
BENCHMARK_TEMPLATE(scan_int_single_charconv, long long);
BENCHMARK_TEMPLATE(scan_int_single_charconv, unsigned);

#endif  // SCN_HAS_INTEGER_CHARCONV

template <typename Int>
static void scan_int_single_fastfloat(benchmark::State& state)
{
    single_state<Int> s{get_integer_list<Int>()};

    for (auto _ : state) {
        s.reset_if_necessary();

        Int i{};
        auto ret = fast_float::from_chars(s.it->data(),
                                          s.it->data() + s.it->size(), i);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        s.push(i);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_single_fastfloat, int);
BENCHMARK_TEMPLATE(scan_int_single_fastfloat, long long);
BENCHMARK_TEMPLATE(scan_int_single_fastfloat, unsigned);
