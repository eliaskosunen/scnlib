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

#include <scn/detail/scan_buffer.h>
#include <scn/util/meta.h>
#include <scn/util/span.h>
#include <scn/util/string_view.h>

namespace scn {
SCN_BEGIN_NAMESPACE

/**
 * \defgroup scannable Scannable sources
 *
 * \brief Description of the `scannable_range` and `scannable_source`
 * concepts.
 *
 * A range is considered scannable, if it models at least `forward_range`,
 * and its character type is correct (its value type is the same as the one
 * of the format string).
 * If the range additionally models `contiguous_range` and `sized_range`,
 * additional optimizations are enabled.
 *
 * \code{.cpp}
 * // Exposition only
 * template <typename Range, typename CharT>
 * concept scannable_range =
 *     ranges::forward_range<Range> &&
 *     std::same_as<ranges::range_value_t<Range>, CharT>;
 * \endcode
 *
 * Additionally, files (`std::FILE*`) can be scanned from.
 * Files are always considered to be narrow (`char`-oriented).
 * Thus, the entire concept is:
 *
 * \code{.cpp}
 * // Exposition only
 * template <typename Source, typename CharT>
 * concept scannable_source =
 *   (std::same_as<std::remove_cvref_t<Source>, std::FILE*> &&
 *    std::same_as<CharT, char>) ||
 *   scannable_range<Source, CharT>;
 * \endcode
 */

/**
 * Tag type to indicate an invalid range given to `scn::scan`
 *
 * \ingroup scannable
 */
struct invalid_input_range {};

struct invalid_char_type : invalid_input_range {};
struct custom_char_traits : invalid_input_range {};
struct file_marker_found : invalid_input_range {};
struct insufficient_range : invalid_input_range {};

namespace detail {
template <typename CharT>
inline constexpr bool is_valid_char_type =
    std::is_same_v<std::remove_const_t<CharT>, char> ||
    std::is_same_v<std::remove_const_t<CharT>, wchar_t>;

namespace _make_scan_buffer {
// buffer -> ref_buffer
inline auto impl(scan_buffer::range_type r,
                 priority_tag<4>) SCN_NOEXCEPT->basic_scan_ref_buffer<char>
{
    if (!r.begin().stores_parent()) {
        return basic_scan_ref_buffer{r.begin().contiguous_segment()};
    }
    return basic_scan_ref_buffer{*r.begin().parent(), r.begin().position()};
}
inline auto impl(wscan_buffer::range_type r,
                 priority_tag<4>) SCN_NOEXCEPT->basic_scan_ref_buffer<wchar_t>
{
    if (!r.begin().stores_parent()) {
        return basic_scan_ref_buffer{r.begin().contiguous_segment()};
    }
    return basic_scan_ref_buffer{*r.begin().parent(), r.begin().position()};
}

// string_view -> string_buffer
template <typename CharT>
auto impl(std::basic_string_view<CharT> r, priority_tag<3>) SCN_NOEXCEPT
{
    if constexpr (is_valid_char_type<CharT>) {
        return r;
        // return make_string_scan_buffer(r);
    }
    else {
        return invalid_char_type{};
    }
}

// string& -> string_buffer
template <typename CharT, typename Traits, typename Allocator>
auto impl(const std::basic_string<CharT, Traits, Allocator>& r,
          priority_tag<3>) SCN_NOEXCEPT
{
    if constexpr (!is_valid_char_type<CharT>) {
        return invalid_char_type{};
    }
    else if constexpr (!std::is_same_v<Traits, std::char_traits<CharT>>) {
        return custom_char_traits{};
    }
    else {
        return std::basic_string_view<CharT>{r};
        // return make_string_scan_buffer(r);
    }
}

// String literals:
// CharT(&)[] -> string_buffer
template <typename CharT,
          std::size_t N,
          std::enable_if_t<is_valid_char_type<CharT>>* = nullptr>
auto impl(const CharT (&r)[N], priority_tag<3>) SCN_NOEXCEPT
{
    return std::basic_string_view<CharT>{r, N - 1};
#if 0
                return make_string_scan_buffer(
                    std::basic_string_view<CharT>{r, N - 1});
#endif
}

// FILE* -> file_buffer
inline auto impl(std::FILE* file, priority_tag<3>)
{
    return make_file_scan_buffer(file);
}

// contiguous + sized -> string_buffer
template <typename Range,
          std::enable_if_t<ranges::contiguous_range<Range> &&
                           ranges::sized_range<Range>>* = nullptr>
auto impl(const Range& r, priority_tag<2>)
{
    if constexpr (is_valid_char_type<detail::char_t<Range>>) {
        return std::basic_string_view{ranges::data(r),
                                      ranges_polyfill::usize(r)};
#if 0
                    return make_string_scan_buffer(std::basic_string_view{
                        ranges::data(r),
                        static_cast<std::size_t>(ranges::size(r))});
#endif
    }
    else {
        return invalid_char_type{};
    }
}

// !contiguous + random-access + iterator can be made into a ptr
// for MSVC debug iterators
//   -> string_buffer
template <typename Range,
          std::enable_if_t<!ranges::contiguous_range<Range> &&
                           ranges::random_access_range<Range> &&
                           can_make_address_from_iterator<
                               ranges::iterator_t<Range>>::value>* = nullptr>
auto impl(const Range& r, priority_tag<1>)
{
    if constexpr (is_valid_char_type<detail::char_t<Range>>) {
        return make_string_view_from_pointers(to_address(ranges::begin(r)),
                                              to_address(ranges::end(r)));
#if 0
                    return make_string_scan_buffer(
                        make_string_view_from_pointers(
                            to_address(ranges::begin(r)),
                            to_address(ranges::end(r))));
#endif
    }
    else {
        return invalid_char_type{};
    }
}

// forward -> forward_buffer<R>
template <typename Range>
auto impl(const Range& r, priority_tag<0>)
{
    if constexpr (std::is_same_v<Range, file_marker>) {
        return file_marker_found{};
    }
    else if constexpr (!ranges::forward_range<Range>) {
        if constexpr (ranges::input_range<Range>) {
            return insufficient_range{};
        }
        else {
            return invalid_input_range{};
        }
    }
    else if constexpr (!is_valid_char_type<detail::char_t<Range>>) {
        return invalid_char_type{};
    }
    else {
        return make_forward_scan_buffer(r);
    }
}
}  // namespace _make_scan_buffer

template <typename Range>
inline constexpr bool is_scannable_range =
    !std::is_base_of_v<invalid_input_range,
                       decltype(_make_scan_buffer::impl(
                                    SCN_DECLVAL(const Range&)),
                                priority_tag<4>{})>;

template <typename Range>
auto make_scan_buffer(const Range& range)
{
    using T = decltype(_make_scan_buffer::impl(range, priority_tag<4>{}));

    static_assert(!std::is_same_v<T, invalid_char_type>,
                  "\n"
                  "Unsupported range type given as input to a scanning "
                  "function.\n"
                  "A range needs to have a character type (value type) "
                  "of either `char` or `wchar_t` to be scannable.\n"
                  "For proper `wchar_t` support, <scn/xchar.h> needs "
                  "to be included.\n"
                  "See the scnlib documentation for more details.");
    static_assert(
        !std::is_same_v<T, custom_char_traits>,
        "\n"
        "Unsupported range type given as input to a scanning "
        "function.\n"
        "String types (std::basic_string, and std::basic_string_view) "
        "need to use std::char_traits. Strings with custom Traits are "
        "not supported.");
    static_assert(!std::is_same_v<T, file_marker_found>,
                  "\n"
                  "Unsupported range type given as input to a scanning "
                  "function.\n"
                  "file_marker_found cannot be used as an "
                  "source range type to scn::scan.\n"
                  "To read from stdin, use scn::input or scn::prompt, "
                  "and do not provide an explicit source range, "
                  "or use scn::scan with a FILE* directly.");
    static_assert(!std::is_same_v<T, insufficient_range>,
                  "\n"
                  "Unsupported range type given as input to a scanning "
                  "function.\n"
                  "In order to be scannable, a range needs to satisfy "
                  "`forward_range`. `input_range` is not sufficient.");
    static_assert(!std::is_same_v<T, invalid_input_range>,
                  "\n"
                  "Unsupported range type given as input to a scanning "
                  "function.\n"
                  "A range needs to model forward_range and have a valid "
                  "character type (char or wchar_t) to be scannable.\n"
                  "Examples of scannable ranges are std::string, "
                  "std::string_view, "
                  "std::vector<char>, and scn::istreambuf_view.\n"
                  "See the scnlib documentation for more details.");

    return _make_scan_buffer::impl(range, priority_tag<4>{});
}
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
