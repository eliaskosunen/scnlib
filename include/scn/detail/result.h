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
        template <typename Range, bool Dangling, bool StdinMarker>
        constexpr auto dangling_iterator()
        {
            if constexpr (StdinMarker) {
                return type_identity<stdin_range_marker>{};
            }
            else if constexpr (Dangling) {
                return type_identity<ranges::dangling>{};
            }
            else {
                return type_identity<ranges::iterator_t<Range>>{};
            }
        }

        template <typename Range, bool Dangling, bool StdinMarker>
        constexpr auto dangling_sentinel()
        {
            if constexpr (StdinMarker) {
                return type_identity<stdin_range_marker>{};
            }
            else if constexpr (Dangling) {
                return type_identity<ranges::dangling>{};
            }
            else {
                return type_identity<ranges::sentinel_t<Range>>{};
            }
        }
    }  // namespace detail

    /**
     * \defgroup result Result types
     *
     * \brief Result and error types
     *
     * Instead of using exceptions, `scn::scan` and others return an object of
     * type `scn::scan_result`, wrapped inside a `scn::scan_expected`.
     */

    /**
     * Type returned by `scan`, contains the unused input as a `subrange`, and
     * the scanned values in a `tuple`.
     *
     * \ingroup result
     */
    template <typename Range, typename... Args>
    class scan_result {
        static constexpr bool is_dangling =
            std::is_same_v<detail::remove_cvref_t<Range>, ranges::dangling>;
        static constexpr bool is_stdin_marker =
            std::is_same_v<detail::remove_cvref_t<Range>, stdin_range_marker>;
        static_assert(ranges::borrowed_range<Range> || is_dangling ||
                      is_stdin_marker);

    public:
        using range_type = Range;
        using iterator = typename decltype(detail::dangling_iterator<
                                           Range,
                                           is_dangling,
                                           is_stdin_marker>())::type;
        using sentinel = typename decltype(detail::dangling_sentinel<
                                           Range,
                                           is_dangling,
                                           is_stdin_marker>())::type;
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

        /// Converting constructor from a range and a tuple
        template <typename OtherR,
                  std::enable_if_t<
                      std::is_constructible_v<range_type, OtherR>>* = nullptr>
        scan_result(OtherR&& r, std::tuple<Args...>&& values)
            : m_range(SCN_FWD(r)), m_values(SCN_MOVE(values))
        {
        }

        template <
            typename OtherR,
            std::enable_if_t<
                std::is_constructible_v<range_type, OtherR> &&
                std::is_convertible_v<const OtherR&, range_type>>* = nullptr>
        /*implicit*/ scan_result(const scan_result<OtherR, Args...>& o)
            : m_range(o.range()), m_values(o.values())
        {
        }
        template <
            typename OtherR,
            std::enable_if_t<
                std::is_constructible_v<range_type, OtherR> &&
                !std::is_convertible_v<const OtherR&, range_type>>* = nullptr>
        explicit scan_result(const scan_result<OtherR, Args...>& o)
            : m_range(o.range()), m_values(o.values())
        {
        }

        template <typename OtherR,
                  std::enable_if_t<
                      std::is_constructible_v<range_type, OtherR> &&
                      std::is_convertible_v<OtherR&&, range_type>>* = nullptr>
        /*implicit*/ scan_result(scan_result<OtherR, Args...>&& o)
            : m_range(o.range()), m_values(SCN_MOVE(o.values()))
        {
        }
        template <typename OtherR,
                  std::enable_if_t<
                      std::is_constructible_v<range_type, OtherR> &&
                      !std::is_convertible_v<OtherR&&, range_type>>* = nullptr>
        explicit scan_result(scan_result<OtherR, Args...>&& o)
            : m_range(o.range()), m_values(SCN_MOVE(o.values()))
        {
        }

        template <typename OtherR,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, OtherR>>>
        scan_result& operator=(const scan_result<OtherR, Args...>& o)
        {
            m_range = o.range();
            m_values = o.values();
            return *this;
        }

        template <typename OtherR,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, OtherR>>>
        scan_result& operator=(scan_result<OtherR, Args...>&& o)
        {
            m_range = o.range();
            m_values = SCN_MOVE(o.values());
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
            else if constexpr (is_stdin_marker) {
                return stdin_range_marker{};
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
            else if constexpr (is_stdin_marker) {
                return stdin_range_marker{};
            }
            else {
                return m_range.end();
            }
        }

        /// Access the scanned values
        tuple_type& values() &
        {
            return m_values;
        }
        /// Access the scanned values
        const tuple_type& values() const&
        {
            return m_values;
        }
        /// Access the scanned values
        tuple_type&& values() &&
        {
            return SCN_MOVE(m_values);
        }
        /// Access the scanned values
        const tuple_type&& values() const&&
        {
            return SCN_MOVE(m_values);
        }

        /// Access the single scanned value
        template <size_t N = sizeof...(Args),
                  typename = std::enable_if_t<N == 1>>
        decltype(auto) value() &
        {
            return std::get<0>(m_values);
        }
        /// Access the single scanned value
        template <size_t N = sizeof...(Args),
                  typename = std::enable_if_t<N == 1>>
        decltype(auto) value() const&
        {
            return std::get<0>(m_values);
        }
        /// Access the single scanned value
        template <size_t N = sizeof...(Args),
                  typename = std::enable_if_t<N == 1>>
        decltype(auto) value() &&
        {
            return SCN_MOVE(std::get<0>(m_values));
        }
        /// Access the single scanned value
        template <size_t N = sizeof...(Args),
                  typename = std::enable_if_t<N == 1>>
        decltype(auto) value() const&&
        {
            return SCN_MOVE(std::get<0>(m_values));
        }

    private:
        SCN_NO_UNIQUE_ADDRESS range_type m_range{};
        SCN_NO_UNIQUE_ADDRESS tuple_type m_values{};
    };

    template <typename R, typename... Args>
    scan_result(R, std::tuple<Args...>) -> scan_result<R, Args...>;

    namespace detail {
        template <typename SourceRange>
        auto make_vscan_result_range_end(SourceRange& source)
        {
            return ranges::end(source);
        }
        template <typename CharT, size_t N>
        auto make_vscan_result_range_end(CharT (&source)[N])
            -> ranges::sentinel_t<CharT (&)[N]>
        {
            return source + N - 1;
        }

        template <typename SourceRange, typename CharT>
        auto make_vscan_result_range(SourceRange&& source,
                                     const basic_scan_buffer<CharT>& buffer,
                                     std::ptrdiff_t n)
            -> borrowed_subrange_with_sentinel_t<SourceRange>
        {
            return {ranges::next(ranges::begin(source), n),
                    make_vscan_result_range_end(source)};
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
