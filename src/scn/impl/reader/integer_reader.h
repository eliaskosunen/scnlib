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

#include <scn/impl/reader/numeric_reader.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
template <typename Iterator>
struct parse_integer_prefix_result {
    SCN_NO_UNIQUE_ADDRESS Iterator iterator;
    int parsed_base{0};
    sign_type sign{sign_type::default_sign};
    bool is_zero{false};
};

template <typename Range>
parse_expected<simple_borrowed_iterator_t<Range>> parse_integer_bin_base_prefix(
    Range&& range)
{
    return read_matching_string_classic_nocase(SCN_FWD(range), "0b");
}

template <typename Range>
parse_expected<simple_borrowed_iterator_t<Range>> parse_integer_hex_base_prefix(
    Range&& range)
{
    return read_matching_string_classic_nocase(SCN_FWD(range), "0x");
}

template <typename Range>
parse_expected<simple_borrowed_iterator_t<Range>> parse_integer_oct_base_prefix(
    Range&& range,
    bool& zero_parsed)
{
    if (auto r = read_matching_string_classic_nocase(range, "0o")) {
        return *r;
    }

    if (auto r = read_matching_code_unit(range, '0')) {
        zero_parsed = true;
        return *r;
    }

    return unexpected(parse_error::error);
}

template <typename Range>
std::tuple<simple_borrowed_iterator_t<Range>, int, bool>
parse_integer_base_prefix_for_detection(Range&& range)
{
    if (auto r = parse_integer_hex_base_prefix(range)) {
        return {*r, 16, false};
    }
    if (auto r = parse_integer_bin_base_prefix(range)) {
        return {*r, 2, false};
    }
    {
        bool zero_parsed{false};
        if (auto r = parse_integer_oct_base_prefix(range, zero_parsed)) {
            return {*r, 8, zero_parsed};
        }
    }
    return {ranges::begin(range), 10, false};
}

template <typename Range>
std::tuple<simple_borrowed_iterator_t<Range>, int, bool>
parse_integer_base_prefix(Range&& range, int base)
{
    switch (base) {
        case 2:
            // allow 0b/0B
            return {apply_opt(parse_integer_bin_base_prefix(range), range), 2,
                    false};

        case 8: {
            // allow 0o/0O/0
            bool zero_parsed = false;
            auto it = apply_opt(
                parse_integer_oct_base_prefix(range, zero_parsed), range);
            return {it, 8, zero_parsed};
        }

        case 16:
            // allow 0x/0X
            return {apply_opt(parse_integer_hex_base_prefix(range), range), 16,
                    false};

        case 0:
            // detect base
            return parse_integer_base_prefix_for_detection(SCN_FWD(range));

        default:
            // no base prefix allowed
            return {ranges::begin(range), base, false};
    }
}

template <typename Range>
auto parse_integer_prefix(Range range, int base)
    -> eof_expected<parse_integer_prefix_result<ranges::iterator_t<Range>>>
{
    SCN_TRY(sign_result, parse_numeric_sign(range));
    auto [base_prefix_begin_it, sign] = sign_result;

    auto [digits_begin_it, parsed_base, parsed_zero] =
        parse_integer_base_prefix(
            ranges::subrange{base_prefix_begin_it, ranges::end(range)}, base);

    if (parsed_zero) {
        if (digits_begin_it == ranges::end(range) ||
            char_to_int(*digits_begin_it) >= 8) {
            digits_begin_it = ranges_polyfill::prev_backtrack(
                digits_begin_it, ranges::begin(range));
        }
        else {
            parsed_zero = false;
        }
    }
    else {
        if (digits_begin_it == ranges::end(range) ||
            char_to_int(*digits_begin_it) >= parsed_base) {
            digits_begin_it = base_prefix_begin_it;
        }
    }

    if (sign == sign_type::default_sign) {
        sign = sign_type::plus_sign;
    }
    return parse_integer_prefix_result<ranges::iterator_t<Range>>{
        digits_begin_it, parsed_base, sign, parsed_zero};
}

