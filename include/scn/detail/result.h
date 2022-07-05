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
#include <scn/detail/error.h>
#include <scn/detail/input_map.h>

#include <tuple>

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename Range>
    struct vscan_result {
        using iterator = ranges::iterator_t<Range>;
        using sentinel = ranges::sentinel_t<Range>;

        std::conditional_t<detail::is_erased_range_or_subrange<Range>::value,
                           Range,
                           ranges::subrange<iterator, sentinel>>
            range;
        scan_error error;
    };

    namespace detail {
        template <typename CharT, typename Source>
        std::basic_string_view<CharT> map_scan_result_range(
            const Source&,
            const vscan_result<std::basic_string_view<CharT>>& result)
            SCN_NOEXCEPT_P(
                std::is_nothrow_constructible_v<std::basic_string_view<CharT>,
                                                const CharT*,
                                                std::size_t>)
        {
            return {result.range.data(),
                    static_cast<std::size_t>(result.range.size())};
        }

        template <typename CharT, typename Source>
        basic_istreambuf_subrange<CharT> map_scan_result_range(
            const Source&,
            const vscan_result<basic_istreambuf_subrange<CharT>>& result)
            SCN_NOEXCEPT_P(
                std::is_nothrow_constructible_v<
                    basic_istreambuf_subrange<CharT>,
                    ranges::iterator_t<basic_istreambuf_subrange<CharT>>&,
                    ranges::sentinel_t<basic_istreambuf_subrange<CharT>>&>)
        {
            return {result.range.begin(), result.range.end()};
        }

        template <typename CharT, typename Source>
        auto map_scan_result_range(
            const Source& source,
            const vscan_result<basic_erased_subrange<CharT>>& result)
        {
            if constexpr (is_erased_range_or_subrange<Source>::value) {
                return result.range;
            }
            else {
                using iterator = ranges::iterator_t<const Source&>;
                using sentinel = ranges::sentinel_t<const Source&>;
                constexpr ranges::subrange_kind kind =
                    ranges::sized_range<const Source&>
                        ? ranges::subrange_kind::sized
                        : ranges::subrange_kind::unsized;

                return ranges::subrange<iterator, sentinel, kind>{
                    ranges::next(ranges::begin(source),
                                 result.range.begin().distance_from_begin()),
                    ranges::end(source)};
            }
        }
    }  // namespace detail

    template <typename ResultMappedRange>
    class scan_result {
    public:
        using range_type = ResultMappedRange;

        scan_result() = default;

        template <typename Range,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, Range>>>
        explicit scan_result(Range&& r) : m_range(SCN_FWD(r))
        {
        }

        template <typename Range,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, Range>>>
        scan_result(Range&& r, scan_error e)
            : m_range(SCN_FWD(r)), m_error(SCN_MOVE(e))
        {
        }

        template <typename Range,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, Range>>>
        scan_result(const scan_result<Range>& o)
            : m_range(o.range()), m_error(o.error())
        {
        }
        template <typename Range,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, Range>>>
        scan_result(scan_result<Range>&& o)
            : m_range(SCN_MOVE(o.range())), m_error(SCN_MOVE(o.error()))
        {
        }

        constexpr explicit operator bool() const
        {
            return m_error.operator bool();
        }
        SCN_NODISCARD constexpr bool good() const
        {
            return operator bool();
        }

        SCN_NODISCARD constexpr scan_error error() const
        {
            return m_error;
        }

        range_type& range() & SCN_NOEXCEPT
        {
            return m_range;
        }
        const range_type& range() const& SCN_NOEXCEPT
        {
            return m_range;
        }
        range_type range() && SCN_NOEXCEPT
        {
            return m_range;
        }

    private:
        range_type m_range;
        scan_error m_error{};
    };

    template <typename Range>
    scan_result(Range) -> scan_result<Range>;
    template <typename Range>
    scan_result(Range, scan_error) -> scan_result<Range>;

    namespace detail {
        template <typename... T, std::size_t... I>
        std::tuple<T&...> make_ref_tuple_impl(std::tuple<T...>& t,
                                              std::index_sequence<I...>)
        {
            return std::tie(std::get<I>(t)...);
        }

        template <typename... T>
        std::tuple<T&...> make_ref_tuple(std::tuple<T...>& t)
        {
            return make_ref_tuple_impl(
                t, std::make_index_sequence<sizeof...(T)>{});
        }
    }  // namespace detail

    template <typename ResultMappedRange, typename... Args>
    class scan_result_tuple {
    public:
        using result_type = scan_result<ResultMappedRange>;
        using range_type = typename result_type::range_type;
        using tuple_type = std::tuple<Args...>;

        scan_result_tuple() = default;

        template <typename Range,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, Range>>>
        scan_result_tuple(scan_result<Range>&& result, tuple_type values)
            : m_result(SCN_MOVE(result)), m_values(SCN_MOVE(values))
        {
        }

        template <typename Range,
                  typename... A,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, Range>>>
        scan_result_tuple(const scan_result_tuple<Range, A...>& other)
            : m_result(other.result()), m_values(other.values())
        {
        }
        template <typename Range,
                  typename... A,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, Range>>>
        scan_result_tuple(scan_result_tuple<Range, A...>&& other)
            : m_result(SCN_MOVE(other.result())),
              m_values(SCN_MOVE(other.values()))
        {
        }

        constexpr result_type& result() & SCN_NOEXCEPT
        {
            return m_result;
        }
        constexpr const result_type& result() const& SCN_NOEXCEPT
        {
            return m_result;
        }
        constexpr result_type result() && SCN_NOEXCEPT
        {
            return m_result;
        }

        constexpr tuple_type& values() & SCN_NOEXCEPT
        {
            return m_values;
        }
        constexpr const tuple_type& values() const& SCN_NOEXCEPT
        {
            return m_values;
        }
        constexpr tuple_type values() && SCN_NOEXCEPT
        {
            return m_values;
        }

        constexpr explicit operator bool() const SCN_NOEXCEPT
        {
            return result().operator bool();
        }
        SCN_NODISCARD constexpr bool good() const SCN_NOEXCEPT
        {
            return operator bool();
        }

        SCN_NODISCARD constexpr scan_error error() const SCN_NOEXCEPT
        {
            return result().error();
        }

        range_type& range() & SCN_NOEXCEPT
        {
            return result().range();
        }
        const range_type& range() const& SCN_NOEXCEPT
        {
            return result().range();
        }
        range_type range() &&
        {
            return result().range();
        }

        operator std::tuple<result_type&, Args&...>()
        {
            return std::tuple_cat(std::tuple<result_type&>{m_result},
                                  detail::make_ref_tuple(m_values));
        }

    private:
        result_type m_result{};
        tuple_type m_values{};
    };

    SCN_END_NAMESPACE
}  // namespace scn

template <std::size_t I, typename R, typename... T>
struct std::tuple_element<I, scn::scan_result_tuple<R, T...>> {
    using type = std::tuple_element_t<I, std::tuple<scn::scan_result<R>, T...>>;
};
template <typename R, typename... T>
struct std::tuple_size<scn::scan_result_tuple<R, T...>>
    : std::integral_constant<std::size_t, 1 + sizeof...(T)> {};

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <std::size_t I, typename R, typename... T>
    decltype(auto) get(scan_result_tuple<R, T...>& r)
    {
        if constexpr (I == 0) {
            return r.result();
        }
        else {
            return std::get<I - 1>(r.values());
        }
    }
    template <std::size_t I, typename R, typename... T>
    decltype(auto) get(const scan_result_tuple<R, T...>& r)
    {
        if constexpr (I == 0) {
            return r.result();
        }
        else {
            return std::get<I - 1>(r.values());
        }
    }
    template <std::size_t I, typename R, typename... T>
    decltype(auto) get(scan_result_tuple<R, T...>&& r)
    {
        if constexpr (I == 0) {
            return r.result();
        }
        else {
            return std::get<I - 1>(r.values());
        }
    }

    SCN_END_NAMESPACE
}  // namespace scn
