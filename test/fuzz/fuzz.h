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

namespace scn_fuzz {
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

    namespace globals {
        std::locale global_locale{};

        constexpr const size_t max_input_bytes = 4096;
        std::string string_buffer(max_input_bytes, '\0');
        std::wstring wstring_buffer(max_input_bytes, L'\0');

        std::string string_buffer1(max_input_bytes, '\0');
        std::wstring wstring_buffer1(max_input_bytes / sizeof(wchar_t), L'\0');
        std::wstring wstring_buffer2(max_input_bytes, L'\0');

        template <typename CharT>
        std::vector<scn::expected<CharT>> init_indirect()
        {
            std::vector<scn::expected<CharT>> r;
            r.resize(max_input_bytes,
                     scn::error(scn::error::end_of_range, "EOF"));
            return r;
        }

        std::vector<scn::string_view> format_string_view_buffer(
            16,
            scn::string_view{});
        std::vector<scn::wstring_view> wformat_string_view_buffer(
            16,
            scn::wstring_view{});

        std::deque<char> deque_buffer(max_input_bytes, 0);
        std::deque<wchar_t> wdeque_buffer(max_input_bytes, 0);
        std::vector<scn::expected<char>> indirect_buffer =
            init_indirect<char>();
        std::vector<scn::expected<wchar_t>> windirect_buffer =
            init_indirect<wchar_t>();
        indirect_range<char> indirect_range_buffer{};
        indirect_range<wchar_t> windirect_range_buffer{};
    }  // namespace globals

    std::string& populate_string(scn::string_view sv)
    {
        globals::string_buffer.resize(sv.size());
        std::copy(sv.begin(), sv.end(), globals::string_buffer.begin());
        return globals::string_buffer;
    }
    std::wstring& populate_string(scn::wstring_view sv)
    {
        globals::wstring_buffer.resize(sv.size());
        std::copy(sv.begin(), sv.end(), globals::wstring_buffer.begin());
        return globals::wstring_buffer;
    }

    inline void populate_views(const uint8_t* data,
                               size_t size,
                               scn::string_view& sv,
                               scn::wstring_view& wsv1,
                               scn::wstring_view& wsv2)
    {
        SCN_EXPECT(size <= globals::max_input_bytes);

        // a b c d
        globals::string_buffer1.resize(size);
        std::memcpy(&globals::string_buffer1[0], data, size);
        sv = scn::string_view(globals::string_buffer1.data(), size);

        // ab cd
        const auto wsv1_size =
            size < sizeof(wchar_t) ? 1 : (size / sizeof(wchar_t));
        globals::wstring_buffer1.resize(wsv1_size);
        std::memcpy(reinterpret_cast<char*>(&globals::wstring_buffer1[0]), data,
                    size);
        wsv1 = scn::wstring_view(globals::wstring_buffer1.data(), wsv1_size);

        // a b c d
        globals::wstring_buffer2.resize(size);
        std::copy(data, data + size, &globals::wstring_buffer2[0]);
        wsv2 = scn::wstring_view(globals::wstring_buffer2.data(), size);
    }

    std::deque<char>& get_deque_buffer(char)
    {
        return globals::deque_buffer;
    }
    std::deque<wchar_t>& get_deque_buffer(wchar_t)
    {
        return globals::wdeque_buffer;
    }
    std::vector<scn::expected<char>>& get_indirect_buffer(char)
    {
        return globals::indirect_buffer;
    }
    std::vector<scn::expected<wchar_t>>& get_indirect_buffer(wchar_t)
    {
        return globals::windirect_buffer;
    }
    indirect_range<char>& get_indirect_range_buffer(char)
    {
        return globals::indirect_range_buffer;
    }
    indirect_range<wchar_t>& get_indirect_range_buffer(wchar_t)
    {
        return globals::windirect_range_buffer;
    }
    std::vector<scn::string_view>& get_format_strings_buffer(char)
    {
        return globals::format_string_view_buffer;
    }
    std::vector<scn::wstring_view>& get_format_strings_buffer(wchar_t)
    {
        return globals::wformat_string_view_buffer;
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
    indirect_range<CharT>& populate_indirect(
        scn::basic_string_view<CharT> source)
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
    T unwrap_expected(T val)
    {
        return val;
    }
    template <typename T>
    T unwrap_expected(scn::expected<T> val)
    {
        return val.value();
    }

    template <typename CharT>
    using format_strings_view = scn::span<scn::basic_string_view<CharT>>;

    template <typename CharT, typename... Args>
    format_strings_view<CharT> get_format_strings(Args... strings)
    {
        scn::detail::array<const CharT*, sizeof...(Args)> tmp = {{strings...}};
        auto& buf = get_format_strings_buffer(CharT{});
        buf.resize(sizeof...(Args));
        for (size_t i = 0; i < sizeof...(Args); ++i) {
            buf[i] = scn::basic_string_view<CharT>{tmp[i],
                                                   scn::detail::strlen(tmp[i])};
        }
        return scn::make_span(buf);
    }

    template <typename CharT, typename T, typename Source>
    void do_basic_run_for_type(Source& source,
                               format_strings_view<CharT> format_strings)
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

        for (const auto& f : format_strings) {
            auto result = scn::make_result(source);
            T val{};
            while (true) {
                result = scn::scan(result.range(), f, val);
                if (!result) {
                    break;
                }
            }
        }

        {
            auto result = scn::make_result(source);
            T val{};
            while (true) {
                result = scn::scan_localized(
                    globals::global_locale, result.range(),
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
    void do_basic_run_for_source(Source&, format_strings_view<CharT>);

    template <typename CharT, typename Source>
    void do_basic_run(Source data, format_strings_view<CharT> format_strings)
    {
        auto source_sv = data;
        do_basic_run_for_source<CharT>(source_sv, format_strings);

        auto& source_str = populate_string(source_sv);
        do_basic_run_for_source<CharT>(source_str, format_strings);

        auto& source_deque = populate_deque(source_sv);
        do_basic_run_for_source<CharT>(source_deque, format_strings);

        auto& source_indirect = populate_indirect(source_sv);
        do_basic_run_for_source<CharT>(source_indirect, format_strings);
        reset_indirect(SCN_MOVE(source_indirect));
    }
}  // namespace scn_fuzz
