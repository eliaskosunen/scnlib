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

#include <scn/detail/unicode.h>
#include <scn/impl/algorithms/common.h>
#include <scn/impl/util/function_ref.h>
#include <scn/util/expected.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {

template <typename CharT>
constexpr bool validate_unicode(std::basic_string_view<CharT> src)
{
    auto it = src.begin();
    while (it != src.end()) {
        const auto len = detail::code_point_length_by_starting_code_unit(*it);
        if (len == 0) {
            return false;
        }
        if (src.end() - it < len) {
            return false;
        }
        const auto cp = detail::decode_code_point_exhaustive(
            detail::make_string_view_from_iterators<CharT>(it, it + len));
        if (cp >= detail::invalid_code_point) {
            return false;
        }
        it += len;
    }
    return true;
}

template <typename Range>
constexpr auto get_start_for_next_code_point(const Range& input)
    -> ranges::const_iterator_t<Range>
{
    auto it = input.begin();
    for (; it != input.end(); ++it) {
        if (detail::code_point_length_by_starting_code_unit(*it) != 0) {
            break;
        }
    }
    return it;
}

template <typename CharT>
constexpr auto get_next_code_point(std::basic_string_view<CharT> input)
    -> iterator_value_result<typename std::basic_string_view<CharT>::iterator,
                             char32_t>
{
    SCN_EXPECT(!input.empty());

    const auto len = detail::code_point_length_by_starting_code_unit(input[0]);
    if (SCN_UNLIKELY(len == 0)) {
        return {get_start_for_next_code_point(input),
                detail::invalid_code_point};
    }
    if (SCN_UNLIKELY(len > input.size())) {
        return {input.end(), detail::invalid_code_point};
    }

    return {input.begin() + len,
            detail::decode_code_point_exhaustive(input.substr(0, len))};
}

template <typename CharT>
constexpr auto get_next_code_point_valid(std::basic_string_view<CharT> input)
    -> iterator_value_result<typename std::basic_string_view<CharT>::iterator,
                             char32_t>
{
    SCN_EXPECT(!input.empty());

    const auto len = detail::code_point_length_by_starting_code_unit(input[0]);
    SCN_EXPECT(len <= input.size());

    return {input.begin() + len,
            detail::decode_code_point_exhaustive_valid(input.substr(0, len))};
}

constexpr bool is_cp_space(char32_t cp) noexcept
{
    // Pattern_White_Space property
    return (cp >= 0x09 && cp <= 0x0d) ||
           cp == 0x20 ||    // ASCII space characters
           cp == 0x85 ||    // NEXT LINE (NEL)
           cp == 0x200e ||  // LEFT-TO-RIGHT MARK
           cp == 0x200f ||  // RIGHT-TO-LEFT MARK
           cp == 0x2028 ||  // LINE SEPARATOR
           cp == 0x2029;    // PARAGRAPH SEPARATOR
}

template <typename CharT>
struct is_first_char_space_result {
    ranges::iterator_t<std::basic_string_view<CharT>> iterator;
    char32_t cp;
    bool is_space;
};

template <typename CharT>
inline constexpr auto is_first_char_space(std::basic_string_view<CharT> str)
    -> is_first_char_space_result<CharT>
{
    // TODO: optimize
    SCN_EXPECT(!str.empty());
    auto res = get_next_code_point(str);
    return {res.iterator, res.value, is_cp_space(res.value)};
}

inline constexpr scan_expected<wchar_t> encode_code_point_as_wide_character(
    char32_t cp,
    bool error_on_overflow)
{
    SCN_EXPECT(cp < detail::invalid_code_point);
    if constexpr (sizeof(wchar_t) == sizeof(char32_t)) {
        SCN_UNUSED(error_on_overflow);
        return static_cast<wchar_t>(cp);
    }
    else {
        if (cp < 0x10000) {
            return static_cast<wchar_t>(cp);
        }
        if (error_on_overflow) {
            return unexpected_scan_error(scan_error::value_out_of_range,
                                         "Non-BOM code point can't be "
                                         "narrowed to a single 2-byte "
                                         "wchar_t code unit");
        }
        // Return the lead surrogate
        return static_cast<wchar_t>(
            (static_cast<uint32_t>(cp) - 0x10000) / 0x400 + 0xd800);
    }
}

template <typename SourceCharT, typename DestCharT>
void transcode_to_string_impl_to32(std::basic_string_view<SourceCharT> src,
                                   std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(DestCharT) == 4);

    auto it = src.begin();
    while (it != src.end()) {
        auto res = get_next_code_point(
            detail::make_string_view_from_iterators<SourceCharT>(it,
                                                                 src.end()));
        if (SCN_UNLIKELY(res.value == detail::invalid_code_point)) {
            dest.push_back(DestCharT{0xfffd});
        }
        else {
            dest.push_back(res.value);
        }
        it = detail::make_string_view_iterator(src, res.iterator);
    }
}
template <typename SourceCharT, typename DestCharT>
void transcode_valid_to_string_impl_to32(
    std::basic_string_view<SourceCharT> src,
    std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(DestCharT) == 4);

    auto it = src.begin();
    while (it != src.end()) {
        auto res = get_next_code_point_valid(
            detail::make_string_view_from_iterators<SourceCharT>(it,
                                                                 src.end()));
        SCN_EXPECT(res.value < detail::invalid_code_point);
        dest.push_back(res.value);
        it = detail::make_string_view_iterator(src, res.iterator);
    }
}

