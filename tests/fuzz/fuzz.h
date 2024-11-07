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

inline std::locale global_locale{};

inline std::string string_buffer(max_input_bytes, '\0');
// reinterpreted
inline std::wstring wstring_buffer_reinterpreted(max_input_bytes /
                                                     sizeof(wchar_t),
                                                 L'\0');
inline std::wstring wstring_buffer_transcoded_wide(max_input_bytes, L'\0');

inline auto make_input_views(const uint8_t* data, size_t size)
{
    SCN_EXPECT(size <= max_input_bytes);

    // narrow
    string_buffer.resize(size);
    std::copy(data, data + size, reinterpret_cast<uint8_t*>(&string_buffer[0]));
    auto sv = std::string_view{string_buffer};

    // wide, bitwise reinterpret
    const auto wsv_reinterpret_size =
        size < sizeof(wchar_t) ? 1 : (size / sizeof(wchar_t));
    wstring_buffer_reinterpreted.resize(wsv_reinterpret_size);
    std::memcpy(wstring_buffer_reinterpreted.data(), data, size);
    auto wsv_reintepreted = std::wstring_view{wstring_buffer_reinterpreted};

    // wide, transcode to correct encoding (utf16 or utf32)
    scn::impl::transcode_to_string(sv, wstring_buffer_transcoded_wide);
    std::wstring_view wsv_transcoded{wstring_buffer_transcoded_wide};

    return std::make_tuple(sv, wsv_reintepreted, wsv_transcoded);
}

inline std::deque<char> noncontiguous_buffer{};
inline std::deque<wchar_t> wnoncontiguous_buffer{};

template <typename CharT>
auto& get_noncontiguous_buffer()
{
    if constexpr (std::is_same_v<CharT, char>) {
        return noncontiguous_buffer;
    }
    else {
        return wnoncontiguous_buffer;
    }
}

template <typename Source>
const auto& populate_noncontiguous(Source& source)
{
    using char_type = ranges::range_value_t<Source>;
    auto& deque = get_noncontiguous_buffer<char_type>();
    deque.clear();
    std::copy(ranges::begin(source), ranges::end(source),
              std::back_inserter(deque));
    return deque;
}

inline std::vector<std::string_view> format_string_view_buffer(
    16,
    std::string_view{});
inline std::vector<std::wstring_view> wformat_string_view_buffer(
    16,
    std::wstring_view{});

template <typename CharT>
auto& get_format_string_view_buffer()
{
    if constexpr (std::is_same_v<CharT, char>) {
        return format_string_view_buffer;
    }
    else {
        return wformat_string_view_buffer;
    }
}

template <typename CharT>
using format_strings_type = std::vector<std::basic_string_view<CharT>>;

template <typename CharT, typename... Args>
const format_strings_type<CharT>& get_format_strings(Args... strings)
{
    std::array<const CharT*, sizeof...(Args)> tmp = {{strings...}};
    auto& buf = get_format_string_view_buffer<CharT>();
    buf.resize(sizeof...(Args));
    std::copy(tmp.begin(), tmp.end(), buf.begin());
    return buf;
}

template <typename CharT, typename T, typename Source>
void do_basic_run_for_type(Source& source,
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
            it = result->begin();
        }
    }

    // scan localized
    for (const auto& f : format_strings) {
        auto it = scn::ranges::begin(source);
        while (true) {
            SCN_EXPECT(it <= scn::ranges::end(source));
            auto result = scn::scan<T>(
                global_locale,
                scn::ranges::subrange{it, scn::ranges::end(source)},
                scn::runtime_format(f));
            if (!result) {
                break;
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
            it = result->begin();
        }
    }
}

template <typename CharT, typename Source>
void do_basic_run_for_source(Source&, const format_strings_type<CharT>&);

template <typename CharT, typename Source>
void do_basic_run(Source data, const format_strings_type<CharT>& format_strings)
{
    do_basic_run_for_source<CharT>(data, format_strings);
    do_basic_run_for_source<CharT>(populate_noncontiguous(data),
                                   format_strings);
}
}  // namespace scn::fuzz
