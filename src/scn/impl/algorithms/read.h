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
    -> iterator_value_result<detail::simple_borrowed_iterator_t<Range>,
                             contiguous_range_factory<detail::char_t<Range>>>
{
    SCN_EXPECT(ranges_impl::begin(range) != ranges_impl::end(range));

    auto it = ranges_impl::begin(range);
    const auto len = code_point_length_by_starting_code_unit(*it);

    if (SCN_UNLIKELY(len == 0)) {
        for (; it != ranges_impl::end(range); ++it) {
            if (code_point_length_by_starting_code_unit(*it) != 0) {
                break;
            }
        }

        return {it, make_contiguous_buffer(
                        ranges_impl::subrange{ranges_impl::begin(range), it})};
    }

    if (len == 1) {
        ++it;
        return {it, make_contiguous_buffer(
                        ranges_impl::subrange{ranges_impl::begin(range), it})};
    }

    ranges_impl::advance(it, static_cast<ranges_impl::range_difference_t<Range>>(len),
                    ranges_impl::end(range));
    return {it,
            make_contiguous_buffer(ranges_impl::subrange{ranges_impl::begin(range), it})};
}

template <typename Range>
auto read_code_point(Range&& range) -> detail::simple_borrowed_iterator_t<Range>
{
    return read_code_point_into(SCN_FWD(range)).iterator;
}

template <typename Range>
auto read_exactly_n_code_points(Range&& range,
                                ranges_impl::range_difference_t<Range> count)
    -> eof_expected<detail::simple_borrowed_iterator_t<Range>>
{
    SCN_EXPECT(count >= 0);

    if (count > 0) {
        if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
            return unexpected(e);
        }
    }

    auto it = ranges_impl::begin(range);
    for (ranges_impl::range_difference_t<Range> i = 0; i < count; ++i) {
        auto rng = ranges_impl::subrange{it, ranges_impl::end(range)};

        if (auto e = eof_check(rng); SCN_UNLIKELY(!e)) {
            return unexpected(e);
        }

        it = read_code_point(rng);
    }

    return it;
}

template <typename Range>
auto read_exactly_n_width_units(Range&& range,
                                ranges_impl::range_difference_t<Range> count)
    -> detail::simple_borrowed_iterator_t<Range>
{
    auto it = ranges_impl::begin(range);
    ranges_impl::range_difference_t<Range> acc_width = 0;

    while (it != ranges_impl::end(range)) {
        auto [iter, val] =
            read_code_point_into(ranges_impl::subrange{it, ranges_impl::end(range)});

        acc_width += calculate_text_width(val.view());
        if (acc_width > count) {
            break;
        }

        it = iter;
    }

    return it;
}

template <typename Range>
auto read_until_code_unit(Range&& range,
                          function_ref<bool(detail::char_t<Range>)> pred)
    -> detail::simple_borrowed_iterator_t<Range>
{
    return ranges_impl::find_if(range, pred);
}

template <typename Range>
auto read_while_code_unit(Range&& range,
                          function_ref<bool(detail::char_t<Range>)> pred)
    -> detail::simple_borrowed_iterator_t<Range>
{
    return read_until_code_unit(SCN_FWD(range), std::not_fn(pred));
}

