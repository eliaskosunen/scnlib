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

#ifndef SCN_DETAIL_VSCAN_H
#define SCN_DETAIL_VSCAN_H

#include "context.h"
#include "file.h"
#include "visitor.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    // Avoid documentation issues: without this, Doxygen will think
    // SCN_BEGIN_NAMESPACE is a part of the vscan declaration
    namespace dummy {
    }

    /**
     * In the spirit of {fmt}/`std::format` and `vformat`, `vscan` behaves
     * similarly to \ref scan, except instead of taking a variadic argument
     * pack, it takes an object of type `basic_args`, which type-erases the
     * arguments to scan. This, in effect, will decrease generated code size and
     * compile times dramatically.
     */
    template <typename Context, typename ParseCtx>
    error vscan(Context& ctx,
                ParseCtx& pctx,
                basic_args<typename Context::char_type> args)
    {
        return visit(ctx, pctx, std::move(args));
    }

#if !defined(SCN_HEADER_ONLY) || !SCN_HEADER_ONLY

#define SCN_VSCAN_DECLARE(Range)                                            \
    error vscan(basic_context<detail::range_wrapper_for_t<Range>,           \
                              basic_default_locale_ref<char>>&,             \
                basic_parse_context<basic_default_locale_ref<char>>&,       \
                basic_args<char>);                                          \
                                                                            \
    error vscan(basic_context<detail::range_wrapper_for_t<Range>,           \
                              basic_default_locale_ref<char>>&,             \
                basic_empty_parse_context<basic_default_locale_ref<char>>&, \
                basic_args<char>);

    SCN_VSCAN_DECLARE(string_view)
    SCN_VSCAN_DECLARE(file&)

#endif  // !SCN_HEADER_ONLY

    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_VSCAN_CPP)
#include "vscan.cpp"
#endif

#endif  // SCN_DETAIL_VSCAN_H
