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

#include <scn/impl/reader/common.h>

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
#include <regex>
#if SCN_REGEX_BOOST_USE_ICU
#error "Can't use the ICU with std::regex"
#endif
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
#include <boost/regex.hpp>
#if SCN_REGEX_BOOST_USE_ICU
#include <boost/regex/icu.hpp>
#endif
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
#include <re2/re2.h>
#endif

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {

// Forward declaration for C++17 compatibility with regex disabled
template <typename CharT, typename Input>
auto read_regex_matches_impl(std::basic_string_view<CharT> pattern,
                             detail::regex_flags flags,
                             Input input,
                             basic_regex_matches<CharT>& value)
    -> scan_expected<ranges::iterator_t<Input>>;

#if !SCN_DISABLE_REGEX

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
constexpr auto make_regex_flags(detail::regex_flags flags)
    -> scan_expected<std::regex_constants::syntax_option_type>
{
    std::regex_constants::syntax_option_type result{};
    if ((flags & detail::regex_flags::multiline) != detail::regex_flags::none) {
#if SCN_HAS_STD_REGEX_MULTILINE
        result |= std::regex_constants::multiline;
#else
        return unexpected_scan_error(
            scan_error::invalid_format_string,
            "/m flag for regex isn't supported by regex backend");
#endif
    }
    if ((flags & detail::regex_flags::singleline) !=
        detail::regex_flags::none) {
        return unexpected_scan_error(
            scan_error::invalid_format_string,
            "/s flag for regex isn't supported by regex backend");
    }
    if ((flags & detail::regex_flags::nocase) != detail::regex_flags::none) {
        result |= std::regex_constants::icase;
    }
    if ((flags & detail::regex_flags::nocapture) != detail::regex_flags::none) {
        result |= std::regex_constants::nosubs;
    }
    return result;
}
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
constexpr auto make_regex_flags(detail::regex_flags flags)
    -> boost::regex_constants::syntax_option_type
{
    boost::regex_constants::syntax_option_type result{};
    if ((flags & detail::regex_flags::multiline) == detail::regex_flags::none) {
        result |= boost::regex_constants::no_mod_m;
    }
    if ((flags & detail::regex_flags::singleline) !=
        detail::regex_flags::none) {
        result |= boost::regex_constants::mod_s;
    }
    if ((flags & detail::regex_flags::nocase) != detail::regex_flags::none) {
        result |= boost::regex_constants::icase;
    }
    if ((flags & detail::regex_flags::nocapture) != detail::regex_flags::none) {
        result |= boost::regex_constants::nosubs;
    }
    return result;
}
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
inline auto make_regex_flags(detail::regex_flags flags)
    -> std::pair<RE2::Options, std::string_view>
{
    RE2::Options opt{RE2::Quiet};
    std::string_view stringflags{};

    if ((flags & detail::regex_flags::multiline) == detail::regex_flags::none) {
        stringflags = "(?m)";
    }
    if ((flags & detail::regex_flags::singleline) !=
        detail::regex_flags::none) {
        opt.set_dot_nl(true);
    }
    if ((flags & detail::regex_flags::nocase) != detail::regex_flags::none) {
        opt.set_case_sensitive(false);
    }
    if ((flags & detail::regex_flags::nocapture) != detail::regex_flags::none) {
        opt.set_never_capture(true);
    }

    return {opt, stringflags};
}
#endif  // SCN_REGEX_BACKEND == ...

template <typename CharT, typename Input>
auto read_regex_string_impl(std::basic_string_view<CharT> pattern,
                            detail::regex_flags flags,
                            Input input)
    -> scan_expected<ranges::iterator_t<Input>>
{
    static_assert(ranges::contiguous_range<Input> &&
                  ranges::borrowed_range<Input> &&
                  std::is_same_v<ranges::range_value_t<Input>, CharT>);

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
    std::basic_regex<CharT> re{};
    try {
        SCN_TRY(re_flags, make_regex_flags(flags));
        re = std::basic_regex<CharT>{pattern.data(), pattern.size(),
                                     re_flags | std::regex_constants::nosubs};
    }
    catch (const std::regex_error& err) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Invalid regex");
    }

    std::match_results<const CharT*> matches{};
    try {
        bool found = std::regex_search(input.data(),
                                       input.data() + input.size(), matches, re,
                                       std::regex_constants::match_continuous);
        if (!found || matches.prefix().matched) {
            return unexpected_scan_error(scan_error::invalid_scanned_value,
                                         "Regular expression didn't match");
        }
    }
    catch (const std::regex_error& err) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Regex matching failed with an error");
    }

    return input.begin() + ranges::distance(input.data(), matches[0].second);
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
    auto re =