template <typename Range>
auto parse_integer_digits_without_thsep(Range range, int base)
    -> scan_expected<ranges::iterator_t<Range>>
{
    using char_type = detail::char_t<Range>;

    if constexpr (ranges::contiguous_range<Range>) {
        if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
            return unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Failed to parse integer: No digits found");
        }
        return ranges::end(range);
    }
    else {
        return read_while1_code_unit(
                   range, [&](char_type ch)
                              SCN_NOEXCEPT { return char_to_int(ch) < base; })
            .transform_error(map_parse_error_to_scan_error(
                scan_error::invalid_scanned_value,
                "Failed to parse integer: No digits found"));
    }
}

template <typename Range, typename CharT>
auto parse_integer_digits_with_thsep(
    Range range,
    int base,
    const localized_number_formatting_options<CharT>& locale_options)
    -> scan_expected<std::tuple<ranges::iterator_t<Range>,
                                std::basic_string<CharT>,
                                std::string>>
{
    std::basic_string<CharT> output;
    std::string thsep_indices;
    auto it = ranges::begin(range);
    bool digit_matched = false;
    for (; it != ranges::end(range); ++it) {
        if (*it == locale_options.thousands_sep) {
            thsep_indices.push_back(static_cast<char>(
                ranges_polyfill::pos_distance(ranges::begin(range), it)));
        }
        else if (char_to_int(*it) >= base) {
            break;
        }
        else {
            output.push_back(*it);
            digit_matched = true;
        }
    }
    if (SCN_UNLIKELY(!digit_matched)) {
        return unexpected_scan_error(
            scan_error::invalid_scanned_value,
            "Failed to parse integer: No digits found");
    }
    return std::tuple{it, output, thsep_indices};
}

template <typename CharT, typename T>
auto parse_integer_value(std::basic_string_view<CharT> source,
                         T& value,
                         sign_type sign,
                         int base)
    -> scan_expected<typename std::basic_string_view<CharT>::iterator>;

template <typename T>
void parse_integer_value_exhaustive_valid(std::string_view source, T& value);

#define SCN_DECLARE_INTEGER_READER_TEMPLATE(CharT, IntT)                    \
    extern template auto parse_integer_value(                               \
        std::basic_string_view<CharT> source, IntT& value, sign_type sign,  \
        int base)                                                           \
        -> scan_expected<typename std::basic_string_view<CharT>::iterator>; \
    extern template void parse_integer_value_exhaustive_valid(              \
        std::string_view, IntT&);

#if !SCN_DISABLE_TYPE_SCHAR
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, signed char)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, signed char)
#endif
#if !SCN_DISABLE_TYPE_SHORT
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, short)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, short)
#endif
#if !SCN_DISABLE_TYPE_INT
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, int)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, int)
#endif
#if !SCN_DISABLE_TYPE_LONG
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, long)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, long)
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, long long)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, long long)
#endif
#if !SCN_DISABLE_TYPE_UCHAR
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, unsigned char)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, unsigned char)
#endif
#if !SCN_DISABLE_TYPE_USHORT
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, unsigned short)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, unsigned short)
#endif
#if !SCN_DISABLE_TYPE_UINT
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, unsigned int)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, unsigned int)
#endif
#if !SCN_DISABLE_TYPE_ULONG
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, unsigned long)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long)
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, unsigned long long)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long long)
#endif

#undef SCN_DECLARE_INTEGER_READER_TEMPLATE

