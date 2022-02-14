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

#ifndef SCN_DETAIL_FWD_H
#define SCN_DETAIL_FWD_H

#include "config.h"

#include <cstddef>

namespace scn {
    SCN_BEGIN_NAMESPACE

    // args.h

    template <typename CharT>
    class basic_arg;
    template <typename CharT>
    class basic_args;
    template <typename CharT, typename... Args>
    class arg_store;

    template <typename T>
    struct temporary;

    // locale.h

    template <typename CharT>
    class basic_locale_ref;

    // context.h

    template <typename WrappedRange>
    class basic_context;

    // parse_context.h

    template <typename CharT>
    class basic_parse_context;
    template <typename CharT>
    class basic_empty_parse_context;

    namespace detail {
        template <typename T>
        struct parse_context_template_for_format;
    }

    // reader/common.h

    template <typename T, typename Enable = void>
    struct scanner;

    // defined here to avoid including <scn.h> if the user wants to create a
    // scanner for their own type
    /**
     * Base class for all scanners.
     * User-defined scanner must derive from this type.
     */
    struct parser_base {
        /**
         * Returns `true` if `skip_range_whitespace()` is to be called before
         * scanning this value.
         *
         * Defaults to `true`. Is `false` for chars, code points and strings
         * when using set scanning.
         */
        static constexpr bool skip_preceding_whitespace()
        {
            return true;
        }
        /**
         * Returns `true` if this scanner supports parsing align and fill
         * specifiers from the format string, and then scanning them.
         *
         * Defaults to `false`, `true` for all scnlib-defined scanners.
         */
        static constexpr bool support_align_and_fill()
        {
            return false;
        }
    };

    struct empty_parser;
    struct common_parser;
    struct common_parser_default;

    namespace detail {
        template <typename T>
        struct simple_integer_scanner;
    }

    // result.h

    class SCN_TRIVIAL_ABI error;

    template <typename T, typename Error = ::scn::error, typename Enable = void>
    class expected;

    // small_vector.h

    template <typename T, std::size_t StackN>
    class small_vector;

    // span.h

    template <typename T>
    class span;

    // string_view.h

    template <typename CharT>
    class basic_string_view;

    using string_view = basic_string_view<char>;
    using wstring_view = basic_string_view<wchar_t>;

    // visitor.h

    template <typename Context, typename ParseCtx>
    class basic_visitor;

    // util.h

    template <typename T>
    class optional;

    // file.h

    template <typename CharT>
    class basic_mapped_file;
    template <typename CharT>
    class basic_file;
    template <typename CharT>
    class basic_owning_file;

    using mapped_file = basic_mapped_file<char>;
    using file = basic_file<char>;
    using owning_file = basic_owning_file<char>;

    using mapped_wfile = basic_mapped_file<wchar_t>;
    using wfile = basic_file<wchar_t>;
    using owning_wfile = basic_owning_file<wchar_t>;

    // scan.h

    template <typename T>
    struct span_list_wrapper;
    template <typename T>
    struct discard_type;

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_FWD_H
