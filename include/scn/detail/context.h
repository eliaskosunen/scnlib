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
#include <scn/detail/locale_ref.h>
#include <scn/detail/scan_buffer.h>
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

template <typename CharT, typename Args>
struct scan_context_base {
public:
    /// Get argument at index `id`
    constexpr auto arg(size_t id) const SCN_NOEXCEPT
    {
        return m_args.get(id);
    }

    constexpr const Args& args() const
    {
        return m_args;
    }

    SCN_NODISCARD constexpr detail::locale_ref locale() const
    {
        return m_locale;
    }

protected:
    scan_context_base(Args args, detail::locale_ref loc)
        : m_args(SCN_MOVE(args)), m_locale(loc)
    {
    }

    Args m_args;
    detail::locale_ref m_locale;
};
}  // namespace detail

/**
 * \defgroup ctx Contexts and scanners
 *
 * \brief Lower-level APIs used for scanning individual values
 */

/**
 * Scanning context.
 *
 * \ingroup ctx
 */
template <typename CharT>
class basic_scan_context
    : public detail::
          scan_context_base<CharT, basic_scan_args<basic_scan_context<CharT>>> {
    using base =
        detail::scan_context_base<CharT,
                                  basic_scan_args<basic_scan_context<CharT>>>;

public:
    /// Character type of the input
    using char_type = CharT;
    using buffer_type = detail::basic_scan_buffer<char_type>;
    using range_type = typename buffer_type::range_type;
    using iterator = ranges::iterator_t<range_type>;
    using sentinel = ranges::sentinel_t<range_type>;
    using parse_context_type = basic_scan_parse_context<char_type>;

    using args_type = basic_scan_args<basic_scan_context>;
    using arg_type = basic_scan_arg<basic_scan_context>;

    /**
     * The scanner type associated with this scanning context.
     */
    template <typename T>
    using scanner_type = scanner<T, char_type>;

    constexpr basic_scan_context(iterator curr,
                                 args_type a,
                                 detail::locale_ref loc = {})
        : base(SCN_MOVE(a), loc), m_current(curr)
    {
    }

    basic_scan_context(const basic_scan_context&) = delete;
    basic_scan_context& operator=(const basic_scan_context&) = delete;

    basic_scan_context(basic_scan_context&&) = default;
    basic_scan_context& operator=(basic_scan_context&&) = default;
    ~basic_scan_context() = default;

    /**
     * Returns an iterator pointing to the current position in the source
     * range.
     */
    constexpr iterator begin() const
    {
        return m_current;
    }

    /**
     * Returns a sentinel pointing to the end of the source range.
     */
    constexpr sentinel end() const
    {
        return ranges_std::default_sentinel;
    }

    /**
     * Returns a subrange over `[begin(), end())`
     */
    constexpr auto range() const
    {
        return ranges::subrange{begin(), end()};
    }

    /// Advances the beginning of the source range to `it`
    void advance_to(iterator it)
    {
        m_current = SCN_MOVE(it);
    }

private:
    iterator m_current;
};

SCN_END_NAMESPACE
}  // namespace scn
