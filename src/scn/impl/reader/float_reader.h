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

#include <cmath>
#include <limits>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
struct float_reader_base {
    enum options_type {
        allow_hex = 1,
        allow_scientific = 2,
        allow_fixed = 4,
        allow_thsep = 8
    };

    enum class float_kind {
        tbd = 0,
        generic,             // fixed or scientific
        fixed,               // xxx.yyy
        scientific,          // xxx.yyyEzzz
        hex_without_prefix,  // xxx.yyypzzz
        hex_with_prefix,     // 0Xxxx.yyypzzz
        inf_short,           // inf
        inf_long,            // infinity
        nan_simple,          // nan
        nan_with_payload,    // nan(xxx)
    };

    constexpr float_reader_base() = default;
    explicit constexpr float_reader_base(unsigned opt) : m_options(opt) {}

protected:
    unsigned m_options{allow_hex | allow_scientific | allow_fixed};
};

template <typename CharT>
class float_reader : public numeric_reader<CharT>, public float_reader_base {
    using numeric_base = numeric_reader<CharT>;

public:
    using char_type = CharT;

    constexpr float_reader() = default;

    explicit constexpr float_reader(unsigned opt) : float_reader_base(opt) {}

    template <typename Range>
    SCN_NODISCARD scan_expected<ranges::iterator_t<Range>> read_source(
        Range range,
        detail::locale_ref)
    {
        if (SCN_UNLIKELY(m_options & float_reader_base::allow_thsep)) {
            m_locale_options = localized_number_formatting_options<CharT>{
                classic_with_thsep_tag{}};
        }

        return read_source_impl(range);
    }

#if !SCN_DISABLE_LOCALE
    template <typename Range>
    SCN_NODISCARD scan_expected<ranges::iterator_t<Range>>
    read_source_localized(Range range, detail::locale_ref loc)
    {
        m_locale_options = localized_number_formatting_options<CharT>{loc};
        if (SCN_LIKELY((m_options & float_reader_base::allow_thsep) == 0)) {
            m_locale_options.thousands_sep = CharT{0};
        }

        return read_source_impl(range);
    }
#endif