template <typename Range>
auto read_until1_code_unit(Range&& range,
                           function_ref<bool(detail::char_t<Range>)> pred)
    -> parse_expected<detail::simple_borrowed_iterator_t<Range>>
{
    auto it = read_until_code_unit(range, pred);
    if (it == ranges_impl::begin(range)) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range>
auto read_while1_code_unit(Range&& range,
                           function_ref<bool(detail::char_t<Range>)> pred)
    -> parse_expected<detail::simple_borrowed_iterator_t<Range>>
{
    auto it = read_while_code_unit(range, pred);
    if (it == ranges_impl::begin(range)) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range, typename CodeUnits>
auto read_until_code_units(Range&& range, CodeUnits&& needle)
    -> detail::simple_borrowed_iterator_t<Range>
{
    return ranges_impl::search(SCN_FWD(range), SCN_FWD(needle)).begin();
}

template <typename Range, typename CodeUnits>
auto read_while_code_units(Range&& range, CodeUnits&& needle)
    -> detail::simple_borrowed_iterator_t<Range>
{
    auto subr = ranges_impl::subrange{range};
    while (!subr.empty()) {
        auto [beg, cp] = read_code_point_into(subr);
        if (!ranges_impl::equal(cp.view(), needle)) {
            return subr.begin();
        }
        subr = ranges_impl::subrange{beg, subr.end()};
    }
    SCN_ENSURE(subr.begin() == subr.end());
    return subr.begin();
}

template <typename Range>
auto read_until_code_point_eager(Range&& range,
                                 function_ref<bool(char32_t)> pred)
    -> detail::simple_borrowed_iterator_t<Range>
{
    static_assert(ranges_impl::contiguous_range<Range> &&
                  ranges_impl::sized_range<Range>);

    std::array<char32_t, 16> cp_buf{};
    std::array<uint8_t, 16> idx_buf{};
    auto it = ranges_impl::begin(range);
    while (it != ranges_impl::end(range)) {
        auto chunk_begin = it;
        size_t code_point_count = 0;
        uint8_t code_unit_idx = 0;
        while (code_point_count < cp_buf.size() && it != ranges_impl::end(range)) {
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
                    read_code_point_into(ranges_impl::subrange{it, end});
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
auto read_until_code_point(Range&& range, function_ref<bool(char32_t)> pred)
    -> detail::simple_borrowed_iterator_t<Range>
{
    if constexpr (ranges_impl::contiguous_range<Range> &&
                  ranges_impl::sized_range<Range>) {
        return read_until_code_point_eager(SCN_FWD(range), pred);
    }
    else {
        auto it = ranges_impl::begin(range);
        auto seg = get_contiguous_beginning(range);

        if (auto seg_it = read_until_code_point_eager(seg, pred);
            seg_it != seg.end()) {
            return ranges_polyfill::batch_next(
                it, ranges_impl::distance(seg.begin(), seg_it));
        }

        while (it != ranges_impl::end(range)) {
            const auto [iter, value] =
                read_code_point_into(ranges_impl::subrange{it, ranges_impl::end(range)});
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
auto read_while_code_point(Range&& range, function_ref<bool(char32_t)> pred)
    -> detail::simple_borrowed_iterator_t<Range>
{
    return read_until_code_point(SCN_FWD(range), std::not_fn(pred));
}

template <typename Range>
auto read_until_classic_space(Range&& range)
    -> detail::simple_borrowed_iterator_t<Range>
{
    if constexpr (ranges_impl::contiguous_range<Range> &&
                  ranges_impl::sized_range<Range> &&
                  std::is_same_v<detail::char_t<Range>, char>) {
        auto buf = make_contiguous_buffer(SCN_FWD(range));
        auto it = find_classic_space_narrow_fast(buf.view());
        return ranges_impl::next(ranges_impl::begin(range),
                            ranges_impl::distance(buf.view().begin(), it));
    }
    else {
        auto it = ranges_impl::begin(range);

        if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
            auto seg = get_contiguous_beginning(range);
            if (auto seg_it = find_classic_space_narrow_fast(seg);
                seg_it != seg.end()) {
                return ranges_polyfill::batch_next(
                    it, ranges_impl::distance(seg.begin(), seg_it));
            }
            ranges_polyfill::batch_next(it, seg.size());
        }

        return read_until_code_point(
            ranges_impl::subrange{it, ranges_impl::end(range)},
            [](char32_t cp) noexcept { return is_cp_space(cp); });
    }
}

template <typename Range>
auto read_while_classic_space(Range&& range)
    -> detail::simple_borrowed_iterator_t<Range>
{
    if constexpr (ranges_impl::contiguous_range<Range> &&
                  ranges_impl::sized_range<Range> &&
                  std::is_same_v<detail::char_t<Range>, char>) {
        auto buf = make_contiguous_buffer(SCN_FWD(range));
        auto it = find_classic_nonspace_narrow_fast(buf.view());
        return ranges_impl::next(ranges_impl::begin(range),
                            ranges_impl::distance(buf.view().begin(), it));
    }
    else {
        auto it = ranges_impl::begin(range);

        if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
            auto seg = get_contiguous_beginning(range);
            if (auto seg_it = find_classic_nonspace_narrow_fast(seg);
                seg_it != seg.end()) {
                return ranges_polyfill::batch_next(
                    it, ranges_impl::distance(seg.begin(), seg_it));
            }
            ranges_polyfill::batch_next(it, seg.size());
        }

        return read_while_code_point(SCN_FWD(range), [](char32_t cp) noexcept {
            return is_cp_space(cp);
        });
    }
}

template <typename Range>
auto read_matching_code_unit(Range&& range, detail::char_t<Range> ch)
    -> parse_expected<detail::simple_borrowed_iterator_t<Range>>
{
    auto it = read_code_unit(range);
    if (SCN_UNLIKELY(!it)) {
        return unexpected(make_eof_parse_error(it.error()));
    }

    if (SCN_UNLIKELY(*ranges_impl::begin(range) !=
                     static_cast<detail::char_t<Range>>(ch))) {
        return unexpected(parse_error::error);
    }

    return *it;
}

template <typename Range>
auto read_matching_code_point(Range&& range, char32_t cp)
    -> parse_expected<detail::simple_borrowed_iterator_t<Range>>
{
    auto [it, value] = read_code_point_into(range);
    auto decoded_cp = decode_code_point_exhaustive(value.view());
    if (SCN_UNLIKELY(cp != decoded_cp)) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range>
auto read_matching_string(Range&& range,
                          std::basic_string_view<detail::char_t<Range>> str)
    -> parse_expected<detail::simple_borrowed_iterator_t<Range>>
{
    SCN_TRY(it, read_exactly_n_code_units(range, ranges_impl::ssize(str))
                    .transform_error(make_eof_parse_error));

    auto sv =
        make_contiguous_buffer(ranges_impl::subrange{ranges_impl::begin(range), it});
    if (SCN_UNLIKELY(sv.view() != str)) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range>
auto read_matching_string_classic(Range&& range, std::string_view str)
    -> parse_expected<detail::simple_borrowed_iterator_t<Range>>
{
    SCN_TRY(it, read_exactly_n_code_units(range, ranges_impl::ssize(str))
                    .transform_error(make_eof_parse_error));

    if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
        auto sv =
            make_contiguous_buffer(ranges_impl::subrange{ranges_impl::begin(range), it});
        if (SCN_UNLIKELY(sv.view() != str)) {
            return unexpected(parse_error::error);
        }
        return it;
    }
    else {
        auto range_it = ranges_impl::begin(range);
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
auto read_matching_string_classic_nocase(Range&& range, std::string_view str)
    -> parse_expected<detail::simple_borrowed_iterator_t<Range>>
{
    using char_type = detail::char_t<Range>;

    if constexpr (ranges_impl::contiguous_range<Range> &&
                  std::is_same_v<char_type, char>) {
        if (ranges_impl::size(range) < str.size()) {
            return unexpected(make_eof_parse_error(eof_error::eof));
        }
        if (!fast_streq_nocase(ranges_impl::data(range), str.data(), str.size())) {
            return unexpected(parse_error::error);
        }
        return ranges_impl::next(ranges_impl::begin(range), str.size());
    }
    else {
        auto ascii_tolower = [](char_type ch) -> char_type {
            if (ch < 'A' || ch > 'Z') {
                return ch;
            }
            return static_cast<char_type>(ch +
                                          static_cast<char_type>('a' - 'A'));
        };

        SCN_TRY(it, read_exactly_n_code_units(range, ranges_impl::ssize(str))
                        .transform_error(make_eof_parse_error));

        if (SCN_UNLIKELY(!std::equal(
                ranges_impl::begin(range), it, str.begin(), [&](auto a, auto b) {
                    return ascii_tolower(a) ==
                           static_cast<detail::char_t<Range>>(b);
                }))) {
            return unexpected(parse_error::error);
        }

        return it;
    }
}

template <typename Range>
auto read_one_of_code_unit(Range&& range, std::string_view str)
    -> parse_expected<detail::simple_borrowed_iterator_t<Range>>
{
    auto it = read_code_unit(range);
    if (SCN_UNLIKELY(!it)) {
        return unexpected(make_eof_parse_error(it.error()));
    }

    for (auto ch : str) {
        if (*ranges_impl::begin(range) == static_cast<detail::char_t<Range>>(ch)) {
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
                        detail::simple_borrowed_iterator_t<Range>>
{
    if (!result) {
        return ranges_impl::begin(range);
    }
    return *result;
}
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
