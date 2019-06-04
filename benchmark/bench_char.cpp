// Copyright 2017-2019 Elias Kosunen
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

#include "benchmark.h"

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wglobal-constructors")
SCN_CLANG_IGNORE("-Wunused-template")
SCN_CLANG_IGNORE("-Wexit-time-destructors")

namespace {
    template <typename Char>
    scn::basic_string_view<Char> default_format_str()
    {
    }
    template <>
    scn::string_view default_format_str<char>()
    {
        return {"{}"};
    }
    template <>
    scn::wstring_view default_format_str<wchar_t>()
    {
        return {L"{}"};
    }
}  // namespace

template <typename Char>
static void scanchar_scn(benchmark::State& state)
{
    using string_type = std::basic_string<Char>;
    string_type data = generate_data<Char>(static_cast<size_t>(state.range(0)));
    auto stream = scn::make_stream(data);
    Char c{};

    for (auto _ : state) {
        auto e = scn::scan(stream, default_format_str<Char>(), c);

        if (!e) {
            if (e.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_data<Char>(static_cast<size_t>(state.range(0)));
                stream = scn::make_stream(data);
                state.ResumeTiming();
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scanchar_scn, char)->Arg(2 << 15);
BENCHMARK_TEMPLATE(scanchar_scn, wchar_t)->Arg(2 << 15);

template <typename Char>
static void scanchar_scn_default(benchmark::State& state)
{
    using string_type = std::basic_string<Char>;
    string_type data = generate_data<Char>(static_cast<size_t>(state.range(0)));
    auto stream = scn::make_stream(data);
    Char c{};

    for (auto _ : state) {
        auto e = scn::scan(stream, scn::default_tag, c);

        if (!e) {
            if (e.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_data<Char>(static_cast<size_t>(state.range(0)));
                stream = scn::make_stream(data);
                state.ResumeTiming();
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scanchar_scn_default, char)->Arg(2 << 15);
BENCHMARK_TEMPLATE(scanchar_scn_default, wchar_t)->Arg(2 << 15);

template <typename Char>
static void scanchar_scn_getchar(benchmark::State& state)
{
    using string_type = std::basic_string<Char>;
    string_type data = generate_data<Char>(static_cast<size_t>(state.range(0)));
    auto stream = scn::make_stream(data);

    for (auto _ : state) {
        auto r = scn::getchar(stream);
        benchmark::DoNotOptimize(r);
        benchmark::ClobberMemory();

        if (!r) {
            if (r.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_data<Char>(static_cast<size_t>(state.range(0)));
                stream = scn::make_stream(data);
                state.ResumeTiming();
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scanchar_scn_getchar, char)->Arg(2 << 15);
BENCHMARK_TEMPLATE(scanchar_scn_getchar, wchar_t)->Arg(2 << 15);

template <typename Char>
static void scanchar_scn_get_value(benchmark::State& state)
{
    using string_type = std::basic_string<Char>;
    string_type data = generate_data<Char>(static_cast<size_t>(state.range(0)));
    auto stream = scn::make_stream(data);

    for (auto _ : state) {
        auto r = scn::get_value<Char>(stream);
        benchmark::DoNotOptimize(r);
        benchmark::ClobberMemory();

        if (!r) {
            if (r.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_data<Char>(static_cast<size_t>(state.range(0)));
                stream = scn::make_stream(data);
                state.ResumeTiming();
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scanchar_scn_get_value, char)->Arg(2 << 15);
BENCHMARK_TEMPLATE(scanchar_scn_get_value, wchar_t)->Arg(2 << 15);

template <typename Char>
static void scanchar_sstream(benchmark::State& state)
{
    using string_type = std::basic_string<Char>;
    string_type data = generate_data<Char>(static_cast<size_t>(state.range(0)));
    auto stream = std::basic_istringstream<Char>{data};
    Char c{};

    for (auto _ : state) {
        stream >> c;

        if (stream.eof()) {
            state.PauseTiming();
            data = generate_data<Char>(static_cast<size_t>(state.range(0)));
            stream = std::basic_istringstream<Char>{data};
            state.ResumeTiming();
            continue;
        }
        if (stream.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Char)));
}
BENCHMARK_TEMPLATE(scanchar_sstream, char)->Arg(2 << 15);
BENCHMARK_TEMPLATE(scanchar_sstream, wchar_t)->Arg(2 << 15);

template <typename Char>
static void scanchar_control(benchmark::State& state)
{
    using string_type = std::basic_string<Char>;
    string_type data = generate_data<Char>(static_cast<size_t>(state.range(0)));
    auto it = data.begin();
    Char c{};

    for (auto _ : state) {
        c = *it;
        ++it;

        if (it == data.end()) {
            state.PauseTiming();
            data = generate_data<Char>(static_cast<size_t>(state.range(0)));
            it = data.begin();
            state.ResumeTiming();
            continue;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Char)));
}
// BENCHMARK_TEMPLATE(scanchar_control, char)->Arg(2 << 15);
// BENCHMARK_TEMPLATE(scanchar_control, wchar_t)->Arg(2 << 15);

SCN_CLANG_POP
