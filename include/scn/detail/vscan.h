
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

#ifndef SCN_DETAIL_VSCAN_H
#define SCN_DETAIL_VSCAN_H

#include "erased_stream.h"
#include "visitor.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        struct default_t {
        };
    }  // namespace detail
    namespace {
        SCN_CONSTEXPR auto&& default_tag =
            detail::static_const<detail::default_t>::value;
    }

    template <typename Context>
    scan_result vscan(Context& ctx)
    {
        return visit(ctx);
    }

#if SCN_PREDEFINE_VSCAN_OVERLOADS

#define SCN_DECLARE_VSCAN(stream, ch)          \
    scan_result vscan(basic_context<stream>&); \
    scan_result vscan(basic_context<stream, basic_locale_ref<ch>>&); \
    scan_result vscan(basic_empty_context<stream>&); \
    scan_result vscan(basic_empty_context<stream, basic_locale_ref<ch>>&); \
    scan_result vscan(basic_scanf_context<stream>&); \
    scan_result vscan(basic_scanf_context<stream, basic_locale_ref<ch>>&)

#define SCN_DECLARE_VSCAN_TEMPLATE(stream) \
    SCN_DECLARE_VSCAN(stream<char>, char); \
    SCN_DECLARE_VSCAN(stream<wchar_t>, wchar_t)

    SCN_DECLARE_VSCAN_TEMPLATE(basic_null_stream);
    SCN_DECLARE_VSCAN_TEMPLATE(basic_cstdio_stream);

    SCN_DECLARE_VSCAN(basic_bidirectional_iterator_stream<const char*>, char);
    SCN_DECLARE_VSCAN(basic_bidirectional_iterator_stream<const wchar_t*>,
                      wchar_t);

    namespace detail {
        template <typename CharT>
        using string_stream =
            basic_static_container_stream<CharT, std::basic_string<CharT>>;
        template <typename CharT>
        using span_stream =
            basic_static_container_stream<CharT, span<const CharT>>;
    }  // namespace detail
    SCN_DECLARE_VSCAN(detail::string_stream<char>, char);
    SCN_DECLARE_VSCAN(detail::string_stream<wchar_t>, wchar_t);
    SCN_DECLARE_VSCAN(detail::span_stream<char>, char);
    SCN_DECLARE_VSCAN(detail::span_stream<wchar_t>, wchar_t);

#endif // SCN_PREDEFINE_SCN_OVERLOADS

    template <typename CharT>
    using basic_erased_stream_context = basic_context<erased_stream<CharT>>;
    template <typename CharT>
    using basic_erased_sized_stream_context =
        basic_context<erased_sized_stream<CharT>>;

    using erased_stream_context = basic_erased_stream_context<char>;
    using werased_stream_context = basic_erased_stream_context<wchar_t>;
    using erased_sized_stream_context = basic_erased_sized_stream_context<char>;
    using werased_sized_stream_context =
        basic_erased_sized_stream_context<wchar_t>;

    scan_result vscan(erased_stream_context&);
    scan_result vscan(werased_stream_context&);
    scan_result vscan(erased_sized_stream_context&);
    scan_result vscan(werased_sized_stream_context&);

    template <typename Stream, bool Empty = false>
    struct erased_stream_context_type {
        using char_type = typename Stream::char_type;
        using type = typename std::conditional<
            is_sized_stream<Stream>::value,
            basic_erased_sized_stream_context<char_type>,
            basic_erased_stream_context<char_type>>::type;
    };

    template <typename CharT>
    using basic_erased_empty_stream_context =
        basic_empty_context<erased_stream<CharT>>;
    template <typename CharT>
    using basic_erased_empty_sized_stream_context =
        basic_empty_context<erased_sized_stream<CharT>>;

    using erased_empty_stream_context = basic_erased_empty_stream_context<char>;
    using werased_empty_stream_context =
        basic_erased_empty_stream_context<wchar_t>;
    using erased_empty_sized_stream_context =
        basic_erased_empty_sized_stream_context<char>;
    using werased_empty_sized_stream_context =
        basic_erased_empty_sized_stream_context<wchar_t>;

    scan_result vscan(erased_empty_stream_context&);
    scan_result vscan(werased_empty_stream_context&);
    scan_result vscan(erased_empty_sized_stream_context&);
    scan_result vscan(werased_empty_sized_stream_context&);

    template <typename Stream>
    struct erased_stream_context_type<Stream, true> {
        using char_type = typename Stream::char_type;
        using type = typename std::conditional<
            is_sized_stream<Stream>::value,
            basic_erased_empty_sized_stream_context<char_type>,
            basic_erased_empty_stream_context<char_type>>::type;
    };

    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && \
    !defined(SCN_DETAIL_VSCAN_CPP)
#include "vscan.cpp"
#endif

#endif  // SCN_DETAIL_VSCAN_H