#if SCN_REGEX_BOOST_USE_ICU
        boost::make_u32regex(pattern.data(), pattern.data() + pattern.size(),
                             make_regex_flags(flags) |
                                 boost::regex_constants::no_except |
                                 boost::regex_constants::nosubs);
#else
        boost::basic_regex<CharT>{pattern.data(), pattern.size(),
                                  make_regex_flags(flags) |
                                      boost::regex_constants::no_except |
                                      boost::regex_constants::nosubs};
#endif
    if (re.status() != 0) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Invalid regex");
    }

    boost::match_results<const CharT*> matches{};
    try {
        bool found =
#if SCN_REGEX_BOOST_USE_ICU
            boost::u32regex_search(input.data(), input.data() + input.size(),
                                   matches, re,
                                   boost::regex_constants::match_continuous);
#else
            boost::regex_search(input.data(), input.data() + input.size(),
                                matches, re,
                                boost::regex_constants::match_continuous);
#endif
        if (!found || matches.prefix().matched) {
            return unexpected_scan_error(scan_error::invalid_scanned_value,
                                         "Regular expression didn't match");
        }
    }
    catch (const std::runtime_error& err) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Regex matching failed with an error");
    }

    return input.begin() + ranges::distance(input.data(), matches[0].second);
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
    static_assert(std::is_same_v<CharT, char>);
    std::string flagged_pattern{};
    auto re = [&]() {
        auto [opts, flagstr] = make_regex_flags(flags);
        opts.set_never_capture(true);
        if (flagstr.empty()) {
            return re2::RE2{pattern, opts};
        }
        flagged_pattern.reserve(flagstr.size() + pattern.size());
        flagged_pattern.append(flagstr);
        flagged_pattern.append(pattern);
        return re2::RE2{flagged_pattern, opts};
    }();
    if (!re.ok()) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Failed to parse regular expression");
    }

    auto new_input = detail::make_string_view_from_pointers(
        detail::to_address(input.begin()), detail::to_address(input.end()));
    bool found = re2::RE2::Consume(&new_input, re);
    if (!found) {
        return unexpected_scan_error(scan_error::invalid_scanned_value,
                                     "Regular expression didn't match");
    }
    return input.begin() + ranges::distance(input.data(), new_input.data());
#endif  // SCN_REGEX_BACKEND == ...
}

template <typename CharT, typename Input>
auto read_regex_matches_impl(std::basic_string_view<CharT> pattern,
                             detail::regex_flags flags,
                             Input input,
                             basic_regex_matches<CharT>& value)
    -> scan_expected<ranges::iterator_t<Input>>
{
    static_assert(ranges::contiguous_range<Input> &&
                  ranges::borrowed_range<Input> &&
                  std::is_same_v<ranges::range_value_t<Input>, CharT>);

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
    std::basic_regex<CharT> re{};
    try {
        SCN_TRY(re_flags, make_regex_flags(flags));
        re = std::basic_regex<CharT>{pattern.data(), pattern.size(), re_flags};
    }
    catch (const std::regex_error& err) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Invalid regex");
    }

    std::match_results<const CharT*> matches{};
    try {
        bool found = std::regex_search(input.data(),
                                       input.data() + input.size(), matches, re,
                                       std::regex_constants::match_continuous);
        if (!found || matches.prefix().matched) {
            return unexpected_scan_error(scan_error::invalid_scanned_value,
                                         "Regular expression didn't match");
        }
    }
    catch (const std::regex_error& err) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Regex matching failed with an error");
    }

    value.resize(matches.size());
    ranges::transform(
        matches, value.begin(),
        [](auto&& match) -> std::optional<basic_regex_match<CharT>> {
            if (!match.matched)
                return std::nullopt;
            return detail::make_string_view_from_pointers(match.first,
                                                          match.second);
        });
    return input.begin() + ranges::distance(input.data(), matches[0].second);
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
    std::vector<std::basic_string<CharT>> names;
    for (size_t i = 0; i < pattern.size();) {
        if constexpr (std::is_same_v<CharT, char>) {
            i = pattern.find("(?<", i);
        }
        else {
            i = pattern.find(L"(?<", i);
        }

        if (i == std::basic_string_view<CharT>::npos) {
            break;
        }
        if (i > 0 && pattern[i - 1] == CharT{'\\'}) {
            if (i == 1 || pattern[i - 2] != CharT{'\\'}) {
                i += 3;
                continue;
            }
        }

        i += 3;
        auto end_i = pattern.find(CharT{'>'}, i);
        if (end_i == std::basic_string_view<CharT>::npos) {
            break;
        }
        names.emplace_back(pattern.substr(i, end_i - i));
    }

    auto re =
