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

#include "bench_word.h"

#define WORD_DATA_N (2 << 12)

template <typename Char>
static void scan_word_repeated_scn(benchmark::State& state)
{
    auto data = word_list<Char>(WORD_DATA_N);
    std::basic_string<Char> str{};
    auto result = scn::make_result(data);
    size_t size = 0;
    for (auto _ : state) {
        result = scn::scan(result.range(), default_format_str<Char>(), str);

        if (!result) {
            if (result.error() == scn::error::end_of_range) {
                result = scn::make_result(data);
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
        else {
            size += str.size();
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(size * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scan_word_repeated_scn, char);
BENCHMARK_TEMPLATE(scan_word_repeated_scn, wchar_t);

template <typename Char>
static void scan_word_repeated_scn_default(benchmark::State& state)
{
    auto data = word_list<Char>(WORD_DATA_N);
    std::basic_string<Char> str{};
    auto result = scn::make_result(data);
    size_t size = 0;
    for (auto _ : state) {
        result = scn::scan_default(result.range(), str);

        if (!result) {
            if (result.error() == scn::error::end_of_range) {
                result = scn::make_result(data);
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
        else {
            size += str.size();
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(size * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scan_word_repeated_scn_default, char);
BENCHMARK_TEMPLATE(scan_word_repeated_scn_default, wchar_t);

template <typename Char>
static void scan_word_repeated_scn_value(benchmark::State& state)
{
    auto data = word_list<Char>(WORD_DATA_N);
    auto result =
        scn::make_result<scn::expected<std::basic_string<Char>>>(data);
    size_t size = 0;
    for (auto _ : state) {
        result = scn::scan_value<std::basic_string<Char>>(result.range());

        if (!result) {
            if (result.error() == scn::error::end_of_range) {
                result =
                    scn::make_result<scn::expected<std::basic_string<Char>>>(
                        data);
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
        else {
            size += result.value().size();
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(size * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scan_word_repeated_scn_value, char);
BENCHMARK_TEMPLATE(scan_word_repeated_scn_value, wchar_t);

template <typename Char>
static void scan_word_repeated_scn_view(benchmark::State& state)
{
    auto data = word_list<Char>(WORD_DATA_N);
    scn::basic_string_view<Char> str{};
    auto result = scn::make_result(data);
    size_t size = 0;
    for (auto _ : state) {
        result = scn::scan(result.range(), default_format_str<Char>(), str);

        if (!result) {
            if (result.error() == scn::error::end_of_range) {
                result = scn::make_result(data);
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
        else {
            size += str.size();
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(size * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scan_word_repeated_scn_view, char);
BENCHMARK_TEMPLATE(scan_word_repeated_scn_view, wchar_t);

template <typename Char>
static void scan_word_repeated_scn_view_default(benchmark::State& state)
{
    auto data = word_list<Char>(WORD_DATA_N);
    scn::basic_string_view<Char> str{};
    auto result = scn::make_result(data);
    size_t size = 0;
    for (auto _ : state) {
        result = scn::scan_default(result.range(), str);

        if (!result) {
            if (result.error() == scn::error::end_of_range) {
                result = scn::make_result(data);
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
        else {
            size += str.size();
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(size * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scan_word_repeated_scn_view_default, char);
BENCHMARK_TEMPLATE(scan_word_repeated_scn_view_default, wchar_t);

template <typename Char>
static void scan_word_repeated_scn_view_value(benchmark::State& state)
{
    auto data = word_list<Char>(WORD_DATA_N);
    auto result =
        scn::make_result<scn::expected<scn::basic_string_view<Char>>>(data);
    size_t size = 0;
    for (auto _ : state) {
        result = scn::scan_value<scn::basic_string_view<Char>>(result.range());

        if (!result) {
            if (result.error() == scn::error::end_of_range) {
                result = scn::make_result<
                    scn::expected<scn::basic_string_view<Char>>>(data);
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
        else {
            size += result.value().size();
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(size * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scan_word_repeated_scn_view_value, char);
BENCHMARK_TEMPLATE(scan_word_repeated_scn_view_value, wchar_t);

template <typename Char>
static void scan_word_repeated_sstream(benchmark::State& state)
{
    auto data = word_list<Char>(WORD_DATA_N);
    auto stream = std::basic_istringstream<Char>(data);
    std::basic_string<Char> str{};
    size_t size = 0;
    for (auto _ : state) {
        stream >> str;

        if (stream.eof()) {
            stream = std::basic_istringstream<Char>(data);
        }
        else if (stream.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
        else {
            size += str.size();
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(size * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scan_word_repeated_sstream, char);
BENCHMARK_TEMPLATE(scan_word_repeated_sstream, wchar_t);
