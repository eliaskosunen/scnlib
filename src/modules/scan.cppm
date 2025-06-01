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

#define SCNLIB_MODULES
#include <scn/scan.h>
#include <scn/xchar.h>

export module scn.scan;

export namespace scn {
    using scn::unexpected;
    using scn::expected;

    namespace ranges {
        using scn::ranges::enable_borrowed_range;
        using scn::ranges::equality_comparable;
        using scn::ranges::equality_comparable_with;
        using scn::ranges::totally_ordered;
        using scn::ranges::totally_ordered_with;
        using scn::ranges::movable;
        using scn::ranges::copyable;
        using scn::ranges::semiregular;
        using scn::ranges::regular;
        using scn::ranges::incrementable_traits;
        using scn::ranges::iter_difference_t;
        using scn::ranges::readable_traits;
        using scn::ranges::iter_value_t;
        using scn::ranges::bidirectional_iterator_tag;
        using scn::ranges::forward_iterator_tag;
        using scn::ranges::input_iterator_tag;
        using scn::ranges::output_iterator_tag;
        using scn::ranges::random_access_iterator_tag;
        using scn::ranges::contiguous_iterator_tag;
        using scn::ranges::iterator_category;
        using scn::ranges::iterator_category_t;
        using scn::ranges::iter_reference_t;
        using scn::ranges::writable;
        using scn::ranges::weakly_incrementable;
        using scn::ranges::incrementable;
        using scn::ranges::input_or_output_iterator;
        using scn::ranges::sentinel_for;
        using scn::ranges::disable_sized_sentinel;
        using scn::ranges::sized_sentinel_for;
        using scn::ranges::input_iterator;
        using scn::ranges::output_iterator;
        using scn::ranges::forward_iterator;
        using scn::ranges::bidirectional_iterator;
        using scn::ranges::random_access_iterator;
        using scn::ranges::contiguous_iterator;
        using scn::ranges::begin;
        using scn::ranges::end;
        using scn::ranges::range;
        using scn::ranges::iterator_t;
        using scn::ranges::sentinel_t;
        using scn::ranges::range_difference_t;
        using scn::ranges::range_value_t;
        using scn::ranges::range_reference_t;
        using scn::ranges::data;
        using scn::ranges::disable_sized_range;
        using scn::ranges::size;
        using scn::ranges::ssize;
        using scn::ranges::empty;
        using scn::ranges::borrowed_range;
        using scn::ranges::sized_range;
        using scn::ranges::output_range;
        using scn::ranges::input_range;
        using scn::ranges::forward_range;
        using scn::ranges::bidirectional_range;
        using scn::ranges::random_access_range;
        using scn::ranges::contiguous_range;
        using scn::ranges::dangling;
        using scn::ranges::borrowed_iterator_t;
        using scn::ranges::view_interface;
        using scn::ranges::subrange;
        using scn::ranges::enable_borrowed_range;
        using scn::ranges::default_sentinel_t;
        using scn::ranges::enable_borrowed_range;
    }

    using scn::scan_error;
    using scn::operator==;
    using scn::operator!=;
    using scn::scan_format_string_error;
    using scn::scan_expected;

    using scn::invalid_input_range;
    using scn::invalid_char_type;
    using scn::file_marker_found;
    using scn::insufficient_range;

    using scn::basic_scan_arg;
    using scn::make_scan_args;
    using scn::make_wscan_args;
    using scn::basic_scan_args;
    
    using scn::basic_scan_parse_context;
    
    using scn::scan_result;

    using scn::runtime_format;
    using scn::basic_scan_format_string;

    using scn::basic_scan_context;

    using scn::scanner;
    using scn::discard;
    using scn::visit_scan_arg; // Deprecated
    
    using scn::vscan_result;
    using scn::vscan;
    using scn::vscan_value;

    using scn::scan_result_type;
    using scn::fill_scan_result;
    using scn::make_scan_result;
    using scn::scan;
    using scn::scan_value;
    using scn::input;
    using scn::prompt;
    using scn::scan_int;
    using scn::scan_int_exhaustive_valid;
}
