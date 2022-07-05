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
#include <scn/detail/istream_range.h>
#include <scn/detail/locale_ref.h>
#include <scn/detail/ranges.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename Iterator, typename = void>
        struct is_comparable_with_nullptr : std::false_type {};
        template <typename Iterator>
        struct is_comparable_with_nullptr<
            Iterator,
            std::void_t<decltype(SCN_DECLVAL(const Iterator&) == nullptr)>>
            : std::true_type {};

        static_assert(
            is_comparable_with_nullptr<std::string_view::iterator>::value);
        static_assert(
            is_comparable_with_nullptr<ranges::iterator_t<ranges::subrange<
                ranges::iterator_t<std::string_view>,
                ranges::sentinel_t<std::string_view>>>>::value);
    }  // namespace detail

    template <typename Range, typename CharT>
    class basic_scan_context {
    public:
        using char_type = CharT;
        using range_type = Range;
        using iterator = ranges::iterator_t<range_type>;
        using sentinel = ranges::sentinel_t<range_type>;
        using subrange_type = ranges::subrange<iterator, sentinel>;
        using parse_context_type = basic_scan_parse_context<char_type>;

        using arg_type = basic_scan_arg<basic_scan_context>;

        template <typename T>
        using scanner_type = scanner<T, char_type>;

        template <typename R>
        constexpr basic_scan_context(R&& r,
                                     basic_scan_args<basic_scan_context> a,
                                     detail::locale_ref loc = {})
            : m_range(SCN_FWD(r)),
              m_current(ranges::begin(m_range)),
              m_args(SCN_MOVE(a)),
              m_locale(loc)
        {
        }

        constexpr basic_scan_arg<basic_scan_context> arg(size_t id) const
            SCN_NOEXCEPT
        {
            return m_args.get(id);
        }

        constexpr const basic_scan_args<basic_scan_context>& args() const
        {
            return m_args;
        }

        constexpr subrange_type range() const
        {
            return {current(), ranges::end(m_range)};
        }
        constexpr iterator current() const
        {
            return m_current;
        }

        void advance_to(iterator it)
        {
            if constexpr (detail::is_comparable_with_nullptr<iterator>::value) {
                if (it == nullptr) {
                    it = ranges::end(m_range);
                }
            }
            m_current = SCN_MOVE(it);
        }

        constexpr detail::locale_ref locale() const
        {
            return m_locale;
        }

    private:
        range_type m_range;
        iterator m_current;
        basic_scan_args<basic_scan_context> m_args;
        detail::locale_ref m_locale;
    };

    namespace detail {
        struct _map_subrange_to_context_range_type_fn {
            template <typename CharT>
            std::basic_string_view<CharT> operator()(
                tag_type<CharT>,
                std::basic_string_view<CharT> r) const SCN_NOEXCEPT
            {
                return r;
            }

            template <typename CharT>
            std::basic_string_view<CharT> operator()(
                tag_type<CharT>,
                ranges::subrange<
                    ranges::iterator_t<std::basic_string_view<CharT>>,
                    ranges::sentinel_t<std::basic_string_view<CharT>>> r) const
                SCN_NOEXCEPT
            {
                return {r.data(), static_cast<std::size_t>(r.size())};
            }

            template <typename CharT>
            basic_istreambuf_subrange<CharT> operator()(
                tag_type<CharT>,
                basic_istreambuf_subrange<CharT> r) const SCN_NOEXCEPT
            {
                return r;
            }

            template <typename CharT>
            basic_istreambuf_subrange<CharT> operator()(
                tag_type<CharT>,
                ranges::subrange<
                    ranges::iterator_t<basic_istreambuf_subrange<CharT>>,
                    ranges::sentinel_t<basic_istreambuf_subrange<CharT>>> r)
                const SCN_NOEXCEPT
            {
                return {r.begin(), r.end()};
            }

            template <typename CharT>
            basic_erased_subrange<CharT> operator()(
                tag_type<CharT>,
                basic_erased_subrange<CharT> r) const SCN_NOEXCEPT
            {
                return r;
            }

            template <typename CharT>
            basic_erased_subrange<CharT> operator()(
                tag_type<CharT>,
                ranges::subrange<
                    ranges::iterator_t<basic_erased_subrange<CharT>>,
                    ranges::sentinel_t<basic_erased_subrange<CharT>>> r) const
                SCN_NOEXCEPT
            {
                return {r.begin(), r.end()};
            }
        };

        inline constexpr auto map_subrange_to_context_range_type =
            _map_subrange_to_context_range_type_fn{};
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
