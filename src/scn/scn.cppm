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

module;

#define SCN_MODULE 1

#include <scn/macros.h>

// Do `import std` if we're on MSVC or (non-apple) clang with libc++,
// and have C++23
#if defined(__cpp_lib_modules) && !defined(SCN_IMPORT_STD) && \
    SCN_STD >= SCN_STD_23 &&                                  \
    (SCN_MSVC ||                                              \
     (SCN_CLANG && SCN_STDLIB_LIBCPP && !defined(__apple_build_version)))
#define SCN_IMPORT_STD 1
#endif

#include <scn/chrono.h>
#include <scn/istream.h>
#include <scn/ranges.h>
#include <scn/regex.h>
#include <scn/scan.h>
#include <scn/xchar.h>

export module scn;

export namespace scn {
SCN_BEGIN_NAMESPACE

// scan.h

using scn::bad_expected_access;
using scn::expected;
using scn::monostate;
using scn::unexpect;
using scn::unexpect_t;
using scn::unexpected;

#if SCN_HAS_INT128
using scn::int128;
using scn::uint128;
#endif

namespace ranges {
using scn::ranges::begin;
using scn::ranges::bidirectional_iterator;
using scn::ranges::bidirectional_iterator_tag;
using scn::ranges::bidirectional_range;
using scn::ranges::borrowed_iterator_t;
using scn::ranges::borrowed_range;
using scn::ranges::contiguous_iterator;
using scn::ranges::contiguous_iterator_tag;
using scn::ranges::contiguous_range;
using scn::ranges::copyable;
using scn::ranges::dangling;
using scn::ranges::data;
using scn::ranges::default_sentinel;
using scn::ranges::default_sentinel_t;
using scn::ranges::disable_sized_range;
using scn::ranges::disable_sized_sentinel;
using scn::ranges::empty;
using scn::ranges::enable_borrowed_range;
using scn::ranges::end;
using scn::ranges::equality_comparable;
using scn::ranges::equality_comparable_with;
using scn::ranges::forward_iterator;
using scn::ranges::forward_iterator_tag;
using scn::ranges::forward_range;
using scn::ranges::incrementable;
using scn::ranges::incrementable_traits;
using scn::ranges::input_iterator;
using scn::ranges::input_iterator_tag;
using scn::ranges::input_or_output_iterator;
using scn::ranges::input_range;
using scn::ranges::iter_difference_t;
using scn::ranges::iter_reference_t;
using scn::ranges::iter_value_t;
using scn::ranges::iterator_category;
using scn::ranges::iterator_category_t;
using scn::ranges::iterator_t;
using scn::ranges::movable;
using scn::ranges::output_iterator;
using scn::ranges::output_iterator_tag;
using scn::ranges::output_range;
using scn::ranges::random_access_iterator;
using scn::ranges::random_access_iterator_tag;
using scn::ranges::random_access_range;
using scn::ranges::range;
using scn::ranges::range_difference_t;
using scn::ranges::range_reference_t;
using scn::ranges::range_value_t;
using scn::ranges::readable_traits;
using scn::ranges::regular;
using scn::ranges::semiregular;
using scn::ranges::sentinel_for;
using scn::ranges::sentinel_t;
using scn::ranges::size;
using scn::ranges::sized_range;
using scn::ranges::sized_sentinel_for;
using scn::ranges::ssize;
using scn::ranges::subrange;
using scn::ranges::totally_ordered;
using scn::ranges::totally_ordered_with;
using scn::ranges::view_interface;
using scn::ranges::weakly_incrementable;
using scn::ranges::writable;

#if !SCN_DISABLE_IOSTREAM
using scn::ranges::basic_istreambuf_view;
using scn::ranges::istreambuf_view;
using scn::ranges::wistreambuf_view;
namespace views {
using scn::ranges::views::istreambuf;
}  // namespace views
#endif
}  // namespace ranges

// Not part of the public API.
// `scn::scan` finds these overloads by ADL from templates in the
// global module fragment, and MSVC's ADL in importers only sees exported
// declarations, so e.g. scanning from a `std::istream` would fail otherwise.
namespace detail {
using scn::detail::make_scan_buffer;
using scn::detail::make_vscan_result;
}  // namespace detail

using scn::scan_error;
using scn::operator==;
using scn::operator!=;
using scn::scan_expected;
#if SCN_HAS_EXCEPTIONS
using scn::scan_format_string_error;
#endif

using scn::custom_char_traits;
using scn::insufficient_range;
using scn::invalid_char_type;
using scn::invalid_source;

using scn::basic_scan_arg;
using scn::basic_scan_args;
using scn::make_scan_args;
using scn::make_wscan_args;

using scn::basic_scan_parse_context;

using scn::scan_file;

using scn::scan_result;

using scn::source_tag;
using scn::source_tag_type;
using scn::stdin_tag;
using scn::stdin_tag_t;

using scn::basic_scan_format_string;
using scn::runtime_format;

using scn::basic_scan_context;
using scn::scan_args;
using scn::scan_context;
using scn::scan_format_string;
using scn::scan_parse_context;
using scn::wscan_args;
using scn::wscan_context;
using scn::wscan_format_string;
using scn::wscan_parse_context;

using scn::discard;
using scn::scanner;

using scn::vinput;
using scn::vscan;
using scn::vscan_result;
using scn::vscan_value;

using scn::fill_scan_result;
using scn::input;
using scn::make_scan_result;
using scn::prompt;
using scn::scan;
using scn::scan_int;
using scn::scan_int_exhaustive_valid;
using scn::scan_result_type;
using scn::scan_value;

// chrono.h

#if !SCN_DISABLE_CHRONO
using scn::day;
using scn::month;
using scn::weekday;
using scn::year;

using scn::month_day;
using scn::year_month;
using scn::year_month_day;

using scn::Friday;
using scn::Monday;
using scn::Saturday;
using scn::Sunday;
using scn::Thursday;
using scn::Tuesday;
using scn::Wednesday;

using scn::April;
using scn::August;
using scn::December;
using scn::February;
using scn::January;
using scn::July;
using scn::June;
using scn::March;
using scn::May;
using scn::November;
using scn::October;
using scn::September;

using scn::datetime_components;
using scn::tm_with_tz;
#endif

// ranges.h

using scn::range_format;
using scn::range_format_kind;
using scn::range_scanner;

// regex.h

using scn::basic_regex_match;
using scn::basic_regex_matches;
using scn::regex_match;
using scn::regex_matches;
using scn::wregex_match;
using scn::wregex_matches;

// istream.h

#if !SCN_DISABLE_IOSTREAM
using scn::basic_istream_scanner;
using scn::istream_scanner;
using scn::wistream_scanner;
#endif

SCN_END_NAMESPACE
}  // namespace scn
