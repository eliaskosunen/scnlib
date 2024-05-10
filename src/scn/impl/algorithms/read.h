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

#include <scn/impl/algorithms/contiguous_range_factory.h>
#include <scn/impl/algorithms/find_whitespace.h>
#include <scn/impl/algorithms/unicode_algorithms.h>
#include <scn/impl/locale.h>
#include <scn/util/span.h>

#include <algorithm>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
template <typename Range>
auto read_all(const Range& range) -> ranges::const_iterator_t<Range>
{
    return ranges::next(range.begin(), range.end());
}

template <typename Range>
auto read_code_unit(const Range& range)
    -> eof_expected<ranges::const_iterator_t<Range>>
{
    if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
        return unexpected(e);
    }

    return ranges::next(range.begin());
}

template <typename Range>
auto read_exactly_n_code_units(const Range& range, std::ptrdiff_t count)
    -> eof_expected<ranges::const_iterator_t<Range>>
{
    SCN_EXPECT(count >= 0);

    if constexpr (ranges::sized_range<Range>) {
        const auto sz = static_cast<std::ptrdiff_t>(range.size());
        if (sz < count) {
            return unexpected(eof_error::eof);
        }

        return ranges::next(range.begin(), count);
    }
    else {
        auto it = range.begin();
        if (guaranteed_minimum_size(range) >= count) {
            return ranges::next(it, count);
        }

        for (std::ptrdiff_t i = 0; i < count; ++i, (void)++it) {
            if (it == range.end()) {
                return unexpected(eof_error::eof);
            }
        }

        return it;
    }
}

template <typename Range>
auto read_code_point_into(const Range& range)
    -> iterator_value_result<ranges::const_iterator_t<Range>,
                             std::basic_string<detail::char_t<Range>>>
{
    SCN_EXPECT(!is_range_eof(range));
    using string_type = std::basic_string<detail::char_t<Range>>;

    auto it = range.begin();
    const auto len = detail::code_point_length_by_starting_code_unit(*it);

    if (SCN_UNLIKELY(len == 0)) {
        ++it;
        it = get_start_for_next_code_point(ranges::subrange{it, range.end()});
        return {it, string_type{range.begin(), it}};
    }

    if (len == 1) {
        ++it;
        return {it, string_type{range.begin(), it}};
    }

    ranges::advance(it, static_cast<std::ptrdiff_t>(len), range.end());
    return {it, string_type{range.begin(), it}};
}

template <typename Range>
auto read_code_point(const Range& range) -> ranges::const_iterator_t<Range>
{
    return read_code_point_into(range).iterator;
}

template <typename Range>
auto read_exactly_n_code_points(const Range& range, std::ptrdiff_t count)
    -> eof_expected<ranges::const_iterator_t<Range>>
{
    SCN_EXPECT(count >= 0);

    if (count > 0) {
        if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
            return unexpected(e);
        }
    }

    auto it = range.begin();
    for (std::ptrdiff_t i = 0; i < count; ++i) {
        auto rng = ranges::subrange{it, range.end()};

        if (auto e = eof_check(rng); SCN_UNLIKELY(!e)) {
            return unexpected(e);
        }

        it = read_code_point(rng);
    }

    return it;
}

template <typename Range>
auto read_exactly_n_width_units(const Range& range, std::ptrdiff_t count)
    -> ranges::const_iterator_t<Range>
{
    auto it = range.begin();
    std::ptrdiff_t acc_width = 0;

    while (it != range.end()) {
        auto [iter, val] =
            read_code_point_into(ranges::subrange{it, range.end()});

        acc_width += calculate_text_width(val.view());
        if (acc_width > count) {
            break;
        }

        it = iter;
    }

    return it;
}

template <typename Range>
auto read_until_code_unit(const Range& range,
                          function_ref<bool(detail::char_t<Range>)> pred)
    -> ranges::const_iterator_t<Range>
{
    if constexpr (ranges::common_range<Range>) {
        return std::find_if(range.begin(), range.end(), pred);
    }
    else {
        auto first = range.begin();
        for (; first != range.end(); ++first) {
            if (pred(*first)) {
                return first;
            }
        }
        return first;
    }
}

template <typename Range>
auto read_while_code_unit(const Range& range,
                          function_ref<bool(detail::char_t<Range>)> pred)
    -> ranges::const_iterator_t<Range>
{
    return read_until_code_unit(range, std::not_fn(pred));
}

