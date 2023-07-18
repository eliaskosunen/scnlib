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
#include <limits>
#include "scn/external/nanorange/nanorange.hpp"
#include "scn/impl/algorithms/common.h"
#include "scn/impl/algorithms/read_nocopy.h"
#include "scn/impl/util/ascii_ctype.h"
#include "scn/util/expected.h"
#include "scn/util/expected_impl.h"

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
            explicit constexpr float_reader_base(unsigned opt) : m_options(opt)
            {
            }

        protected:
            unsigned m_options{allow_hex | allow_scientific | allow_fixed};
        };

        template <typename CharT>
        class float_reader : public numeric_reader<float_reader<CharT>, CharT>,
                             public float_reader_base {
        public:
            using char_type = CharT;

            constexpr float_reader() = default;

            explicit constexpr float_reader(unsigned opt)
                : float_reader_base(opt)
            {
            }

            template <typename T, typename Range>
            SCN_NODISCARD scan_expected<ranges::borrowed_iterator_t<Range>>
            read_source(detail::tag_type<T>, Range&& range)
            {
                auto digits_begin = ranges::begin(range);
                return numeric_reader_base::read_sign(range, m_sign)
                    .and_then([&](auto it) {
                        digits_begin = it;
                        return read_source_impl(
                            ranges::subrange{it, ranges::end(range)});
                    })
                    .transform([&](auto it) {
                        SCN_EXPECT(m_kind != float_kind::tbd);
                        if (m_nonbuffer_value_len < 0) {
                            this->m_buffer.assign(
                                ranges::subrange{digits_begin, it});
                        }

                        return it;
                    });
            }

            template <typename T>
            SCN_NODISCARD scan_expected<std::ptrdiff_t> parse_value(T& value)
            {
                SCN_EXPECT(m_kind != float_kind::tbd);

                const std::ptrdiff_t sign_len =
                    m_sign != numeric_reader_base::sign::default_sign ? 1 : 0;

                if (m_nonbuffer_value_len < 0) {
                    store_nonbuffer(value);
                    return m_nonbuffer_value_len + sign_len;
                }

                return parse_value_impl(value).transform(
                    [&](auto n) { return n + sign_len; });
            }

        private:
            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_dec_digits(
                Range&& range,
                CharT thsep = 0)
            {
                if (SCN_UNLIKELY(thsep != 0)) {
                    return read_while_code_unit(
                        SCN_FWD(range), [=](char_type ch) {
                            return numeric_reader_base::char_to_int(ch) < 10 ||
                                   ch == thsep;
                        });
                }

                return read_while_code_unit(SCN_FWD(range), [](char_type ch) {
                    return numeric_reader_base::char_to_int(ch) < 10;
                });
            }
            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_hex_digits(
                Range&& range,
                CharT thsep = 0)
            {
                if (SCN_UNLIKELY(thsep != 0)) {
                    return read_while_code_unit(
                        SCN_FWD(range), [=](char_type ch) {
                            return numeric_reader_base::char_to_int(ch) < 16 ||
                                   ch == thsep;
                        });
                }

                return read_while_code_unit(SCN_FWD(range), [](char_type ch) {
                    return numeric_reader_base::char_to_int(ch) < 16;
                });
            }
            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_hex_prefix(
                Range&& range)
            {
                return read_matching_string_nocase(SCN_FWD(range), "0x");
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_inf(
                Range&& range)
            {
                auto it = ranges::begin(range);
                if (auto r = read_matching_string_nocase(range, "inf"); !r) {
                    return unexpected(r.error());
                }
                else {
                    it = *r;
                }

                if (auto r = read_matching_string_nocase(range, "inity"); !r) {
                    m_kind = float_kind::inf_short;
                    m_nonbuffer_value_len = 3;
                    return it;
                }
                else {
                    m_kind = float_kind::inf_long;
                    m_nonbuffer_value_len = 8;
                    return *r;
                }
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_nan(
                Range&& range)
            {
                auto it = ranges::begin(range);
                auto nan_beg_it = it;
                if (auto r = read_matching_string_nocase(range, "nan"); !r) {
                    return unexpected(r.error());
                }
                else {
                    it = *r;
                }

                if (auto r = read_matching_code_unit(
                        ranges::subrange{it, ranges::end(range)}, '(');
                    !r) {
                    m_kind = float_kind::nan_simple;
                    m_nonbuffer_value_len = 3;
                    return it;
                }
                else {
                    it = *r;
                }

                if (auto r = read_while_code_unit(
                        ranges::subrange{it, ranges::end(range)},
                        [](char_type ch) {
                            return is_ascii_char(ch) &&
                                   ((ch >= '0' && ch <= '9') ||
                                    (ch >= 'a' && ch <= 'z') ||
                                    (ch >= 'A' && ch <= 'Z') || ch == '_');
                        })) {
                    it = *r;
                }

                m_kind = float_kind::nan_with_payload;
                if (auto r = read_matching_code_unit(
                        ranges::subrange{it, ranges::end(range)}, ')')) {
                    m_nonbuffer_value_len = ranges::distance(nan_beg_it, *r);
                    return *r;
                }
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Invalid NaN payload");
            }

            template <typename Range>
            ranges::borrowed_iterator_t<Range> read_exponent(
                Range&& range,
                std::string_view exp)
            {
                if (auto r = read_one_of_code_unit(range, exp)) {
                    auto beg_exp_it = ranges::begin(range);
                    auto it = *r;
                    numeric_reader_base::sign exp_sign{
                        numeric_reader_base::sign::default_sign};

                    if (auto r_sign = numeric_reader_base::read_sign(
                            ranges::subrange{it, ranges::end(range)},
                            exp_sign)) {
                        it = *r_sign;
                    }

                    if (auto r_exp = read_while1_code_unit(
                            ranges::subrange{it, ranges::end(range)},
                            [](char_type ch) {
                                return numeric_reader_base::char_to_int(ch) <
                                       10;
                            });
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
            scan_expected<ranges::borrowed_iterator_t<Range>> read_hexfloat(
                Range&& range)
            {
                auto it = ranges::begin(range);

                std::ptrdiff_t digits_count = 0;
                if (auto r = read_hex_digits(
                        ranges::subrange{it, ranges::end(range)});
                    SCN_UNLIKELY(!r)) {
                    return unexpected(r.error());
                }
                else {
                    digits_count += ranges::distance(it, *r);
                    it = *r;
                }

                if (auto r = read_matching_code_unit(
                        ranges::subrange{it, ranges::end(range)}, '.')) {
                    it = *r;
                }

                if (auto r = read_hex_digits(
                        ranges::subrange{it, ranges::end(range)})) {
                    digits_count += ranges::distance(it, *r);
                    it = *r;
                }

                if (SCN_UNLIKELY(digits_count == 0)) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "No significand digits in hexfloat");
                }

                it = read_exponent(ranges::subrange{it, ranges::end(range)},
                                   "pP");

                return it;
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>>
            read_regular_float(Range&& range)
            {
                const bool allowed_exp = (m_options & allow_scientific) != 0;
                const bool required_exp =
                    allowed_exp && (m_options & allow_fixed) == 0;

                auto it = ranges::begin(range);
                std::ptrdiff_t digits_count = 0;

                if (auto r = read_dec_digits(
                        ranges::subrange{it, ranges::end(range)});
                    SCN_UNLIKELY(!r)) {
                    return unexpected(r.error());
                }
                else {
                    digits_count += ranges::distance(it, *r);
                    it = *r;
                }

                if (auto r = read_matching_code_unit(
                        ranges::subrange{it, ranges::end(range)}, '.')) {
                    it = *r;
                }

                if (auto r = read_dec_digits(
                        ranges::subrange{it, ranges::end(range)})) {
                    digits_count += ranges::distance(it, *r);
                    it = *r;
                }

                if (SCN_UNLIKELY(digits_count == 0)) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "No significand digits in float");
                }

                auto beg_exp_it = it;
                if (allowed_exp) {
                    it = read_exponent(ranges::subrange{it, ranges::end(range)},
                                       "eE");
                }
                if (required_exp && beg_exp_it == it) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "No exponent given to scientific float");
                }

                m_kind = (beg_exp_it == it) ? float_kind::fixed
                                            : float_kind::scientific;

                return it;
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_source_impl(
                Range&& range)
            {
                const bool allowed_hex = (m_options & allow_hex) != 0;
                const bool allowed_nonhex =
                    (m_options & ~allow_thsep & ~allow_hex) != 0;
                const bool allowed_thsep = (m_options & allow_thsep) != 0;

                if (auto r = read_inf(range); !r && m_kind != float_kind::tbd) {
                    return unexpected(r.error());
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

                    return read_hexfloat(
                        ranges::subrange{it, ranges::end(range)});
                }
                else if (!allowed_hex && allowed_nonhex) {
                    // only nonhex allowed:
                    // no prefix allowed
                    return read_regular_float(SCN_FWD(range));
                }
                else {
                    // both hex and nonhex allowed:
                    // check for "0x" prefix -> hex,
                    // regular otherwise

                    if (auto r = read_hex_prefix(range); SCN_UNLIKELY(r)) {
                        m_kind = float_kind::hex_with_prefix;
                        return read_hexfloat(
                            ranges::subrange{*r, ranges::end(range)});
                    }

                    return read_regular_float(SCN_FWD(range));
                }
            }

            template <typename T>
            T setsign(T value) const
            {
                SCN_EXPECT(value >= static_cast<T>(0.0));
                if (m_sign == numeric_reader_base::sign::minus_sign) {
                    return -value;
                }
                return value;
            }

            template <typename T>
            void store_nonbuffer(T& value) const
            {
                switch (m_kind) {
                    case float_kind::inf_short:
                        SCN_EXPECT(m_nonbuffer_value_len == 3);
                        value = setsign(std::numeric_limits<T>::infinity());
                        break;

                    case float_kind::inf_long:
                        SCN_EXPECT(m_nonbuffer_value_len == 8);
                        value = setsign(std::numeric_limits<T>::infinity());
                        break;

                    case float_kind::nan_simple:
                        SCN_EXPECT(m_nonbuffer_value_len == 3);
                        value = setsign(std::numeric_limits<T>::quiet_NaN());
                        break;

                    case float_kind::nan_with_payload:
                        SCN_EXPECT(m_nonbuffer_value_len >= 5);
                        value = setsign(std::numeric_limits<T>::quiet_NaN());
                        break;

                    default:
                        SCN_EXPECT(false);
                        SCN_UNREACHABLE;
                }
            }

            template <typename T>
            scan_expected<std::ptrdiff_t> parse_value_impl(T& value);

            std::ptrdiff_t m_nonbuffer_value_len{-1};
            numeric_reader_base::sign m_sign{
                numeric_reader_base::sign::default_sign};
            float_kind m_kind{float_kind::tbd};
        };

#define SCN_DECLARE_FLOAT_READER_TEMPLATE_IMPL(CharT, FloatT)           \
    extern template auto float_reader<CharT>::parse_value_impl(FloatT&) \
        ->scan_expected<std::ptrdiff_t>;

#define SCN_DECLARE_FLOAT_READER_TEMPLATE(CharT)          \
    SCN_DECLARE_FLOAT_READER_TEMPLATE_IMPL(CharT, float)  \
    SCN_DECLARE_FLOAT_READER_TEMPLATE_IMPL(CharT, double) \
    SCN_DECLARE_FLOAT_READER_TEMPLATE_IMPL(CharT, long double)

        SCN_DECLARE_FLOAT_READER_TEMPLATE(char)
        SCN_DECLARE_FLOAT_READER_TEMPLATE(wchar_t)

#undef SCN_DECLARE_FLOAT_READER_TEMPLATE
#undef SCN_DECLARE_FLOAT_READER_TEMPLATE_IMPL
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
