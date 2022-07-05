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

#pragma once

#include <scn/detail/context.h>
#include <scn/detail/erased_range.h>
#include <scn/detail/istream_range.h>
#include <scn/detail/scanner.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename Range>
        scan_expected<ranges::iterator_t<Range>>
        internal_skip_classic_whitespace(Range r, bool allow_exhaustion);

#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(T, Context) \
    extern template scan_expected<Context::iterator>         \
    scanner_scan_for_builtin_type(                           \
        T&, Context&, const basic_format_specs<Context::char_type>&);

#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(Context)                      \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(Context::char_type, Context)     \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(signed char, Context)            \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(short, Context)                  \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(int, Context)                    \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(long, Context)                   \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(long long, Context)              \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(unsigned char, Context)          \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(unsigned short, Context)         \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(unsigned int, Context)           \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(unsigned long, Context)          \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(unsigned long long, Context)     \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(float, Context)                  \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(double, Context)                 \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(long double, Context)            \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(                                 \
        std::basic_string<Context::char_type>, Context)                       \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(                                 \
        std::basic_string_view<Context::char_type>, Context)                  \
    extern template scan_expected<ranges::iterator_t<Context::subrange_type>> \
    internal_skip_classic_whitespace(Context::subrange_type, bool);

        namespace scanner_scan_contexts {
            using sv = basic_scan_context<std::string_view, char>;
            using wsv = basic_scan_context<std::wstring_view, wchar_t>;
            using is = basic_scan_context<istreambuf_subrange, char>;
            using wis = basic_scan_context<wistreambuf_subrange, wchar_t>;
            using es = basic_scan_context<basic_erased_subrange<char>, char>;
            using wes =
                basic_scan_context<basic_erased_subrange<wchar_t>, wchar_t>;
        }  // namespace scanner_scan_contexts

        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::sv)
        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::wsv)

#if SCN_USE_IOSTREAMS
        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::is)
        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::wis)
#endif

        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::es)
        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::wes)

#undef SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX
#undef SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE

    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
