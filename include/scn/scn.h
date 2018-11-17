// Copyright 2017-2018 Elias Kosunen
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

// The contents of this library are heavily influenced by fmtlib and its
// derivative works:
//     https://github.com/fmtlib/fmt
//     https://fmtlib.net
//     https://wg21.link/p0645
// fmtlib is licensed under the BSD 2-clause license.
// Copyright (c) 2012-2018 Victor Zverovich

#ifndef SCN_SCN_H
#define SCN_SCN_H

#include "scn/args.h"
#include "scn/core.h"
#include "scn/locale.h"
#include "scn/stream.h"
#include "scn/types.h"

namespace scn {
    template <typename Stream, typename Context>
    expected<void, error> vscan(Stream s, Context ctx, basic_args<Context> a)
    {
        return a.visit(ctx);
    }

    template <typename Stream, typename... Args>
    expected<void, error> scan(Stream s,
                               basic_string_view<typename Stream::char_type> f,
                               Args&... a)
    {
        using context_type = basic_context<Stream>;
        using args_type = basic_arg<context_type>;

        auto args = make_args<context_type>(a...);
        auto ctx = context_type(s, f);
        return vscan<Stream, context_type>(s, ctx, args);
    }
    template <typename Locale, typename Stream, typename... Args>
    expected<void, error> scan(const Locale& loc,
                               Stream s,
                               basic_string_view<typename Stream::char_type> f,
                               Args&... a)
    {
        using context_type = basic_context<Stream>;
        using args_type = basic_arg<context_type>;

        auto args = make_args<context_type>(a...);
        auto locale = locale_ref<typename Stream::char_type>(
            static_cast<const void*>(std::addressof(loc)));
        auto ctx = context_type(s, f);
        return vscan<Stream, context_type>(s, ctx, args);
    }

#if 0
    template <typename Source,
              typename CharT =
                  decltype(make_stream(std::declval<const Source&>()), void()),
              typename... Args>
    expected<void, error> sscan(const Source& s,
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