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
#include <scn/impl/util/ascii_ctype.h>
#include <scn/util/span.h>

#include <algorithm>
#include <utility>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {

        template <typename Range>
        scan_expected<iterator_value_result<
            ranges::borrowed_iterator_t<Range>,
            contiguous_range_factory<detail::char_t<Range>>>>
        read_code_point_into(Range&& range)
        {
            using rettype = iterator_value_result<
                ranges::borrowed_iterator_t<Range>,
                contiguous_range_factory<detail::char_t<Range>>>;

            if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
                return unexpected(e);
            }

            auto it = ranges::begin(range);
            const auto len = code_point_length_by_starting_code_unit(*it);
            if (SCN_UNLIKELY(!len)) {
                return unexpected(len.error());
            }

            if (*len == 1) {
                ++it;
                auto cp_view = make_contiguous_buffer(
                    ranges::subrange{ranges::begin(range), it});
                return rettype{it, cp_view};
            }

            if constexpr (ranges::sized_range<Range>) {
                auto sz = ranges_polyfill::usize(range);
                if (SCN_UNLIKELY(sz < *len)) {
                    return unexpected_scan_error(scan_error::invalid_encoding,
                                                 "Incomplete code point");
                }
                ranges::advance(
                    it, static_cast<ranges::range_difference_t<Range>>(*len));
            }
            else {
                ++it;
                size_t i = 1;
                for (; i < *len && it != ranges::end(range); ++i, (void)++it) {}
                if (SCN_UNLIKELY(i != *len)) {
                    return unexpected_scan_error(scan_error::invalid_encoding,
                                                 "Incomplete code point");
                }
            }

            auto cp_view = make_contiguous_buffer(
                ranges::subrange{ranges::begin(range), it});
            if (SCN_UNLIKELY(!validate_unicode(cp_view.view()))) {
                return unexpected_scan_error(scan_error::invalid_encoding,
                                             "Invalid code point");
            }

            return rettype{it, cp_view};
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>> read_code_point(
            Range&& range)
        {
            return read_code_point_into(SCN_FWD(range))
                .transform([](auto&& result) { return result.iterator; });
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
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

                auto result = read_code_point(rng);
                if (SCN_UNLIKELY(!result)) {
                    return unexpected(result.error());
                }

                it = *result;
            }

            return it;
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_exactly_n_width_units(Range&& range,
                                   ranges::range_difference_t<Range> count)
        {
            auto it = ranges::begin(range);
            ranges::range_difference_t<Range> acc_width = 0;

            while (it != ranges::end(range)) {
                auto read_result = read_code_point_into(
                    ranges::subrange{it, ranges::end(range)});
                if (SCN_UNLIKELY(!read_result)) {
                    return unexpected(read_result.error());
                }

                acc_width +=
                    calculate_valid_text_width(read_result->value.view());
                if (acc_width > count) {
                    break;
                }

                it = read_result->iterator;
            }

            return it;
        }

        template <typename Range, typename Predicate>
        scan_expected<ranges::borrowed_iterator_t<Range>> read_until_code_unit(
            Range&& range,
            Predicate pred)
        {
            return ranges::find_if(range, pred);
        }

        template <typename Range, typename Predicate>
        scan_expected<ranges::borrowed_iterator_t<Range>> read_while_code_unit(
            Range&& range,
            Predicate pred)
        {
            return ranges::find_if_not(range, pred);
        }

        template <typename Range, typename Predicate>
        scan_expected<ranges::borrowed_iterator_t<Range>> read_until1_code_unit(
            Range&& range,
            Predicate pred)
        {
            return read_until_code_unit(range, pred)
                .and_then([&](auto it) -> scan_expected<
                                           ranges::borrowed_iterator_t<Range>> {
                    if (it == ranges::begin(range)) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "read_until1_code_unit: No matching code units");
                    }
                    return it;
                });
        }

        template <typename Range, typename Predicate>
        scan_expected<ranges::borrowed_iterator_t<Range>> read_while1_code_unit(
            Range&& range,
            Predicate pred)
        {
            return read_while_code_unit(range, pred)
                .and_then([&](auto it) -> scan_expected<
                                           ranges::borrowed_iterator_t<Range>> {
                    if (it == ranges::begin(range)) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "read_while1_code_unit: No matching code units");
                    }
                    return it;
                });
        }

        template <typename Range, typename CodeUnits>
        scan_expected<ranges::borrowed_iterator_t<Range>> read_until_code_units(
            Range&& range,
            CodeUnits&& needle)
        {
            return ranges::search(SCN_FWD(range), SCN_FWD(needle)).begin();
        }

        template <typename Range, typename Predicate>
        scan_expected<ranges::borrowed_iterator_t<Range>> read_until_code_point(
            Range&& range,
            Predicate pred)
        {
            auto it = ranges::begin(range);

            while (it != ranges::end(range)) {
                const auto result = read_code_point_into(
                    ranges::subrange{it, ranges::end(range)});
                if (SCN_UNLIKELY(!result)) {
                    return unexpected(result.error());
                }

                const auto cp =
                    decode_code_point_exhaustive_valid(result->value.view());
                if (pred(cp)) {
                    break;
                }

                it = result->iterator;
            }

            return it;
        }

        template <typename Range, typename Predicate>
        scan_expected<ranges::borrowed_iterator_t<Range>> read_while_code_point(
            Range&& range,
            Predicate pred)
        {
            return read_until_code_point(
                SCN_FWD(range), [&](code_point cp) { return !pred(cp); });
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_until_classic_space(Range&& range)
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
                return read_until_code_unit(
                    SCN_FWD(range), [](auto ch) { return is_ascii_space(ch); });
            }
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_while_classic_space(Range&& range)
        {
            if constexpr (ranges::contiguous_range<Range> &&
                          ranges::sized_range<Range> &&
                          std::is_same_v<detail::char_t<Range>, char>) {
                return find_classic_nonspace_narrow_fast(
                    make_contiguous_buffer(SCN_FWD(range)).view());
            }
            else {
                return read_while_code_unit(
                    SCN_FWD(range), [](auto ch) { return is_ascii_space(ch); });
            }
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_matching_code_unit(Range&& range, detail::char_t<Range> ch)
        {
            return read_code_unit(range).and_then(
                [&](auto it)
                    -> scan_expected<ranges::borrowed_iterator_t<Range>> {
                    if (SCN_UNLIKELY(*ranges::begin(range) !=
                                     static_cast<detail::char_t<Range>>(ch))) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "read_matching_code_unit: No match");
                    }

                    return it;
                });
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_matching_code_point(Range&& range, code_point cp)
        {
            return read_code_point_into(range).and_then(
                [&](auto result)
                    -> scan_expected<ranges::borrowed_iterator_t<Range>> {
                    auto decoded_cp =
                        decode_code_point_exhaustive_valid(result.value.view());

                    if (SCN_UNLIKELY(decoded_cp != cp)) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "read_matching_code_point: No match");
                    }

                    return result.iterator;
                });
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>> read_matching_string(
            Range&& range,
            std::basic_string_view<detail::char_t<Range>> str)
        {
            return read_exactly_n_code_units(range, ranges::ssize(str))
                .and_then(
                    [&](auto it)
                        -> scan_expected<ranges::borrowed_iterator_t<Range>> {
                        auto sv = make_contiguous_buffer(
                            ranges::subrange{ranges::begin(range), it});
                        if (SCN_UNLIKELY(sv.view() != str)) {
                            return unexpected_scan_error(
                                scan_error::invalid_scanned_value,
                                "read_matching_string: No match");
                        }
                        return it;
                    });
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_matching_string_classic(Range&& range, std::string_view str)
        {
            return read_exactly_n_code_units(range, ranges::ssize(str))
                .and_then([&](auto it) -> scan_expected<
                                           ranges::borrowed_iterator_t<Range>> {
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
                        for (size_t i = 0; i < str.size();
                             ++i, (void)++range_it) {
                            if (SCN_UNLIKELY(*range_it !=
                                             static_cast<detail::char_t<Range>>(
                                                 str[i]))) {
                                return unexpected_scan_error(
                                    scan_error::invalid_scanned_value,
                                    "read_matching_string: No match");
                            }
                        }
                        return it;
                    }
                });
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_matching_string_classic_nocase(Range&& range, std::string_view str)
        {
            auto ascii_tolower =
                [](detail::char_t<Range> ch) -> detail::char_t<Range> {
                if (!is_ascii_char(ch)) {
                    return ch;
                }
                if (ch < 'A' || ch > 'Z') {
                    return ch;
                }
                return ch + ('a' - 'A');
            };

            using return_type =
                scan_expected<ranges::borrowed_iterator_t<Range>>;

            auto check_narrow_allocated_string = [&](auto it,
                                                     auto& buf) -> return_type {
                SCN_EXPECT(buf.stores_allocated_string());

                // buf is a string, make buf lowercase
                for (auto& ch : buf.get_allocated_string()) {
                    ch = ascii_tolower(ch);
                }
                if (SCN_UNLIKELY(buf.view() != str)) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "read_matching_string_nocase: No match");
                }

                return it;
            };
            auto check_other = [&](auto it, auto& buf) -> return_type {
                // buf is a string_view, compare char-by-char
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
            };

            return read_exactly_n_code_units(range, ranges::ssize(str))
                .and_then([&](auto it) {
                    auto buf = make_contiguous_buffer(
                        ranges::subrange{ranges::begin(range), it});

                    if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
                        if (buf.stores_allocated_string()) {
                            return check_narrow_allocated_string(it, buf);
                        }
                    }

                    return check_other(it, buf);
                });
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>> read_one_of_code_unit(
            Range&& range,
            std::string_view str)
        {
            return read_code_unit(range).and_then(
                [&](auto it)
                    -> scan_expected<ranges::borrowed_iterator_t<Range>> {
                    for (auto ch : str) {
                        if (*ranges::begin(range) ==
                            static_cast<detail::char_t<Range>>(ch)) {
                            return it;
                        }
                    }

                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "read_one_of_code_unit: No match");
                });
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_localized_mask_impl(Range&& range,
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
                return read_until_code_point(
                    SCN_FWD(range), [&](code_point cp) {
                        auto ch =
                            *encode_code_point_as_wide_character(cp, false);
                        return ctype_facet.is(mask, ch) == read_until;
                    });
            }
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_until_localized_mask(Range&& range,
                                  detail::locale_ref loc,
                                  std::ctype_base::mask mask)
        {
            return read_localized_mask_impl(SCN_FWD(range), loc, mask, true);
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_while_localized_mask(Range&& range,
                                  detail::locale_ref loc,
                                  std::ctype_base::mask mask)
        {
            return read_localized_mask_impl(SCN_FWD(range), loc, mask, false);
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_until_localized_space(Range&& range, detail::locale_ref loc)
        {
            return read_until_localized_mask(SCN_FWD(range), loc,
                                             std::ctype_base::space);
        }

        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_while_localized_space(Range&& range, detail::locale_ref loc)
        {
            return read_while_localized_mask(SCN_FWD(range), loc,
                                             std::ctype_base::space);
        }

        template <typename Range, typename Predicate>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_until_localized_mask_or_code_point(Range&& range,
                                                detail::locale_ref loc,
                                                std::ctype_base::mask mask,
                                                Predicate&& pred)
        {
            const auto& ctype_facet = get_facet<std::ctype<wchar_t>>(loc);

            return read_until_code_point(SCN_FWD(range), [&](code_point cp) {
                auto ch = *encode_code_point_as_wide_character(cp, false);
                return pred(cp) || ctype_facet.is(mask, ch);
            });
        }

        template <typename Range, typename Predicate>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_while_localized_mask_or_code_point(Range&& range,
                                                detail::locale_ref loc,
                                                std::ctype_base::mask mask,
                                                Predicate&& pred)
        {
            const auto& ctype_facet = get_facet<std::ctype<wchar_t>>(loc);

            return read_while_code_point(SCN_FWD(range), [&](code_point cp) {
                auto ch = *encode_code_point_as_wide_character(cp, false);
                return pred(cp) || ctype_facet.is(mask, ch);
            });
        }
        template <typename Range, typename Iterator>
        ranges::borrowed_iterator_t<Range> apply_opt(
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
