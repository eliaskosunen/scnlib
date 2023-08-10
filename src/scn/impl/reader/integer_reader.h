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

            template <typename T, typename Range>
            SCN_NODISCARD scan_expected<ranges::borrowed_iterator_t<Range>>
            read_source(detail::tag_type<T>, Range&& range)
            {
                if (SCN_UNLIKELY(m_options &
                                 integer_reader_base::allow_thsep)) {
                    m_locale_options =
                        localized_number_formatting_options<CharT>{
                            classic_with_thsep_tag{}};

                    return read_source_with_thsep_impl<std::is_signed_v<T>>(
                        SCN_FWD(range));
                }

                return read_source_impl<std::is_signed_v<T>>(SCN_FWD(range));
            }

            template <typename T, typename Range>
            SCN_NODISCARD scan_expected<ranges::borrowed_iterator_t<Range>>
            read_source_localized(detail::tag_type<T>,
                                  Range&& range,
                                  detail::locale_ref loc)
            {
                if ((m_options & integer_reader_base::allow_thsep) == 0) {
                    return read_source(detail::tag_type<T>{}, SCN_FWD(range));
                }

                m_locale_options =
                    localized_number_formatting_options<CharT>{loc};
                return read_source_with_thsep_impl<std::is_signed_v<T>>(
                    SCN_FWD(range));
            }

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

                return parse_value_impl(value).transform([&](auto it) {
                    return ranges::distance(
                               numeric_base::m_buffer.view().begin(), it) +
                           m_nondigit_prefix_len +
                           ranges::ssize(m_thsep_indices);
                });
            }

        private:
            template <bool IsSigned, typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>>
            read_source_with_thsep_impl(Range&& range)
            {
                SCN_EXPECT(m_locale_options.thousands_sep != 0);
                SCN_EXPECT(!m_locale_options.grouping.empty());

                return read_source_impl<IsSigned>(range).and_then(
                    [&](auto it) -> scan_expected<decltype(it)> {
                        if (!m_thsep_indices.empty()) {
                            if (auto e = this->check_thsep_grouping(
                                    ranges::subrange{ranges::begin(range), it},
                                    m_thsep_indices, m_locale_options.grouping);
                                SCN_UNLIKELY(!e)) {
                                return unexpected(e);
                            }
                        }

                        return it;
                    });
            }

            template <bool IsSigned, typename Range>
            SCN_NODISCARD scan_expected<ranges::borrowed_iterator_t<Range>>
            read_source_impl(Range&& range)
            {
                const auto make_subrange = [&](auto it) {
                    return ranges::subrange{it, ranges::end(range)};
                };

#if 0
                auto it = ranges::begin(range);
                if (auto r = read_sign<IsSigned>(range)) {
                    SCN_LIKELY_ATTR
                    it = *r;
                }
                else {
                    return unexpected(r.error());
                }

                auto original_base = m_base;
                if (auto r = read_base_prefix(make_subrange(it))) {
                    SCN_LIKELY_ATTR
                    it = *r;
                }
                else {
                    return unexpected(r.error());
                }

                auto digits_begin = it;
                if (m_zero_parsed) {
                    digits_begin = ranges_polyfill::prev_backtrack(
                        it, ranges::begin(range));
                    if (auto r =
                            read_digits_zero(make_subrange(digits_begin))) {
                        SCN_LIKELY_ATTR
                        it = *r;
                    }
                    else {
                        return unexpected(r.error());
                    }
                }
                else {
                    if (auto r = read_digits(make_subrange(digits_begin))) {
                        SCN_LIKELY_ATTR
                        it = *r;
                    }
                    else {
                        return unexpected(r.error());
                    }
                }

                m_nondigit_prefix_len = ranges::distance(ranges::begin(range), digits_begin);
                this->m_buffer.assign(ranges::subrange{digits_begin, it});
#endif

                auto base_prefix_begin = ranges::begin(range);
                auto digits_begin = ranges::begin(range);

                return read_sign<IsSigned>(range)
                    .and_then([&](auto it) {
                        base_prefix_begin = it;
                        return read_base_prefix(make_subrange(it));
                    })
                    .and_then([&](auto it) {
                        if (m_zero_parsed) {
                            digits_begin = ranges_polyfill::prev_backtrack(
                                it, ranges::begin(range));
                            return read_digits_zero(
                                make_subrange(digits_begin));
                        }

                        digits_begin = it;
                        return read_digits(make_subrange(it), base_prefix_begin);
                    })
                    .transform([&](auto it) {
                        m_nondigit_prefix_len = ranges::distance(
                            ranges::begin(range), digits_begin);
                        numeric_base::m_buffer.assign(
                            ranges::subrange{digits_begin, it});
                        if (!m_thsep_indices.empty()) {
                            numeric_base::m_buffer.make_into_allocated_string();
                            for (size_t i = 0; i < m_thsep_indices.size();
                                 ++i) {
                                const auto idx =
                                    static_cast<size_t>(m_thsep_indices[i]) - i;
                                auto erase_it =
                                    numeric_base::m_buffer
                                        .get_allocated_string()
                                        .begin() +
                                    static_cast<std::ptrdiff_t>(idx);
                                numeric_base::m_buffer.get_allocated_string()
                                    .erase(erase_it);
                            }
                        }
                        return it;
                    });
            }

            template <bool IsSigned, typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_sign(
                Range&& range)
            {
                return numeric_base::read_sign(SCN_FWD(range), m_sign)
                    .and_then([&](auto it)
                                  -> scan_expected<
                                      ranges::borrowed_iterator_t<Range>> {
                        if (m_sign == numeric_base::sign::minus_sign) {
                            if constexpr (IsSigned) {
                                if (SCN_UNLIKELY((m_options & only_unsigned) !=
                                                 0)) {
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
                    });
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>>
            read_bin_base_prefix(Range&& range)
            {
                return read_matching_string_classic_nocase(SCN_FWD(range),
                                                           "0b");
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>>
            read_hex_base_prefix(Range&& range)
            {
                return read_matching_string_classic_nocase(SCN_FWD(range),
                                                           "0x");
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>>
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
            scan_expected<ranges::borrowed_iterator_t<Range>>
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
            scan_expected<ranges::borrowed_iterator_t<Range>> read_base_prefix(
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
            scan_expected<ranges::borrowed_iterator_t<Range>> read_digits_impl(
                Range&& range)
            {
                SCN_EXPECT(m_base != 0);

                if (SCN_UNLIKELY(m_locale_options.thousands_sep != 0)) {
                    auto it = ranges::begin(range);
                    for (; it != ranges::end(range); ++it) {
                        if (*it == m_locale_options.thousands_sep) {
                            m_thsep_indices.push_back(static_cast<char>(
                                ranges::distance(ranges::begin(range), it)));
                        }
                        else if (numeric_reader_base::char_to_int(*it) >=
                                 m_base) {
                            break;
                        }
                    }
                    if (it == ranges::begin(range)) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "No matching characters");
                    }
                    return it;
                }

                return read_while1_code_unit(range, [&](char_type ch) {
                    return numeric_reader_base::char_to_int(ch) < m_base;
                });
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_digits(
                Range&& range,
                ranges::iterator_t<Range> base_prefix_begin)
            {
                SCN_EXPECT(!m_zero_parsed);

                if (auto r = read_digits_impl(SCN_FWD(range))) {
                    SCN_LIKELY_ATTR
                    return *r;
                }
                else if (r.error() == scan_error::invalid_scanned_value && m_base_prefix_parsed) {
                    m_zero_parsed = true;
                    return ranges::next(base_prefix_begin);
                }
                else {
                    SCN_UNLIKELY_ATTR
                    return unexpected(r.error());
                }
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_digits_zero(
                Range&& range)
            {
                SCN_EXPECT(m_zero_parsed);

                return read_digits_impl(range).transform([&](auto it) {
                    if (it != ranges::begin(range)) {
                        m_zero_parsed = false;
                    }
                    return it;
                });
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

#define SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, IntT)           \
    extern template auto integer_reader<CharT>::parse_value_impl(IntT&) \
        -> scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>;

#define SCN_DECLARE_INTEGER_READER_TEMPLATE(CharT)                  \
    SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, signed char)    \
    SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, short)          \
    SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, int)            \
    SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, long)           \
    SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, long long)      \
    SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, unsigned char)  \
    SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, unsigned short) \
    SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, unsigned int)   \
    SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, unsigned long)  \
    SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, unsigned long long)

        SCN_DECLARE_INTEGER_READER_TEMPLATE(char)
        SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t)