    template <typename T>
    SCN_NODISCARD scan_expected<std::ptrdiff_t> parse_value(T& value)
    {
        SCN_EXPECT(m_kind != float_kind::tbd);

        const std::ptrdiff_t sign_len =
            m_sign != sign_type::default_sign ? 1 : 0;

        SCN_TRY(n, parse_value_impl(value));
        return n + sign_len + ranges::ssize(m_thsep_indices);
    }

private:
    template <typename Range>
    scan_expected<simple_borrowed_iterator_t<Range>> read_source_impl(
        Range&& range)
    {
        SCN_TRY(sign_result,
                parse_numeric_sign(range).transform_error(make_eof_scan_error));
        auto it = sign_result.first;
        m_sign = sign_result.second;

        auto digits_begin = it;
        auto r = ranges::subrange{it, ranges::end(range)};
        if constexpr (ranges::contiguous_range<Range> &&
                      ranges::sized_range<Range>) {
            if (SCN_UNLIKELY(m_locale_options.thousands_sep != 0 ||
                             m_locale_options.decimal_point != CharT{'.'})) {
                SCN_TRY_ASSIGN(
                    it,
                    do_read_source_impl(
                        r,
                        [&](auto&& rr) {
                            return read_regular_float(SCN_FWD(rr));
                        },
                        [&](auto&& rr) { return read_hexfloat(SCN_FWD(rr)); }));
            }
            else {
                auto cb = [&](auto&& rr)
                    -> scan_expected<simple_borrowed_iterator_t<decltype(rr)>> {
                    auto res = read_all(rr);
                    if (SCN_UNLIKELY(res == ranges::begin(r))) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "Invalid float value");
                    }
                    return res;
                };
                SCN_TRY_ASSIGN(it, do_read_source_impl(r, cb, cb));
            }
        }
        else {
            SCN_TRY_ASSIGN(
                it,
                do_read_source_impl(
                    r,
                    [&](auto&& rr) { return read_regular_float(SCN_FWD(rr)); },
                    [&](auto&& rr) { return read_hexfloat(SCN_FWD(rr)); }));
        }

        SCN_EXPECT(m_kind != float_kind::tbd);

        if (m_kind != float_kind::inf_short && m_kind != float_kind::inf_long &&
            m_kind != float_kind::nan_simple &&
            m_kind != float_kind::nan_with_payload) {
            this->m_buffer.assign(ranges::subrange{digits_begin, it});
        }

        handle_separators();

        if (!m_thsep_indices.empty()) {
            SCN_EXPECT(m_integral_part_length >= 0);
            if (auto e = check_thsep_grouping(
                    ranges::subrange{
                        digits_begin,
                        ranges::next(digits_begin, m_integral_part_length)},
                    m_thsep_indices, m_locale_options.grouping);
                SCN_UNLIKELY(!e)) {
                return unexpected(e);
            }
        }

        return it;
    }

    template <typename Range>
    parse_expected<simple_borrowed_iterator_t<Range>> read_dec_digits(
        Range&& range,
        bool thsep_allowed)
    {
        if (SCN_UNLIKELY(m_locale_options.thousands_sep != 0 &&
                         thsep_allowed)) {
            return read_while1_code_unit(
                SCN_FWD(range), [&](char_type ch) SCN_NOEXCEPT {
                    return char_to_int(ch) < 10 ||
                           ch == m_locale_options.thousands_sep;
                });
        }

        return read_while1_code_unit(
            SCN_FWD(range),
            [](char_type ch) SCN_NOEXCEPT { return char_to_int(ch) < 10; });
    }
    template <typename Range>
    parse_expected<simple_borrowed_iterator_t<Range>> read_hex_digits(
        Range&& range,
        bool thsep_allowed)
    {
        if (SCN_UNLIKELY(m_locale_options.thousands_sep != 0 &&
                         thsep_allowed)) {
            return read_while1_code_unit(
                SCN_FWD(range), [&](char_type ch) SCN_NOEXCEPT {
                    return char_to_int(ch) < 16 ||
                           ch == m_locale_options.thousands_sep;
                });
        }

        return read_while1_code_unit(
            SCN_FWD(range),
            [](char_type ch) SCN_NOEXCEPT { return char_to_int(ch) < 16; });
    }
    template <typename Range>
    parse_expected<simple_borrowed_iterator_t<Range>> read_hex_prefix(
        Range&& range)
    {
        return read_matching_string_classic_nocase(SCN_FWD(range), "0x");
    }

    template <typename Range>
    parse_expected<simple_borrowed_iterator_t<Range>> read_inf(Range&& range)
    {
        auto it = ranges::begin(range);
        if (auto r = read_matching_string_classic_nocase(range, "inf"); !r) {
            return unexpected(r.error());
        }
        else {
            it = *r;
        }

        if (auto r = read_matching_string_classic_nocase(
                ranges::subrange{it, ranges::end(range)}, "inity");
            !r) {
            m_kind = float_kind::inf_short;
            return it;
        }
        else {
            m_kind = float_kind::inf_long;
            return *r;
        }
    }

    template <typename Range>
    scan_expected<simple_borrowed_iterator_t<Range>> read_nan(Range&& range)
    {
        auto it = ranges::begin(range);
        if (auto r = read_matching_string_classic_nocase(range, "nan"); !r) {
            return r.transform_error(map_parse_error_to_scan_error(
                scan_error::invalid_scanned_value,
                "Invalid floating-point NaN value"));
        }
        else {
            it = *r;
        }

        if (auto r = read_matching_code_unit(
                ranges::subrange{it, ranges::end(range)}, '(');
            !r) {
            m_kind = float_kind::nan_simple;
            return it;
        }
        else {
            it = *r;
        }

        auto payload_beg_it = it;
        it = read_while_code_unit(
            ranges::subrange{it, ranges::end(range)},
            [](char_type ch) SCN_NOEXCEPT {
                return is_ascii_char(ch) &&
                       ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') ||
                        (ch >= 'A' && ch <= 'Z') || ch == '_');
            });
        m_nan_payload_buffer.assign(ranges::subrange{payload_beg_it, it});

        m_kind = float_kind::nan_with_payload;
        if (auto r = read_matching_code_unit(
                ranges::subrange{it, ranges::end(range)}, ')')) {
            return *r;
        }
        return unexpected_scan_error(scan_error::invalid_scanned_value,
                                     "Invalid floating-point NaN payload");
    }

    template <typename Range>
    simple_borrowed_iterator_t<Range> read_exponent(Range&& range,
                                                    std::string_view exp)
    {
        if (auto r = read_one_of_code_unit(range, exp)) {
            auto beg_exp_it = ranges::begin(range);
            auto it = *r;

            if (auto r_sign = parse_numeric_sign(
                    ranges::subrange{it, ranges::end(range)})) {
                it = r_sign->first;
            }

            if (auto r_exp = read_while1_code_unit(
                    ranges::subrange{it, ranges::end(range)},
                    [](char_type ch)
                        SCN_NOEXCEPT { return char_to_int(ch) < 10; });
                SCN_UNLIKELY(!r_exp)) {
                it = beg_exp_it;
            }
            else {
                it = *r_exp;
            }

            return it;
        }
        else {
            return ranges::begin(range);
        }
    }

    template <typename Range>
    scan_expected<simple_borrowed_iterator_t<Range>> read_hexfloat(
        Range&& range)
    {
        auto it = ranges::begin(range);

        std::ptrdiff_t digits_count = 0;
        if (auto r =
                read_hex_digits(ranges::subrange{it, ranges::end(range)}, true);
            SCN_UNLIKELY(!r)) {
            return r.transform_error(map_parse_error_to_scan_error(
                scan_error::invalid_scanned_value,
                "Invalid hexadecimal floating-point value"));
        }
        else {
            digits_count += ranges::distance(it, *r);
            it = *r;
        }

        m_integral_part_length = digits_count;
        if (auto r = read_matching_code_unit(
                ranges::subrange{it, ranges::end(range)},
                m_locale_options.decimal_point)) {
            it = *r;
        }

        if (auto r = read_hex_digits(ranges::subrange{it, ranges::end(range)},
                                     false)) {
            digits_count += ranges::distance(it, *r);
            it = *r;
        }

        if (SCN_UNLIKELY(digits_count == 0)) {
            return unexpected_scan_error(scan_error::invalid_scanned_value,
                                         "No significand digits in hexfloat");
        }

        it = read_exponent(ranges::subrange{it, ranges::end(range)}, "pP");

        return it;
    }

    template <typename Range>
    scan_expected<simple_borrowed_iterator_t<Range>> read_regular_float(
        Range&& range)
    {
        const bool allowed_exp = (m_options & allow_scientific) != 0;
        const bool required_exp = allowed_exp && (m_options & allow_fixed) == 0;

        auto it = ranges::begin(range);
        std::ptrdiff_t digits_count = 0;

        if (auto r =
                read_dec_digits(ranges::subrange{it, ranges::end(range)}, true);
            SCN_UNLIKELY(!r)) {
            return r.transform_error(
                map_parse_error_to_scan_error(scan_error::invalid_scanned_value,
                                              "Invalid floating-point value"));
        }
        else {
            digits_count += ranges::distance(it, *r);
            it = *r;
        }

        m_integral_part_length = digits_count;
        if (auto r = read_matching_code_unit(
                ranges::subrange{it, ranges::end(range)},
                m_locale_options.decimal_point)) {
            it = *r;
        }

        if (auto r = read_dec_digits(ranges::subrange{it, ranges::end(range)},
                                     false)) {
            digits_count += ranges::distance(it, *r);
            it = *r;
        }

        if (SCN_UNLIKELY(digits_count == 0)) {
            return unexpected_scan_error(scan_error::invalid_scanned_value,
                                         "No significand digits in float");
        }

        auto beg_exp_it = it;
        if (allowed_exp) {
            it = read_exponent(ranges::subrange{it, ranges::end(range)}, "eE");
        }
        if (required_exp && beg_exp_it == it) {
            return unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "No exponent given to scientific float");
        }

        m_kind =
            (beg_exp_it == it) ? float_kind::fixed : float_kind::scientific;

        return it;
    }

    template <typename Range, typename ReadRegular, typename ReadHex>
    scan_expected<simple_borrowed_iterator_t<Range>> do_read_source_impl(
        Range&& range,
        ReadRegular&& read_regular,
        ReadHex&& read_hex)
    {
        const bool allowed_hex = (m_options & allow_hex) != 0;
        const bool allowed_nonhex =
            (m_options & ~static_cast<unsigned>(allow_thsep) &
             ~static_cast<unsigned>(allow_hex)) != 0;

        if (auto r = read_inf(range); !r && m_kind != float_kind::tbd) {
            return r.transform_error(map_parse_error_to_scan_error(
                scan_error::invalid_scanned_value,
                "Invalid infinite floating-point value"));
        }
        else if (r) {
            return *r;
        }

        if (auto r = read_nan(range); !r && m_kind != float_kind::tbd) {
            return unexpected(r.error());
        }
        else if (r) {
            return *r;
        }

        if (allowed_hex && !allowed_nonhex) {
            // only hex allowed:
            // prefix "0x" allowed, not required
            auto it = ranges::begin(range);

            if (auto r = read_hex_prefix(range)) {
                m_kind = float_kind::hex_with_prefix;
                it = *r;
            }
            else {
                m_kind = float_kind::hex_without_prefix;
            }

            return read_hex(ranges::subrange{it, ranges::end(range)});
        }
        else if (!allowed_hex && allowed_nonhex) {
            // only nonhex allowed:
            // no prefix allowed
            m_kind = float_kind::generic;
            return read_regular_float(SCN_FWD(range));
        }
        else {
            // both hex and nonhex allowed:
            // check for "0x" prefix -> hex,
            // regular otherwise

            if (auto r = read_hex_prefix(range); SCN_UNLIKELY(r)) {
                m_kind = float_kind::hex_with_prefix;
                return read_hex(ranges::subrange{*r, ranges::end(range)});
            }

            m_kind = float_kind::generic;
            return read_regular(SCN_FWD(range));
        }
    }

    void handle_separators()
    {
        if (m_locale_options.thousands_sep == 0 &&
            m_locale_options.decimal_point == CharT{'.'}) {
            return;
        }

        auto& str = this->m_buffer.make_into_allocated_string();
        if (m_locale_options.decimal_point != CharT{'.'}) {
            for (auto& ch : str) {
                if (ch == m_locale_options.decimal_point) {
                    ch = CharT{'.'};
                }
            }
        }

        if (m_locale_options.thousands_sep == 0) {
            return;
        }

        auto first = ranges::find(str, m_locale_options.thousands_sep);
        if (first == str.end()) {
            return;
        }

        m_thsep_indices.push_back(
            static_cast<char>(ranges::distance(str.begin(), first)));

        for (auto it = first; ++it != str.end();) {
            if (*it != m_locale_options.thousands_sep) {
                *first++ = std::move(*it);
            }
            else {
                m_thsep_indices.push_back(
                    static_cast<char>(ranges::distance(str.begin(), it)));
            }
        }

        str.erase(first, str.end());
    }

    template <typename T>
    T setsign(T value) const
    {
        SCN_EXPECT(std::isnan(value) || value >= static_cast<T>(0.0));
        if (m_sign == sign_type::minus_sign) {
            return -value;
        }
        return value;
    }

    template <typename T>
    scan_expected<std::ptrdiff_t> parse_value_impl(T& value);

    localized_number_formatting_options<CharT> m_locale_options{};
    std::string m_thsep_indices{};
    contiguous_range_factory<CharT> m_nan_payload_buffer{};
    std::ptrdiff_t m_integral_part_length{-1};
    sign_type m_sign{sign_type::default_sign};
    float_kind m_kind{float_kind::tbd};
};

