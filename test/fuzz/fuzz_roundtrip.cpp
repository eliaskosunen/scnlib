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

template <typename T, typename Result>
void check_roundtrip(const T& value, const T& original, Result& result)
{
    if (!result) {
        throw std::runtime_error("Failed to read");
    }
    if (value != original) {
        throw std::runtime_error("Roundtrip failure");
    }
    if (!result.range().empty()) {
        if (!result.range().is_direct) {
            if (result.range().size() == 1) {
                auto e = scn::read_code_unit(result.range(), false);
                if (!e && e.error() == scn::error::end_of_range) {
                    return;
                }
            }
        }
        throw std::runtime_error("Unparsed input");
    }
}

template <typename CharT, typename T, typename Source>
void do_roundtrip(const T& original_value, const Source& s)
{
    {
        T value{};
        auto result =
            scn::scan(s, scn_fuzz::default_format_string<CharT>::value, value);
        check_roundtrip(value, original_value, result);
    }
    {
        T value{};
        auto result = scn::scan_default(s, value);
        check_roundtrip(value, original_value, result);
    }
    {
        T value{};
        auto result = scn::scan_localized(
            scn_fuzz::globals::global_locale, s,
            scn_fuzz::default_format_string<CharT>::value, value);
        check_roundtrip(value, original_value, result);
    }
}

template <typename T, typename Source>
T blip_for_roundtrip(Source data)
{
    char buffer[10] = {0};
    char* end = buffer;
    for (auto it = data.begin(); it != data.end() && end != buffer + 10;
         ++it, ++end) {
        *end = static_cast<char>(*it);
    }

    T value{};
    std::memcpy(&value, buffer, sizeof(T));
    return value;
}

template <typename T, typename = void>
struct widen_to_64;
template <typename T>
struct widen_to_64<T, typename std::enable_if<std::is_signed<T>::value>::type> {
    using type = long long;
};
template <typename T>
struct widen_to_64<T,
                   typename std::enable_if<std::is_unsigned<T>::value>::type> {
    using type = unsigned long long;
};

template <typename CharT, typename T, typename Source>
void roundtrip_for_type(Source data)
{
    SCN_EXPECT(data.size() >= sizeof(T));

    auto original_value = blip_for_roundtrip<T>(data);

    std::basic_ostringstream<CharT> oss;
    oss << static_cast<typename widen_to_64<T>::type>(original_value);

    auto source_str = SCN_MOVE(oss).str();
    do_roundtrip<CharT>(original_value, source_str);

    auto source_sv =
        scn::basic_string_view<CharT>(source_str.data(), source_str.size());
    do_roundtrip<CharT>(original_value, source_sv);

    auto& source_deque = scn_fuzz::populate_deque(source_sv);
    do_roundtrip<CharT>(original_value, source_deque);

    auto& source_indirect = scn_fuzz::populate_indirect(source_sv);
    do_roundtrip<CharT>(original_value, source_indirect);
    scn_fuzz::reset_indirect(SCN_MOVE(source_indirect));
}

template <typename Source>
void roundtrip_for_source(Source source)
{
    using char_type = typename Source::value_type;

    roundtrip_for_type<char_type, signed char>(source);
    roundtrip_for_type<char_type, short>(source);
    roundtrip_for_type<char_type, int>(source);
    roundtrip_for_type<char_type, long>(source);
    roundtrip_for_type<char_type, long long>(source);
    roundtrip_for_type<char_type, unsigned char>(source);
    roundtrip_for_type<char_type, unsigned short>(source);
    roundtrip_for_type<char_type, unsigned int>(source);
    roundtrip_for_type<char_type, unsigned long>(source);
    roundtrip_for_type<char_type, unsigned long long>(source);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size != sizeof(long long)) {
        return 0;
    }

    scn::string_view sv;
    scn::wstring_view wsv1, wsv2;
    scn_fuzz::populate_views(data, size, sv, wsv1, wsv2);

    roundtrip_for_source(sv);
    roundtrip_for_source(wsv2);

    return 0;
}
