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

#include <scn/detail/ranges.h>
#include <scn/impl/algorithms/contiguous_range_factory.h>
#include <scn/impl/algorithms/find_whitespace.h>
#include <scn/impl/algorithms/read_simple.h>
#include <scn/impl/locale.h>
#include <scn/impl/unicode/unicode.h>
#include <scn/impl/unicode/unicode_whitespace.h>
#include <scn/impl/util/function_ref.h>
#include <scn/util/span.h>

#include <algorithm>
#include <utility>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
template <typename Range>
auto read_code_point_into(Range&& range)
    -> iterator_value_result<simple_borrowed_iterator_t<Range>,
                             contiguous_range_factory<detail::char_t<Range>>>
{
    SCN_EXPECT(ranges::begin(range) != ranges::end(range));

    auto it = ranges::begin(range);
    const auto len = code_point_length_by_starting_code_unit(*it);

    if (SCN_UNLIKELY(len == 0)) {
        for (; it != ranges::end(range); ++it) {
            if (code_point_length_by_starting_code_unit(*it) != 0) {
                break;
            }
        }

        return {it, make_contiguous_buffer(
                        ranges::subrange{ranges::begin(range), it})};
    }

    if (len == 1) {
        ++it;
        return {it, make_contiguous_buffer(
                        ranges::subrange{ranges::begin(range), it})};
    }

    ranges::advance(it, static_cast<ranges::range_difference_t<Range>>(len),
                    ranges::end(range));
    return {it,
            make_contiguous_buffer(ranges::subrange{ranges::begin(range), it})};
}

template <typename Range>
simple_borrowed_iterator_t<Range> read_code_point(Range&& range)
{
    return read_code_point_into(SCN_FWD(range)).iterator;
}

template <typename Range>
eof_expected<simple_borrowed_iterator_t<Range>> read_exactly_n_code_points(
    Range&& range,
    ranges::range_difference_t<Range> count)
{
    SCN_EXPECT(count >= 0);

    if (count > 0) {
        if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
            return unexpected(e);
        }
    }

    auto it = ranges::begin(range);
    for (ranges::range_difference_t<Range> i = 0; i < count; ++i) {
        auto rng = ranges::subrange{it, ranges::end(range)};

        if (auto e = eof_check(rng); SCN_UNLIKELY(!e)) {
            return unexpected(e);
        }

        it = read_code_point(rng);
    }

    return it;
}

template <typename Range>
simple_borrowed_iterator_t<Range> read_exactly_n_width_units(
    Range&& range,
    ranges::range_difference_t<Range> count)
{
    auto it = ranges::begin(range);
    ranges::range_difference_t<Range> acc_width = 0;

    while (it != ranges::end(range)) {
        auto [iter, val] =
            read_code_point_into(ranges::subrange{it, ranges::end(range)});

        acc_width += calculate_text_width(val.view());
        if (acc_width > count) {
            break;
        }

        it = iter;
    }

    return it;
}

template <typename Range>
simple_borrowed_iterator_t<Range> read_until_code_unit(
    Range&& range,
    function_ref<bool(detail::char_t<Range>)> pred)
{
    return ranges::find_if(range, pred);
}

template <typename Range>
simple_borrowed_iterator_t<Range> read_while_code_unit(
    Range&& range,
    function_ref<bool(detail::char_t<Range>)> pred)
{
    return read_until_code_unit(SCN_FWD(range), std::not_fn(pred));
}

