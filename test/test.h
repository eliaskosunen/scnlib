// Copyright 2017-2019 Elias Kosunen
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

#include <doctest.h>
#include <scn/scn.h>
#include <algorithm>
#include <array>
#include <ostream>
#include <string>
#include <vector>

template <typename CharT>
std::basic_string<CharT> widen(std::string)
{
}
template <>
inline std::basic_string<char> widen<char>(std::string str)
{
    return str;
}
template <>
inline std::basic_string<wchar_t> widen<wchar_t>(std::string str)
{
    return std::wstring(str.begin(), str.end());
}

template <typename Stream>
struct nonsized_stream : Stream {
    using is_sized_stream = std::false_type;

    nonsized_stream(Stream s) : Stream(std::move(s)) {}
};

template <typename Stream>
nonsized_stream<Stream> make_nonsized_stream(Stream s)
{
    return nonsized_stream<Stream>(std::move(s));
}

template <typename CharT, typename... T>
scn::scan_result scan_value(scn::options o,
                            std::string source,
                            std::string f,
                            T&... value)
{
    auto wsource = widen<CharT>(source);
    auto stream = scn::make_stream(wsource);
    auto fstr = widen<CharT>(f);
    auto ret = scn::scan(
        o, stream, scn::basic_string_view<CharT>(fstr.data(), fstr.size()),
        value...);
    return ret;
}
template <typename CharT, typename... T>
scn::scan_result scan_value(std::string source, std::string f, T&... value)
{
    return scan_value<CharT>(scn::options{}, std::move(source), std::move(f),
                             value...);
}
template <typename CharT, typename... T>
scn::scan_result scan_value(const std::locale& loc,
                            std::string source,
                            std::string f,
                            T&... value)
{
    return scan_value<CharT>(scn::options::builder{}.locale(loc).make(),
                             std::move(source), std::move(f), value...);
}

template <typename CharT, typename... T>
scn::scan_result scanf_value(scn::options o,
                             std::string source,
                             std::string f,
                             T&... value)
{
    auto wsource = widen<CharT>(source);
    auto stream = scn::make_stream(wsource);
    auto fstr = widen<CharT>(f);
    auto ret = scn::scanf(
        o, stream, scn::basic_string_view<CharT>(fstr.data(), fstr.size()),
        value...);
    return ret;
}
template <typename CharT, typename... T>
scn::scan_result scanf_value(std::string source, std::string f, T&... value)
{
    return scanf_value<CharT>(scn::options{}, std::move(source), std::move(f),
                              value...);
}

#define DOCTEST_VALUE_PARAMETERIZED_DATA(data, data_array)                     \
    SCN_CLANG_PUSH SCN_CLANG_IGNORE("-Wexit-time-destructors") {}              \
    static std::vector<std::string> _doctest_subcases = [&data_array]() {      \
        std::vector<std::string> out;                                          \
        while (out.size() != data_array.size())                                \
            out.push_back(std::string(#data_array "[") +                       \
                          std::to_string(out.size() + 1) + "]");               \
        return out;                                                            \
    }();                                                                       \
    size_t _doctest_subcase_idx = 0;                                           \
    std::for_each(                                                             \
        data_array.begin(), data_array.end(), [&](const decltype(data)& in) {  \
            DOCTEST_SUBCASE(_doctest_subcases[_doctest_subcase_idx++].c_str()) \
            {                                                                  \
                data = in;                                                     \
            }                                                                  \
        }) SCN_CLANG_POP
