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

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY
#define SCN_VSCAN_CPP
#endif

#include <scn/detail/small_vector.h>
#include <scn/detail/vscan.h>

#include <scn/detail/context.h>
#include <scn/detail/parse_context.h>
#include <scn/detail/visitor.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

#if !defined(SCN_HEADER_ONLY) || !SCN_HEADER_ONLY

#define SCN_VSCAN_DEFINE(Range, WrappedAlias, CharAlias)                       \
    vscan_result<detail::vscan_macro::WrappedAlias> vscan(                     \
        detail::vscan_macro::WrappedAlias&& range,                             \
        basic_string_view<detail::vscan_macro::CharAlias> fmt,                 \
        basic_args<detail::vscan_macro::CharAlias>&& args)                     \
    {                                                                          \
        return detail::vscan_boilerplate(SCN_MOVE(range), fmt,                 \
                                         SCN_MOVE(args));                      \
    }                                                                          \
                                                                               \
    vscan_result<detail::vscan_macro::WrappedAlias> vscan_default(             \
        detail::vscan_macro::WrappedAlias&& range, int n_args,                 \
        basic_args<detail::vscan_macro::CharAlias>&& args)                     \
    {                                                                          \
        return detail::vscan_boilerplate_default(SCN_MOVE(range), n_args,      \
                                                 SCN_MOVE(args));              \
    }                                                                          \
                                                                               \
    error vscan_usertype(                                                      \
        basic_context<                                                         \
            detail::vscan_macro::WrappedAlias,                                 \
            basic_default_locale_ref<detail::vscan_macro::CharAlias>>& ctx,    \
        basic_string_view<detail::vscan_macro::CharAlias> f,                   \
        basic_args<detail::vscan_macro::CharAlias>&& args)                     \
    {                                                                          \
        auto pctx = basic_parse_context<                                       \
            basic_default_locale_ref<detail::vscan_macro::CharAlias>>(f, ctx); \
        return visit(ctx, pctx, SCN_MOVE(args));                               \
    }

    SCN_VSCAN_DEFINE(string_view, string_view_wrapped, string_view_char)
    SCN_VSCAN_DEFINE(file&, file_ref_wrapped, file_ref_char)

#endif  // !SCN_HEADER_ONLY

    SCN_END_NAMESPACE
}  // namespace scn
