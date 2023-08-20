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

#include <scn/detail/args.h>
#include <scn/detail/erased_range.h>
#include <scn/detail/error.h>

#include <tuple>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename Range, bool Dangling = false>
        struct dangling_iterator {
            using type = ranges::iterator_t<Range>;
        };
        template <typename Range>
        struct dangling_iterator<Range, true> {
            using type = ranges::dangling;
        };

        template <typename Range, bool Dangling = false>
        struct dangling_sentinel {
            using type = ranges::sentinel_t<Range>;
        };
        template <typename Range>
        struct dangling_sentinel<Range, true> {
            using type = ranges::dangling;
        };
    }  // namespace detail

    /**
     * Type returned by `scan`, contains the unused input as a `subrange`, and
     * the scanned values in a `tuple`.
     */
    template <typename Range, typename... Args>
    class scan_result {
        static constexpr bool is_dangling =
            std::is_same_v<detail::remove_cvref_t<Range>, ranges::dangling>;
        static_assert(ranges::borrowed_range<Range> || is_dangling);

    public:
        using range_type = Range;
        using iterator = detail::dangling_iterator<Range, is_dangling>;
        using sentinel = detail::dangling_sentinel<Range, is_dangling>;
        using tuple_type = std::tuple<Args...>;

        constexpr scan_result() = default;

        constexpr scan_result(const scan_result&) = default;
        constexpr scan_result(scan_result&&) = default;
        constexpr scan_result& operator=(const scan_result&) = default;
        constexpr scan_result& operator=(scan_result&&) = default;
        ~scan_result() = default;

        /// Construct from a range and a tuple
        scan_result(range_type r, std::tuple<Args...>&& values)
            : m_range(SCN_MOVE(r)), m_values(SCN_MOVE(values))
        {
        }

        template <typename OtherR,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, OtherR>>>
        scan_result(OtherR&& r, std::tuple<Args...>&& values)
            : m_range(SCN_FWD(r)), m_values(SCN_MOVE(values))
        {
        }

        template <typename OtherR,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, OtherR>>>
        explicit scan_result(const scan_result<OtherR, Args...>& o)
            : m_range(o.m_range), m_values(o.m_values)
        {
        }

        template <typename OtherR,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, OtherR>>>
        explicit scan_result(scan_result<OtherR, Args...>&& o)
            : m_range(SCN_MOVE(o.m_range)), m_values(SCN_MOVE(o.m_values))
        {
        }

        template <typename OtherR,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, OtherR>>>
        scan_result& operator=(const scan_result<OtherR, Args...>& o)
        {
            m_range = o.m_range;
            m_values = o.m_values;
            return *this;
        }

        template <typename OtherR,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, OtherR>>>
        scan_result& operator=(scan_result<OtherR, Args...>&& o)
        {
            m_range = SCN_MOVE(o.m_range);
            m_values = SCN_MOVE(o.m_values);
            return *this;
        }

        /// Access the ununsed input range
        range_type range() const
        {
            return m_range;
        }

        /// Access the beginning of the unused input range
        /// If `range_type` is `dangling`, returns `dangling`.
        auto begin() const
        {
            if constexpr (is_dangling) {
                return ranges::dangling{};
            }
            else {
                return m_range.begin();
            }
        }
        /// Access the end of the unused input range
        /// If `range_type` is `dangling`, returns `dangling`.
        auto end() const
        {
            if constexpr (is_dangling) {
                return ranges::dangling{};
            }
            else {
                return m_range.end();
            }
        }

        /// Access the scanned values
        /// @{
        tuple_type& values() &
        {
            return m_values;
        }
        const tuple_type& values() const&
        {
            return m_values;
        }
        tuple_type&& values() &&
        {
            return SCN_MOVE(m_values);
        }
        const tuple_type&& values() const&&
        {
            return SCN_MOVE(m_values);
        }
        /// @}

        /// Access the single scanned value
        /// @{
        template <size_t N = sizeof...(Args),
                  typename = std::enable_if_t<N == 1>>
        decltype(auto) value() &
        {
            return std::get<0>(m_values);
        }
        template <size_t N = sizeof...(Args),
                  typename = std::enable_if_t<N == 1>>
        decltype(auto) value() const&
        {
            return std::get<0>(m_values);
        }
        template <size_t N = sizeof...(Args),
                  typename = std::enable_if_t<N == 1>>
        decltype(auto) value() &&
        {
            return SCN_MOVE(std::get<0>(m_values));
        }
        template <size_t N = sizeof...(Args),
                  typename = std::enable_if_t<N == 1>>
        decltype(auto) value() const&&
        {
            return SCN_MOVE(std::get<0>(m_values));
        }
        /// @}

    private:
        range_type m_range{};
        tuple_type m_values{};
    };

    template <typename R, typename... Args>
    scan_result(R, std::tuple<Args...>) -> scan_result<R, Args...>;

    namespace detail {
        template <typename SourceRange, typename ResultIterator>
        auto map_scan_result_iterator(SourceRange&& source,
                                      const ResultIterator& mapped_begin,
                                      const ResultIterator& result)
            -> ranges::borrowed_iterator_t<SourceRange>
        {
            if constexpr (is_erased_range_iterator<ResultIterator>::value) {
                return ranges::next(ranges::begin(source),
                                    result.distance_from_begin());
            }
            else {
                return ranges::next(ranges::begin(source),
                                    ranges::distance(mapped_begin, result));
            }
        }

        template <typename SourceRange, typename ResultIterator>
        auto map_scan_result_range(SourceRange&& source,
                                   const ResultIterator& mapped_begin,
                                   const ResultIterator& result)
            -> borrowed_ssubrange_t<SourceRange>
        {
            auto end = ranges::end(source);
            return {
                map_scan_result_iterator(SCN_FWD(source), mapped_begin, result),
                end};
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
