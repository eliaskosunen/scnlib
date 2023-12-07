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
#include <scn/detail/result.h>
#include <scn/util/expected.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    /**
     * \defgroup vscan Type-erased scanning API
     *
     * \brief Lower-level scanning API with type-erased arguments
     */

    namespace detail {
        scan_expected<std::ptrdiff_t> vscan_impl(scan_buffer& source,
                                                 std::string_view format,
                                                 scan_args args);

        scan_expected<std::ptrdiff_t> vscan_impl(wscan_buffer& source,
                                                 std::wstring_view format,
                                                 wscan_args args);

#if !SCN_DISABLE_LOCALE
        template <typename Locale>
        scan_expected<std::ptrdiff_t> vscan_localized_impl(
            const Locale& loc,
            scan_buffer& source,
            std::string_view format,
            scan_args args);

        template <typename Locale>
        scan_expected<std::ptrdiff_t> vscan_localized_impl(
            const Locale& loc,
            wscan_buffer& source,
            std::wstring_view format,
            wscan_args args);
#endif

        scan_expected<std::ptrdiff_t> vscan_value_impl(
            scan_buffer& source,
            std::string_view format,
            basic_scan_arg<scan_context> arg);

        scan_expected<std::ptrdiff_t> vscan_value_impl(
            wscan_buffer& source,
            std::wstring_view format,
            basic_scan_arg<wscan_context> arg);
    }  // namespace detail

    /**
     * Result type returned by `vscan`.
     *
     * \ingroup vscan
     */
    template <typename Range>
    using vscan_result =
        scan_expected<borrowed_subrange_with_sentinel_t<Range>>;

    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wnoexcept")

    /**
     * Perform actual scanning from `range`, according to `format`, into the
     * type-erased arguments at `args`. Called by `scan`.
     *
     * \ingroup vscan
     */
    template <typename Range>
    auto vscan(Range&& range, std::string_view format, scan_args args)
        -> vscan_result<Range>
    {
        auto buffer = detail::make_scan_buffer(range);

        auto result = detail::vscan_impl(buffer, format, args);
        if (SCN_UNLIKELY(!result)) {
            return unexpected(result.error());
        }
        return detail::make_vscan_result_range(SCN_FWD(range), buffer, *result);
    }

    /**
     * Perform actual scanning from `range`, according to `format`, into the
     * type-erased arguments at `args`, using `loc`, if requested. Called by
     * `scan`.
     *
     * \ingroup locale
     */
    template <typename Range,
              typename Locale,
              typename = std::void_t<decltype(Locale::classic())>>
    auto vscan(const Locale& loc,
               Range&& range,
               std::string_view format,
               scan_args args) -> vscan_result<Range>
    {
#if !SCN_DISABLE_LOCALE
        auto buffer = detail::make_scan_buffer(range);

        SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
        auto result = detail::vscan_localized_impl(loc, buffer, format, args);
        SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

        if (SCN_UNLIKELY(!result)) {
            return unexpected(result.error());
        }
        return detail::make_vscan_result_range(SCN_FWD(range), buffer, *result);
#else
        static_assert(dependent_false<Locale>::value,
                      "Can't use scan(locale, ...) with SCN_DISABLE_LOCALE on");

        return {};
#endif
    }

    /**
     * Perform actual scanning from `range` into the type-erased argument at
     * `arg`. Called by `scan_value`.
     *
     * \ingroup vscan
     */
    template <typename Range>
    auto vscan_value(Range&& range, scan_args arg) -> vscan_result<Range>
    {
        auto buffer = detail::make_scan_buffer(range);

        auto result = detail::vscan_value_impl(buffer, arg);
        if (SCN_UNLIKELY(!result)) {
            return unexpected(result.error());
        }
        return detail::make_vscan_result_range(SCN_FWD(range), buffer, *result);
    }

    scan_error vinput(std::string_view format, scan_args args);

#if !SCN_DISABLE_LOCALE
    template <typename Locale,
              typename = std::void_t<decltype(Locale::classic())>>
    scan_error vinput(const Locale& loc,
                      std::string_view format,
                      scan_args args);
#endif

    namespace detail {
        template <typename T>
        auto scan_int_impl(std::string_view source, T& value, int base)
            -> scan_expected<std::string_view::iterator>;

        template <typename T>
        auto scan_int_exhaustive_valid_impl(std::string_view source) -> T;

#if !SCN_DISABLE_TYPE_SCHAR
        extern template auto scan_int_impl(std::string_view source,
                                           signed char& value,
                                           int base)
            -> scan_expected<std::string_view::iterator>;
        extern template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> signed char;
#endif
#if !SCN_DISABLE_TYPE_SHORT
        extern template auto scan_int_impl(std::string_view source,
                                           short& value,
                                           int base)
            -> scan_expected<std::string_view::iterator>;
        extern template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> short;
#endif
#if !SCN_DISABLE_TYPE_INT
        extern template auto scan_int_impl(std::string_view source,
                                           int& value,
                                           int base)
            -> scan_expected<std::string_view::iterator>;
        extern template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> int;
#endif
#if !SCN_DISABLE_TYPE_LONG
        extern template auto scan_int_impl(std::string_view source,
                                           long& value,
                                           int base)
            -> scan_expected<std::string_view::iterator>;
        extern template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> long;
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
        extern template auto scan_int_impl(std::string_view source,
                                           long long& value,
                                           int base)
            -> scan_expected<std::string_view::iterator>;
        extern template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> long long;
#endif
#if !SCN_DISABLE_TYPE_UCHAR
        extern template auto scan_int_impl(std::string_view source,
                                           unsigned char& value,
                                           int base)
            -> scan_expected<std::string_view::iterator>;
        extern template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned char;
#endif
#if !SCN_DISABLE_TYPE_USHORT
        extern template auto scan_int_impl(std::string_view source,
                                           unsigned short& value,
                                           int base)
            -> scan_expected<std::string_view::iterator>;
        extern template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned short;
#endif
#if !SCN_DISABLE_TYPE_UINT
        extern template auto scan_int_impl(std::string_view source,
                                           unsigned int& value,
                                           int base)
            -> scan_expected<std::string_view::iterator>;
        extern template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned int;
#endif
#if !SCN_DISABLE_TYPE_ULONG
        extern template auto scan_int_impl(std::string_view source,
                                           unsigned long& value,
                                           int base)
            -> scan_expected<std::string_view::iterator>;
        extern template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned long;
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
        extern template auto scan_int_impl(std::string_view source,
                                           unsigned long long& value,
                                           int base)
            -> scan_expected<std::string_view::iterator>;
        extern template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned long long;
#endif

    }  // namespace detail

    SCN_GCC_POP  // -Wnoexcept

        SCN_END_NAMESPACE
}  // namespace scn
