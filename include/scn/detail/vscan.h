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

#include "context.h"
#include "file.h"
#include "visitor.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        struct default_t {
        };
    }  // namespace detail
    namespace {
        constexpr auto& default_tag =
            detail::static_const<detail::default_t>::value;
    }

    /**
     * \defgroup scan_erase Type-erased scanning operations
     *
     * These functions are called by \ref scan and alike.
     * The passed arguments are type-erased and passed in the context to avoid
     * generated code size blowup.
     */

    /// @{

    template <typename Context, typename ParseCtx>
    scan_result_for_t<Context> vscan(Context& ctx,
                                     ParseCtx& pctx,
                                     basic_args<Context> args)
    {
        return visit(ctx, pctx, args);
    }

#if !defined(SCN_HEADER_ONLY) || !SCN_HEADER_ONLY

#define SCN_VSCAN_DECLARE(Range)                                        \
    scan_result_for_t<basic_context<detail::range_wrapper_for_t<Range>, \
                                    basic_default_locale_ref<char>>>    \
    vscan(basic_context<detail::range_wrapper_for_t<Range>,             \
                        basic_default_locale_ref<char>>&,               \
          basic_parse_context<basic_default_locale_ref<char>>&,         \
          basic_args<basic_context<detail::range_wrapper_for_t<Range>,  \
                                   basic_default_locale_ref<char>>>);   \
                                                                        \
    scan_result_for_t<basic_context<detail::range_wrapper_for_t<Range>, \
                                    basic_default_locale_ref<char>>>    \
    vscan(basic_context<detail::range_wrapper_for_t<Range>,             \
                        basic_default_locale_ref<char>>&,               \
          basic_empty_parse_context<basic_default_locale_ref<char>>&,   \
          basic_args<basic_context<detail::range_wrapper_for_t<Range>,  \
                                   basic_default_locale_ref<char>>>);

    SCN_VSCAN_DECLARE(string_view)
    SCN_VSCAN_DECLARE(string_view&)
    SCN_VSCAN_DECLARE(file_view)
    SCN_VSCAN_DECLARE(file_view&)

#endif  // !SCN_HEADER_ONLY

    /// @}

    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_VSCAN_CPP)
#include "vscan.cpp"
#endif

#endif  // SCN_DETAIL_VSCAN_H
