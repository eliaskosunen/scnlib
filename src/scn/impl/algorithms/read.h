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
#include <scn/util/span.h>

#include <algorithm>
#include <utility>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {

        template <typename Range>
        auto read_code_point_into(Range&& range) -> iterator_value_result<
            simple_borrowed_iterator_t<Range>,
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

                auto cp_view = make_contiguous_buffer(
                    ranges::subrange{ranges::begin(range), it});
                return {it, cp_view};
            }

            if (len == 1) {
                ++it;
                auto cp_view = make_contiguous_buffer(
                    ranges::subrange{ranges::begin(range), it});
                return {it, cp_view};
            }

            ranges::advance(it,
                            static_cast<ranges::range_difference_t<Range>>(len),
                            ranges::end(range));
            auto cp_view = make_contiguous_buffer(
                ranges::subrange{ranges::begin(range), it});
            return {it, cp_view};
        }

        template <typename Range>
        simple_borrowed_iterator_t<Range> read_code_point(Range&& range)
        {
            return read_code_point_into(SCN_FWD(range)).iterator;
        }

        template <typename Range>
        scan_expected<simple_borrowed_iterator_t<Range>>
        read_exactly_n_code_points(Range&& range,
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
                auto [iter, val] = read_code_point_into(
                    ranges::subrange{it, ranges::end(range)});

                acc_width += calculate_text_width(val.view());
                if (acc_width > count) {
                    break;
                }

                it = iter;
            }

            return it;
        }

        template <typename Range, typename Predicate>
        simple_borrowed_iterator_t<Range> read_until_code_unit(Range&& range,
                                                               Predicate pred)
        {
            if constexpr (ranges::contiguous_range<Range>) {
                return ranges::find_if(range, pred);
            }
            else {
                auto it = ranges::begin(range);
                auto buf = buffered_range_segment{range, it};
                if (buf.potential_size() == 0) {
                    return ranges::find_if(range, pred);
                }

                buf.acquire();
                if (auto buf_it = ranges::find_if(buf, pred);
                    buf_it != buf.end()) {
                    buf.set_amount_read(ranges::distance(buf.begin(), buf_it));
                    buf.advance_iterator();
                    return it;
                }
                else {
                    buf.set_amount_read(buf.size());
                    buf.advance_iterator();
                }

                return ranges::find_if(ranges::subrange{it, range.end()}, pred);
            }
        }

        template <typename Range, typename Predicate>
        simple_borrowed_iterator_t<Range> read_while_code_unit(Range&& range,
                                                               Predicate pred)
        {
            return read_until_code_unit(SCN_FWD(range), std::not_fn(pred));
        }

        template <typename Range, typename Predicate>
        scan_expected<simple_borrowed_iterator_t<Range>> read_until1_code_unit(
            Range&& range,
            Predicate pred)
        {
            auto it = read_until_code_unit(range, pred);
            if (it == ranges::begin(range)) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "read_until1_code_unit: No matching code units");
            }
            return it;
        }

        template <typename Range, typename Predicate>
        scan_expected<simple_borrowed_iterator_t<Range>> read_while1_code_unit(
            Range&& range,
            Predicate pred)
        {
            auto it = read_while_code_unit(range, pred);
            if (it == ranges::begin(range)) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "read_while1_code_unit: No matching code units");
            }
            return it;
        }

        template <typename Range, typename CodeUnits>
        simple_borrowed_iterator_t<Range> read_until_code_units(
            Range&& range,
            CodeUnits&& needle)
        {
            return ranges::search(SCN_FWD(range), SCN_FWD(needle)).begin();
        }

        template <typename Range, typename Predicate>
        simple_borrowed_iterator_t<Range> read_until_code_point(Range&& range,
                                                                Predicate pred)
        {
            auto it = ranges::begin(range);

            while (it != ranges::end(range)) {
                const auto [iter, value] = read_code_point_into(
                    ranges::subrange{it, ranges::end(range)});
                const auto cp = decode_code_point_exhaustive(value.view());
                if (pred(cp)) {
                    break;
                }
                it = iter;
            }

            return it;
        }

        template <typename Range, typename Predicate>
        simple_borrowed_iterator_t<Range> read_while_code_point(Range&& range,
                                                                Predicate pred)
        {
            return read_until_code_point(
                SCN_FWD(range), [&](char32_t cp) { return !pred(cp); });
        }

        template <typename Range>
        simple_borrowed_iterator_t<Range> read_until_classic_space(
            Range&& range)
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
                return read_until_code_point(
                    SCN_FWD(range),
                    [](char32_t cp) SCN_NOEXCEPT { return is_cp_space(cp); });
            }
        }

        template <typename Range>
        simple_borrowed_iterator_t<Range> read_while_classic_space(
            Range&& range)
        {
            if constexpr (ranges::contiguous_range<Range> &&
                          ranges::sized_range<Range> &&
                          std::is_same_v<detail::char_t<Range>, char>) {
                return find_classic_nonspace_narrow_fast(
                    make_contiguous_buffer(SCN_FWD(range)).view());
            }
            else {
                return read_while_code_point(
                    SCN_FWD(range),
                    [](char32_t cp) SCN_NOEXCEPT { return is_cp_space(cp); });
            }
        }

        template <typename Range>
        scan_expected<simple_borrowed_iterator_t<Range>>
        read_matching_code_unit(Range&& range, detail::char_t<Range> ch)
        {
            auto it = read_code_unit(range);
            if (SCN_UNLIKELY(!it)) {
                return unexpected(it.error());
            }

            if (SCN_UNLIKELY(*ranges::begin(range) !=
                             static_cast<detail::char_t<Range>>(ch))) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "read_matching_code_unit: No match");
            }

            return it;
        }

        template <typename Range>
        scan_expected<simple_borrowed_iterator_t<Range>>
        read_matching_code_point(Range&& range, char32_t cp)
        {
            auto [it, value] = read_code_point_into(range);
            auto decoded_cp = decode_code_point_exhaustive(value.view());
            if (SCN_UNLIKELY(cp != decoded_cp)) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "read_matching_code_point: No match");
            }
            return it;
        }

        template <typename Range>
        scan_expected<simple_borrowed_iterator_t<Range>> read_matching_string(
            Range&& range,
            std::basic_string_view<detail::char_t<Range>> str)
        {
            SCN_TRY(it, read_exactly_n_code_units(range, ranges::ssize(str)));

            auto sv = make_contiguous_buffer(
                ranges::subrange{ranges::begin(range), it});
            if (SCN_UNLIKELY(sv.view() != str)) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "read_matching_string: No match");
            }
            return it;
        }

        template <typename Range>
        scan_expected<simple_borrowed_iterator_t<Range>>
        read_matching_string_classic(Range&& range, std::string_view str)
        {
            SCN_TRY(it, read_exactly_n_code_units(range, ranges::ssize(str)));

            if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
                auto sv = make_contiguous_buffer(
                    ranges::subrange{ranges::begin(range), it});
                if (SCN_UNLIKELY(sv.view() != str)) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "read_matching_string: No match");
                }
                return it;
            }
            else {
                auto range_it = ranges::begin(range);
                for (size_t i = 0; i < str.size(); ++i, (void)++range_it) {
                    if (SCN_UNLIKELY(
                            *range_it !=
                            static_cast<detail::char_t<Range>>(str[i]))) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "read_matching_string: No match");
                    }
                }
                return it;
            }
        }

        template <typename Range>
        scan_expected<simple_borrowed_iterator_t<Range>>
        read_matching_string_classic_nocase(Range&& range, std::string_view str)
        {
            using char_type = detail::char_t<Range>;

            auto ascii_tolower = [](char_type ch) -> char_type {
                if (!is_ascii_char(ch)) {
                    return ch;
                }
                if (ch < 'A' || ch > 'Z') {
                    return ch;
                }
                return static_cast<char_type>(
                    ch + static_cast<char_type>('a' - 'A'));
            };

            SCN_TRY(it, read_exactly_n_code_units(range, ranges::ssize(str)));

            auto buf = make_contiguous_buffer(
                ranges::subrange{ranges::begin(range), it});

            if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
                if (buf.stores_allocated_string()) {
                    for (auto& ch : buf.get_allocated_string()) {
                        ch = ascii_tolower(ch);
                    }
                    if (SCN_UNLIKELY(buf.view() != str)) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "read_matching_string_nocase: No match");
                    }

                    return it;
                }
            }

            if (SCN_UNLIKELY(!std::equal(
                    buf.view().begin(), buf.view().end(), str.begin(),
                    [&](auto a, auto b) {
                        return ascii_tolower(a) ==
                               static_cast<detail::char_t<Range>>(b);
                    }))) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "read_matching_string_nocase: No match");
            }

            return it;
        }

        template <typename Range>
        scan_expected<simple_borrowed_iterator_t<Range>> read_one_of_code_unit(
            Range&& range,
            std::string_view str)
        {
            auto it = read_code_unit(range);
            if (SCN_UNLIKELY(!it)) {
                return unexpected(it.error());
            }

            for (auto ch : str) {
                if (*ranges::begin(range) ==
                    static_cast<detail::char_t<Range>>(ch)) {
                    return *it;
                }
            }

            return unexpected_scan_error(scan_error::invalid_scanned_value,
                                         "read_one_of_code_unit: No match");
        }