#if SCN_REGEX_BOOST_USE_ICU
        boost::make_u32regex(
            pattern.data(), pattern.data() + pattern.size(),
            make_regex_flags(flags) | boost::regex_constants::no_except);
#else
        boost::basic_regex<CharT>{
            pattern.data(), pattern.size(),
            make_regex_flags(flags) | boost::regex_constants::no_except};
#endif
    if (re.status() != 0) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Invalid regex");
    }

    boost::match_results<const CharT*> matches{};
    try {
        bool found =
#if SCN_REGEX_BOOST_USE_ICU
            boost::u32regex_search(input.data(), input.data() + input.size(),
                                   matches, re,
                                   boost::regex_constants::match_continuous);
#else
            boost::regex_search(input.data(), input.data() + input.size(),
                                matches, re,
                                boost::regex_constants::match_continuous);
#endif
        if (!found || matches.prefix().matched) {
            return unexpected_scan_error(scan_error::invalid_scanned_value,
                                         "Regular expression didn't match");
        }
    }
    catch (const std::runtime_error& err) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Regex matching failed with an error");
    }

    value.resize(matches.size());
    ranges::transform(
        matches, value.begin(),
        [&](auto&& match) -> std::optional<basic_regex_match<CharT>> {
            if (!match.matched)
                return std::nullopt;
            auto sv = detail::make_string_view_from_pointers(match.first,
                                                             match.second);

            if (auto name_it = ranges::find_if(
                    names,
                    [&](const auto& name) { return match == matches[name]; });
                name_it != names.end()) {
                return basic_regex_match<CharT>{sv, *name_it};
            }
            return sv;
        });
    return input.begin() + ranges::distance(input.data(), matches[0].second);
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
    static_assert(std::is_same_v<CharT, char>);
    std::string flagged_pattern{};
    auto re = [&]() {
        auto [opts, flagstr] = make_regex_flags(flags);
        if (flagstr.empty()) {
            return re2::RE2{pattern, opts};
        }
        flagged_pattern.reserve(flagstr.size() + pattern.size());
        flagged_pattern.append(flagstr);
        flagged_pattern.append(pattern);
        return re2::RE2{flagged_pattern, opts};
    }();
    if (!re.ok()) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Failed to parse regular expression");
    }
    // TODO: Optimize into a single batch allocation
    const auto max_matches_n =
        static_cast<size_t>(re.NumberOfCapturingGroups());
    std::vector<std::optional<std::string_view>> matches(max_matches_n);
    std::vector<re2::RE2::Arg> match_args(max_matches_n);
    std::vector<re2::RE2::Arg*> match_argptrs(max_matches_n);
    ranges::transform(matches, match_args.begin(),
                      [](auto& val) { return re2::RE2::Arg{&val}; });
    ranges::transform(match_args, match_argptrs.begin(),
                      [](auto& arg) { return &arg; });
    auto new_input = detail::make_string_view_from_pointers(
        detail::to_address(input.begin()), detail::to_address(input.end()));
    bool found = re2::RE2::ConsumeN(&new_input, re, match_argptrs.data(),
                                    match_argptrs.size());
    if (!found) {
        return unexpected_scan_error(scan_error::invalid_scanned_value,
                                     "Regular expression didn't match");
    }
    value.resize(matches.size() + 1);
    value[0] =
        detail::make_string_view_from_pointers(input.data(), new_input.data());
    ranges::transform(matches, value.begin() + 1,
                      [&](auto&& match) -> std::optional<regex_match> {
                          if (!match)
                              return std::nullopt;
                          return *match;
                      });
    {
        const auto& capturing_groups = re.CapturingGroupNames();
        for (size_t i = 1; i < value.size(); ++i) {
            if (auto it = capturing_groups.find(static_cast<int>(i));
                it != capturing_groups.end()) {
                auto val = value[i]->get();
                value[i].emplace(val, it->second);
            };
        }
    }
    return input.begin() + ranges::distance(input.data(), new_input.data());
