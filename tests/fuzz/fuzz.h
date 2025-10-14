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

#include <scn/impl.h>
#include <scn/scan.h>
#include <scn/xchar.h>

#include <deque>
#include <sstream>

namespace scn::fuzz {

inline constexpr const char* default_narrow_format_string = "{}";
inline constexpr const wchar_t* default_wide_format_string = L"{}";

template <typename CharT>
constexpr auto get_default_format_string()
{
    if constexpr (std::is_same_v<CharT, char>) {
        return default_narrow_format_string;
    }
    else {
        return default_wide_format_string;
    }
}

inline constexpr const size_t max_input_bytes = 4096;

struct input_views {
    std::string narrow;
    // Byte-by-byte widened
    std::wstring wide_copied;
    // Bitwise reinterpreted
    std::wstring wide_reinterpreted;
    // Transcoded to utf16/utf32, may be empty if `narrow` is invalid utf8
    std::wstring wide_transcoded;
};

inline input_views make_input_views(const uint8_t* data, size_t size)
{
    SCN_EXPECT(size <= max_input_bytes);

    std::string narrow(size, '\0');
    std::memcpy(&narrow[0], data, size);

    std::wstring wcopied{};
    wcopied.reserve(size);
    for (char ch : narrow) {
        wcopied.push_back(static_cast<wchar_t>(ch));
    }

    const auto wreinterpret_size =
        size < sizeof(wchar_t) ? 1 : (size / sizeof(wchar_t));
    std::wstring wreinterpreted(wreinterpret_size, L'\0');
    std::memcpy(wreinterpreted.data(), data, size);

    std::wstring wtranscoded{};
    scn::impl::transcode_to_string(std::string_view{narrow}, wtranscoded);

    return {std::move(narrow), std::move(wcopied), std::move(wreinterpreted),
            std::move(wtranscoded)};
}

template <typename CharT, typename Source>
void populate_random_access(std::deque<CharT>& deque, Source&& source)
{
    deque.clear();
    std::copy(ranges::begin(source), ranges::end(source),
              std::back_inserter(deque));
}

template <typename CharT>
using format_strings_type = std::vector<std::basic_string_view<CharT>>;

template <typename CharT, typename T, typename Source>
void do_basic_run_for_type(Source&& source,
                           const format_strings_type<CharT>& format_strings)
{
    // Regular scan
    for (const auto& f : format_strings) {
        auto it = scn::ranges::begin(source);
        while (true) {
            SCN_EXPECT(it <= scn::ranges::end(source));
            auto result = scn::scan<T>(
                scn::ranges::subrange{it, scn::ranges::end(source)},
                scn::runtime_format(f));
            if (!result) {
                break;
            }
            if constexpr (std::is_same_v<detail::remove_cvref_t<T>,
                                         std::basic_string<CharT>> ||
                          std::is_same_v<detail::remove_cvref_t<T>,
                                         std::basic_string_view<CharT>>) {
                if (result->value().empty()) {
                    break;
                }
            }
            it = result->begin();
        }
    }

    // scan localized
    for (const auto& f : format_strings) {
        auto it = scn::ranges::begin(source);
        while (true) {
            SCN_EXPECT(it <= scn::ranges::end(source));
            auto result = scn::scan<T>(
                std::locale::classic(),
                scn::ranges::subrange{it, scn::ranges::end(source)},
                scn::runtime_format(f));
            if (!result) {
                break;
            }
            if constexpr (std::is_same_v<detail::remove_cvref_t<T>,
                                         std::basic_string<CharT>> ||
                          std::is_same_v<detail::remove_cvref_t<T>,
                                         std::basic_string_view<CharT>>) {
                if (result->value().empty()) {
                    break;
                }
            }
            it = result->begin();
        }
    }

    // scan_value
    {
        auto it = scn::ranges::begin(source);
        while (true) {
            SCN_EXPECT(it <= scn::ranges::end(source));
            auto result = scn::scan_value<T>(
                scn::ranges::subrange{it, scn::ranges::end(source)});
            if (!result) {
                break;
            }
            if constexpr (std::is_same_v<detail::remove_cvref_t<T>,
                                         std::basic_string<CharT>> ||
                          std::is_same_v<detail::remove_cvref_t<T>,
                                         std::basic_string_view<CharT>>) {
                if (result->value().empty()) {
                    break;
                }
            }
            it = result->begin();
        }
    }
}

template <typename CharT, typename Source>
void do_basic_run_for_source(Source&&, const format_strings_type<CharT>&);

template <typename CharT>
void do_basic_run(const std::basic_string<CharT>& data,
                  const format_strings_type<CharT>& format_strings)
{
    do_basic_run_for_source<CharT>(data, format_strings);

    std::deque<CharT> deque{};
    populate_random_access(deque, data);
    do_basic_run_for_source<CharT>(deque, format_strings);

    // TODO: Other sources, like input_ranges, files, and streams
    //
    // auto input = scn::ranges::views::to_input(data);
    //  do_basic_run_for_source<CharT>(input, format_strings);
}
}  // namespace scn::fuzz
