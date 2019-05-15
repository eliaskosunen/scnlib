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
#define SCN_DETAIL_VSCAN_CPP
#endif

#include <scn/detail/vscan.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

#define SCN_DEFINE_VSCAN_CTX(context)        \
    SCN_FUNC scan_result vscan(context& ctx) \
    {                                        \
        return visit(ctx);                   \
    }

    SCN_DEFINE_VSCAN_CTX(erased_stream_context)
    SCN_DEFINE_VSCAN_CTX(werased_stream_context)
    SCN_DEFINE_VSCAN_CTX(erased_sized_stream_context)
    SCN_DEFINE_VSCAN_CTX(werased_sized_stream_context)

    SCN_DEFINE_VSCAN_CTX(erased_empty_stream_context)
    SCN_DEFINE_VSCAN_CTX(werased_empty_stream_context)
    SCN_DEFINE_VSCAN_CTX(erased_empty_sized_stream_context)
    SCN_DEFINE_VSCAN_CTX(werased_empty_sized_stream_context)

#if SCN_PREDEFINE_VSCAN_OVERLOADS

#define SCN_DEFINE_VSCAN(stream, ch)                             \
    SCN_FUNC scan_result vscan(basic_context<stream>& ctx)       \
    {                                                            \
        return visit(ctx);                                       \
    }                                                            \
    SCN_FUNC scan_result vscan(                                  \
        basic_context<stream, basic_locale_ref<ch>>& ctx)        \
    {                                                            \
        return visit(ctx);                                       \
    }                                                            \
    SCN_FUNC scan_result vscan(basic_empty_context<stream>& ctx) \
    {                                                            \
        return visit(ctx);                                       \
    }                                                            \
    SCN_FUNC scan_result vscan(                                  \
        basic_empty_context<stream, basic_locale_ref<ch>>& ctx)  \
    {                                                            \
        return visit(ctx);                                       \
    }                                                            \
    SCN_FUNC scan_result vscan(basic_scanf_context<stream>& ctx) \
    {                                                            \
        return visit(ctx);                                       \
    }                                                            \
    SCN_FUNC scan_result vscan(                                  \
        basic_scanf_context<stream, basic_locale_ref<ch>>& ctx)  \
    {                                                            \
        return visit(ctx);                                       \
    }
#define SCN_DEFINE_VSCAN_TEMPLATE(stream) \
    SCN_DEFINE_VSCAN(stream<char>, char)  \
    SCN_DEFINE_VSCAN(stream<wchar_t>, wchar_t)

    SCN_DEFINE_VSCAN_TEMPLATE(basic_null_stream)
    SCN_DEFINE_VSCAN_TEMPLATE(basic_cstdio_stream)

    SCN_DEFINE_VSCAN(basic_bidirectional_iterator_stream<const char*>, char)
    SCN_DEFINE_VSCAN(basic_bidirectional_iterator_stream<const wchar_t*>,
                     wchar_t)

    SCN_DEFINE_VSCAN(detail::string_stream<char>, char)
    SCN_DEFINE_VSCAN(detail::string_stream<wchar_t>, wchar_t)

    SCN_DEFINE_VSCAN(detail::span_stream<char>, char)
    SCN_DEFINE_VSCAN(detail::span_stream<wchar_t>, wchar_t)

#endif // SCN_PREDEFINE_VSCAN_OVERLOADS

    SCN_END_NAMESPACE
}  // namespace scn
