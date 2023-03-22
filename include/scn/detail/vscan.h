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

    vscan_result<std::string_view> vscan(
        std::string_view source,
        std::string_view format,
        scan_args_for<std::string_view, char> args);
    vscan_result<erased_subrange> vscan(
        erased_subrange source,
        std::string_view format,
        scan_args_for<erased_subrange, char> args);
#if SCN_USE_IOSTREAMS
    vscan_result<istreambuf_subrange> vscan(
        istreambuf_subrange source,
        std::string_view format,
        scan_args_for<istreambuf_subrange, char> args);
#endif

    template <typename Locale,
              typename = std::void_t<decltype(Locale::classic())>>
    vscan_result<std::string_view> vscan(
        Locale& loc,
        std::string_view source,
        std::string_view format,
        scan_args_for<std::string_view, char> args);
    template <typename Locale,
              typename = std::void_t<decltype(Locale::classic())>>
    vscan_result<erased_subrange> vscan(
        Locale& loc,
        erased_subrange source,
        std::string_view format,
        scan_args_for<erased_subrange, char> args);
#if SCN_USE_IOSTREAMS
    template <typename Locale,
              typename = std::void_t<decltype(Locale::classic())>>
    vscan_result<istreambuf_subrange> vscan(
        Locale& loc,
        istreambuf_subrange source,
        std::string_view format,
        scan_args_for<istreambuf_subrange, char> args);
#endif

    vscan_result<std::string_view> vscan_value(
        std::string_view source,
        scan_arg_for<std::string_view, char> arg);
    vscan_result<erased_subrange> vscan_value(
        erased_subrange source,
        scan_arg_for<erased_subrange, char> arg);
#if SCN_USE_IOSTREAMS
    vscan_result<istreambuf_subrange> vscan_value(
        istreambuf_subrange source,
        scan_arg_for<istreambuf_subrange, char> arg);
#endif

#if SCN_USE_IOSTREAMS
    vscan_result<istreambuf_subrange> vscan_and_sync(
        istreambuf_subrange source,
        std::string_view format,
        scan_args_for<istreambuf_subrange, char> args);
#endif

    SCN_END_NAMESPACE
}  // namespace scn
