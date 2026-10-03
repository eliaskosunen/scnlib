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

#include <cstddef>
#include <cstdio>
#include <deque>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

import scn;

struct streamable {
    int i{};

    friend std::istream& operator>>(std::istream& is, streamable& val)
    {
        return is >> val.i;
    }
};
template <>
struct scn::scanner<streamable, char> : scn::istream_scanner {};

// Names that should be usable through the module
static_assert(
    std::is_same_v<scn::scan_args, scn::basic_scan_args<scn::scan_context>>);
static_assert(
    std::is_same_v<scn::wscan_args, scn::basic_scan_args<scn::wscan_context>>);
static_assert(std::is_same_v<scn::scan_parse_context,
                             scn::basic_scan_parse_context<char>>);
static_assert(std::is_same_v<scn::wscan_parse_context,
                             scn::basic_scan_parse_context<wchar_t>>);
static_assert(std::is_same_v<scn::regex_match, scn::basic_regex_match<char>>);
static_assert(
    std::is_same_v<scn::wregex_match, scn::basic_regex_match<wchar_t>>);
static_assert(
    std::is_same_v<scn::regex_matches, scn::basic_regex_matches<char>>);
static_assert(
    std::is_same_v<scn::wregex_matches, scn::basic_regex_matches<wchar_t>>);
static_assert(
    std::is_same_v<scn::wistream_scanner, scn::basic_istream_scanner<wchar_t>>);
static_assert(std::is_same_v<scn::ranges::wistreambuf_view,
                             scn::ranges::basic_istreambuf_view<wchar_t>>);

#if defined(__SIZEOF_INT128__) && (defined(__GNUC__) || defined(__clang__)) && \
    !defined(_MSC_VER)
static_assert(sizeof(scn::int128) == 16);
static_assert(sizeof(scn::uint128) == 16);
#endif
static_assert(std::is_empty_v<scn::monostate>);
static_assert(std::is_same_v<decltype(scn::unexpect), const scn::unexpect_t>);
static_assert(std::is_same_v<decltype(scn::stdin_tag), const scn::stdin_tag_t>);
static_assert(std::is_same_v<decltype(scn::source_tag<int>),
                             const scn::source_tag_type<int>>);
static_assert(std::is_base_of_v<scn::bad_expected_access<void>,
                                scn::bad_expected_access<scn::scan_error>>);
static_assert(std::is_same_v<decltype(scn::ranges::default_sentinel),
                             const scn::ranges::default_sentinel_t>);

// Defined in module_test_layout.cpp, which uses the headers instead of the
// module, like the compiled library does
std::size_t header_sizeof_scan_expected();
std::size_t header_alignof_scan_expected();
std::size_t header_sizeof_scan_result();
std::size_t header_alignof_scan_result();

int main()
{
    using expected_type = scn::scan_expected<std::ptrdiff_t>;
    using result_type = decltype(scn::scan<int>(std::string_view{}, "{}"));
    if (sizeof(expected_type) != header_sizeof_scan_expected() ||
        alignof(expected_type) != header_alignof_scan_expected() ||
        sizeof(result_type) != header_sizeof_scan_result() ||
        alignof(result_type) != header_alignof_scan_result()) {
        std::fprintf(stderr,
                     "Layout mismatch between module and header: "
                     "scan_expected %zu/%zu vs. %zu/%zu, "
                     "scan result %zu/%zu vs. %zu/%zu (size/alignment)\n",
                     sizeof(expected_type), alignof(expected_type),
                     header_sizeof_scan_expected(),
                     header_alignof_scan_expected(), sizeof(result_type),
                     alignof(result_type), header_sizeof_scan_result(),
                     header_alignof_scan_result());
        return 1;
    }

    auto result = scn::scan<int>("42", "{}");

    if (!result) {
        std::fprintf(stderr, "Failed to scan: %s\n", result.error().msg());
        return 1;
    }
    if (result->value() != 42) {
        std::fprintf(stderr, "Invalid value: %d\n", result->value());
        return 1;
    }

    scn::expected<int, scn::scan_error> failed{
        scn::unexpect, scn::scan_error{scn::scan_error::end_of_input, "EOF"}};
    if (failed.has_value() ||
        failed.error().code() != scn::scan_error::end_of_input) {
        std::fprintf(stderr, "Invalid scn::unexpect result\n");
        return 1;
    }
    scn::monostate monostate{};
    (void)monostate;

    auto custom = scn::scan<streamable>("123", "{}");
    if (!custom || custom->value().i != 123) {
        std::fprintf(stderr, "Failed to scan with istream_scanner\n");
        return 1;
    }

    std::istringstream stream{"456"};
    auto view = scn::ranges::views::istreambuf(stream);
    static_assert(std::is_same_v<decltype(view), scn::ranges::istreambuf_view>);
    auto from_stream = scn::scan<int>(view, "{}");
    if (!from_stream || from_stream->value() != 456) {
        std::fprintf(stderr, "Failed to scan from istreambuf_view\n");
        return 1;
    }

    std::string lvalue_string{"11"};
    auto from_lvalue = scn::scan<int>(lvalue_string, "{}");
    if (!from_lvalue || from_lvalue->value() != 11) {
        std::fprintf(stderr, "Failed to scan from std::string lvalue\n");
        return 1;
    }

    auto from_rvalue = scn::scan<int>(std::string{"12"}, "{}");
    if (!from_rvalue || from_rvalue->value() != 12) {
        std::fprintf(stderr, "Failed to scan from std::string rvalue\n");
        return 1;
    }

    auto from_string_view = scn::scan<int>(std::string_view{"13"}, "{}");
    if (!from_string_view || from_string_view->value() != 13) {
        std::fprintf(stderr, "Failed to scan from std::string_view\n");
        return 1;
    }

    std::deque<char> deque{'1', '4'};
    auto from_deque = scn::scan<int>(deque, "{}");
    if (!from_deque || from_deque->value() != 14) {
        std::fprintf(stderr, "Failed to scan from std::deque\n");
        return 1;
    }

    std::FILE* file = std::tmpfile();
    if (!file) {
        std::fprintf(stderr, "Failed to create a temporary file\n");
        return 1;
    }
    std::fputs("15", file);
    std::rewind(file);
    scn::scan_file scannable_file{file};
    auto from_file = scn::scan<int>(scannable_file, "{}");
    std::fclose(file);
    if (!from_file || from_file->value() != 15) {
        std::fprintf(stderr, "Failed to scan from scn::scan_file\n");
        return 1;
    }

    std::istringstream direct_stream{"16"};
    auto from_istream = scn::scan<int>(direct_stream, "{}");
    if (!from_istream || from_istream->value() != 16) {
        std::fprintf(stderr, "Failed to scan from std::istream\n");
        return 1;
    }

    auto two_values = scn::scan<int, double>("1 2.5", "{} {}");
    if (!two_values || std::get<0>(two_values->values()) != 1 ||
        std::get<1>(two_values->values()) != 2.5) {
        std::fprintf(stderr, "Failed to scan two values\n");
        return 1;
    }
}