#endif  // SCN_REGEX_BACKEND == ...
}

inline std::string get_unescaped_regex_pattern(std::string_view pattern)
{
    std::string result{pattern};
    for (size_t n = 0; (n = result.find("\\/", n)) != std::string::npos;) {
        result.replace(n, 2, "/");
        ++n;
    }
    return result;
}
inline std::wstring get_unescaped_regex_pattern(std::wstring_view pattern)
{
    std::wstring result{pattern};
    for (size_t n = 0; (n = result.find(L"\\/", n)) != std::wstring::npos;) {
        result.replace(n, 2, L"/");
        ++n;
    }
    return result;
}

#endif  // !SCN_DISABLE_REGEX

template <typename SourceCharT>
struct regex_matches_reader
    : public reader_base<regex_matches_reader<SourceCharT>, SourceCharT> {
    void check_specs_impl(const detail::format_specs& specs,
                          reader_error_handler& eh)
    {
        detail::check_regex_type_specs(specs, eh);
        SCN_EXPECT(specs.charset_string_data != nullptr);
        SCN_EXPECT(specs.charset_string_size > 0);
    }

    template <typename Range, typename DestCharT>
    scan_expected<simple_borrowed_iterator_t<Range>> read_default(
        Range&&,
        basic_regex_matches<DestCharT>&,
        detail::locale_ref = {})
    {
        return unexpected_scan_error(
            scan_error::invalid_format_string,
            "No regex given in format string for scanning regex_matches");
    }

    template <typename Range, typename DestCharT>
    scan_expected<simple_borrowed_iterator_t<Range>> read_specs(
        Range&& range,
        const detail::format_specs& specs,
        basic_regex_matches<DestCharT>& value,
        detail::locale_ref = {})
    {
        if constexpr (!std::is_same_v<SourceCharT, DestCharT>) {
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
            if (!is_entire_source_contiguous(range)) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Cannot use regex with a non-contiguous source "
                    "range");
            }

            auto input = get_as_contiguous(range);
            SCN_TRY(it,
                    impl(input,
                         specs.type == detail::presentation_type::regex_escaped,
                         specs.charset_string<SourceCharT>(),
                         specs.regexp_flags, value));
            return ranges_polyfill::batch_next(
                ranges::begin(range), ranges::distance(input.begin(), it));
        }
    }

private:
    template <typename Range, typename DestCharT>
    auto impl(const Range& input,
              bool is_escaped,
              std::basic_string_view<SourceCharT> pattern,
              detail::regex_flags flags,
              basic_regex_matches<DestCharT>& value)
    {
        if constexpr (detail::is_type_disabled<
                          basic_regex_matches<DestCharT>>) {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }
        else {
            if (is_escaped) {
                return read_regex_matches_impl<SourceCharT>(
                    get_unescaped_regex_pattern(pattern), flags, input, value);
            }
            return read_regex_matches_impl(pattern, flags, input, value);
        }
    }
};

template <typename CharT>
struct reader_impl_for_regex_matches : public regex_matches_reader<CharT> {};
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
