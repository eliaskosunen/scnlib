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

#pragma once

#include <scn/detail/context.h>
#include <scn/detail/istream_range.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

#define SCN_DECLARE_VSCAN(Range, CharT)                             \
    vscan_result<Range> vscan(Range source,                         \
                              std::basic_string_view<CharT> format, \
                              scan_args_for<Range, CharT> args);    \
    template <typename Locale,                                      \
              typename = std::void_t<decltype(Locale::classic())>>  \
    vscan_result<Range> vscan(Locale& loc, Range source,            \
                              std::basic_string_view<CharT> format, \
                              scan_args_for<Range, CharT> args);    \
    vscan_result<Range> vscan_value(Range source,                   \
                                    scan_arg_for<Range, CharT> arg);

    SCN_DECLARE_VSCAN(std::string_view, char)
    SCN_DECLARE_VSCAN(std::wstring_view, wchar_t)

#if SCN_USE_IOSTREAMS
    SCN_DECLARE_VSCAN(istreambuf_subrange, char)
    SCN_DECLARE_VSCAN(wistreambuf_subrange, wchar_t)
#endif

    SCN_DECLARE_VSCAN(erased_subrange, char)
    SCN_DECLARE_VSCAN(werased_subrange, wchar_t)

#undef SCN_DECLARE_VSCAN

#if SCN_USE_IOSTREAMS
    vscan_result<istreambuf_subrange> vscan_and_sync(
        istreambuf_subrange source,
        std::string_view format,
        scan_args_for<istreambuf_subrange, char> args);
    vscan_result<wistreambuf_subrange> vscan_and_sync(
        wistreambuf_subrange source,
        std::wstring_view format,
        scan_args_for<wistreambuf_subrange, wchar_t> args);
#endif

    SCN_END_NAMESPACE
}  // namespace scn
