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
#include <scn/istream.h>
#include <scn/scan.h>
#include <scn/xchar.h>

#include <deque>
#include <fstream>
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

template <typename CharT,
          typename T,
          typename Source,
          std::enable_if_t<scn::ranges::forward_range<Source>>* = nullptr>
void do_basic_run_for_type(Source&& source,
                           std::basic_string_view<CharT> format_string)
{
    auto do_basic_run = [&](auto&& scan) {
        auto it = scn::ranges::begin(source);

        while (true) {
            if constexpr (scn::ranges::random_access_range<Source>) {
                SCN_EXPECT(it <= scn::ranges::end(source));
            }

            auto result = scan(it);
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
    };

    // Regular scan
    do_basic_run([&](auto it) {
        return scn::scan<T>(scn::ranges::subrange{it, scn::ranges::end(source)},
                            scn::runtime_format(format_string));
    });

    // scan localized
    do_basic_run([&](auto it) {
        return scn::scan<T>(std::locale::classic(),
                            scn::ranges::subrange{it, scn::ranges::end(source)},
                            scn::runtime_format(format_string));
    });

    // scan_value
    if (format_string == get_default_format_string<CharT>()) {
        do_basic_run([&](auto it) {
            return scn::scan_value<T>(
                scn::ranges::subrange{it, scn::ranges::end(source)});
        });
    }
}

template <typename CharT,
          typename T,
          typename Source,
          std::enable_if_t<scn::ranges::input_range<Source> &&
                           !scn::ranges::forward_range<Source>>* = nullptr>
void do_basic_run_for_type(Source&& source,
                           std::basic_string_view<CharT> format_string)
{
    CharT null_ch{};
    auto first_result = scn::scan<>(source, scn::runtime_format(&null_ch));
    auto range = std::move(first_result->range());

    while (true) {
        auto result = scn::scan<T>(range, scn::runtime_format(format_string));
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
        range = std::move(result->range());
    }
}

template <typename CharT,
          typename T,
          typename Source,
          std::enable_if_t<!scn::ranges::range<Source>>* = nullptr>
void do_basic_run_for_type(Source&& source,
                           std::basic_string_view<CharT> format_string)
{
    while (true) {
        auto result = scn::scan<T>(source, scn::runtime_format(format_string));
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
    }
}

template <typename CharT, typename Source>
void do_basic_run_for_source(Source&&, std::basic_string_view<CharT>);

template <typename CharT>
void do_basic_run(const std::basic_string<CharT>& data,
                  const format_strings_type<CharT>& format_strings)
{
    static constexpr const char* path = "file_fuzz_tmp_file";

    if constexpr (std::is_same_v<CharT, char>) {
        auto out = std::ofstream{
            path, std::ios::out | std::ios::binary | std::ios::trunc};
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
    }

    std::deque<CharT> deque{};
    populate_random_access(deque, data);

    for (auto format_string : format_strings) {
        do_basic_run_for_source<CharT>(data, format_string);

        do_basic_run_for_source<CharT>(deque, format_string);

        {
            auto input = scn::ranges::views::to_input(data);
            do_basic_run_for_source<CharT>(input, format_string);
        }

        {
            auto strm = std::basic_istringstream<CharT>{data};
            do_basic_run_for_source<CharT>(strm, format_string);
        }

        if constexpr (std::is_same_v<CharT, char>) {
            struct cfile_guard {
                cfile_guard()
                {
                    file = std::fopen(path, "rb");
                    SCN_ENSURE(file);
                }

                ~cfile_guard()
                {
                    std::fclose(file);
                }

                std::FILE* file;
            };

            const cfile_guard guard{};
            scn::scan_file file{guard.file};
            do_basic_run_for_source<char>(file, format_string);
        }
    }
}
}  // namespace scn::fuzz