template <typename Range>
parse_expected<simple_borrowed_iterator_t<Range>> read_until1_code_unit(
    Range&& range,
    function_ref<bool(detail::char_t<Range>)> pred)
{
    auto it = read_until_code_unit(range, pred);
    if (it == ranges::begin(range)) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range>
parse_expected<simple_borrowed_iterator_t<Range>> read_while1_code_unit(
    Range&& range,
    function_ref<bool(detail::char_t<Range>)> pred)
{
    auto it = read_while_code_unit(range, pred);
    if (it == ranges::begin(range)) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range, typename CodeUnits>
simple_borrowed_iterator_t<Range> read_until_code_units(Range&& range,
                                                        CodeUnits&& needle)
{
    return ranges::search(SCN_FWD(range), SCN_FWD(needle)).begin();
}

template <typename Range, typename CodeUnits>
simple_borrowed_iterator_t<Range> read_while_code_units(Range&& range,
                                                        CodeUnits&& needle)
{
    auto subr = ranges::subrange{range};
    while (!subr.empty()) {
        auto [beg, cp] = read_code_point_into(subr);
        if (!ranges::equal(cp.view(), needle)) {
            return subr.begin();
        }
        subr = ranges::subrange{beg, subr.end()};
    }
    SCN_ENSURE(subr.begin() == subr.end());
    return subr.begin();
}

template <typename Range>
simple_borrowed_iterator_t<Range> read_until_code_point_eager(
    Range&& range,
    function_ref<bool(char32_t)> pred)
{
    static_assert(ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>);

    std::array<char32_t, 16> cp_buf{};
    std::array<uint8_t, 16> idx_buf{};
    auto it = ranges::begin(range);
    while (it != ranges::end(range)) {
        auto chunk_begin = it;
        size_t code_point_count = 0;
        uint8_t code_unit_idx = 0;
        while (code_point_count < cp_buf.size() && it != ranges::end(range)) {
            if (code_point_length_by_starting_code_unit(*it) != 0) {
                idx_buf[code_point_count] = code_unit_idx;
                ++code_point_count;
            }
            ++it;
            ++code_unit_idx;
        }

        auto input = detail::make_string_view_from_pointers(
            detail::to_address(chunk_begin), detail::to_address(it));
        auto codepoints = span{cp_buf.data(), code_point_count};
        auto transcode_result = transcode_possibly_invalid(input, codepoints);
        if (SCN_UNLIKELY(!transcode_result)) {
            auto end = it;
            it = chunk_begin;
            while (it != end) {
                const auto [iter, value] =
                    read_code_point_into(ranges::subrange{it, end});
                const auto cp = decode_code_point_exhaustive(value.view());
                if (pred(cp)) {
                    return it;
                }
                it = iter;
            }
            continue;
        }

        for (size_t i = 0; i < code_point_count; ++i) {
            if (pred(cp_buf[i])) {
                return chunk_begin + idx_buf[i];
            }
        }
    }

    return it;
}

template <typename Range>
simple_borrowed_iterator_t<Range> read_until_code_point(
    Range&& range,
    function_ref<bool(char32_t)> pred)
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return read_until_code_point_eager(SCN_FWD(range), pred);
    }
    else {
        auto it = ranges::begin(range);
        auto seg = get_contiguous_beginning(range);

        if (auto seg_it = read_until_code_point_eager(seg, pred);
            seg_it != seg.end()) {
            return ranges_polyfill::batch_next(
                it, ranges::distance(seg.begin(), seg_it));
        }

        while (it != ranges::end(range)) {
            const auto [iter, value] =
                read_code_point_into(ranges::subrange{it, ranges::end(range)});
            const auto cp = decode_code_point_exhaustive(value.view());
            if (pred(cp)) {
                return it;
            }
            it = iter;
        }

        return it;
    }
}

template <typename Range>
simple_borrowed_iterator_t<Range> read_while_code_point(
    Range&& range,
    function_ref<bool(char32_t)> pred)
{
    return read_until_code_point(SCN_FWD(range), std::not_fn(pred));
}

template <typename Range>
simple_borrowed_iterator_t<Range> read_until_classic_space(Range&& range)
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range> &&
                  std::is_same_v<detail::char_t<Range>, char>) {
        auto buf = make_contiguous_buffer(SCN_FWD(range));
        auto it = find_classic_space_narrow_fast(buf.view());
        return ranges::next(ranges::begin(range),
                            ranges::distance(buf.view().begin(), it));
    }
    else {
        auto it = ranges::begin(range);

        if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
            auto seg = get_contiguous_beginning(range);
            if (auto seg_it = find_classic_space_narrow_fast(seg);
                seg_it != seg.end()) {
                return ranges_polyfill::batch_next(
                    it, ranges::distance(seg.begin(), seg_it));
            }
            ranges_polyfill::batch_next(it, seg.size());
        }

        return read_until_code_point(
            ranges::subrange{it, ranges::end(range)},
            [](char32_t cp) SCN_NOEXCEPT { return is_cp_space(cp); });
    }
}

template <typename Range>
simple_borrowed_iterator_t<Range> read_while_classic_space(Range&& range)
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range> &&
                  std::is_same_v<detail::char_t<Range>, char>) {
        auto buf = make_contiguous_buffer(SCN_FWD(range));
        auto it = find_classic_nonspace_narrow_fast(buf.view());
        return ranges::next(ranges::begin(range),
                            ranges::distance(buf.view().begin(), it));
    }
    else {
        auto it = ranges::begin(range);

        if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
            auto seg = get_contiguous_beginning(range);
            if (auto seg_it = find_classic_nonspace_narrow_fast(seg);
                seg_it != seg.end()) {
                return ranges_polyfill::batch_next(
                    it, ranges::distance(seg.begin(), seg_it));
            }
            ranges_polyfill::batch_next(it, seg.size());
        }

        return read_while_code_point(
            SCN_FWD(range),
            [](char32_t cp) SCN_NOEXCEPT { return is_cp_space(cp); });
    }
}

