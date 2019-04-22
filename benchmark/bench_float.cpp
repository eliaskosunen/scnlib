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
    const auto options =
        scn::options::builder{}
            .float_method(static_cast<scn::method>(state.range(0)))
            .make();
    Float f{};
    for (auto _ : state) {
        auto e = scn::scan(options, stream, "{}", f);

        benchmark::DoNotOptimize(f);
        benchmark::DoNotOptimize(stream);
        benchmark::DoNotOptimize(e);
        benchmark::ClobberMemory();
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
BENCHMARK_TEMPLATE(scanfloat_scn, float)->Arg(STRTO_METHOD)->Arg(STO_METHOD);
BENCHMARK_TEMPLATE(scanfloat_scn, double)->Arg(STRTO_METHOD)->Arg(STO_METHOD);
BENCHMARK_TEMPLATE(scanfloat_scn, long double)
    ->Arg(STRTO_METHOD)
    ->Arg(STO_METHOD);

template <typename Float>
static void scanfloat_scn_default(benchmark::State& state)
{
    auto data = generate_float_data<Float>(FLOAT_DATA_N);
    auto stream = scn::make_stream(data);
    const auto options =
        scn::options::builder{}
            .float_method(static_cast<scn::method>(state.range(0)))
            .make();
    Float f{};
    for (auto _ : state) {
        auto e = scn::scan_default(options, stream, f);

        benchmark::DoNotOptimize(f);
        benchmark::DoNotOptimize(stream);
        benchmark::DoNotOptimize(e);
        benchmark::ClobberMemory();
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
BENCHMARK_TEMPLATE(scanfloat_scn_default, float)
    ->Arg(STRTO_METHOD)
    ->Arg(STO_METHOD);
BENCHMARK_TEMPLATE(scanfloat_scn_default, double)
    ->Arg(STRTO_METHOD)
    ->Arg(STO_METHOD);
BENCHMARK_TEMPLATE(scanfloat_scn_default, long double)
    ->Arg(STRTO_METHOD)
    ->Arg(STO_METHOD);

template <typename Float>
static void scanfloat_sstream(benchmark::State& state)
{
    auto data = generate_float_data<Float>(FLOAT_DATA_N);
    auto stream = std::istringstream(data);
    Float f{};
    for (auto _ : state) {
        benchmark::DoNotOptimize(stream >> f);

        benchmark::DoNotOptimize(f);
        benchmark::ClobberMemory();
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

SCN_CLANG_POP
