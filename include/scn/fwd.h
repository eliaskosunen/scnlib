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

#include <scn/detail/pp.h>

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")
SCN_GCC_IGNORE("-Wrestrict")

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

SCN_GCC_POP

/**
 * scnlib namespace, containing the library interface
 */
namespace scn {
    SCN_BEGIN_NAMESPACE

    /// Placeholder monostate type
    struct monostate {};

    // detail/args.h

    template <typename Context>
    class basic_scan_arg;
    template <typename Context, typename... Args>
    class scan_arg_store;
    template <typename Context>
    class basic_scan_args;

    // detail/caching_view.h

    template <typename Range>
    class basic_caching_view;
    template <typename Range>
    class basic_caching_subrange;

    // detail/context.h

    template <typename Range, typename CharT>
    class basic_scan_context;

#if !SCN_DISABLE_ERASED_RANGE

    // detail/erased_range.h

    template <typename CharT>
    class basic_erased_range;

    ///
    using erased_range = basic_erased_range<char>;
    ///
    using werased_range = basic_erased_range<wchar_t>;

    template <typename CharT>
    struct basic_erased_subrange;

    ///
    using erased_subrange = basic_erased_subrange<char>;
    ///
    using werased_subrange = basic_erased_subrange<wchar_t>;

#endif  // !SCN_DISABLE_ERASED_RANGE

    // detail/error.h

    class scan_error;

    // detail/format_string.h

    template <typename CharT>
    struct basic_runtime_format_string;
    template <typename CharT, typename... Args>
    class basic_format_string;

    namespace detail {
        template <typename T>
        struct type_identity {
            using type = T;
        };
        template <typename T>
        using type_identity_t = typename type_identity<T>::type;
    }  // namespace detail

    template <typename... Args>
    using format_string =
        basic_format_string<char, detail::type_identity_t<Args>...>;
    template <typename... Args>
    using wformat_string =
        basic_format_string<wchar_t, detail::type_identity_t<Args>...>;

    // detail/format_string_parser.h: empty

    // detail/input_map.h

    struct invalid_input_range;

#if !SCN_DISABLE_IOSTREAM

    // detail/istream_scanner.h

    template <typename CharT>
    struct basic_istream_scanner;

    ///
    using istream_scanner = basic_istream_scanner<char>;
    ///
    using wistream_scanner = basic_istream_scanner<wchar_t>;

#endif  // SCN_USE_IOSTREAMS

    // detail/locale_ref.h: empty

    // detail/parse_context.h

    template <typename CharT>
    class basic_scan_parse_context;

    ///
    using scan_parse_context = basic_scan_parse_context<char>;
    ///
    using wscan_parse_context = basic_scan_parse_context<wchar_t>;

    namespace detail {
        template <typename CharT>
        class compile_parse_context;
    }

    // detail/ranges.h: empty

    // detail/result.h

    template <typename Iterator, typename... Args>
    class scan_result;

    // detail/scan.h

    struct stdin_range_marker {
        constexpr stdin_range_marker() SCN_NOEXCEPT = default;
        template <typename... Args>
        constexpr stdin_range_marker(Args&&...) SCN_NOEXCEPT
        {
        }
    };

    // detail/scanner.h

    /**
     * Scanner type, can be customized to enable scanning of user-defined types
     *
     * \ingroup ctx
     */
    template <typename T, typename CharT = char, typename Enable = void>
    struct scanner {
        scanner() = delete;
    };

    template <typename T, typename CharT>
    inline constexpr bool has_scanner =
        std::is_constructible_v<scanner<T, CharT>>;

    template <typename T>
    struct discard;

    // detail/scanner_builtin.h: empty

    // detail/unicode.h: empty

    // detail/visitor.h, detail/vscan.h: empty

    // util/algorithm.h: empty

    // util/buffer.h

    namespace detail {
        template <typename T, size_t N>
        class basic_buffer;
    }  // namespace detail

    // util/expected.h: empty

    // util/memory.h

    namespace detail {
        template <typename T, typename = void>
        struct pointer_traits;
    }  // namespace detail

    // util/meta.h

    namespace detail {
        struct dummy_type {};

        template <typename T>
        struct tag_type {
            using type = T;
        };

        template <typename>
        struct dependent_false : std::false_type {};

        template <typename T>
        struct remove_reference {
            using type = T;
        };
        template <typename T>
        struct remove_reference<T&> {
            using type = T;
        };
        template <typename T>
        struct remove_reference<T&&> {
            using type = T;
        };
    }  // namespace detail

    // util/span.h

    template <typename T>
    class span;

    // util/string_view.h: empty

    /// Private implementation namespace
    namespace detail {
    }

    // detail/regex.h:

    template <typename CharT>
    struct basic_regex_matches;

    using regex_matches = basic_regex_matches<char>;
    using wregex_matches = basic_regex_matches<wchar_t>;

    SCN_END_NAMESPACE
}  // namespace scn
