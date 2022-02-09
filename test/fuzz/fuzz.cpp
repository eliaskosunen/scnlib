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

template <typename T, typename C>
void run(scn::basic_string_view<C> source)
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
            result =
                scn::scan(result.range(), default_format_string<C>::value, val);
            if (!result) {
                break;
            }
        }
    }

    {
        auto result = scn::make_result(source);
        T val{};
        while (true) {
            result = scn::scan_localized(global_locale, result.range(),
                                         default_format_string<C>::value, val);
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
}

template <typename C>
void run_getline(scn::basic_string_view<C> source)
{
    auto result = scn::make_result(source);
    std::basic_string<C> str;
    while (true) {
        result = scn::getline(result.range(), str);
        if (!result) {
            break;
        }
    }
}

template <typename C>
void run_ignore(scn::basic_string_view<C> source)
{
    if (source.size() < 2) {
        return;
    }
    C until = source[0];
    source.remove_prefix(1);
    scn::ignore_until(source, until);
}

template <typename T, typename C>
void run_list(scn::basic_string_view<C> source)
{
    std::vector<T> list;
    scn::scan_list(source, list);
}

template <typename T, typename C>
void roundtrip(scn::basic_string_view<C> data)
{
    if (data.size() < sizeof(T)) {
        return;
    }
    T original_value{};
    std::memcpy(&original_value, data.data(), sizeof(T));

    std::basic_ostringstream<C> oss;
    oss << original_value;
    auto source = std::move(oss).str();

#define SCAN_BOILERPLATE_BEFORE T value{};
#define SCAN_BOILERPLATE_AFTER                         \
    if (!result) {                                     \
        throw std::runtime_error("Failed to read");    \
    }                                                  \
    if (value != original_value) {                     \
        throw std::runtime_error("Roundtrip failure"); \
    }                                                  \
    if (!result.range().empty()) {                     \
        throw std::runtime_error("Unparsed input");    \
    }
#define SCAN_BOILERPLATE_AFTER_INDIRECT                                  \
    if (!result) {                                                       \
        throw std::runtime_error("Failed to read");                      \
    }                                                                    \
    if (value != original_value) {                                       \
        throw std::runtime_error("Roundtrip failure");                   \
    }                                                                    \
    if (result.range().size() != 1 && result.range().begin()->error()) { \
        throw std::runtime_error("Unparsed input");                      \
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan_default(source, value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan(source, default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan_localized(
            global_locale, source, default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER
    }
    using string_type = std::basic_string<C>;
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan_default(string_type{source}, value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan(string_type{source},
                                default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result =
            scn::scan_localized(global_locale, string_type{source},
                                default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER
    }
    using string_view_type = scn::basic_string_view<C>;
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan_default(
            string_view_type{source.data(), source.size()}, value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan(string_view_type{source.data(), source.size()},
                                default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan_localized(
            global_locale, string_view_type{source.data(), source.size()},
            default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan_default(get_indirect<C>(source), value);
        SCAN_BOILERPLATE_AFTER_INDIRECT
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan(get_indirect<C>(source),
                                default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER_INDIRECT
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result =
            scn::scan_localized(global_locale, get_indirect<C>(source),
                                default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER_INDIRECT
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto r = get_indirect<C>(source);
        auto result = scn::scan_default(r, value);
        SCAN_BOILERPLATE_AFTER_INDIRECT
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto r = get_indirect<C>(source);
        auto result = scn::scan(r, default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER_INDIRECT
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto r = get_indirect<C>(source);
        auto result = scn::scan_localized(
            global_locale, r, default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER_INDIRECT
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan_default(get_deque<C>(source), value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result = scn::scan(get_deque<C>(source),
                                default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto result =
            scn::scan_localized(global_locale, get_deque<C>(source),
                                default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto r = get_deque<C>(source);
        auto result = scn::scan_default(r, value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto r = get_deque<C>(source);
        auto result = scn::scan(r, default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER
    }
    {
        SCAN_BOILERPLATE_BEFORE
        auto r = get_deque<C>(source);
        auto result = scn::scan_localized(
            global_locale, r, default_format_string<C>::value, value);
        SCAN_BOILERPLATE_AFTER
    }
#undef SCAN_BOILERPLATE_BEFORE
#undef SCAN_BOILERPLATE_AFTER
#undef SCAN_BOILERPLATE_AFTER_INDIRECT
}

#define SCN_FUZZ_RUN_BASIC(source, Char)                \
    do {                                                \
        run<Char>(source);                              \
        run<bool>(source);                              \
        run<short>(source);                             \
        run<int>(source);                               \
        run<long>(source);                              \
        run<long long>(source);                         \
        run<unsigned short>(source);                    \
        run<unsigned int>(source);                      \
        run<unsigned long>(source);                     \
        run<unsigned long long>(source);                \
        run<float>(source);                             \
        run<double>(source);                            \
        run<long double>(source);                       \
        run<std::basic_string<Char>>(source);           \
        run<scn::basic_string_view<Char>>(source);      \
        run_getline<Char>(source);                      \
        run_ignore<Char>(source);                       \
        run_list<Char>(source);                         \
        run_list<short>(source);                        \
        run_list<int>(source);                          \
        run_list<long>(source);                         \
        run_list<long long>(source);                    \
        run_list<unsigned short>(source);               \
        run_list<unsigned int>(source);                 \
        run_list<unsigned long>(source);                \
        run_list<unsigned long long>(source);           \
        run_list<float>(source);                        \
        run_list<double>(source);                       \
        run_list<long double>(source);                  \
        run_list<std::basic_string<Char>>(source);      \
        run_list<scn::basic_string_view<Char>>(source); \
    } while (false)

#define SCN_FUZZ_RUN_ROUNDTRIP(source, Char)   \
    do {                                       \
        roundtrip<short>(source);              \
        roundtrip<int>(source);                \
        roundtrip<long>(source);               \
        roundtrip<long long>(source);          \
        roundtrip<unsigned short>(source);     \
        roundtrip<unsigned int>(source);       \
        roundtrip<unsigned long>(source);      \
        roundtrip<unsigned long long>(source); \
    } while (false)

#define SCN_FUZZ_RUN_FORMAT_TYPE(source, Char, T)                  \
    do {                                                           \
        T value{};                                                 \
        scn::scan(source, source, value);                          \
        scn::scan_localized(std::locale{}, source, source, value); \
    } while (false);

#define SCN_FUZZ_RUN_FORMAT(source, Char)                            \
    SCN_FUZZ_RUN_FORMAT_TYPE(source, Char, Char);                    \
    SCN_FUZZ_RUN_FORMAT_TYPE(source, Char, int);                     \
    SCN_FUZZ_RUN_FORMAT_TYPE(source, Char, bool);                    \
    SCN_FUZZ_RUN_FORMAT_TYPE(source, Char, unsigned);                \
    SCN_FUZZ_RUN_FORMAT_TYPE(source, Char, double);                  \
    SCN_FUZZ_RUN_FORMAT_TYPE(source, Char, std::basic_string<Char>); \
    SCN_FUZZ_RUN_FORMAT_TYPE(source, Char, scn::basic_string_view<Char>);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // a b c d
    scn::string_view source(reinterpret_cast<const char*>(data), size);

    // ab cd
    const auto wdata1_size =
        size < sizeof(wchar_t) ? 0 : (size / sizeof(wchar_t));
    std::wstring wdata1(wdata1_size, 0);
    std::memcpy(reinterpret_cast<char*>(&wdata1[0]), data, size);
    scn::wstring_view wsource1(wdata1.data(), wdata1.size());

    // a b c d
    std::wstring wdata2(size, 0);
    std::copy(data, data + size, &wdata2[0]);
    scn::wstring_view wsource2(wdata2.data(), size);

    {
        SCN_FUZZ_RUN_BASIC(source, char);
        SCN_FUZZ_RUN_ROUNDTRIP(source, char);
    }

    {
        SCN_FUZZ_RUN_BASIC(wsource1, wchar_t);
        SCN_FUZZ_RUN_ROUNDTRIP(wsource1, wchar_t);
    }

    {
        SCN_FUZZ_RUN_BASIC(wsource2, wchar_t);
        SCN_FUZZ_RUN_ROUNDTRIP(wsource2, wchar_t);
    }

    // format string
    if (size != 0) {
        // narrow
        {
            SCN_FUZZ_RUN_FORMAT(source, char);
        }
        // wide1
        {
            SCN_FUZZ_RUN_FORMAT(wsource1, wchar_t);
        }
        // wide2
        {
            SCN_FUZZ_RUN_FORMAT(wsource2, wchar_t);
        }
    }

    return 0;
}
