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

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

namespace scn {
    SCN_BEGIN_NAMESPACE

    // detail/args.h

    template <typename Context>
    class basic_scan_arg;
    template <typename Context, typename... Args>
    class scan_arg_store;
    template <typename Context>
    class basic_scan_args;

    // detail/caching_view.h

    namespace detail {
        template <typename Range>
        class basic_caching_view;
        template <typename Range>
        class basic_caching_subrange;
    }  // namespace detail

    // detail/context.h

    template <typename Range, typename CharT>
    class basic_scan_context;

    template <typename Range, typename CharT>
    using scan_arg_for = basic_scan_arg<basic_scan_context<Range, CharT>>;
    template <typename Range, typename CharT>
    using scan_args_for = basic_scan_args<basic_scan_context<Range, CharT>>;

    // detail/erased_range.h

    template <typename CharT>
    class basic_erased_range;

    using erased_range = basic_erased_range<char>;
    using werased_range = basic_erased_range<wchar_t>;

    template <typename CharT>
    struct basic_erased_subrange;

    using erased_subrange = basic_erased_subrange<char>;
    using werased_subrange = basic_erased_subrange<wchar_t>;

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

#if SCN_USE_IOSTREAMS

    // detail/istream_range.h

    namespace detail {
        template <typename CharT>
        class basic_input_istreambuf_view;
    }

    template <typename CharT>
    class basic_istreambuf_view;

    using istreambuf_view = basic_istreambuf_view<char>;
    using wistreambuf_view = basic_istreambuf_view<wchar_t>;

    template <typename CharT>
    class basic_istreambuf_subrange;

    using istreambuf_subrange = basic_istreambuf_subrange<char>;
    using wistreambuf_subrange = basic_istreambuf_subrange<wchar_t>;

    // detail/istream_scanner.h

    template <typename CharT>
    struct basic_istream_scanner;

    using istream_scanner = basic_istream_scanner<char>;
    using wistream_scanner = basic_istream_scanner<wchar_t>;

#endif  // SCN_USE_IOSTREAMS

    // detail/locale_ref.h: empty

    // detail/parse_context.h

    template <typename CharT>
    class basic_scan_parse_context;

    using scan_parse_context = basic_scan_parse_context<char>;
    using wscan_parse_context = basic_scan_parse_context<wchar_t>;

    namespace detail {
        template <typename CharT>
        class compile_parse_context;
    }

    // detail/ranges.h: empty

    // detail/result.h

    template <typename Range>
    struct vscan_result;

    template <typename ResultMappedRange>
    class scan_result;
    template <typename ResultMappedRange, typename... Args>
    class scan_result_tuple;

    // detail/scan.h: empty

    // detail/scanner.h

    template <typename T, typename CharT, typename Enable = void>
    struct scanner {
        scanner() = delete;
    };

    template <typename T, typename CharT>
    inline constexpr bool has_scanner =
        std::is_constructible_v<scanner<T, CharT>>;

    template <typename T>
    struct discard;

    // detail/scanner_builtin.h: empty

    // detail/unicode.h

    enum class code_point : std::uint32_t {};

    // detail/visitor.h, detail/vscan.h: empty

    // util/algorithm.h, util/expected.h: empty

    // util/memory.h

    namespace detail {
        template <typename T>
        struct pointer_traits;
    }  // namespace detail

    // util/meta.h

    namespace detail {
        struct dummy_type {};

        template <typename>
        struct tag_type {};

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

    // util/optional.h: empty

    // util/span.h

    template <typename T>
    class span;

    // util/string_view.h: empty

    // util/unique_ptr.h

    namespace detail {
        template <typename T>
        class unique_ptr;
    }

    SCN_END_NAMESPACE
}  // namespace scn
