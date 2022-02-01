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

#ifndef SCN_DETAIL_READER_READER_H
#define SCN_DETAIL_READER_READER_H

#include "common.h"
#include "float.h"
#include "int.h"
#include "string.h"
#include "types.h"

#include "../args.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename CharT>
    struct scanner<CharT, CharT> : public detail::char_scanner {
    };
    template <typename CharT>
    struct scanner<CharT, span<CharT>> : public detail::buffer_scanner {
    };
    template <typename CharT>
    struct scanner<CharT, bool> : public detail::bool_scanner {
    };
    template <typename CharT>
    struct scanner<CharT, short> : public detail::integer_scanner<short> {
    };
    template <typename CharT>
    struct scanner<CharT, int> : public detail::integer_scanner<int> {
    };
    template <typename CharT>
    struct scanner<CharT, long> : public detail::integer_scanner<long> {
    };
    template <typename CharT>
    struct scanner<CharT, long long>
        : public detail::integer_scanner<long long> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned short>
        : public detail::integer_scanner<unsigned short> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned int>
        : public detail::integer_scanner<unsigned int> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned long>
        : public detail::integer_scanner<unsigned long> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned long long>
        : public detail::integer_scanner<unsigned long long> {
    };
    template <typename CharT>
    struct scanner<CharT, float> : public detail::float_scanner<float> {
    };
    template <typename CharT>
    struct scanner<CharT, double> : public detail::float_scanner<double> {
    };
    template <typename CharT>
    struct scanner<CharT, long double>
        : public detail::float_scanner<long double> {
    };
    template <typename CharT, typename Allocator>
    struct scanner<CharT,
                   std::basic_string<CharT, std::char_traits<CharT>, Allocator>>
        : public detail::string_scanner {
    };
    template <typename CharT>
    struct scanner<CharT, basic_string_view<CharT>>
        : public detail::string_view_scanner {
    };
#if SCN_HAS_STRING_VIEW
    template <typename CharT>
    struct scanner<CharT, std::basic_string_view<CharT>>
        : public detail::std_string_view_scanner {
    };
#endif
    template <typename CharT>
    struct scanner<CharT, detail::monostate>;

    SCN_END_NAMESPACE
}  // namespace scn

#endif