template <typename CharT>
class reader_impl_for_int
    : public reader_base<reader_impl_for_int<CharT>, CharT> {
public:
    constexpr reader_impl_for_int() = default;

    void check_specs_impl(const detail::format_specs& specs,
                          reader_error_handler& eh)
    {
        detail::check_int_type_specs(specs, eh);
    }

    template <typename Range, typename T>
    scan_expected<simple_borrowed_iterator_t<Range>>
    read_default_with_base(Range&& range, T& value, int base)
    {
        SCN_TRY(prefix_result, parse_integer_prefix(range, base)
                                   .transform_error(make_eof_scan_error));

        if constexpr (!std::is_signed_v<T>) {
            if (prefix_result.sign == sign_type::minus_sign) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Unexpected '-' sign when parsing an "
                    "unsigned value");
            }
        }

        if (prefix_result.is_zero) {
            value = T{0};
            return std::next(prefix_result.iterator);
        }

        SCN_TRY(after_digits_it, parse_integer_digits_without_thsep(
                                     ranges::subrange{prefix_result.iterator,
                                                      ranges::end(range)},
                                     prefix_result.parsed_base));

        auto buf = make_contiguous_buffer(
            ranges::subrange{prefix_result.iterator, after_digits_it});
        SCN_TRY(result_it,
                parse_integer_value(buf.view(), value, prefix_result.sign,
                                    prefix_result.parsed_base));

        return ranges_polyfill::batch_next(
            prefix_result.iterator,
            ranges::distance(buf.view().begin(), result_it));
    }

    template <typename Range, typename T>
    scan_expected<simple_borrowed_iterator_t<Range>>
    read_default(Range&& range, T& value, detail::locale_ref loc)
    {
        SCN_UNUSED(loc);
        return read_default_with_base(range, value, 0);
    }

    template <typename Range, typename T>
    scan_expected<simple_borrowed_iterator_t<Range>> read_specs(
        Range&& range,
        const detail::format_specs& specs,
        T& value,
        detail::locale_ref loc)
    {
        SCN_TRY(prefix_result, parse_integer_prefix(range, specs.get_base(0))
                                   .transform_error(make_eof_scan_error));

        if (prefix_result.sign == sign_type::minus_sign) {
            if constexpr (!std::is_signed_v<T>) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Unexpected '-' sign when parsing an "
                    "unsigned value");
            }
            else {
                if (specs.type ==
                    detail::presentation_type::int_unsigned_decimal) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "'u'-option disallows negative values");
                }
            }
        }

        if (prefix_result.is_zero) {
            value = T{0};
            return std::next(prefix_result.iterator);
        }

        if (SCN_LIKELY(!specs.localized)) {
            SCN_TRY(after_digits_it,
                    parse_integer_digits_without_thsep(
                        ranges::subrange{prefix_result.iterator,
                                         ranges::end(range)},
                        prefix_result.parsed_base));

            auto buf = make_contiguous_buffer(
                ranges::subrange{prefix_result.iterator, after_digits_it});
            SCN_TRY(result_it,
                    parse_integer_value(buf.view(), value, prefix_result.sign,
                                        prefix_result.parsed_base));

            return ranges_polyfill::batch_next(
                prefix_result.iterator,
                ranges::distance(buf.view().begin(), result_it));
        }

        auto locale_options =
#if SCN_DISABLE_LOCALE
            localized_number_formatting_options<CharT>{};
#else
            localized_number_formatting_options<CharT>{loc};
#endif

        SCN_TRY(
            parse_digits_result,
            parse_integer_digits_with_thsep(
                ranges::subrange{prefix_result.iterator, ranges::end(range)},
                prefix_result.parsed_base, locale_options));
        const auto& [after_digits_it, nothsep_source, thsep_indices] =
            parse_digits_result;

        if (!thsep_indices.empty()) {
            if (auto e = check_thsep_grouping(
                    ranges::subrange{prefix_result.iterator, after_digits_it},
                    thsep_indices, locale_options.grouping);
                SCN_UNLIKELY(!e)) {
                return unexpected(e);
            }
        }

        auto nothsep_source_view =
            std::basic_string_view<CharT>{nothsep_source};
        SCN_TRY(
            nothsep_source_it,
            parse_integer_value(nothsep_source_view, value, prefix_result.sign,
                                prefix_result.parsed_base));

        return ranges_polyfill::batch_next(
            prefix_result.iterator,
            ranges::distance(nothsep_source_view.begin(), nothsep_source_it) +
                ranges::ssize(thsep_indices));
    }
};
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
