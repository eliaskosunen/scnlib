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

#include <scn/vscan_impl.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename T>
        auto scan_int_impl(std::string_view source, T& value, int base)
            -> scan_expected<std::string_view::iterator>
        {
            SCN_TRY(beg, impl::skip_classic_whitespace(source));

            auto reader = impl::integer_reader<char>{0, base};
            SCN_TRY(_, reader.read_source(tag_type<T>{},
                                          ranges::subrange(beg, source.end())));
            SCN_TRY(n, reader.parse_value(value));
            return ranges::next(beg, n);
        }

        template <typename T>
        auto scan_int_exhaustive_valid_impl(std::string_view source) -> T
        {
            T value{};
            impl::parse_int_value_exhaustive_valid(source, value);
            return value;
        }

#if !SCN_DISABLE_TYPE_SCHAR
        template auto scan_int_impl(std::string_view, signed char&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> signed char;
#endif
#if !SCN_DISABLE_TYPE_SHORT
        template auto scan_int_impl(std::string_view, short&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view) -> short;
#endif
#if !SCN_DISABLE_TYPE_INT
        template auto scan_int_impl(std::string_view, int&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view) -> int;
#endif
#if !SCN_DISABLE_TYPE_LONG
        template auto scan_int_impl(std::string_view, long&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view) -> long;
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
        template auto scan_int_impl(std::string_view, long long&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> long long;
#endif
#if !SCN_DISABLE_TYPE_UCHAR
        template auto scan_int_impl(std::string_view, unsigned char&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned char;
#endif
#if !SCN_DISABLE_TYPE_USHORT
        template auto scan_int_impl(std::string_view, unsigned short&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned short;
#endif
#if !SCN_DISABLE_TYPE_UINT
        template auto scan_int_impl(std::string_view, unsigned int&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned int;
#endif
#if !SCN_DISABLE_TYPE_ULONG
        template auto scan_int_impl(std::string_view, unsigned long&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned long;
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
        template auto scan_int_impl(std::string_view, unsigned long long&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned long long;
#endif
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
