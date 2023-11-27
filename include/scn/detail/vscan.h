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

    /**
     * `basic_scan_arg` type used in the signature of `vscan_value`, when
     * scanning a range of type `Range`.
     *
     * \ingroup vscan
     */
    template <typename Range, typename CharT>
    using scan_arg_for = basic_scan_arg<
        basic_scan_context<detail::decayed_mapped_source_range<Range>, CharT>>;

    /**
     * `basic_scan_args` type used in the signature of `vscan`, when scanning a
     * range of type `Range`.
     *
     * \ingroup vscan
     */
    template <typename Range, typename CharT>
    using scan_args_for = basic_scan_args<
        basic_scan_context<detail::decayed_mapped_source_range<Range>, CharT>>;

    namespace detail {
        template <typename R>
        using vscan_impl_result = scan_expected<typename R::iterator>;

        vscan_impl_result<std::string_view> vscan_impl(
            std::string_view source,
            std::string_view format,
            scan_args_for<std::string_view, char> args);
#if !SCN_DISABLE_ERASED_RANGE
        vscan_impl_result<erased_subrange> vscan_impl(
            erased_subrange source,
            std::string_view format,
            scan_args_for<erased_subrange, char> args);
#endif

        vscan_impl_result<std::wstring_view> vscan_impl(
            std::wstring_view source,
            std::wstring_view format,
            scan_args_for<std::wstring_view, wchar_t> args);
#if !SCN_DISABLE_ERASED_RANGE
        vscan_impl_result<werased_subrange> vscan_impl(
            werased_subrange source,
            std::wstring_view format,
            scan_args_for<werased_subrange, wchar_t> args);
#endif

#if !SCN_DISABLE_LOCALE
        template <typename Locale>
        vscan_impl_result<std::string_view> vscan_localized_impl(
            const Locale& loc,
            std::string_view source,
            std::string_view format,
            scan_args_for<std::string_view, char> args);
#if !SCN_DISABLE_ERASED_RANGE
        template <typename Locale>
        vscan_impl_result<erased_subrange> vscan_localized_impl(
            const Locale& loc,
            erased_subrange source,
            std::string_view format,
            scan_args_for<erased_subrange, char> args);
#endif

        template <typename Locale>
        vscan_impl_result<std::wstring_view> vscan_localized_impl(
            const Locale& loc,
            std::wstring_view source,
            std::wstring_view format,
            scan_args_for<std::wstring_view, wchar_t> args);
#if !SCN_DISABLE_ERASED_RANGE
        template <typename Locale>
        vscan_impl_result<werased_subrange> vscan_localized_impl(
            const Locale& loc,
            werased_subrange source,
            std::wstring_view format,
            scan_args_for<werased_subrange, wchar_t> args);
#endif
#endif  // !SCN_DISABLE_LOCALE

        vscan_impl_result<std::string_view> vscan_value_impl(
            std::string_view source,
            scan_arg_for<std::string_view, char> arg);
#if !SCN_DISABLE_ERASED_RANGE
        vscan_impl_result<erased_subrange> vscan_value_impl(
            erased_subrange source,
            scan_arg_for<erased_subrange, char> arg);
#endif

        vscan_impl_result<std::wstring_view> vscan_value_impl(
            std::wstring_view source,
            scan_arg_for<std::wstring_view, wchar_t> arg);
#if !SCN_DISABLE_ERASED_RANGE
        vscan_impl_result<werased_subrange> vscan_value_impl(
            werased_subrange source,
            scan_arg_for<werased_subrange, wchar_t> arg);
#endif
    }  // namespace detail

    /**
     * Result type returned by `vscan`.
     *
     * \ingroup vscan
     */
    template <typename Range>
    using vscan_result =
        scan_expected<borrowed_subrange_with_sentinel_t<Range>>;

    namespace detail {
        template <typename Range, typename Format, typename Args>
        auto vscan_generic(Range&& range, Format format, Args args)
            -> vscan_result<Range>
        {
            auto mapped_range = scan_map_input_range(range);

            auto result = vscan_impl(mapped_range, format, args);
            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }
            return map_scan_result_range(SCN_FWD(range), mapped_range.begin(),
                                         *result);
        }

        template <typename Locale,
                  typename Range,
                  typename Format,
                  typename Args>
        auto vscan_localized_generic(const Locale& loc,
                                     Range&& range,
                                     Format format,
                                     Args args) -> vscan_result<Range>
        {
#if !SCN_DISABLE_LOCALE
            auto mapped_range = scan_map_input_range(range);

            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
            auto result = vscan_localized_impl(loc, mapped_range, format, args);
            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }
            return map_scan_result_range(SCN_FWD(range), mapped_range.begin(),
                                         *result);
#else
            static_assert(
                dependent_false<Locale>::value,
                "Can't use scan(locale, ...) with SCN_DISABLE_LOCALE on");

            return {};
#endif
        };

        template <typename Range, typename Arg>
        auto vscan_value_generic(Range&& range, Arg arg) -> vscan_result<Range>
        {
            auto mapped_range = scan_map_input_range(range);

            auto result = vscan_value_impl(mapped_range, arg);
            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }
            return map_scan_result_range(SCN_FWD(range), mapped_range.begin(),
                                         *result);
        }

#if !SCN_DISABLE_IOSTREAM
        template <typename Range, typename Format, typename Args>
        auto vscan_and_sync_generic(Range&& range, Format format, Args args)
            -> vscan_result<Range>
        {
            auto mapped_range = scan_map_input_range(range);

            auto result = vscan_and_sync_impl(mapped_range, format, args);
            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }
            return map_scan_result_range(SCN_FWD(range), mapped_range.begin(),
                                         *result);
        }
#endif
    }  // namespace detail

    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wnoexcept")

    /**
     * Perform actual scanning from `range`, according to `format`, into the
     * type-erased arguments at `args`. Called by `scan`.
     *
     * \ingroup vscan
     */
    template <typename Range>
    auto vscan(Range&& range,
               std::string_view format,
               scan_args_for<Range, char> args) -> vscan_result<Range>
    {
        return detail::vscan_generic(SCN_FWD(range), format, args);
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
               scan_args_for<Range, char> args) -> vscan_result<Range>
    {
        return detail::vscan_localized_generic(loc, SCN_FWD(range), format,
                                               args);
    }

    /**
     * Perform actual scanning from `range` into the type-erased argument at
     * `arg`. Called by `scan_value`.
     *
     * \ingroup vscan
     */
    template <typename Range>
    auto vscan_value(Range&& range, scan_arg_for<Range, char> arg)
        -> vscan_result<Range>
    {
        return detail::vscan_value_generic(SCN_FWD(range), arg);
    }

    scan_error vinput(std::string_view format,
                      scan_args_for<detail::stdin_subrange, char> args);

#if !SCN_DISABLE_LOCALE
    template <typename Locale,
              typename = std::void_t<decltype(Locale::classic())>>
    scan_error vinput(const Locale& loc,
                      std::string_view format,
                      scan_args_for<detail::stdin_subrange, char> args);
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