#define SCN_DECLARE_FLOAT_READER_TEMPLATE(CharT, FloatT)                \
    extern template auto float_reader<CharT>::parse_value_impl(FloatT&) \
        -> scan_expected<std::ptrdiff_t>;

#if !SCN_DISABLE_TYPE_FLOAT
SCN_DECLARE_FLOAT_READER_TEMPLATE(char, float)
SCN_DECLARE_FLOAT_READER_TEMPLATE(wchar_t, float)
#endif
#if !SCN_DISABLE_TYPE_DOUBLE
SCN_DECLARE_FLOAT_READER_TEMPLATE(char, double)
SCN_DECLARE_FLOAT_READER_TEMPLATE(wchar_t, double)
#endif
#if !SCN_DISABLE_TYPE_LONG_DOUBLE
SCN_DECLARE_FLOAT_READER_TEMPLATE(char, long double)
SCN_DECLARE_FLOAT_READER_TEMPLATE(wchar_t, long double)
#endif

#undef SCN_DECLARE_FLOAT_READER_TEMPLATE

template <typename CharT>
class reader_impl_for_float
    : public reader_base<reader_impl_for_float<CharT>, CharT> {
public:
    constexpr reader_impl_for_float() = default;

    void check_specs_impl(const detail::format_specs& specs,
                          reader_error_handler& eh)
    {
        detail::check_float_type_specs(specs, eh);
    }

    template <typename Range, typename T>
    scan_expected<simple_borrowed_iterator_t<Range>>
    read_default(Range&& range, T& value, detail::locale_ref loc)
    {
        SCN_UNUSED(loc);
        using range_nocvref_t = detail::remove_cvref_t<Range>;

        float_reader<CharT> rd{};
        return read_impl<range_nocvref_t>(
            range, rd,
            [](float_reader<CharT>& r, auto&&... args) {
                return r.read_source(SCN_FWD(args)...);
            },
            value);
    }

    template <typename Range, typename T>
    scan_expected<simple_borrowed_iterator_t<Range>> read_specs(
        Range&& range,
        const detail::format_specs& specs,
        T& value,
        detail::locale_ref loc)
    {
        using range_nocvref_t = detail::remove_cvref_t<Range>;
        float_reader<CharT> rd{get_options(specs)};

#if !SCN_DISABLE_LOCALE
        if (specs.localized) {
            return read_impl<range_nocvref_t>(
                range, rd,
                [](float_reader<CharT>& r, auto&&... args) {
                    return r.read_source_localized(SCN_FWD(args)...);
                },
                value, loc);
        }
#endif

        return read_impl<range_nocvref_t>(
            range, rd,
            [](float_reader<CharT>& r, auto&&... args) {
                return r.read_source(SCN_FWD(args)...);
            },
            value);
    }

private:
    template <typename Range>
    using read_source_callback_type =
        scan_expected<ranges::iterator_t<Range>>(float_reader<CharT>&,
                                                 Range,
                                                 detail::locale_ref);

    template <typename Range, typename T>
    scan_expected<ranges::iterator_t<Range>> read_impl(
        Range range,
        float_reader<CharT>& rd,
        function_ref<read_source_callback_type<Range>> read_source_cb,
        T& value,
        detail::locale_ref loc = {})
    {
        if (auto r = std::invoke(read_source_cb, rd, range, loc);
            SCN_UNLIKELY(!r)) {
            return unexpected(r.error());
        }

        SCN_TRY(n, rd.parse_value(value));
        return ranges::next(ranges::begin(range), n);
    }

    static unsigned get_options(const detail::format_specs& specs)
    {
        unsigned options{};
        if (specs.localized) {
            options |= float_reader_base::allow_thsep;
        }

        SCN_GCC_COMPAT_PUSH
        SCN_GCC_COMPAT_IGNORE("-Wswitch-enum")

        switch (specs.type) {
            case detail::presentation_type::float_fixed:
                return options | float_reader_base::allow_fixed;

            case detail::presentation_type::float_scientific:
                return options | float_reader_base::allow_scientific;

            case detail::presentation_type::float_hex:
                return options | float_reader_base::allow_hex;

            case detail::presentation_type::float_general:
                return options | float_reader_base::allow_scientific |
                       float_reader_base::allow_fixed;

            case detail::presentation_type::none:
                return options | float_reader_base::allow_scientific |
                       float_reader_base::allow_fixed |
                       float_reader_base::allow_hex;

            default:
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
        }

        SCN_GCC_COMPAT_POP  // -Wswitch-enum
    }
};
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
