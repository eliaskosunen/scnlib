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

#include <array>
#include <cwctype>
#include <locale>

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wglobal-constructors")
SCN_CLANG_IGNORE("-Wunused-template")
SCN_CLANG_IGNORE("-Wunused-function")
SCN_CLANG_IGNORE("-Wexit-time-destructors")

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wunused-function")

namespace detail {
    static inline bool is_space_cctype(char ch)
    {
        return std::isspace(static_cast<unsigned char>(ch)) != 0;
    }
    static inline bool is_space_cctype(wchar_t ch)
    {
        return std::iswspace(static_cast<wint_t>(ch)) != 0;
    }
}  // namespace detail

template <typename Char>
static void isspace_cctype(benchmark::State& state)
{
    auto data = generate_data<Char>(4096);
    auto it = data.begin();

    for (auto _ : state) {
        benchmark::DoNotOptimize(detail::is_space_cctype(*it));
        ++it;

        if (SCN_UNLIKELY(it == data.end())) {
            state.PauseTiming();
            data = generate_data<Char>(4096);
            it = data.begin();
            state.ResumeTiming();
        }
    }
}
BENCHMARK_TEMPLATE(isspace_cctype, char);
BENCHMARK_TEMPLATE(isspace_cctype, wchar_t);

template <typename Char>
static void isspace_locale(benchmark::State& state)
{
    auto data = generate_data<Char>(4096);
    auto it = data.begin();

    for (auto _ : state) {
        benchmark::DoNotOptimize(std::isspace(*it, std::locale()));
        ++it;

        if (SCN_UNLIKELY(it == data.end())) {
            state.PauseTiming();
            data = generate_data<Char>(4096);
            it = data.begin();
            state.ResumeTiming();
        }
    }
}
BENCHMARK_TEMPLATE(isspace_locale, char);
BENCHMARK_TEMPLATE(isspace_locale, wchar_t);

namespace detail {
    SCN_CONSTEXPR static inline bool is_space_eq(char ch)
    {
        return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r' ||
               ch == '\v' || ch == '\f';
    }
    SCN_CONSTEXPR static inline bool is_space_eq(wchar_t ch)
    {
        return ch == L' ' || ch == L'\n' || ch == L'\t' || ch == L'\r' ||
               ch == L'\v' || ch == L'\f';
    }
}  // namespace detail

template <typename Char>
static void isspace_eq(benchmark::State& state)
{
    auto data = generate_data<Char>(4096);
    auto it = data.begin();

    for (auto _ : state) {
        benchmark::DoNotOptimize(detail::is_space_eq(*it));
        ++it;

        if (SCN_UNLIKELY(it == data.end())) {
            state.PauseTiming();
            data = generate_data<Char>(4096);
            it = data.begin();
            state.ResumeTiming();
        }
    }
}
BENCHMARK_TEMPLATE(isspace_eq, char);
BENCHMARK_TEMPLATE(isspace_eq, wchar_t);

namespace detail {
    SCN_CONSTEXPR static inline bool is_space_cmp(char ch) noexcept
    {
        return ch == 0x20 || (ch >= 0x09 && ch <= 0x0d);
    }
    SCN_CONSTEXPR static inline bool is_space_cmp(wchar_t ch) noexcept
    {
        return ch == 0x20 || (ch >= 0x09 && ch <= 0x0d);
    }
}  // namespace detail

template <typename Char>
static void isspace_cmp(benchmark::State& state)
{
    auto data = generate_data<Char>(4096);
    auto it = data.begin();

    for (auto _ : state) {
        benchmark::DoNotOptimize(detail::is_space_cmp(*it));
        ++it;

        if (SCN_UNLIKELY(it == data.end())) {
            state.PauseTiming();
            data = generate_data<Char>(4096);
            it = data.begin();
            state.ResumeTiming();
        }
    }
}
BENCHMARK_TEMPLATE(isspace_cmp, char);
BENCHMARK_TEMPLATE(isspace_cmp, wchar_t);

namespace detail {
    static inline bool is_space_table_static(char ch) noexcept
    {
        static const std::array<bool, 256> lookup = {
            {true,  true, true,  true,  true, true, true, true, true, false,
             false, true, true,  false, true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, false, true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true}};
        return lookup[static_cast<size_t>(ch)];
    }
    SCN_CONSTEXPR14 static inline bool is_space_table_auto(char ch) noexcept
    {
        const std::array<bool, 256> lookup = {
            {true,  true, true,  true,  true, true, true, true, true, false,
             false, true, true,  false, true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, false, true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true, true, true, true, true,
             true,  true, true,  true,  true, true}};
        return lookup[static_cast<size_t>(ch)];
    }
}  // namespace detail