template <typename Range>
auto read_until1_code_unit(const Range& range,
                           function_ref<bool(detail::char_t<Range>)> pred)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    auto it = read_until_code_unit(range, pred);
    if (it == range.begin()) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range>
auto read_while1_code_unit(const Range& range,
                           function_ref<bool(detail::char_t<Range>)> pred)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    auto it = read_while_code_unit(range, pred);
    if (it == range.begin()) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range, typename CodeUnits>
auto read_until_code_units(const Range& range, const CodeUnits& needle)
    -> ranges::const_iterator_t<Range>
{
    static_assert(ranges::common_range<CodeUnits>);

    if constexpr (ranges::common_range<Range>) {
        return std::search(range.begin(), range.end(), needle.begin(),
                           needle.end());
    }
    else {
        auto first = range.begin();
        while (true) {
            auto it = first;
            for (auto needle_it = needle.begin();; ++it, (void)++needle_it) {
                if (needle_it == needle.end()) {
                    return first;
                }
                if (it == range.end()) {
                    return it;
                }
                if (*it != *needle_it) {
                    break;
                }
            }
            ++first;
        }
    }
}

template <typename Range, typename CodeUnits>
auto read_while_code_units(const Range& range, const CodeUnits& needle)
    -> ranges::const_iterator_t<Range>
{
    static_assert(ranges::common_range<CodeUnits>);

    auto it = range.begin();
    while (it != range.end()) {
        auto r = read_exactly_n_code_units(ranges::subrange{it, range.end()},
                                           needle.size());
        if (!r) {
            return it;
        }
        static_assert(
            std::is_same_v<decltype(it), detail::remove_cvref_t<decltype(*r)>>);
        if (!std::equal(it, *r, needle.begin())) {
            return it;
        }
        it = *r;
    }
    SCN_ENSURE(it == range.end());
    return it;
}

template <typename Range>
auto read_until_code_point(const Range& range,
                           function_ref<bool(char32_t)> pred)
    -> ranges::const_iterator_t<Range>
{
    auto it = range.begin();
    while (it != range.end()) {
        const auto val =
            read_code_point_into(ranges::subrange{it, range.end()});
        const auto cp = detail::decode_code_point_exhaustive(
            std::basic_string_view<detail::char_t<Range>>{val.value});
        if (pred(cp)) {
            return it;
        }
        it = val.iterator;
    }

    return it;
}

template <typename Range>
auto read_while_code_point(const Range& range,
                           function_ref<bool(char32_t)> pred)
    -> ranges::const_iterator_t<Range>
{
    return read_until_code_point(range, std::not_fn(pred));
}

template <typename Range>
auto read_until_classic_space(const Range& range)
    -> ranges::const_iterator_t<Range>
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range> &&
                  std::is_same_v<detail::char_t<Range>, char>) {
        auto buf = make_contiguous_buffer(SCN_FWD(range));
        auto it = find_classic_space_narrow_fast(buf.view());
        return ranges::next(range.begin(),
                            ranges::distance(buf.view().begin(), it));
    }
    else {
        auto it = range.begin();

        if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
            auto seg = get_contiguous_beginning(range);
            if (auto seg_it = find_classic_space_narrow_fast(seg);
                seg_it != seg.end()) {
                return ranges::next(it, ranges::distance(seg.begin(), seg_it));
            }
            ranges::advance(it, seg.size());
        }

        return read_until_code_point(
            ranges::subrange{it, range.end()},
            [](char32_t cp) noexcept { return is_cp_space(cp); });
    }
}

template <typename Range>
auto read_while_classic_space(const Range& range)
    -> ranges::const_iterator_t<Range>
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range> &&
                  std::is_same_v<detail::char_t<Range>, char>) {
        auto buf = make_contiguous_buffer(SCN_FWD(range));
        auto it = find_classic_nonspace_narrow_fast(buf.view());
        return ranges::next(range.begin(),
                            ranges::distance(buf.view().begin(), it));
    }
    else {
        auto it = range.begin();

        if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
            auto seg = get_contiguous_beginning(range);
            if (auto seg_it = find_classic_nonspace_narrow_fast(seg);
                seg_it != seg.end()) {
                return ranges::next(it, ranges::distance(seg.begin(), seg_it));
            }
            ranges::advance(it, seg.size());
        }

        return read_while_code_point(SCN_FWD(range), [](char32_t cp) noexcept {
            return is_cp_space(cp);
        });
    }
}

