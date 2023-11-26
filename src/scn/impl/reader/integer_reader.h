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
        struct integer_reader_base {
            enum options_type {
                // ' option -> accept thsep (',')
                allow_thsep = 1,
                // 'u' option -> don't allow sign
                only_unsigned = 2,
            };

            constexpr integer_reader_base() = default;
            constexpr integer_reader_base(unsigned opt, int b)
                : m_options(opt), m_base(b)
            {
            }

            template <typename T>
            static constexpr unsigned get_default_options()
            {
                return 0;
            }

        protected:
            unsigned m_options{0};
            int m_base{0};
        };

        template <typename CharT>
        class integer_reader : public numeric_reader<CharT>,
                               public integer_reader_base {
            using numeric_base = numeric_reader<CharT>;

        public:
            using char_type = CharT;

            constexpr integer_reader(unsigned opt, int base)
                : integer_reader_base(opt, base)
            {
            }

            template <typename T>
            constexpr explicit integer_reader(detail::tag_type<T>)
                : integer_reader_base(get_default_options<T>(), 0)
            {
            }

            template <typename Range>
            SCN_NODISCARD scan_expected<ranges::iterator_t<Range>>
            read_source(Range range, bool is_signed, detail::locale_ref = {})
            {
                if (SCN_UNLIKELY(m_options &
                                 integer_reader_base::allow_thsep)) {
                    m_locale_options =
                        localized_number_formatting_options<CharT>{
                            classic_with_thsep_tag{}};

                    return read_source_with_thsep_impl(range, is_signed);
                }

                return read_source_impl(range, is_signed);
            }

#if !SCN_DISABLE_LOCALE
            template <typename Range>
            SCN_NODISCARD scan_expected<ranges::iterator_t<Range>>
            read_source_localized(Range range,
                                  bool is_signed,
                                  detail::locale_ref loc)
            {
                if ((m_options & integer_reader_base::allow_thsep) == 0) {
                    return read_source(range, is_signed);
                }

                m_locale_options =
                    localized_number_formatting_options<CharT>{loc};
                return read_source_with_thsep_impl(range, is_signed);
            }
#endif

            template <typename T>
            SCN_NODISCARD scan_expected<std::ptrdiff_t> parse_value(T& value)
            {
                SCN_EXPECT(!numeric_base::m_buffer.view().empty());

                if (m_zero_parsed) {
                    value = 0;
                    return 1 + (m_sign != numeric_base::sign::default_sign);
                }

                if (m_sign == numeric_base::sign::default_sign) {
                    m_sign = numeric_base::sign::plus_sign;
                }
                SCN_EXPECT(m_base != 0);

                SCN_TRY(it, parse_value_impl(value));
                return ranges::distance(numeric_base::m_buffer.view().begin(),
                                        it) +
                       m_nondigit_prefix_len + ranges::ssize(m_thsep_indices);
            }

        private:
            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>>
            read_source_with_thsep_impl(Range&& range, bool is_signed)
            {
                SCN_EXPECT(m_locale_options.thousands_sep != 0);
                SCN_EXPECT(!m_locale_options.grouping.empty());

                SCN_TRY(it, read_source_impl(range, is_signed));
                if (!m_thsep_indices.empty()) {
                    if (auto e = this->check_thsep_grouping(
                            ranges::subrange{ranges::begin(range), it},
                            m_thsep_indices, m_locale_options.grouping);
                        SCN_UNLIKELY(!e)) {
                        return unexpected(e);
                    }
                }

                return it;
            }

            template <typename Range>
            SCN_NODISCARD scan_expected<simple_borrowed_iterator_t<Range>>
            read_source_impl(Range&& range, bool is_signed)
            {
                const auto make_subrange = [&](auto it) {
                    return ranges::subrange{it, ranges::end(range)};
                };

                SCN_TRY(it, read_sign(range, is_signed));

                auto base_prefix_begin = it;
                SCN_TRY_ASSIGN(it, read_base_prefix(make_subrange(it)));

                auto digits_begin = it;
                if (m_zero_parsed) {
                    digits_begin = ranges_polyfill::prev_backtrack(
                        it, ranges::begin(range));
                    SCN_TRY_ASSIGN(
                        it, read_digits_zero(make_subrange(digits_begin)));
                }
                else {
                    SCN_TRY_ASSIGN(
                        it, read_digits(make_subrange(it), base_prefix_begin));
                }

                if (it == digits_begin ||
                    ranges_polyfill::less_backtrack(it, digits_begin,
                                                    ranges::begin(range))) {
                    digits_begin = base_prefix_begin;
                }

                m_nondigit_prefix_len =
                    ranges::distance(ranges::begin(range), digits_begin);
                numeric_base::m_buffer.assign(
                    ranges::subrange{digits_begin, it});
                if (!m_thsep_indices.empty()) {
                    numeric_base::m_buffer.make_into_allocated_string();
                    for (size_t i = 0; i < m_thsep_indices.size(); ++i) {
                        const auto idx =
                            static_cast<size_t>(m_thsep_indices[i]) - i;
                        auto erase_it =
                            numeric_base::m_buffer.get_allocated_string()
                                .begin() +
                            static_cast<std::ptrdiff_t>(idx);
                        numeric_base::m_buffer.get_allocated_string().erase(
                            erase_it);
                    }
                }

                return it;
            }

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>> read_sign(
                Range&& range,
                bool is_signed)
            {
                SCN_TRY(it, numeric_base::read_sign(SCN_FWD(range), m_sign));

                if (m_sign == numeric_base::sign::minus_sign) {
                    if (is_signed) {
                        if (SCN_UNLIKELY((m_options & only_unsigned) != 0)) {
                            return unexpected_scan_error(
                                scan_error::invalid_scanned_value,
                                "'u'-option disallows negative values");
                        }
                    }
                    else {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "Unexpected '-' sign when parsing an "
                            "unsigned value");
                    }
                }

                return it;
            }

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>>
            read_bin_base_prefix(Range&& range)
            {
                return read_matching_string_classic_nocase(SCN_FWD(range),
                                                           "0b");
            }

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>>
            read_hex_base_prefix(Range&& range)
            {
                return read_matching_string_classic_nocase(SCN_FWD(range),
                                                           "0x");
            }

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>>
            read_oct_base_prefix(Range&& range)
            {
                if (auto r = read_matching_string_classic_nocase(range, "0o")) {
                    return *r;
                }

                if (auto r = read_matching_code_unit(range, '0')) {
                    m_zero_parsed = true;
                    return *r;
                }

                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "read_oct_base_prefix: No match");
            }

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>>
            read_base_prefix_for_detection(Range&& range)
            {
                if (auto r = read_hex_base_prefix(range)) {
                    m_base = 16;
                    m_base_prefix_parsed = true;
                    return *r;
                }
                if (auto r = read_bin_base_prefix(range)) {
                    m_base = 2;
                    m_base_prefix_parsed = true;
                    return *r;
                }
                if (auto r = read_oct_base_prefix(range)) {
                    m_base = 8;
                    m_base_prefix_parsed = true;
                    return *r;
                }
                m_base = 10;
                return ranges::begin(range);
            }

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>> read_base_prefix(
                Range&& range)
            {
                switch (m_base) {
                    case 2:
                        // allow 0b/0B
                        return apply_opt(read_bin_base_prefix(range), range);

                    case 8:
                        // allow 0o/0O/0
                        return apply_opt(read_oct_base_prefix(range), range);

                    case 16:
                        // allow 0x/0X
                        return apply_opt(read_hex_base_prefix(range), range);

                    case 0:
                        // detect base
                        return read_base_prefix_for_detection(SCN_FWD(range));

                    default:
                        // no base prefix allowed
                        return ranges::begin(range);
                }
            }

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>> read_digits_impl(
                Range&& range)
            {
                SCN_EXPECT(m_base != 0);

                if (SCN_UNLIKELY(m_locale_options.thousands_sep != 0)) {
                    auto it = ranges::begin(range);
                    bool digit_matched = false;
                    for (; it != ranges::end(range); ++it) {
                        if (*it == m_locale_options.thousands_sep) {
                            m_thsep_indices.push_back(static_cast<char>(
                                ranges::distance(ranges::begin(range), it)));
                        }
                        else if (numeric_reader_base::char_to_int(*it) >=
                                 m_base) {
                            break;
                        }
                        else {
                            digit_matched = true;
                        }
                    }
                    if (SCN_UNLIKELY(!digit_matched)) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "No matching characters");
                    }
                    return it;
                }

                return read_while1_code_unit(
                    range, [&](char_type ch) SCN_NOEXCEPT {
                        return numeric_reader_base::char_to_int(ch) < m_base;
                    });
            }

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>> read_digits(
                Range&& range,
                ranges::iterator_t<Range> base_prefix_begin)
            {
                SCN_EXPECT(!m_zero_parsed);

                if (auto r = read_digits_impl(SCN_FWD(range))) {
                    SCN_LIKELY_ATTR
                    return *r;
                }
                else if (r.error() == scan_error::invalid_scanned_value &&
                         m_base_prefix_parsed) {
                    m_zero_parsed = true;
                    return ranges::next(base_prefix_begin);
                }
                else {
                    SCN_UNLIKELY_ATTR
                    return unexpected(r.error());
                }
            }

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>> read_digits_zero(
                Range&& range)
            {
                SCN_EXPECT(m_zero_parsed);

                SCN_TRY(it, read_digits_impl(range));
                if (it != ranges::begin(range)) {
                    m_zero_parsed = false;
                }
                return it;
            }

            template <typename T>
            scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>
            parse_value_impl(T& value);

            localized_number_formatting_options<CharT> m_locale_options{};
            std::string m_thsep_indices{};
            std::ptrdiff_t m_nondigit_prefix_len{0};
            typename numeric_base::sign m_sign{
                numeric_base::sign::default_sign};
            bool m_zero_parsed{false}, m_base_prefix_parsed{false};
        };

        template <typename T>
        void parse_int_value_exhaustive_valid(std::string_view source,
                                              T& value);

