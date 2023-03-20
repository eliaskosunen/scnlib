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
#include <scn/util/string_view.h>

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
    }  // namespace detail

    /// Scanning context
    template <typename Range, typename CharT>
    class basic_scan_context {
    public:
        using char_type = CharT;
        using range_type = Range;
        using iterator = ranges::iterator_t<range_type>;
        using sentinel = ranges::sentinel_t<range_type>;
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

        basic_scan_context(const basic_scan_context&) = delete;
        basic_scan_context& operator=(const basic_scan_context&) = delete;

        basic_scan_context(basic_scan_context&&) = default;
        basic_scan_context& operator=(basic_scan_context&&) = default;
        ~basic_scan_context() = default;

        /// Get argument at index `id`
        constexpr basic_scan_arg<basic_scan_context> arg(size_t id) const
            SCN_NOEXCEPT
        {
            return m_args.get(id);
        }

        constexpr const basic_scan_args<basic_scan_context>& args() const
        {
            return m_args;
        }

        /// Returns a view over the input range, starting at `current()`
        constexpr range_type range() const
        {
            if constexpr (detail::is_string_view<range_type>::value) {
                return detail::make_string_view_from_iterators<char_type>(
                    current(), ranges::end(m_range));
            }
            else {
                return {current(), ranges::end(m_range)};
            }
        }
        /// Returns an iterator pointing to
        /// the beginning of the current input range
        constexpr iterator current() const
        {
            return m_current;
        }

        /// Advances the beginning of the input range to `it`
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

    SCN_END_NAMESPACE
}  // namespace scn
