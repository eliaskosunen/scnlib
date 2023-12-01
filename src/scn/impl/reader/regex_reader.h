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

#include <scn/detail/regex.h>

#if !SCN_DISABLE_REGEX

#include <scn/impl/reader/common.h>

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
#include <regex>
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
#include <boost/regex.hpp>
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
#include <re2/re2.h>
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        auto read_regex_string_impl(std::basic_string_view<CharT> pattern,
                                    std::basic_string_view<CharT> input)
            -> scan_expected<typename std::basic_string_view<CharT>::iterator>
        {
#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
            auto re = std::basic_regex<CharT>{pattern.data(), pattern.size()};
            std::match_results<const CharT*> matches{};
            bool found = std::regex_search(
                input.data(), input.data() + input.size(), matches, re,
                std::regex_constants::match_continuous);
            if (!found || matches.prefix().matched) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Regular expression didn't match");
            }
            return input.begin() +
                   ranges::distance(input.data(), matches[0].second);
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
            auto re = boost::basic_regex<CharT>{pattern.data(), pattern.size(),
                                                boost::regex_constants::normal};
            boost::match_results<const CharT*> matches{};
            bool found = boost::regex_search(
                input.data(), input.data() + input.size(), matches, re,
                boost::regex_constants::match_continuous);
            if (!found || matches.prefix().matched) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Regular expression didn't match");
            }
            return input.begin() +
                   ranges::distance(input.data(), matches[0].second);
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
            static_assert(std::is_same_v<CharT, char>);
            auto re = re2::RE2{pattern, RE2::Quiet};
            if (!re.ok()) {
                return unexpected_scan_error(
                    scan_error::invalid_format_string,
                    "Failed to parse regular expression");
            }
            auto new_input = input;
            bool found = re2::RE2::Consume(&new_input, re);
            if (!found) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Regular expression didn't match");
            }
            return input.begin() +
                   ranges::distance(input.data(), new_input.data());
#else
#error TODO
#endif
        }

        template <typename CharT>
        auto read_regex_matches_impl(std::basic_string_view<CharT> pattern,
                                     std::basic_string_view<CharT> input,
                                     basic_regex_matches<CharT>& value)
            -> scan_expected<typename std::basic_string_view<CharT>::iterator>
        {
#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
            auto re = std::basic_regex<CharT>{pattern.data(), pattern.size()};
            std::match_results<const CharT*> matches{};
            bool found = std::regex_search(
                input.data(), input.data() + input.size(), matches, re,
                std::regex_constants::match_continuous);
            if (!found || matches.prefix().matched) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Regular expression didn't match");
            }
            value.matches.reserve(matches.size());
            ranges::transform(
                matches, std::back_inserter(value.matches),
                [](auto&& match)
                    -> std::optional<
                        typename basic_regex_matches<CharT>::match> {
                    if (!match.matched)
                        return std::nullopt;
                    return detail::make_string_view_from_pointers(match.first,
                                                                  match.second);
                });
            return input.begin() +
                   ranges::distance(input.data(), matches[0].second);
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
            auto re = boost::basic_regex<CharT>{pattern.data(), pattern.size(),
                                                boost::regex_constants::normal};
            boost::match_results<const CharT*> matches{};
            bool found = boost::regex_search(
                input.data(), input.data() + input.size(), matches, re,
                boost::regex_constants::match_continuous);
            if (!found || matches.prefix().matched) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Regular expression didn't match");
            }
            value.matches.reserve(matches.size());
            ranges::transform(
                matches, std::back_inserter(value.matches),
                [](auto&& match)
                    -> std::optional<
                        typename basic_regex_matches<CharT>::match> {
                    if (!match.matched)
                        return std::nullopt;
                    return detail::make_string_view_from_pointers(match.first,
                                                                  match.second);
                });
            return input.begin() +
                   ranges::distance(input.data(), matches[0].second);
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
            static_assert(std::is_same_v<CharT, char>);
            auto re = re2::RE2{pattern, RE2::Quiet};
            if (!re.ok()) {
                return unexpected_scan_error(
                    scan_error::invalid_format_string,
                    "Failed to parse regular expression");
            }
            size_t max_matches_n =
                static_cast<size_t>(re.NumberOfCapturingGroups());
            std::vector<std::optional<std::string_view>> matches(max_matches_n);
            std::vector<re2::RE2::Arg> match_args(max_matches_n);
            std::vector<re2::RE2::Arg*> match_argptrs(max_matches_n);
            ranges::transform(matches, match_args.begin(),
                              [](auto& val) { return re2::RE2::Arg{&val}; });
            ranges::transform(match_args, match_argptrs.begin(),
                              [](auto& arg) { return &arg; });
            auto new_input = input;
            bool found = re2::RE2::ConsumeN(
                &new_input, re, match_argptrs.data(), match_argptrs.size());
            if (!found) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Regular expression didn't match");
            }
            value.matches.reserve(matches.size() + 1);
            value.matches.emplace_back(detail::make_string_view_from_pointers(
                input.data(), new_input.data()));
            ranges::transform(
                matches, std::back_inserter(value.matches),
                [](auto&& match)
                    -> std::optional<
                        typename basic_regex_matches<CharT>::match> {
                    if (!match)
                        return std::nullopt;
                    return *match;
                });
            return input.begin() +
                   ranges::distance(input.data(), new_input.data());
#else
#error TODO
#endif
        }

        template <typename SourceCharT>
        struct regex_matches_reader
            : public reader_base<regex_matches_reader<SourceCharT>,
                                 SourceCharT> {
            void check_specs_impl(
                const detail::basic_format_specs<SourceCharT>& specs,
                reader_error_handler& eh)
            {
                detail::check_regex_type_specs(specs, eh);
                SCN_EXPECT(!specs.charset_string.empty());
            }

            template <typename Range, typename DestCharT>
            scan_expected<simple_borrowed_iterator_t<Range>> read_default(
                Range&&,
                basic_regex_matches<DestCharT>&,
                detail::locale_ref = {})
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

            template <typename Range, typename DestCharT>
            scan_expected<simple_borrowed_iterator_t<Range>> read_specs(
                Range&& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                basic_regex_matches<DestCharT>& value,
                detail::locale_ref = {})
            {
                if constexpr (!ranges::contiguous_range<Range>) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "Cannot use regex with a non-contiguous source range");
                }
                else if constexpr (!std::is_same_v<SourceCharT, DestCharT>) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "Cannot transcode is regex_matches_reader");
                }
                else if constexpr (!SCN_REGEX_SUPPORTS_WIDE_STRINGS &&
                                   !std::is_same_v<SourceCharT, char>) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "Regex backend doesn't support wide strings as input");
                }
                else {
                    auto input = detail::make_string_view_from_pointers(
                        ranges::data(range),
                        ranges::data(range) + ranges::size(range));
                    SCN_TRY(it, read_regex_matches_impl(specs.charset_string,
                                                        input, value));
                    return ranges::begin(range) +
                           ranges::distance(input.begin(), it);
                }
            }
        };

        template <typename CharT>
        struct reader_impl_for_regex_matches
            : public regex_matches_reader<CharT> {};
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn

#endif
