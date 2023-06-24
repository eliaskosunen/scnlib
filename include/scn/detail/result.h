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

    template <typename Iterator, typename... Args>
    class scan_result {
    public:
        using iterator = Iterator;
        using tuple_type = std::tuple<Args...>;

        scan_result() = default;

        scan_result(iterator it, std::tuple<Args...>&& values)
            : m_begin(SCN_MOVE(it)), m_values(SCN_MOVE(values))
        {
        }

        template <typename OtherIt,
                  typename = std::enable_if_t<
                      std::is_constructible_v<iterator, OtherIt>>>
        scan_result(OtherIt&& it, std::tuple<Args...>&& values)
            : m_begin(SCN_FWD(it)), m_values(SCN_MOVE(values))
        {
        }

        template <typename OtherIt,
                  typename = std::enable_if_t<
                      std::is_constructible_v<iterator, OtherIt>>>
        explicit scan_result(const scan_result<OtherIt, Args...>& o)
            : m_begin(o.m_begin), m_values(o.m_values)
        {
        }

        template <typename OtherIt,
                  typename = std::enable_if_t<
                      std::is_constructible_v<iterator, OtherIt>>>
        explicit scan_result(scan_result<OtherIt, Args...>&& o)
            : m_begin(SCN_MOVE(o.m_begin)), m_values(SCN_MOVE(o.m_values))
        {
        }

        template <typename OtherIt,
                  typename = std::enable_if_t<
                      std::is_constructible_v<iterator, OtherIt>>>
        scan_result& operator=(const scan_result<OtherIt, Args...>& o)
        {
            m_begin = o.m_begin;
            m_values = o.m_values;
            return *this;
        }

        template <typename OtherIt,
                  typename = std::enable_if_t<
                      std::is_constructible_v<iterator, OtherIt>>>
        scan_result& operator=(scan_result<OtherIt, Args...>&& o)
        {
            m_begin = SCN_MOVE(o.m_begin);
            m_values = SCN_MOVE(o.m_values);
            return *this;
        }

        iterator begin() const
        {
            return m_begin;
        }

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

    private:
        iterator m_begin{};
        tuple_type m_values{};
    };

    template <typename It, typename... Args>
    scan_result(It, std::tuple<Args...>) -> scan_result<It, Args...>;

    namespace detail {
        template <typename SourceRange, typename ResultIterator>
        SCN_MAYBE_UNUSED constexpr bool is_map_scan_result_iterator_noexcept()
        {
            if constexpr (!ranges::borrowed_range<SourceRange>) {
                return true;
            }
            else if constexpr (std::is_constructible_v<
                                   ranges::iterator_t<SourceRange>,
                                   const ResultIterator&>) {
                return std::is_nothrow_constructible_v<
                    ranges::iterator_t<SourceRange>, const ResultIterator&>;
            }
            else {
                return false;
            }
        }

        template <typename SourceRange, typename ResultIterator>
        auto map_scan_result_iterator(SourceRange&& source,
                                      const ResultIterator& mapped_begin,
                                      const ResultIterator& result)
            SCN_NOEXCEPT_P(
                is_map_scan_result_iterator_noexcept<SourceRange,
                                                     const ResultIterator&>())
        {
            if constexpr (!ranges::borrowed_range<SourceRange>) {
                return ranges::dangling{};
            }
            else if constexpr (std::is_constructible_v<
                                   ranges::iterator_t<SourceRange>,
                                   const ResultIterator&>) {
                return result;
            }
            else if constexpr (is_erased_range_iterator<
                                   ResultIterator>::value) {
                return ranges::next(ranges::begin(source),
                                    result.distance_from_begin());
            }
            else {
                return ranges::next(ranges::begin(source),
                                    ranges::distance(mapped_begin, result));
            }
        }

#if 0
        // Make a user-friendly range value from the return value of vscan

        template <typename SourceRange, typename ResultRange>
        auto map_scan_result_range(SourceRange&& source,
                                   const ResultRange& result)
        {
            if constexpr (!ranges::borrowed_range<SourceRange>) {
                return ranges::dangling{};
            }
            else if constexpr (is_erased_range_or_subrange<
                                   ResultRange>::value &&
                               !is_erased_range_or_subrange<
                                   remove_cvref_t<SourceRange>>::value) {
                using cref_source_range = const remove_cvref_t<SourceRange>&;
                using iterator = ranges::iterator_t<cref_source_range>;
                using sentinel = ranges::sentinel_t<cref_source_range>;
                constexpr ranges::subrange_kind kind =
                    ranges::sized_range<cref_source_range>
                        ? ranges::subrange_kind::sized
                        : ranges::subrange_kind::unsized;

                return ranges::subrange<iterator, sentinel, kind>{
                    ranges::next(ranges::begin(source),
                                 result.begin().distance_from_begin()),
                    ranges::end(source)};
            }
            else {
                return result;
            }
        }
#endif
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