template <bool VerifiedValid, typename SourceCharT, typename DestCharT>
void transcode_to_string_impl_32to8(std::basic_string_view<SourceCharT> src,
                                    std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(SourceCharT) == 4);
    static_assert(sizeof(DestCharT) == 1);

    for (auto cp : src) {
        const auto u32cp = static_cast<uint32_t>(cp);
        if (SCN_UNLIKELY(!VerifiedValid && cp >= detail::invalid_code_point)) {
            // Replacement character
            dest.push_back(static_cast<char>(0xef));
            dest.push_back(static_cast<char>(0xbf));
            dest.push_back(static_cast<char>(0xbd));
        }
        else if (cp < 128) {
            dest.push_back(static_cast<char>(cp));
        }
        else if (cp < 2048) {
            dest.push_back(
                static_cast<char>(0xc0 | (static_cast<char>(u32cp >> 6))));
            dest.push_back(
                static_cast<char>(0x80 | (static_cast<char>(u32cp) & 0x3f)));
        }
        else if (cp < 65536) {
            dest.push_back(
                static_cast<char>(0xe0 | (static_cast<char>(u32cp >> 12))));
            dest.push_back(static_cast<char>(
                0x80 | (static_cast<char>(u32cp >> 6) & 0x3f)));
            dest.push_back(
                static_cast<char>(0x80 | (static_cast<char>(u32cp) & 0x3f)));
        }
        else {
            dest.push_back(
                static_cast<char>(0xf0 | (static_cast<char>(u32cp >> 18))));
            dest.push_back(static_cast<char>(
                0x80 | (static_cast<char>(u32cp >> 12) & 0x3f)));
            dest.push_back(static_cast<char>(
                0x80 | (static_cast<char>(u32cp >> 6) & 0x3f)));
            dest.push_back(
                static_cast<char>(0x80 | (static_cast<char>(u32cp) & 0x3f)));
        }
    }
}

template <bool VerifiedValid, typename SourceCharT, typename DestCharT>
void transcode_to_string_impl_32to16(std::basic_string_view<SourceCharT> src,
                                     std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(SourceCharT) == 4);
    static_assert(sizeof(DestCharT) == 2);

    for (auto cp : src) {
        const auto u32cp = static_cast<uint32_t>(cp);
        if (SCN_UNLIKELY(!VerifiedValid && cp >= detail::invalid_code_point)) {
            dest.push_back(char16_t{0xfffd});
        }
        else if (cp < 0x10000) {
            dest.push_back(static_cast<char16_t>(cp));
        }
        else {
            dest.push_back(
                static_cast<char16_t>((u32cp - 0x10000) / 0x400 + 0xd800));
            dest.push_back(
                static_cast<char16_t>((u32cp - 0x10000) % 0x400 + 0xd800));
        }
    }
}

template <typename SourceCharT, typename DestCharT>
void transcode_to_string(std::basic_string_view<SourceCharT> src,
                         std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(SourceCharT) != sizeof(DestCharT));

    if constexpr (sizeof(DestCharT) == 4) {
        transcode_to_string_impl_to32(src, dest);
    }
    else if constexpr (sizeof(SourceCharT) == 4) {
        if constexpr (sizeof(DestCharT) == 1) {
            transcode_to_string_impl_32to8<false>(src, dest);
        }
        else {
            transcode_to_string_impl_32to16<false>(src, dest);
        }
    }
    else if constexpr (sizeof(DestCharT) == 2) {
        // TODO: optimize by removing utf32 step
        std::u32string tmp;
        transcode_to_string_impl_to32(src, tmp);
        if constexpr (sizeof(SourceCharT) == 1) {
            transcode_to_string_impl_32to8<false>(std::u32string_view{tmp},
                                                  dest);
        }
        else {
            transcode_to_string_impl_32to16<false>(std::u32string_view{tmp},
                                                   dest);
        }
    }
}
template <typename SourceCharT, typename DestCharT>
void transcode_valid_to_string(std::basic_string_view<SourceCharT> src,
                               std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(SourceCharT) != sizeof(DestCharT));

    SCN_EXPECT(validate_unicode(src));
    if constexpr (sizeof(DestCharT) == 4) {
        transcode_valid_to_string_impl_to32(src, dest);
    }
    else if constexpr (sizeof(SourceCharT) == 4) {
        if constexpr (sizeof(DestCharT) == 1) {
            transcode_to_string_impl_32to8<true>(src, dest);
        }
        else {
            transcode_to_string_impl_32to16<true>(src, dest);
        }
    }
    else if constexpr (sizeof(DestCharT) == 2) {
        // TODO: optimize by removing utf32 step
        std::u32string tmp;
        transcode_valid_to_string_impl_to32(src, tmp);
        if constexpr (sizeof(SourceCharT) == 1) {
            transcode_to_string_impl_32to8<true>(std::u32string_view{tmp},
                                                 dest);
        }
        else {
            transcode_to_string_impl_32to16<true>(std::u32string_view{tmp},
                                                  dest);
        }
    }
}

template <typename CharT>
constexpr void for_each_code_point(std::basic_string_view<CharT> input,
                                   function_ref<void(char32_t)> cb)
{
    // TODO: Could be optimized by being eager
    auto it = input.begin();
    while (it != input.end()) {
        auto res = get_next_code_point(
            detail::make_string_view_from_iterators<CharT>(it, input.end()));
        cb(res.value);
        it = detail::make_string_view_iterator(input, res.iterator);
    }
}

template <typename CharT>
constexpr void for_each_code_point_valid(std::basic_string_view<CharT> input,
                                         function_ref<void(char32_t)> cb)
{
    auto it = input.begin();
    while (it != input.end()) {
        auto res = get_next_code_point_valid(
            detail::make_string_view_from_iterators<CharT>(it, input.end()));
        cb(res.value);
        it = detail::make_string_view_iterator(input, res.iterator);
    }
}

}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
