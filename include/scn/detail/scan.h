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

#ifndef SCN_DETAIL_SCAN_H
#define SCN_DETAIL_SCAN_H

#include "vscan.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename Stream, typename... Args>
    scan_result scan(Stream& s,
                     basic_string_view<typename Stream::char_type> f,
                     Args&... a)
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_context<Stream>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(s, f, args);
        return vscan(ctx);
    }
    template <typename Stream, typename... Args>
    scan_result scan(options opt,
                     Stream& s,
                     basic_string_view<typename Stream::char_type> f,
                     Args&... a)
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type =
            basic_context<Stream, basic_locale_ref<typename Stream::char_type>,
                          options>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(s, f, args, opt);
        return vscan(ctx);
    }

    template <typename Stream, typename... Args>
    scan_result scanf(Stream& s,
                      basic_string_view<typename Stream::char_type> f,
                      Args&... a)
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_scanf_context<Stream>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(s, f, args);
        return vscan(ctx);
    }
    template <typename Stream, typename... Args>
    scan_result scanf(options opt,
                      Stream& s,
                      basic_string_view<typename Stream::char_type> f,
                      Args&... a)
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_scanf_context<
            Stream, basic_locale_ref<typename Stream::char_type>, options>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(s, f, args, opt);
        return vscan(ctx);
    }

    template <typename Stream, typename... Args>
    scan_result scan(Stream& s, detail::default_t, Args&... a)
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_empty_context<Stream>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(s, static_cast<int>(sizeof...(Args)), args);
        return vscan(ctx);
    }
    template <typename Stream, typename... Args>
    scan_result scan(options opt, Stream& s, detail::default_t, Args&... a)
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_empty_context<
            Stream, basic_locale_ref<typename Stream::char_type>, options>;

        auto args = make_args<context_type>(a...);
        auto ctx =
            context_type(s, static_cast<int>(sizeof...(Args)), args, opt);
        return vscan(ctx);
    }

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wexit-time-destructors")

    // Reference to global stdin stream.
    // Not safe to use during static construction or destruction.
    template <typename CharT>
    erased_stream<CharT>& stdin_stream()
    {
        static erased_stream<CharT> stream{basic_cstdio_stream<CharT>(stdin)};
        return stream;
    }
    inline erased_stream<char>& cstdin()
    {
        return stdin_stream<char>();
    }
    inline erased_stream<wchar_t>& wcstdin()
    {
        return stdin_stream<wchar_t>();
    }

    SCN_CLANG_POP

    // Read from stdin
    template <typename... Args>
    scan_result input(string_view f, Args&... a)
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        auto& stream = stdin_stream<char>();

        using stream_type =
            typename std::remove_reference<decltype(stream)>::type;
        using context_type =
            typename erased_stream_context_type<stream_type>::type;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(stream, f, args);
        return vscan(ctx);
    }
    template <typename... Args>
    scan_result input(wstring_view f, Args&... a)
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        auto& stream = stdin_stream<wchar_t>();

        using stream_type =
            typename std::remove_reference<decltype(stream)>::type;
        using context_type =
            typename erased_stream_context_type<stream_type>::type;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(stream, f, args);
        return vscan(ctx);
    }

    // Read from stdin with prompt
    // Like Python's input()
    template <typename... Args>
    scan_result prompt(const char* p, string_view f, Args&... a)
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");
        SCN_EXPECT(p != nullptr);

        std::printf("%s", p);

        auto& stream = stdin_stream<char>();
        using stream_type =
            typename std::remove_reference<decltype(stream)>::type;
        using context_type =
            typename erased_stream_context_type<stream_type>::type;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(stream, f, args);
        return vscan(ctx);
    }
    template <typename... Args>
    scan_result prompt(const wchar_t* p, wstring_view f, Args&... a)
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");
        SCN_EXPECT(p != nullptr);

        std::wprintf(L"%ls", p);

        auto& stream = stdin_stream<wchar_t>();
        using stream_type =
            typename std::remove_reference<decltype(stream)>::type;
        using context_type =
            typename erased_stream_context_type<stream_type>::type;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(stream, f, args);
        return vscan(ctx);
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_SCAN_H