#undef SCN_DECLARE_INTEGER_READER_TEMPLATE
#undef SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL

        template <typename T>
        inline constexpr bool is_integer_reader_type =
            std::is_integral_v<T> && !std::is_same_v<T, char> &&
            !std::is_same_v<T, wchar_t> && !std::is_same_v<T, bool>;

        template <typename T, typename CharT>
        class reader<T, CharT, std::enable_if_t<is_integer_reader_type<T>>>
            : public reader_base<reader<T, CharT>, CharT> {
        public:
            constexpr reader() = default;

            void check_specs_impl(
                const detail::basic_format_specs<CharT>& specs,
                reader_error_handler& eh)
            {
                detail::check_int_type_specs(specs, eh);
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>>
            read_default(Range&& range, T& value, detail::locale_ref loc)
            {
                SCN_UNUSED(loc);

                integer_reader<CharT> rd{detail::tag_type<T>{}};
                return read_impl(
                    SCN_FWD(range), rd,
                    [&](auto&& rng) {
                        return rd.read_source(detail::tag_type<T>{},
                                              SCN_FWD(rng));
                    },
                    value);
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_specs(
                Range&& range,
                const detail::basic_format_specs<CharT>& specs,
                T& value,
                detail::locale_ref loc)
            {
                auto rd = get_reader_from_specs(specs);

                if (specs.localized) {
                    return read_impl(
                        SCN_FWD(range), rd,
                        [&](auto&& rng) {
                            return rd.read_source_localized(
                                detail::tag_type<T>{}, SCN_FWD(rng), loc);
                        },
                        value);
                }

                return read_impl(
                    SCN_FWD(range), rd,
                    [&](auto&& rng) {
                        return rd.read_source(detail::tag_type<T>{},
                                              SCN_FWD(rng));
                    },
                    value);
            }

        private:
            template <typename Range, typename ReadSource>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_impl(
                Range&& range,
                integer_reader<CharT>& rd,
                ReadSource&& read_source_cb,
                T& value)
            {
                return read_source_cb(range)
                    .and_then([&](auto _) {
                        SCN_UNUSED(_);
                        return rd.parse_value(value);
                    })
                    .transform([&](auto n) {
                        return ranges::next(ranges::begin(range), n);
                    });
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
    };  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
