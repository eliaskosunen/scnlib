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

#ifndef SCN_SCN_H
#define SCN_SCN_H

#include "detail/args.h"
#include "detail/context.h"
#include "detail/core.h"
#include "detail/locale.h"
#include "detail/result.h"
#include "detail/stream.h"
#include "detail/types.h"
#include "detail/visitor.h"

namespace scn {
    /**
     * Non-variadic version of scan(), to prevent bloat in generated code.
     */
    template <typename Context>
    error vscan(Context& ctx)
    {
        return visit(ctx);
    }

    /**
     * Scan the given arguments from the stream.
     * \param f Scanning options
     */
    template <typename Stream, typename... Args>
    error scan(Stream& s,
               basic_string_view<typename Stream::char_type> f,
               Args&... a)
    {
        using context_type = basic_context<Stream>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(s, f, args);
        return vscan<context_type>(ctx);
    }
    /**
     * Scan the given arguments from the stream, using locale loc.
     * Noticeably slower compared to non-localized scan(), avoid if possible.
     * \param loc `std::locale` to use
     * \param f   Scanning options
     */
    template <typename Locale, typename Stream, typename... Args>
    error scan(const Locale& loc,
               Stream& s,
               basic_string_view<typename Stream::char_type> f,
               Args&... a)
    {
        using context_type = basic_context<Stream>;

        auto args = make_args<context_type>(a...);
        auto locale = basic_locale_ref<typename Stream::char_type>(
            static_cast<const void*>(std::addressof(loc)));
        auto ctx = context_type(s, f, std::move(args), std::move(locale));
        return vscan<context_type>(ctx);
    }

    /**
     * Reference to global stdin stream.
     * Not safe to use during static construction or destruction.
     */
    template <typename CharT>
    basic_cstdio_stream<CharT>& stdin_stream()
    {
        static basic_cstdio_stream<CharT> stream(stdin);
        return stream;
    }

    /**
     * Reference to narrow global stdin stream.
     * Not safe to use during static construction or destruction.
     */
    inline basic_cstdio_stream<char>& cstdin()
    {
        return stdin_stream<char>();
    }
    /**
     * Reference to wide global stdin stream.
     * Not safe to use during static construction or destruction.
     */
    inline basic_cstdio_stream<wchar_t>& wcstdin()
    {
        return stdin_stream<wchar_t>();
    }

    /**
     * Scan from stdin.
     * \param f Scanning options
     */
    template <typename... Args>
    error input(string_view f, Args&... a)
    {
        auto& stream = stdin_stream<char>();
        using stream_type = std::remove_reference<decltype(stream)>::type;
        using context_type = basic_context<stream_type>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(stream, f, args);
        return vscan<context_type>(ctx);
    }
    /**
     * Wide scan from stdin.
     * \param f Scanning options
     */
    template <typename... Args>
    error winput(wstring_view f, Args&... a)
    {
        auto& stream = stdin_stream<wchar_t>();
        using stream_type = std::remove_reference<decltype(stream)>::type;
        using context_type = basic_context<stream_type>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(stream, f, args);
        return vscan<context_type>(ctx);
    }

    template <typename... Args>
    error prompt(const char* p, string_view f, Args&... a)
    {
        std::printf("%s", p);

        auto& stream = stdin_stream<char>();
        using stream_type = std::remove_reference<decltype(stream)>::type;
        using context_type = basic_context<stream_type>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(stream, f, args);
        return vscan<context_type>(ctx);
    }
    template <typename... Args>
    error wprompt(const wchar_t* p, wstring_view f, Args&... a)
    {
        std::wprintf(L"%ls", p);

        auto& stream = stdin_stream<wchar_t>();
        using stream_type = std::remove_reference<decltype(stream)>::type;
        using context_type = basic_context<stream_type>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(stream, f, args);
        return vscan<context_type>(ctx);
    }

#if 0
    template <typename Source,
              typename CharT =
                  decltype(make_stream(std::declval<const Source&>()), void()),
              typename... Args>
    error sscan(const Source& s,
                                basic_string_view<CharT> f,
                                Args&... a)
    {
        auto stream = make_stream(s);

        using stream_type = decltype(stream);
        using context_type = basic_context<stream_type>;
        using args_type = basic_arg<context_type>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(s, f);
        return vscan<stream_type, context_type>(s, ctx, args);
    }
#endif
}  // namespace scn

#endif  // SCN_SCN_H

