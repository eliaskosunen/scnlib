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
static void scanword_scn(benchmark::State& state)
{
    using string_type = std::basic_string<Char>;
    string_type data = generate_data<Char>(static_cast<size_t>(state.range(0)));
    auto stream = scn::make_stream(data);
    string_type str{};

    for (auto _ : state) {
        auto e = scn::scan(stream, default_format_str<Char>(), str);

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
}
BENCHMARK_TEMPLATE(scanword_scn, char)->Arg(2 << 15);
BENCHMARK_TEMPLATE(scanword_scn, wchar_t)->Arg(2 << 15);

template <typename Char>
static void scanword_scn_default(benchmark::State& state)
{
    using string_type = std::basic_string<Char>;
    string_type data = generate_data<Char>(static_cast<size_t>(state.range(0)));
    auto stream = scn::make_stream(data);
    string_type str{};

    for (auto _ : state) {
        auto e = scn::scan(stream, scn::default_tag, str);

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
}
BENCHMARK_TEMPLATE(scanword_scn_default, char)->Arg(2 << 15);
BENCHMARK_TEMPLATE(scanword_scn_default, wchar_t)->Arg(2 << 15);

template <typename Char>
static void scanword_scn_get_value(benchmark::State& state)
{
    using string_type = std::basic_string<Char>;
    string_type data = generate_data<Char>(static_cast<size_t>(state.range(0)));
    auto stream = scn::make_stream(data);
    string_type str{};

    for (auto _ : state) {
        auto e = scn::get_value<string_type>(stream);

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
        str = e.value();
    }
}
BENCHMARK_TEMPLATE(scanword_scn_get_value, char)->Arg(2 << 15);
BENCHMARK_TEMPLATE(scanword_scn_get_value, wchar_t)->Arg(2 << 15);

template <typename Char>
static void scanword_sstream(benchmark::State& state)
{
    using string_type = std::basic_string<Char>;
    string_type data = generate_data<Char>(static_cast<size_t>(state.range(0)));
    auto stream = std::basic_istringstream<Char>{data};
    string_type str{};

    for (auto _ : state) {
        stream >> str;

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
}
BENCHMARK_TEMPLATE(scanword_sstream, char)->Arg(2 << 15);
BENCHMARK_TEMPLATE(scanword_sstream, wchar_t)->Arg(2 << 15);

SCN_CLANG_POP
