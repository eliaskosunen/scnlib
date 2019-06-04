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

#define FLOAT_DATA_N (static_cast<size_t>(2 << 15))

template <typename Float>
static void scanfloat_scn(benchmark::State& state)
{
    auto data = generate_float_data<Float>(FLOAT_DATA_N);
    auto stream = scn::make_stream(data);
    Float f{};
    for (auto _ : state) {
        auto e = scn::scan(stream, "{}", f);

        if (!e) {
            if (e.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_float_data<Float>(FLOAT_DATA_N);
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
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scanfloat_scn, float);
BENCHMARK_TEMPLATE(scanfloat_scn, double);
BENCHMARK_TEMPLATE(scanfloat_scn, long double);

template <typename Float>
static void scanfloat_scn_default(benchmark::State& state)
{
    auto data = generate_float_data<Float>(FLOAT_DATA_N);
    auto stream = scn::make_stream(data);
    Float f{};
    for (auto _ : state) {
        auto e = scn::scan(stream, scn::default_tag, f);

        if (!e) {
            if (e.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_float_data<Float>(FLOAT_DATA_N);
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
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scanfloat_scn_default, float);
BENCHMARK_TEMPLATE(scanfloat_scn_default, double);
BENCHMARK_TEMPLATE(scanfloat_scn_default, long double);

template <typename Float>
static void scanfloat_scn_get_value(benchmark::State& state)
{
    auto data = generate_float_data<Float>(FLOAT_DATA_N);
    auto stream = scn::make_stream(data);
    for (auto _ : state) {
        auto e = scn::get_value<Float>(stream);

        if (!e) {
            if (e.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_float_data<Float>(FLOAT_DATA_N);
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
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scanfloat_scn_get_value, float);
BENCHMARK_TEMPLATE(scanfloat_scn_get_value, double);
BENCHMARK_TEMPLATE(scanfloat_scn_get_value, long double);

template <typename Float>
static void scanfloat_sstream(benchmark::State& state)
{
    auto data = generate_float_data<Float>(FLOAT_DATA_N);
    auto stream = std::istringstream(data);
    Float f{};
    for (auto _ : state) {
        stream >> f;

        if (stream.eof()) {
            state.PauseTiming();
            data = generate_float_data<Float>(FLOAT_DATA_N);
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
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scanfloat_sstream, float);
BENCHMARK_TEMPLATE(scanfloat_sstream, double);
BENCHMARK_TEMPLATE(scanfloat_sstream, long double);

SCN_MSVC_PUSH
SCN_MSVC_IGNORE(4996)

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wunused-function")

namespace detail {
    static int scanf_float(char*& ptr, float& f)
    {
        int n;
        auto ret = sscanf(ptr, "%f%n", &f, &n);
        ptr += n + 1;
        return ret;
    }
    static int scanf_float(char*& ptr, double& d)
    {
        int n;
        auto ret = sscanf(ptr, "%lf%n", &d, &n);
        ptr += n + 1;
        return ret;
    }
    static int scanf_float(char*& ptr, long double& ld)
    {
        int n;
        auto ret = sscanf(ptr, "%Lf%n", &ld, &n);
        ptr += n + 1;
        return ret;
    }
}  // namespace detail

SCN_MSVC_POP

template <typename Float>
static void scanfloat_scanf(benchmark::State& state)
{
    auto data = generate_float_data<Float>(FLOAT_DATA_N);
    auto ptr = &data[0];
    Float f{};
    for (auto _ : state) {
        auto ret = detail::scanf_float(ptr, f);

        if (ret != 1) {
            if (ret == EOF) {
                state.PauseTiming();
                data = generate_float_data<Float>(FLOAT_DATA_N);
                ptr = &data[0];
                state.ResumeTiming();
                continue;
            }
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
BENCHMARK_TEMPLATE(scanfloat_scanf, float);
BENCHMARK_TEMPLATE(scanfloat_scanf, double);
BENCHMARK_TEMPLATE(scanfloat_scanf, long double);

SCN_GCC_POP
SCN_CLANG_POP
