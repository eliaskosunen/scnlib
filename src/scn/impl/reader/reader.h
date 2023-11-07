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

#include <scn/detail/args.h>
#include <scn/detail/format_string.h>
#include <scn/detail/xchar.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename Context>
        struct default_arg_reader {
            using context_type = Context;
            using char_type = typename context_type::char_type;
            using args_type = basic_scan_args<context_type>;
            using range_type = typename context_type::range_type;
            using iterator = typename context_type::iterator;

            template <typename T>
            scan_expected<iterator> operator()(T& value);

            scan_expected<iterator> operator()(
                typename basic_scan_arg<context_type>::handle h);

            range_type range;
            args_type args;
            detail::locale_ref loc;
        };

        template <typename Context>
        struct arg_reader {
            using context_type = Context;
            using char_type = typename context_type::char_type;
            using range_type = typename context_type::range_type;
            using iterator = typename context_type::iterator;

            template <typename T>
            scan_expected<iterator> operator()(T& value);

            scan_expected<iterator> operator()(
                typename basic_scan_arg<context_type>::handle)
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

            range_type range;
            const detail::basic_format_specs<char_type>& specs;
            detail::locale_ref loc;
        };

        template <typename Context>
        struct custom_reader {
            using context_type = Context;
            using parse_context_type =
                typename context_type::parse_context_type;
            using char_type = typename context_type::char_type;
            using iterator = typename context_type::iterator;

            template <typename T>
            scan_expected<iterator> operator()(T&) const
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

            scan_expected<iterator> operator()(
                typename basic_scan_arg<context_type>::handle h) const;

            parse_context_type& parse_ctx;
            context_type& ctx;
        };

#define SCN_DECLARE_READER_T(Ctx, T)                             \
    extern template auto default_arg_reader<Ctx>::operator()(T&) \
        -> scan_expected<iterator>;                              \
    extern template auto arg_reader<Ctx>::operator()(T&)         \
        -> scan_expected<iterator>;

#define SCN_DECLARE_READER_CUSTOM(Ctx)                        \
    extern template auto default_arg_reader<Ctx>::operator()( \
        typename basic_scan_arg<context_type>::handle)        \
        -> scan_expected<iterator>;                           \
    extern template auto custom_reader<Ctx>::operator()(      \
        typename basic_scan_arg<context_type>::handle) const  \
        -> scan_expected<iterator>;

        SCN_DECLARE_READER_CUSTOM(detail::scanner_scan_contexts::sv)
        SCN_DECLARE_READER_CUSTOM(detail::scanner_scan_contexts::wsv)
#if !SCN_DISABLE_ERASED_RANGE
        SCN_DECLARE_READER_CUSTOM(detail::scanner_scan_contexts::es)
        SCN_DECLARE_READER_CUSTOM(detail::scanner_scan_contexts::wes)
#endif
#if !SCN_DISABLE_IOSTREAM
        SCN_DECLARE_READER_CUSTOM(detail::scanner_scan_contexts::is)
        SCN_DECLARE_READER_CUSTOM(detail::scanner_scan_contexts::wis)
#endif

#define SCN_DECLARE_READER(Ctx)                   \
    SCN_DECLARE_READER_T(Ctx, signed char)        \
    SCN_DECLARE_READER_T(Ctx, short)              \
    SCN_DECLARE_READER_T(Ctx, int)                \
    SCN_DECLARE_READER_T(Ctx, long)               \
    SCN_DECLARE_READER_T(Ctx, long long)          \
    SCN_DECLARE_READER_T(Ctx, unsigned char)      \
    SCN_DECLARE_READER_T(Ctx, unsigned short)     \
    SCN_DECLARE_READER_T(Ctx, unsigned int)       \
    SCN_DECLARE_READER_T(Ctx, unsigned long)      \
    SCN_DECLARE_READER_T(Ctx, unsigned long long) \
    SCN_DECLARE_READER_T(Ctx, float)              \
    SCN_DECLARE_READER_T(Ctx, double)             \
    SCN_DECLARE_READER_T(Ctx, long double)        \
    SCN_DECLARE_READER_T(Ctx, char)               \
    SCN_DECLARE_READER_T(Ctx, wchar_t)            \
    SCN_DECLARE_READER_T(Ctx, char32_t)           \
    SCN_DECLARE_READER_T(Ctx, bool)               \
    SCN_DECLARE_READER_T(Ctx, void*)              \
    SCN_DECLARE_READER_T(Ctx, std::string)        \
    SCN_DECLARE_READER_T(Ctx, std::wstring)       \
    SCN_DECLARE_READER_T(Ctx, std::string_view)   \
    SCN_DECLARE_READER_T(Ctx, std::wstring_view)  \
    SCN_DECLARE_READER_T(Ctx, monostate)

        SCN_DECLARE_READER(detail::scanner_scan_contexts::sv)
        SCN_DECLARE_READER(detail::scanner_scan_contexts::wsv)
#if !SCN_DISABLE_ERASED_RANGE
        SCN_DECLARE_READER(detail::scanner_scan_contexts::es)
        SCN_DECLARE_READER(detail::scanner_scan_contexts::wes)
#endif
#if !SCN_DISABLE_IOSTREAM
        SCN_DECLARE_READER(detail::scanner_scan_contexts::is)
        SCN_DECLARE_READER(detail::scanner_scan_contexts::wis)
#endif

#undef SCN_DECLARE_READER_T
#undef SCN_DECLARE_READER_CUSTOM
#undef SCN_DECLARE_READER

    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
