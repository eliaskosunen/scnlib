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
static void scan_int_repeated_scn(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        auto result = scn::scan<Int>(s.view(), "{}");

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
BENCHMARK_TEMPLATE(scan_int_repeated_scn, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scn, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scn, unsigned);

template <typename Int>
static void scan_int_repeated_scn_value(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        auto result = scn::scan_value<Int>(s.view());

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
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, unsigned);

template <typename Int>
static void scan_int_repeated_scn_decimal(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        auto result = scn::scan<Int>(s.view(), "{:d}");

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
BENCHMARK_TEMPLATE(scan_int_repeated_scn_decimal, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_decimal, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_decimal, unsigned);

template <typename Int>
static void scan_int_repeated_scn_int(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        s.skip_classic_ascii_space();
        auto sv = std::string_view{scn::ranges::data(s.view()),
                                   scn::ranges::size(s.view())};

        auto result = scn::scan_int<Int>(sv);

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
BENCHMARK_TEMPLATE(scan_int_repeated_scn_int, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_int, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_int, unsigned);

template <typename Int>
static void scan_int_repeated_sstream(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};
    std::istringstream stream{s.source};

    for (auto _ : state) {
        Int i{};
        stream >> i;

        if (stream.eof()) {
            stream = std::istringstream(s.source);
            s.reset();
        }
        else if (stream.fail()) {
            state.SkipWithError("Scan error");
            break;
        }
        else {
            s.push(i);
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, int);
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, unsigned);

template <typename Int>
static void scan_int_repeated_scanf(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        Int i{};

        auto ret = sscanf_integral_n(s.it, i);
        if (ret != 1) {
            if (ret == EOF) {
                s.reset();
                continue;
            }

            state.SkipWithError("Scan error");
            break;
        }
        s.push(i);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, unsigned);

template <typename Int>
static void scan_int_repeated_strtol(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        Int i{};
        s.skip_classic_ascii_space();

        auto ret = strtol_integral_n(s.it, i);
        if (ret != 0) {
            if (ret == EOF) {
                s.reset();
                continue;
            }

            state.SkipWithError("Scan error");
            break;
        }
        s.push(i);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_repeated_strtol, int);
BENCHMARK_TEMPLATE(scan_int_repeated_strtol, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_strtol, unsigned);

#if SCN_HAS_INTEGER_CHARCONV

template <typename Int>
static void scan_int_repeated_charconv(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        Int i{};
        s.skip_classic_ascii_space();

        auto ret = std::from_chars(s.view().begin(), s.view().end(), i);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Scan error");
            break;
        }
        s.it = ret.ptr;
        s.push(i);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_repeated_charconv, int);
BENCHMARK_TEMPLATE(scan_int_repeated_charconv, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_charconv, unsigned);

#endif  // SCN_HAS_INTEGER_CHARCONV

template <typename Int>
static void scan_int_repeated_fastfloat(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        Int i{};
        s.skip_classic_ascii_space();

        auto ret = fast_float::from_chars(s.view().begin(), s.view().end(), i);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Scan error");
            break;
        }
        s.it = ret.ptr;
        s.push(i);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_repeated_fastfloat, int);
BENCHMARK_TEMPLATE(scan_int_repeated_fastfloat, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_fastfloat, unsigned);
