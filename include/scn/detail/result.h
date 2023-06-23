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
#include <scn/detail/erased_range.h>

#include <tuple>

namespace scn {
    SCN_BEGIN_NAMESPACE

    /**
     * Scan result type, containing the unparsed input, and a possible error.
     * The first element in the tuple returned by scan().
     */
    template <typename ResultMappedRange>
    class scan_result {
    public:
        using range_type = ResultMappedRange;

        scan_result() = default;

        template <typename Range,
                  typename = std::enable_if_t<
                      std::is_constructible_v<range_type, Range>>>
        explicit scan_result(Range&& r, scan_error e = {})
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

        template <typename Range,
                  typename =
                      std::enable_if_t<std::is_assignable_v<range_type, Range>>>
        scan_result& operator=(Range&& r)
        {
            m_range = SCN_FWD(r);
            return *this;
        }
        template <typename Range,
                  typename =
                      std::enable_if_t<std::is_assignable_v<range_type, Range>>>
        scan_result& operator=(scan_result<Range>&& r)
        {
            m_range = SCN_FWD(r.range());
            m_error = SCN_FWD(r.error());
            return *this;
        }

        /// True, if the operation succeeded
        constexpr explicit operator bool() const
        {
            return m_error.operator bool();
        }
        /// True, if the operation succeeded
        SCN_NODISCARD constexpr bool good() const
        {
            return operator bool();
        }

        /// Error, if one occured
        SCN_NODISCARD constexpr scan_error error() const
        {
            return m_error;
        }

        /// The unparsed input
        range_type& range() & SCN_NOEXCEPT
        {
            return m_range;
        }
        const range_type& range() const& SCN_NOEXCEPT
        {
            return m_range;
        }
        range_type&& range() && SCN_NOEXCEPT
        {
            return SCN_MOVE(m_range);
        }
        range_type&& range() const&& SCN_NOEXCEPT
        {
            return SCN_MOVE(m_range);
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
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
