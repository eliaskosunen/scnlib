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

#include <scn/scn.h>
#include <sstream>
#include <vector>

#define SCN_SKIP_DOCTEST
#include "../test.h"

template <typename CharT>
struct default_format_string;
template <>
struct default_format_string<char> {
    static const char* value;
};
template <>
struct default_format_string<wchar_t> {
    static const wchar_t* value;
};
const char* default_format_string<char>::value = "{}";
const wchar_t* default_format_string<wchar_t>::value = L"{}";

std::locale global_locale{};

constexpr const size_t max_input_bytes = 4096;
std::string string_buffer(max_input_bytes, '\0');
std::wstring wstring_buffer(max_input_bytes, L'\0');

std::string& populate_string(scn::string_view sv)
{
    string_buffer.resize(sv.size());
    std::copy(sv.begin(), sv.end(), string_buffer.begin());
    return string_buffer;
}
std::wstring& populate_string(scn::wstring_view sv)
{
    wstring_buffer.resize(sv.size());
    std::copy(sv.begin(), sv.end(), string_buffer.begin());
    return wstring_buffer;
}

std::wstring wstring_buffer1(max_input_bytes / sizeof(wchar_t), L'\0');
std::wstring wstring_buffer2(max_input_bytes, L'\0');

inline void populate_views(const uint8_t* data,
                           size_t size,
                           scn::string_view& sv,
                           scn::wstring_view& wsv1,
                           scn::wstring_view& wsv2)
{
    SCN_EXPECT(size <= max_input_bytes);

    // a b c d
    sv = scn::string_view(reinterpret_cast<const char*>(data), size);

    // ab cd
    const auto wsv1_size =
        size < sizeof(wchar_t) ? 1 : (size / sizeof(wchar_t));
    std::memcpy(reinterpret_cast<char*>(&wstring_buffer1[0]), data, size);
    wsv1 = scn::wstring_view(wstring_buffer1.data(), wsv1_size);

    // a b c d

    std::copy(data, data + size, &wstring_buffer2[0]);
    wsv2 = scn::wstring_view(wstring_buffer2.data(), size);
}

template <typename CharT>
std::vector<scn::expected<CharT>> init_indirect()
{
    std::vector<scn::expected<CharT>> r;
    r.resize(max_input_bytes, scn::error(scn::error::end_of_range, "EOF"));
    return r;
}

std::deque<char> deque_buffer(max_input_bytes, 0);
std::deque<wchar_t> wdeque_buffer(max_input_bytes, 0);
std::vector<scn::expected<char>> indirect_buffer = init_indirect<char>();
std::vector<scn::expected<wchar_t>> windirect_buffer = init_indirect<wchar_t>();
indirect_range<char> indirect_range_buffer{};
indirect_range<wchar_t> windirect_range_buffer{};

std::deque<char>& get_deque_buffer(char)
{
    return deque_buffer;
}
std::deque<wchar_t>& get_deque_buffer(wchar_t)
{
    return wdeque_buffer;
}
std::vector<scn::expected<char>>& get_indirect_buffer(char)
{
    return indirect_buffer;
}
std::vector<scn::expected<wchar_t>>& get_indirect_buffer(wchar_t)
{
    return windirect_buffer;
}
indirect_range<char>& get_indirect_range_buffer(char)
{
    return indirect_range_buffer;
}
indirect_range<wchar_t>& get_indirect_range_buffer(wchar_t)
{
    return windirect_range_buffer;
}

template <typename CharT>
std::deque<CharT>& populate_deque(scn::basic_string_view<CharT> source)
{
    auto& deque = get_deque_buffer(CharT{});
    deque.resize(source.size());
    std::copy(source.begin(), source.end(), deque.begin());
    return deque;
}
template <typename CharT>
indirect_range<CharT>& populate_indirect(scn::basic_string_view<CharT> source)
{
    auto& b = get_indirect_buffer(CharT{});
    auto& r = get_indirect_range_buffer(CharT{});

    b.resize(source.size());
    std::copy(source.begin(), source.end(), b.begin());
    r.set(SCN_MOVE(b));
    return r;
}
template <typename CharT>
void reset_indirect(indirect_range<CharT>&& r)
{
    get_indirect_buffer(CharT{}) = SCN_MOVE(r).extract();
}

template <typename T>
std::vector<T>& get_vector()
{
    static std::vector<T> vec;
    vec.clear();
    return vec;
}

template <typename T>
T unwrap_expected(T val) {
    return val;
}
template <typename T>
T unwrap_expected(scn::expected<T> val) {
    return val.value();
}

template <typename CharT, typename T, typename Source>
void basic_do_run(Source& source)
{
    {
        auto result = scn::make_result(source);
        T val{};
        while (true) {
            result = scn::scan_default(result.range(), val);
            if (!result) {
                break;
            }
        }
    }

    {
        auto result = scn::make_result(source);
        T val{};
        while (true) {
            result = scn::scan(result.range(),
                               default_format_string<CharT>::value, val);
            if (!result) {
                break;
            }
        }
    }

    {
        auto result = scn::make_result(source);
        T val{};
        while (true) {
            result =
                scn::scan_localized(global_locale, result.range(),
                                    default_format_string<CharT>::value, val);
            if (!result) {
                break;
            }
        }
    }

    {
        auto result = scn::make_result<scn::expected<T>>(source);
        while (true) {
            result = scn::scan_value<T>(result.range());
            if (!result) {
                break;
            }
        }
    }

    {
        auto& vec = get_vector<T>();
        auto result = scn::scan_list(source, vec);
    }

    if (source.size() > 4) {
        auto sep = unwrap_expected(source[source.size() / 4]);
        auto until = unwrap_expected(source[source.size() / 2]);
        auto& vec = get_vector<T>();
        auto result = scn::scan_list_ex(
            source, vec, scn::list_separator_and_until(sep, until));
    }
}

template <typename CharT, typename Source>
void basic_run_for_source(Source& source)
{
    basic_do_run<CharT, CharT>(source);
    basic_do_run<CharT, scn::code_point>(source);
    basic_do_run<CharT, short>(source);
    basic_do_run<CharT, unsigned short>(source);
    basic_do_run<CharT, int>(source);
    basic_do_run<CharT, unsigned>(source);
    basic_do_run<CharT, long>(source);
    basic_do_run<CharT, unsigned long>(source);
    basic_do_run<CharT, long long>(source);
    basic_do_run<CharT, unsigned long long>(source);
    basic_do_run<CharT, float>(source);
    basic_do_run<CharT, double>(source);
    basic_do_run<CharT, long double>(source);
    basic_do_run<CharT, bool>(source);
    basic_do_run<CharT, std::basic_string<CharT>>(source);
    basic_do_run<CharT, scn::basic_string_view<CharT>>(source);
}
