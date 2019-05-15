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

#include <scn/tuple_return.h>

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wglobal-constructors")
SCN_CLANG_IGNORE("-Wunused-template")
SCN_CLANG_IGNORE("-Wunused-function")
SCN_CLANG_IGNORE("-Wexit-time-destructors")

#define INT_DATA_N (static_cast<size_t>(2 << 15))

template <typename Int>
static void scanint_scn(benchmark::State& state)
{
    auto data = generate_int_data<Int>(INT_DATA_N);
    auto stream = scn::make_stream(data);
    Int i{};
    for (auto _ : state) {
        auto e = scn::scan(stream, "{}", i);

        benchmark::DoNotOptimize(i);
        benchmark::DoNotOptimize(e);
        benchmark::DoNotOptimize(stream);
        benchmark::ClobberMemory();
        if (!e) {
            if (e.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_int_data<Int>(INT_DATA_N);
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
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scanint_scn, int);
BENCHMARK_TEMPLATE(scanint_scn, long long);
BENCHMARK_TEMPLATE(scanint_scn, unsigned);

#if defined(__cpp_structured_bindings) && __cpp_structured_bindings >= 201606
template <typename Int>
static void scanint_tuple_scn(benchmark::State& state)
{
    auto data = generate_int_data<Int>(INT_DATA_N);
    auto stream = scn::make_stream(data);
    for (auto _ : state) {
        // Structured bindings do not work with gcc
#if SCN_GCC
        scn::result<int> e(0);
        Int i;
        std::tie(e, i) = scn::scan_return<Int>(stream, "{}");
#else
        auto [e, i] = scn::scan_return<Int>(stream, "{}");
#endif

        benchmark::DoNotOptimize(i);
        benchmark::DoNotOptimize(e);
        benchmark::DoNotOptimize(stream);
        benchmark::ClobberMemory();
        if (!e) {
            if (e.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_int_data<Int>(INT_DATA_N);
                stream = scn::make_stream(data);
                state.ResumeTiming();
            }
            else {
                std::cout << e.error().code() << '\n';
                std::cout << e.error().msg() << '\n';
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scanint_tuple_scn, int);
BENCHMARK_TEMPLATE(scanint_tuple_scn, long long);
BENCHMARK_TEMPLATE(scanint_tuple_scn, unsigned);
#endif

template <typename Int>
static void scanint_scn_default(benchmark::State& state)
{
    auto data = generate_int_data<Int>(INT_DATA_N);
    auto stream = scn::make_stream(data);
    Int i{};
    for (auto _ : state) {
        auto e = scn::scan(stream, scn::default_tag, i);

        benchmark::DoNotOptimize(i);
        benchmark::DoNotOptimize(e);
        benchmark::DoNotOptimize(stream);
        benchmark::ClobberMemory();
        if (!e) {
            if (e.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_int_data<Int>(INT_DATA_N);
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
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scanint_scn_default, int);
BENCHMARK_TEMPLATE(scanint_scn_default, long long);
BENCHMARK_TEMPLATE(scanint_scn_default, unsigned);

template <typename Int>
static void scanint_sstream(benchmark::State& state)
{
    auto data = generate_int_data<Int>(INT_DATA_N);
    auto stream = std::istringstream(data);
    Int i{};
    for (auto _ : state) {
        stream >> i;

        benchmark::DoNotOptimize(i);
        benchmark::DoNotOptimize(stream);
        benchmark::ClobberMemory();
        if (stream.eof()) {
            state.PauseTiming();
            data = generate_int_data<Int>(INT_DATA_N);
            stream = std::istringstream(data);
            state.ResumeTiming();
            continue;
        }
        if (stream.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
BENCHMARK_TEMPLATE(scanint_sstream, int);
BENCHMARK_TEMPLATE(scanint_sstream, long long);
BENCHMARK_TEMPLATE(scanint_sstream, unsigned);

SCN_MSVC_PUSH
SCN_MSVC_IGNORE(4996)

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wunused-function")

namespace detail {
    static int scanf_integral(char*& ptr, int& i)
    {
        int n;
        auto ret = sscanf(ptr, "%d%n", &i, &n);
        ptr += n + 1;
        return ret;
    }
    static int scanf_integral(char*& ptr, long long& i)
    {
        int n;
        auto ret = sscanf(ptr, "%lld%n", &i, &n);
        ptr += n + 1;
        return ret;
    }
    static int scanf_integral(char*& ptr, unsigned& i)
    {
        int n;
        auto ret = sscanf(ptr, "%u%n", &i, &n);
        ptr += n + 1;
        return ret;
    }
}  // namespace detail

SCN_MSVC_POP

template <typename Int>
static void scanint_scanf(benchmark::State& state)
{
    auto data = generate_int_data<Int>(INT_DATA_N);
    auto ptr = &data[0];
    Int i{};
    for (auto _ : state) {
        auto ret = detail::scanf_integral(ptr, i);

        benchmark::DoNotOptimize(i);
        if (ret != 1) {
            if (ret == EOF) {
                state.PauseTiming();
                data = generate_int_data<Int>(INT_DATA_N);
                ptr = &data[0];
                state.ResumeTiming();
                continue;
            }
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Int)));
}
// BENCHMARK_TEMPLATE(scanint_scanf, int);
// BENCHMARK_TEMPLATE(scanint_scanf, long long);
// BENCHMARK_TEMPLATE(scanint_scanf, unsigned);

SCN_GCC_POP
SCN_CLANG_POP
