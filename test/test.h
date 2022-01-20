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

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")
#include <algorithm>
#include <array>
#include <deque>
#include <exception>
#include <ostream>
#include <string>
#include <vector>
SCN_GCC_POP

#include <doctest/doctest.h>

template <typename T>
struct debug;

template <typename CharT>
std::basic_string<CharT> widen(const std::string&)
{
    return {};
}
template <>
inline std::basic_string<char> widen<char>(const std::string& str)
{
    return str;
}
template <>
inline std::basic_string<wchar_t> widen<wchar_t>(const std::string& str)
{
    return std::wstring(str.begin(), str.end());
}

template <typename CharT, typename Input, typename Fmt, typename... T>
auto do_scan(Input&& i, Fmt f, T&... a)
    -> decltype(scn::scan(widen<CharT>(std::forward<Input>(i)),
                          widen<CharT>(f).c_str(),
                          a...))
{
    return scn::scan(widen<CharT>(std::forward<Input>(i)),
                     widen<CharT>(f).c_str(), a...);
}
template <typename CharT,
          typename Locale,
          typename Input,
          typename Fmt,
          typename... T>
auto do_scan_localized(const Locale& loc, Input&& i, Fmt f, T&... a)
    -> decltype(scn::scan_localized(loc,
                                    widen<CharT>(std::forward<Input>(i)),
                                    widen<CharT>(f).c_str(),
                                    a...))
{
    return scn::scan_localized(loc, widen<CharT>(std::forward<Input>(i)),
                               widen<CharT>(f).c_str(), a...);
}

inline std::deque<char> get_deque(const std::string& content = "123")
{
    std::deque<char> src{};
    for (auto ch : content) {
        src.push_back(ch);
    }
    return src;
}
inline std::deque<char> get_empty_deque()
{
    return {};
}

template <typename T>
bool consistency_iostream(std::string& source, T& val)
{
    std::istringstream ss{source};
    ss >> val;
    bool res = !(ss.fail() || ss.bad());

    source.clear();
    auto in_avail = ss.rdbuf()->in_avail();
    source.resize(static_cast<size_t>(in_avail));
    ss.rdbuf()->sgetn(&source[0], in_avail);

    return res;
}
template <typename T>
bool consistency_scanf(std::string& source, const std::string& fmt, T& val)
{
    size_t nchar{0};
    auto f = fmt + "%n";

    SCN_GCC_COMPAT_PUSH
    SCN_GCC_COMPAT_IGNORE("-Wformat-nonliteral")
    int nargs = std::sscanf(source.c_str(), f.c_str(), &val, &nchar);
    SCN_GCC_COMPAT_POP

    source = source.substr(nchar);
    return nargs == 1;
}

#define DOCTEST_VALUE_PARAMETERIZED_DATA(data, data_array)                     \
    SCN_CLANG_PUSH SCN_CLANG_IGNORE("-Wexit-time-destructors") {}              \
    static std::vector<std::string> _doctest_subcases = [&(data_array)]() {    \
        std::vector<std::string> out;                                          \
        while (out.size() != (data_array).size())                              \
            out.push_back(std::string(#data_array "[") +                       \
                          std::to_string(out.size() + 1) + "]");               \
        return out;                                                            \
    }();                                                                       \
    size_t _doctest_subcase_idx = 0;                                           \
    std::for_each(                                                             \
        (data_array).begin(), (data_array).end(),                              \
        [&](const decltype(data)& in) {                                        \
            DOCTEST_SUBCASE(_doctest_subcases[_doctest_subcase_idx++].c_str()) \
            {                                                                  \
                (data) = in;                                                   \
            }                                                                  \
        }) SCN_CLANG_POP
