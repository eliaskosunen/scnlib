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

#ifndef SCN_BENCHMARK_WORD_H
#define SCN_BENCHMARK_WORD_H

#include "../benchmark.h"

#include <cstdio>
#include <limits>
#include <sstream>
#include <vector>

template <typename CharT>
inline const std::vector<CharT>& chars_nospaces()
{
}
template <>
inline const std::vector<char>& chars_nospaces()
{
    static const std::vector<char> chars = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
        'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',
        'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
    return chars;
}
template <>
inline const std::vector<wchar_t>& chars_nospaces()
{
    static const std::vector<wchar_t> chars = {
        L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A',
        L'B', L'C', L'D', L'E', L'F', L'G', L'H', L'I', L'J', L'K', L'L',
        L'M', L'N', L'O', L'P', L'Q', L'R', L'S', L'T', L'U', L'V', L'W',
        L'X', L'Y', L'Z', L'a', L'b', L'c', L'd', L'e', L'f', L'g', L'h',
        L'i', L'j', L'k', L'l', L'm', L'n', L'o', L'p', L'q', L'r', L's',
        L't', L'u', L'v', L'w', L'x', L'y', L'z'};
    return chars;
}

template <typename CharT>
inline const std::vector<CharT>& chars_spaces()
{
}
template <>
inline const std::vector<char>& chars_spaces()
{
    static const std::vector<char> chars = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  'A',  'B',
        'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',  'M',  'N',
        'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',  'Y',  'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',  'k',  'l',
        'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',  'w',  'x',
        'y', 'z', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\n', '\n', '\t'};
    return chars;
}
template <>
inline const std::vector<wchar_t>& chars_spaces()
{
    static const std::vector<wchar_t> chars = {
        L'0', L'1', L'2', L'3',  L'4',  L'5', L'6', L'7', L'8', L'9', L'A',
        L'B', L'C', L'D', L'E',  L'F',  L'G', L'H', L'I', L'J', L'K', L'L',
        L'M', L'N', L'O', L'P',  L'Q',  L'R', L'S', L'T', L'U', L'V', L'W',
        L'X', L'Y', L'Z', L'a',  L'b',  L'c', L'd', L'e', L'f', L'g', L'h',
        L'i', L'j', L'k', L'l',  L'm',  L'n', L'o', L'p', L'q', L'r', L's',
        L't', L'u', L'v', L'w',  L'x',  L'y', L'z', L' ', L' ', L' ', L' ',
        L' ', L' ', L' ', L'\n', L'\n', L'\t'};
    return chars;
}

template <typename Char>
std::vector<std::basic_string<Char>> words_list(size_t n)
{
    static const auto& chars = chars_nospaces<Char>();
    static std::uniform_int_distribution<> dist(
        0, static_cast<int>(chars.size() - 1));

    std::vector<std::basic_string<Char>> ret;
    ret.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        auto len = dist(get_rng());
        if (len == 0) {
            len = 3;
        }
        std::basic_string<Char> str;
        str.reserve(static_cast<size_t>(len));
        for (int j = 0; j < len; ++j) {
            str.push_back(chars[static_cast<size_t>(dist(get_rng()))]);
        }
        ret.push_back(std::move(str));
    }
    return ret;
}
template <typename Char>
std::basic_string<Char> word_list(size_t n)
{
    static const auto& chars = chars_spaces<Char>();
    static std::uniform_int_distribution<> dist(
        0, static_cast<int>(chars.size() - 1));

    std::basic_string<Char> ret;
    ret.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        ret.push_back(chars[static_cast<size_t>(dist(get_rng()))]);
    }
    return ret;
}

template <typename Char>
inline scn::basic_string_view<Char> default_format_str()
{
}
template <>
inline scn::string_view default_format_str<char>()
{
    return {"{}"};
}
template <>
inline scn::wstring_view default_format_str<wchar_t>()
{
    return {L"{}"};
}

#endif  // SCN_BENCHMARK_WORD_H
