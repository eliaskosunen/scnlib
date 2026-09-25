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

#include "wrapped_gtest.h"

#include <scn/scan.h>

#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

namespace scn_test_detail {
template <typename T>
inline constexpr bool is_char_type =
    std::is_same_v<T, char> || std::is_same_v<T, wchar_t> ||
#if SCN_HAS_CHAR8
    std::is_same_v<T, char8_t> ||
#endif
    std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>;

template <typename T, typename = void>
inline constexpr bool has_range_member = false;
template <typename T>
inline constexpr bool has_range_member<
    T,
    std::enable_if_t<
        scn::ranges::range<decltype(std::declval<const T&>().range())>>> = true;

template <typename It>
inline constexpr bool is_scan_buffer_iterator =
    std::is_same_v<It,
                   typename scn::detail::basic_scan_buffer<char>::iterator> ||
    std::is_same_v<It,
                   typename scn::detail::basic_scan_buffer<wchar_t>::iterator>;

template <typename CharT>
void print_string(std::basic_string_view<CharT> str, std::ostream* os)
{
    if constexpr (std::is_same_v<CharT, char> ||
                  std::is_same_v<CharT, wchar_t>) {
        *os << ::testing::PrintToString(str);
    }
    else {
        *os << ::testing::PrintToString(std::basic_string<CharT>{str});
    }
}
}  // namespace scn_test_detail

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
template <
    typename It,
    std::enable_if_t<scn_test_detail::is_scan_buffer_iterator<It>>* = nullptr>
std::ostream& operator<<(std::ostream& os, const It& it)
{
    os << "scan_buffer::iterator{position: " << it.position() << ", buffered: ";
    scn_test_detail::print_string(it.contiguous_segment(), &os);
    return os << '}';
}
}  // namespace detail

namespace ranges::detail::subrange_ {
template <typename I, typename S>
void PrintTo(const subrange<I, S>& r, std::ostream* os)
{
    using char_type = iter_value_t<I>;
    if constexpr (!scn_test_detail::is_char_type<char_type>) {
        *os << "subrange{";
        bool first = true;
        for (auto it = r.begin(); it != r.end(); ++it) {
            *os << (first ? "" : ", ") << ::testing::PrintToString(*it);
            first = false;
        }
        *os << '}';
    }
    else if constexpr (contiguous_range<subrange<I, S>> &&
                       sized_range<subrange<I, S>>) {
        scn_test_detail::print_string(
            std::basic_string_view<char_type>(ranges::data(r), ranges::size(r)),
            os);
    }
    else if constexpr (scn_test_detail::is_scan_buffer_iterator<I>) {
        scn_test_detail::print_string(r.begin().contiguous_segment(), os);
        if (r.begin().stores_parent()) {
            *os << "...";
        }
    }
    else {
        std::basic_string<char_type> str;
        for (auto it = r.begin(); it != r.end(); ++it) {
            str.push_back(*it);
        }
        scn_test_detail::print_string(std::basic_string_view<char_type>(str),
                                      os);
    }
}
}  // namespace ranges::detail::subrange_

template <typename CharT>
void PrintTo(const basic_regex_match<CharT>& m, std::ostream* os)
{
    *os << "regex_match{";
    scn_test_detail::print_string(m.get(), os);
#if SCN_REGEX_SUPPORTS_NAMED_CAPTURES
    if (auto name = m.name(); name) {
        *os << ", name: ";
        scn_test_detail::print_string(*name, os);
    }
#endif
    *os << '}';
}

template <typename CharT>
void PrintTo(const basic_regex_matches<CharT>& m, std::ostream* os)
{
    *os << "regex_matches{";
    for (std::size_t i = 0; i < m.size(); ++i) {
        *os << (i == 0 ? "" : ", ") << ::testing::PrintToString(m[i]);
    }
    *os << '}';
}

inline void PrintTo(const scan_error& err, std::ostream* os)
{
    *os << "scan_error{code: " << static_cast<int>(err.code()) << ", msg: \""
        << err.msg() << "\"}";
}

template <typename T, typename E>
void PrintTo(const expected<T, E>& e, std::ostream* os)
{
    if (e) {
        *os << "(success)";
        if constexpr (!std::is_void_v<T>) {
            *os << ' ' << ::testing::PrintToString(*e);
        }
    }
    else {
        *os << "(error) " << ::testing::PrintToString(e.error());
    }
}

template <typename Source, typename... Args>
void PrintTo(const scan_result<Source, Args...>& r, std::ostream* os)
{
    *os << "scan_result{values: " << ::testing::PrintToString(r.values());
    if constexpr (scn_test_detail::has_range_member<
                      scan_result<Source, Args...>>) {
        *os << ", remaining: " << ::testing::PrintToString(r.range());
    }
    *os << '}';
}

template <typename T>
void PrintTo(const scan_expected<T>& e, std::ostream* os)
{
    PrintTo(static_cast<const expected<T, scan_error>&>(e), os);
}

SCN_END_NAMESPACE
}  // namespace scn

MATCHER(Succeeded, negation ? "has an error" : "has a value")
{
    return arg.has_value();
}

MATCHER(Failed, negation ? "has a value" : "has an error")
{
    return !arg.has_value();
}

MATCHER_P(FailedWith,
          code,
          std::string{negation ? "doesn't have" : "has"} +
              " an error with code " + ::testing::PrintToString(code))
{
    return !arg.has_value() && arg.error().code() == code;
}

MATCHER(IsEmptyRange, negation ? "isn't empty" : "is empty")
{
    return scn::ranges::begin(arg) == scn::ranges::end(arg);
}