template <typename Range>
auto read_matching_code_unit(const Range& range, detail::char_t<Range> ch)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    auto it = read_code_unit(range);
    if (SCN_UNLIKELY(!it)) {
        return unexpected(make_eof_parse_error(it.error()));
    }

    if (SCN_UNLIKELY(*range.begin() !=
                     static_cast<detail::char_t<Range>>(ch))) {
        return unexpected(parse_error::error);
    }

    return *it;
}

template <typename Range>
auto read_matching_code_point(const Range& range, char32_t cp)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    auto [it, value] = read_code_point_into(range);
    auto decoded_cp = decode_code_point_exhaustive(value.view());
    if (SCN_UNLIKELY(cp != decoded_cp)) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range>
auto read_matching_string(const Range& range,
                          std::basic_string_view<detail::char_t<Range>> str)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    SCN_TRY(it, read_exactly_n_code_units(
                    range, static_cast<std::ptrdiff_t>(str.size()))
                    .transform_error(make_eof_parse_error));

    auto sv = make_contiguous_buffer(ranges::subrange{range.begin(), it});
    if (SCN_UNLIKELY(sv.view() != str)) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range>
auto read_matching_string_classic(const Range& range, std::string_view str)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    SCN_TRY(it, read_exactly_n_code_units(
                    range, static_cast<std::ptrdiff_t>(str.size()))
                    .transform_error(make_eof_parse_error));

    if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
        auto sv = make_contiguous_buffer(ranges::subrange{range.begin(), it});
        if (SCN_UNLIKELY(sv.view() != str)) {
            return unexpected(parse_error::error);
        }
        return it;
    }
    else {
        auto range_it = range.begin();
        for (size_t i = 0; i < str.size(); ++i, (void)++range_it) {
            if (SCN_UNLIKELY(*range_it !=
                             static_cast<detail::char_t<Range>>(str[i]))) {
                return unexpected(parse_error::error);
            }
        }
        return it;
    }
}

// Ripped from fast_float
constexpr bool fast_streq_nocase(const char* a, const char* b, size_t len)
{
    unsigned char running_diff{0};
    for (size_t i = 0; i < len; ++i) {
        running_diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return running_diff == 0 || running_diff == 32;
}

template <typename Range>
auto read_matching_string_classic_nocase(const Range& range,
                                         std::string_view str)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    using char_type = detail::char_t<Range>;

    if constexpr (ranges::contiguous_range<Range> &&
                  std::is_same_v<char_type, char>) {
        if (range.size() < str.size()) {
            return unexpected(make_eof_parse_error(eof_error::eof));
        }
        if (!fast_streq_nocase(range.data(), str.data(), str.size())) {
            return unexpected(parse_error::error);
        }
        return ranges::next(range.begin(), str.size());
    }
    else {
        auto ascii_tolower = [](char_type ch) -> char_type {
            if (ch < 'A' || ch > 'Z') {
                return ch;
            }
            return static_cast<char_type>(ch +
                                          static_cast<char_type>('a' - 'A'));
        };

        SCN_TRY(it, read_exactly_n_code_units(
                        range, static_cast<std::ptrdiff_t>(str.size()))
                        .transform_error(make_eof_parse_error));

        if (SCN_UNLIKELY(!std::equal(
                range.begin(), it, str.begin(), [&](auto a, auto b) {
                    return ascii_tolower(a) ==
                           static_cast<detail::char_t<Range>>(b);
                }))) {
            return unexpected(parse_error::error);
        }

        return it;
    }
}

template <typename Range>
auto read_one_of_code_unit(const Range& range, std::string_view str)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    auto it = read_code_unit(range);
    if (SCN_UNLIKELY(!it)) {
        return unexpected(make_eof_parse_error(it.error()));
    }

    for (auto ch : str) {
        if (*range.begin() == static_cast<detail::char_t<Range>>(ch)) {
            return *it;
        }
    }

    return unexpected(parse_error::error);
}

template <typename Range, template <class> class Expected, typename Iterator>
auto apply_opt(Expected<Iterator>&& result, const Range& range)
    -> std::enable_if_t<detail::is_expected<Expected<Iterator>>::value,
                        ranges::const_iterator_t<Range>>
{
    if (!result) {
        return range.begin();
    }
    return *result;
}
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