static void isspace_table_static(benchmark::State& state)
{
    auto data = generate_data<char>(4096);
    auto it = data.begin();

    for (auto _ : state) {
        benchmark::DoNotOptimize(detail::is_space_table_static(*it));
        ++it;

        if (SCN_UNLIKELY(it == data.end())) {
            state.PauseTiming();
            data = generate_data<char>(4096);
            it = data.begin();
            state.ResumeTiming();
        }
    }
}
BENCHMARK(isspace_table_static);

static void isspace_table_auto(benchmark::State& state)
{
    auto data = generate_data<char>(4096);
    auto it = data.begin();

    for (auto _ : state) {
        benchmark::DoNotOptimize(detail::is_space_table_auto(*it));
        ++it;

        if (SCN_UNLIKELY(it == data.end())) {
            state.PauseTiming();
            data = generate_data<char>(4096);
            it = data.begin();
            state.ResumeTiming();
        }
    }
}
BENCHMARK(isspace_table_auto);

namespace detail {
    SCN_CONSTEXPR bool has_zero(uint64_t v)
    {
        return (v - UINT64_C(0x0101010101010101)) & ~v &
               UINT64_C(0x8080808080808080);
    }

    inline bool is_space_bit_twiddle_static(char ch) noexcept
    {
        static uint64_t mask_space =
            ~UINT64_C(0) / 255 * static_cast<uint64_t>(' ');
        static uint64_t mask_nl =
            ~UINT64_C(0) / 255 * static_cast<uint64_t>('\n');
        static uint64_t mask_tab =
            ~UINT64_C(0) / 255 * static_cast<uint64_t>('\t');
        static uint64_t mask_cr =
            ~UINT64_C(0) / 255 * static_cast<uint64_t>('\r');
        static uint64_t mask_vtab =
            ~UINT64_C(0) / 255 * static_cast<uint64_t>('\v');
        static uint64_t mask_ff =
            ~UINT64_C(0) / 255 * static_cast<uint64_t>('\f');

        auto word = static_cast<uint64_t>(ch);
        return has_zero(word ^ mask_space) || has_zero(word ^ mask_nl) ||
               has_zero(word ^ mask_tab) || has_zero(word ^ mask_cr) ||
               has_zero(word ^ mask_vtab) || has_zero(word ^ mask_ff);
    }
    inline bool is_space_bit_twiddle(char ch) noexcept
    {
        uint64_t mask_space = ~UINT64_C(0) / 255 * static_cast<uint64_t>(' ');
        uint64_t mask_nl = ~UINT64_C(0) / 255 * static_cast<uint64_t>('\n');
        uint64_t mask_tab = ~UINT64_C(0) / 255 * static_cast<uint64_t>('\t');
        uint64_t mask_cr = ~UINT64_C(0) / 255 * static_cast<uint64_t>('\r');
        uint64_t mask_vtab = ~UINT64_C(0) / 255 * static_cast<uint64_t>('\v');
        uint64_t mask_ff = ~UINT64_C(0) / 255 * static_cast<uint64_t>('\f');

        auto word = static_cast<uint64_t>(ch);
        return has_zero(word ^ mask_space) || has_zero(word ^ mask_nl) ||
               has_zero(word ^ mask_tab) || has_zero(word ^ mask_cr) ||
               has_zero(word ^ mask_vtab) || has_zero(word ^ mask_ff);
    }
}  // namespace detail

static void isspace_bit_twiddle_static(benchmark::State& state)
{
    auto data = generate_data<char>(4096);
    auto it = data.begin();

    for (auto _ : state) {
        benchmark::DoNotOptimize(detail::is_space_bit_twiddle_static(*it));
        ++it;

        if (SCN_UNLIKELY(it == data.end())) {
            state.PauseTiming();
            data = generate_data<char>(4096);
            it = data.begin();
            state.ResumeTiming();
        }
    }
}
BENCHMARK(isspace_bit_twiddle_static);

static void isspace_bit_twiddle(benchmark::State& state)
{
    auto data = generate_data<char>(4096);
    auto it = data.begin();

    for (auto _ : state) {
        benchmark::DoNotOptimize(detail::is_space_bit_twiddle(*it));
        ++it;

        if (SCN_UNLIKELY(it == data.end())) {
            state.PauseTiming();
            data = generate_data<char>(4096);
            it = data.begin();
            state.ResumeTiming();
        }
    }
}
BENCHMARK(isspace_bit_twiddle);
