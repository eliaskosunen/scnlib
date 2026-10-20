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

#include "fuzz.h"

#include <scn/chrono.h>

namespace scn::fuzz {
namespace {
template <typename Duration>
using sys_time = std::chrono::time_point<std::chrono::system_clock, Duration>;
}  // namespace

template <typename CharT, typename Source>
void do_basic_run_for_source(Source&& source,
                             std::basic_string_view<CharT> format_string)
{
    do_basic_run_for_type<CharT, std::tm>(source, format_string);
    do_basic_run_for_type<CharT, tm_with_tz>(source, format_string);
    do_basic_run_for_type<CharT, datetime_components>(source, format_string);
    do_basic_run_for_type<CharT, sys_time<std::chrono::seconds>>(source,
                                                                 format_string);
    do_basic_run_for_type<CharT, sys_time<std::chrono::milliseconds>>(
        source, format_string);
    do_basic_run_for_type<CharT, sys_time<std::chrono::nanoseconds>>(
        source, format_string);
    do_basic_run_for_type<CharT, year_month_day>(source, format_string);
}

namespace {
// If the first byte of the input is this, the input is
// `format string NUL data`. Otherwise, the data is scanned with a fixed set of
// format strings.
constexpr std::uint8_t custom_format_mode = 1;

template <typename CharT>
std::basic_string<CharT> widen(std::string_view str)
{
    std::basic_string<CharT> result;
    result.reserve(str.size());
    for (char ch : str) {
        result.push_back(static_cast<CharT>(static_cast<unsigned char>(ch)));
    }
    return result;
}

template <typename CharT>
void run_with_custom_format(std::string_view format, std::string_view data)
{
    // Both as is, and as the contents of a replacement field
    const auto raw_format = widen<CharT>(format);
    const auto field_format =
        widen<CharT>("{:") + raw_format + widen<CharT>("}");
    const auto formats = format_strings_type<CharT>{raw_format, field_format};

    do_basic_run(widen<CharT>(data), formats);
}

void run_with_custom_format(const std::uint8_t* data, std::size_t size)
{
    const std::string_view input{reinterpret_cast<const char*>(data) + 1,
                                 size - 1};
    const auto nul = input.find('\0');
    const auto format = input.substr(0, nul);
    const auto rest = nul == std::string_view::npos ? std::string_view{}
                                                    : input.substr(nul + 1);

    run_with_custom_format<char>(format, rest);
    run_with_custom_format<wchar_t>(format, rest);
}

void run(const std::uint8_t* data, std::size_t size)
{
    if (size > max_input_bytes || size == 0) {
        return;
    }

    if (data[0] == custom_format_mode) {
        run_with_custom_format(data, size);
        return;
    }

    const auto inputs = make_input_views(data, size);

    const auto f = format_strings_type<char>{"{:%T}",
                                             "{:%R}",
                                             "{:%D}",
                                             "{:%F}",
                                             "{:%Y-%m-%dT%H:%M:%S%z}",
                                             "{:%F %T}",
                                             "{:%Y-%m-%d %H:%M:%.S}",
                                             "{:%a, %d %b %Y %H:%M:%S}",
                                             "{:L%c}",
                                             "{:%a}",
                                             "{:%b}"};
    do_basic_run(inputs.narrow, f);

    const auto wf = format_strings_type<wchar_t>{L"{:%T}",
                                                 L"{:%R}",
                                                 L"{:%D}",
                                                 L"{:%F}",
                                                 L"{:%Y-%m-%dT%H:%M:%S%z}",
                                                 L"{:%F %T}",
                                                 L"{:%Y-%m-%d %H:%M:%.S}",
                                                 L"{:%a, %d %b %Y %H:%M:%S}",
                                                 L"{:L%c}",
                                                 L"{:%a}",
                                                 L"{:%b}"};
    do_basic_run(inputs.wide_copied, wf);
    do_basic_run(inputs.wide_reinterpreted, wf);
    if (!inputs.wide_transcoded.empty()) {
        do_basic_run(inputs.wide_transcoded, wf);
    }
}
}  // namespace
}  // namespace scn::fuzz

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size)
{
    scn::fuzz::run(data, size);
    return 0;
}
