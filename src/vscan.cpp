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

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY
#define SCN_VSCAN_CPP
#endif

#include <scn/detail/vscan.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

#if !defined(SCN_HEADER_ONLY) || !SCN_HEADER_ONLY

#define SCN_VSCAN_DEFINE(Range)                                            \
    scan_result_for_t<basic_context<detail::range_wrapper_for_t<Range>,    \
                                    basic_default_locale_ref<char>>>       \
    vscan(basic_context<detail::range_wrapper_for_t<Range>,                \
                        basic_default_locale_ref<char>>& ctx,              \
          basic_parse_context<basic_default_locale_ref<char>>& pctx,       \
          basic_args<basic_context<detail::range_wrapper_for_t<Range>,     \
                                   basic_default_locale_ref<char>>> args)  \
    {                                                                      \
        return visit(ctx, pctx, std::move(args));                          \
    }                                                                      \
                                                                           \
    scan_result_for_t<basic_context<detail::range_wrapper_for_t<Range>,    \
                                    basic_default_locale_ref<char>>>       \
    vscan(basic_context<detail::range_wrapper_for_t<Range>,                \
                        basic_default_locale_ref<char>>& ctx,              \
          basic_empty_parse_context<basic_default_locale_ref<char>>& pctx, \
          basic_args<basic_context<detail::range_wrapper_for_t<Range>,     \
                                   basic_default_locale_ref<char>>> args)  \
    {                                                                      \
        return visit(ctx, pctx, std::move(args));                          \
    }

    SCN_VSCAN_DEFINE(string_view)
    SCN_VSCAN_DEFINE(string_view&)
    SCN_VSCAN_DEFINE(file_view)
    SCN_VSCAN_DEFINE(file_view&)

#endif  // !SCN_HEADER_ONLY

    SCN_END_NAMESPACE
}  // namespace scn