#define SCN_DECLARE_INTEGER_READER_TEMPLATE(CharT, IntT)                     \
    extern template auto integer_reader<CharT>::parse_value_impl(IntT&)      \
        -> scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>; \
    extern template void parse_int_value_exhaustive_valid(std::string_view,  \
                                                          IntT&);

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

            void check_specs_impl(
                const detail::basic_format_specs<CharT>& specs,
                reader_error_handler& eh)
            {
                detail::check_int_type_specs(specs, eh);
            }

            template <typename>
            struct debug;

            template <typename Range, typename T>
            scan_expected<simple_borrowed_iterator_t<Range>>
            read_default(Range&& range, T& value, detail::locale_ref loc)
            {
                using range_nocvref_t = detail::remove_cvref_t<Range>;
                SCN_UNUSED(loc);

                integer_reader<CharT> rd{detail::tag_type<T>{}};
                return read_impl<range_nocvref_t>(
                    range, rd,
                    [](integer_reader<CharT>& r, auto&&... args) {
                        return r.read_source(SCN_FWD(args)...);
                    },
                    value, loc);
            }

            template <typename Range, typename T>
            scan_expected<simple_borrowed_iterator_t<Range>> read_specs(
                Range&& range,
                const detail::basic_format_specs<CharT>& specs,
                T& value,
                detail::locale_ref loc)
            {
                using range_nocvref_t = detail::remove_cvref_t<Range>;
                auto rd = get_reader_from_specs(specs);

#if !SCN_DISABLE_LOCALE
                if (specs.localized) {
                    return read_impl<range_nocvref_t>(
                        range, rd,
                        [](integer_reader<CharT>& r, auto&&... args) {
                            return r.read_source_localized(SCN_FWD(args)...);
                        },
                        value, loc);
                }
#endif

                return read_impl<range_nocvref_t>(
                    range, rd,
                    [](integer_reader<CharT>& r, auto&&... args) {
                        return r.read_source(SCN_FWD(args)...);
                    },
                    value, loc);
            }

        private:
            template <typename Range>
            using read_source_callback_type =
                scan_expected<ranges::iterator_t<Range>>(integer_reader<CharT>&,
                                                         Range,
                                                         bool,
                                                         detail::locale_ref);

            template <typename Range, typename T>
            scan_expected<ranges::iterator_t<Range>> read_impl(
                Range range,
                integer_reader<CharT>& rd,
                function_ref<read_source_callback_type<Range>> read_source_cb,
                T& value,
                detail::locale_ref loc = {})
            {
                if (auto r =
                        read_source_cb(rd, range, std::is_signed_v<T>, loc);
                    SCN_UNLIKELY(!r)) {
                    return unexpected(r.error());
                }

                SCN_TRY(n, rd.parse_value(value));
                return ranges::next(ranges::begin(range), n);
            }

            static integer_reader<CharT> get_reader_from_specs(
                const detail::basic_format_specs<CharT>& specs)
            {
                unsigned options{};
                if (specs.thsep) {
                    options |= integer_reader_base::allow_thsep;
                }
                if (specs.type ==
                    detail::presentation_type::int_unsigned_decimal) {
                    options |= integer_reader_base::only_unsigned;
                }

                return {options, specs.get_base(0)};
            }
        };
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
