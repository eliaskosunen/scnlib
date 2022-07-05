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
#include <scn/impl/algorithms/common.h>
#include <scn/impl/algorithms/find_whitespace.h>
#include <scn/impl/unicode/unicode.h>
#include <scn/impl/util/ascii_ctype.h>
#include <scn/util/span.h>

#include <algorithm>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename T>
        constexpr bool range_supports_nocopy() SCN_NOEXCEPT
        {
            return ranges::contiguous_range<T>;
        }

        template <typename R>
        using read_nocopy_result = iterator_value_result<
            ranges::borrowed_iterator_t<R>,
            std::conditional_t<ranges::borrowed_range<R>,
                               std::basic_string_view<ranges::range_value_t<R>>,
                               ranges::dangling>>;

        template <typename Range>
        SCN_NODISCARD read_nocopy_result<Range> read_all_nocopy(Range&& range)
        {
            static_assert(range_supports_nocopy<Range>());

            const auto size = ranges::size(range);
            return {ranges::begin(range) + static_cast<std::ptrdiff_t>(size),
                    {ranges::data(range), static_cast<size_t>(size)}};
        }

        template <typename Range>
        SCN_NODISCARD read_nocopy_result<Range> read_n_nocopy(
            Range&& range,
            ranges::range_difference_t<Range> n)
        {
            static_assert(range_supports_nocopy<Range>());
            SCN_EXPECT(n >= 0);

            const auto size =
                (std::min)(n, static_cast<ranges::range_difference_t<Range>>(
                                  ranges::ssize(range)));

            return {ranges::begin(range) + size,
                    {ranges::data(range), static_cast<size_t>(size)}};
        }

        template <typename Range, typename Predicate>
        SCN_NODISCARD read_nocopy_result<Range> read_until_classic_nocopy(
            Range&& range,
            Predicate&& until)
        {
            static_assert(range_supports_nocopy<Range>());

            const auto found = ranges::find_if(range, until);
            auto str = std::basic_string_view<ranges::range_value_t<Range>>{
                ranges::data(range), static_cast<size_t>(ranges::distance(
                                         ranges::begin(range), found))};
            return {found, str};
        }

        template <typename Range>
        read_nocopy_result<Range> read_until_classic_space_nocopy(Range&& range)
        {
            if constexpr (range_supports_nocopy<Range>() &&
                          std::is_same_v<ranges::range_value_t<Range>, char>) {
                const auto sv =
                    std::string_view{ranges::data(range),
                                     static_cast<size_t>(ranges::size(range))};
                const auto it = find_classic_space_narrow_fast(sv);
                const auto n = ranges::distance(sv.begin(), it);
                return {ranges::next(ranges::begin(range), n),
                        std::string_view{ranges::data(range),
                                         static_cast<size_t>(n)}};
            }
            else {
                return read_until_classic_nocopy(
                    SCN_FWD(range),
                    [](ranges::range_value_t<Range> ch)
                        SCN_NOEXCEPT { return is_ascii_space(ch); });
            }
        }

        template <typename SourceRange, typename NeedleRange>
        SCN_NODISCARD read_nocopy_result<SourceRange>
        read_until_code_units_nocopy(SourceRange&& range,
                                     NeedleRange&& code_units)
        {
            SCN_EXPECT(!ranges::empty(range));
            SCN_EXPECT(!ranges::empty(code_units));

            auto result = ranges::search(range, SCN_FWD(code_units));
            const auto n =
                ranges::distance(ranges::begin(range), result.begin());
            return {result.begin(),
                    {ranges::data(range), static_cast<size_t>(n)}};
        }

        template <typename SourceRange, typename Pred>
        SCN_NODISCARD scan_expected<read_nocopy_result<SourceRange>>
        read_until_code_point_nocopy(SourceRange&& input, Pred&& until)
        {
            static_assert(range_supports_nocopy<SourceRange>());
            SCN_EXPECT(!ranges::empty(input));

            const auto make_result =
                [&](auto it) -> read_nocopy_result<SourceRange> {
                const auto n = static_cast<size_t>(
                    ranges::distance(ranges::begin(input), it));
                return {it, {ranges::data(input), n}};
            };

            auto it = ranges::begin(input);
            while (it != ranges::end(input)) {
                auto len = code_point_length(*it);
                if (!len) {
                    return unexpected(len.error());
                }

                if (*len == 1) {
                    if (until(static_cast<code_point>(*it))) {
                        return {make_result(it)};
                    }
                    ++it;
                    continue;
                }

                const auto string_view_of_rest = [&]() {
                    auto begin = detail::to_address_safe(
                        it, ranges::data(input),
                        ranges::data(input) + ranges::size(input));
                    auto n = static_cast<size_t>(
                        (ranges::data(input) + ranges::size(input)) - begin);
                    return std::basic_string_view<
                        ranges::range_value_t<SourceRange>>{begin, n};
                };

                code_point cp{};
                auto decode_sv = string_view_of_rest();
                auto decode_result = decode_code_point(decode_sv, cp);
                if (!decode_result) {
                    return unexpected(decode_result.error());
                }

                if (until(cp)) {
                    return make_result(it);
                }
                it += *len;
            }
            return make_result(it);
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