template <typename Range>
parse_expected<simple_borrowed_iterator_t<Range>> read_matching_code_unit(
    Range&& range,
    detail::char_t<Range> ch)
{
    auto it = read_code_unit(range);
    if (SCN_UNLIKELY(!it)) {
        return unexpected(make_eof_parse_error(it.error()));
    }

    if (SCN_UNLIKELY(*ranges::begin(range) !=
                     static_cast<detail::char_t<Range>>(ch))) {
        return unexpected(parse_error::error);
    }

    return *it;
}

template <typename Range>
parse_expected<simple_borrowed_iterator_t<Range>> read_matching_code_point(
    Range&& range,
    char32_t cp)
{
    auto [it, value] = read_code_point_into(range);
    auto decoded_cp = decode_code_point_exhaustive(value.view());
    if (SCN_UNLIKELY(cp != decoded_cp)) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range>
parse_expected<simple_borrowed_iterator_t<Range>> read_matching_string(
    Range&& range,
    std::basic_string_view<detail::char_t<Range>> str)
{
    SCN_TRY(it, read_exactly_n_code_units(range, ranges::ssize(str))
                    .transform_error(make_eof_parse_error));

    auto sv =
        make_contiguous_buffer(ranges::subrange{ranges::begin(range), it});
    if (SCN_UNLIKELY(sv.view() != str)) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range>
parse_expected<simple_borrowed_iterator_t<Range>> read_matching_string_classic(
    Range&& range,
    std::string_view str)
{
    SCN_TRY(it, read_exactly_n_code_units(range, ranges::ssize(str))
                    .transform_error(make_eof_parse_error));

    if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
        auto sv =
            make_contiguous_buffer(ranges::subrange{ranges::begin(range), it});
        if (SCN_UNLIKELY(sv.view() != str)) {
            return unexpected(parse_error::error);
        }
        return it;
    }
    else {
        auto range_it = ranges::begin(range);
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
inline constexpr bool fast_streq_nocase(const char* a,
                                        const char* b,
                                        size_t len)
{
    unsigned char running_diff{0};
    for (size_t i = 0; i < len; ++i) {
        running_diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return running_diff == 0 || running_diff == 32;
}

template <typename Range>
parse_expected<simple_borrowed_iterator_t<Range>>
read_matching_string_classic_nocase(Range&& range, std::string_view str)
{
    using char_type = detail::char_t<Range>;

    if constexpr (ranges::contiguous_range<Range> &&
                  std::is_same_v<char_type, char>) {
        if (ranges::size(range) < str.size()) {
            return unexpected(make_eof_parse_error(eof_error::eof));
        }
        if (!fast_streq_nocase(ranges::data(range), str.data(), str.size())) {
            return unexpected(parse_error::error);
        }
        return ranges::next(ranges::begin(range), str.size());
    }
    else {
        auto ascii_tolower = [](char_type ch) -> char_type {
            if (ch < 'A' || ch > 'Z') {
                return ch;
            }
            return static_cast<char_type>(ch +
                                          static_cast<char_type>('a' - 'A'));
        };

        SCN_TRY(it, read_exactly_n_code_units(range, ranges::ssize(str))
                        .transform_error(make_eof_parse_error));

        if (SCN_UNLIKELY(!std::equal(
                ranges::begin(range), it, str.begin(), [&](auto a, auto b) {
                    return ascii_tolower(a) ==
                           static_cast<detail::char_t<Range>>(b);
                }))) {
            return unexpected(parse_error::error);
        }

        return it;
    }
}

template <typename Range>
parse_expected<simple_borrowed_iterator_t<Range>> read_one_of_code_unit(
    Range&& range,
    std::string_view str)
{
    auto it = read_code_unit(range);
    if (SCN_UNLIKELY(!it)) {
        return unexpected(make_eof_parse_error(it.error()));
    }

    for (auto ch : str) {
        if (*ranges::begin(range) == static_cast<detail::char_t<Range>>(ch)) {
            return *it;
        }
    }

    return unexpected(parse_error::error);
}

template <typename Range,
          template <class>
          class Expected,

          typename Iterator>
auto apply_opt(Expected<Iterator>&& result, Range&& range)
    -> std::enable_if_t<detail::is_expected<Expected<Iterator>>::value,
                        simple_borrowed_iterator_t<Range>>
{
    if (!result) {
        return ranges::begin(range);
    }
    return *result;
}
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