#if !SCN_DISABLE_LOCALE
        template <typename Range>
        simple_borrowed_iterator_t<Range> read_localized_mask_impl(
            Range&& range,
            detail::locale_ref loc,
            std::ctype_base::mask mask,
            bool read_until)
        {
            const auto& ctype_facet = get_facet<std::ctype<wchar_t>>(loc);

            if constexpr (std::is_same_v<detail::char_t<Range>, wchar_t>) {
                if constexpr (ranges::contiguous_range<Range> &&
                              ranges::sized_range<Range>) {
                    if (read_until) {
                        const auto ptr = ctype_facet.scan_is(
                            mask, ranges::data(range),
                            ranges::data(range) + ranges::size(range));
                        return ranges::next(
                            ranges::begin(range),
                            ranges::distance(ranges::data(range), ptr));
                    }
                    else {
                        const auto ptr = ctype_facet.scan_not(
                            mask, ranges::data(range),
                            ranges::data(range) + ranges::size(range));
                        return ranges::next(
                            ranges::begin(range),
                            ranges::distance(ranges::data(range), ptr));
                    }
                }
                else {
                    return read_until_code_unit(
                        SCN_FWD(range), [&](wchar_t ch) {
                            return ctype_facet.is(mask, ch) == read_until;
                        });
                }
            }
            else {
                return read_until_code_point(SCN_FWD(range), [&](char32_t cp) {
                    auto ch = *encode_code_point_as_wide_character(cp, false);
                    return ctype_facet.is(mask, ch) == read_until;
                });
            }
        }

        template <typename Range>
        simple_borrowed_iterator_t<Range> read_until_localized_mask(
            Range&& range,
            detail::locale_ref loc,
            std::ctype_base::mask mask)
        {
            return read_localized_mask_impl(SCN_FWD(range), loc, mask, true);
        }

        template <typename Range>
        simple_borrowed_iterator_t<Range> read_while_localized_mask(
            Range&& range,
            detail::locale_ref loc,
            std::ctype_base::mask mask)
        {
            return read_localized_mask_impl(SCN_FWD(range), loc, mask, false);
        }

        template <typename Range>
        simple_borrowed_iterator_t<Range> read_until_localized_space(
            Range&& range,
            detail::locale_ref loc)
        {
            return read_until_localized_mask(SCN_FWD(range), loc,
                                             std::ctype_base::space);
        }

        template <typename Range>
        simple_borrowed_iterator_t<Range> read_while_localized_space(
            Range&& range,
            detail::locale_ref loc)
        {
            return read_while_localized_mask(SCN_FWD(range), loc,
                                             std::ctype_base::space);
        }

        template <typename Range, typename Predicate>
        simple_borrowed_iterator_t<Range>
        read_until_localized_mask_or_code_point(Range&& range,
                                                detail::locale_ref loc,
                                                std::ctype_base::mask mask,
                                                Predicate&& pred)
        {
            const auto& ctype_facet = get_facet<std::ctype<wchar_t>>(loc);

            return read_until_code_point(SCN_FWD(range), [&](char32_t cp) {
                auto ch = *encode_code_point_as_wide_character(cp, false);
                return pred(cp) || ctype_facet.is(mask, ch);
            });
        }

        template <typename Range, typename Predicate>
        simple_borrowed_iterator_t<Range>
        read_while_localized_mask_or_code_point(Range&& range,
                                                detail::locale_ref loc,
                                                std::ctype_base::mask mask,
                                                Predicate&& pred)
        {
            const auto& ctype_facet = get_facet<std::ctype<wchar_t>>(loc);

            return read_while_code_point(SCN_FWD(range), [&](char32_t cp) {
                auto ch = *encode_code_point_as_wide_character(cp, false);
                return pred(cp) || ctype_facet.is(mask, ch);
            });
        }
#endif  // !SCN_DISABLE_LOCALE

        template <typename Range, typename Iterator>
        simple_borrowed_iterator_t<Range> apply_opt(
            scan_expected<Iterator>&& result,
            Range&& range)
        {
            if (!result) {
                return ranges::begin(range);
            }
            return *result;
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
