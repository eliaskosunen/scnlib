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
#include "scn/impl/locale.h"

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
        class integer_reader
            : public numeric_reader<integer_reader<CharT>, CharT>,
              public integer_reader_base {
            using numeric_base = numeric_reader<integer_reader<CharT>, CharT>;

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
                        localized_number_formatting_options<CharT>{};
                    m_locale_options.thousands_sep = CharT{','};
                    m_locale_options.grouping = "\3";

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
                           m_nondigit_prefix_len + m_thsep_indices.size();
                });
            }

        private:
            template <bool IsSigned, typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>>
            read_source_with_thsep_impl(Range&& range)
            {
                SCN_EXPECT(m_locale_options.thsep != 0);
                SCN_EXPECT(!m_locale_options.grouping.empty());

                return read_source_impl(range).and_then(
                    [&](auto it) -> scan_expected<decltype(it)> {
                        if (!m_thsep_indices.empty()) {
                            return this->check_thsep_grouping(
                                ranges::subrange{ranges::begin(this->m_buffer),
                                                 it},
                                m_thsep_indices, m_locale_options.grouping);
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

                auto digits_begin = ranges::begin(range);
                return read_sign<IsSigned>(range)
                    .and_then([&](auto it) {
                        return read_base_prefix(make_subrange(it));
                    })
                    .and_then([&](auto it) {
                        digits_begin = it;
                        return read_digits(make_subrange(it));
                    })
                    .transform([&](auto it) {
                        m_nondigit_prefix_len = ranges::distance(
                            ranges::begin(range), digits_begin);
                        numeric_base::m_buffer.assign(
                            ranges::subrange{digits_begin, it});
                        if (!m_thsep_indices.empty()) {
                            numeric_base::m_buffer.make_into_allocated_string();
                            for (auto idx : m_thsep_indices) {
                                numeric_base::m_buffer.get_allocated_string()
                                    .erase(numeric_base::m_buffer.begin() +
                                           static_cast<std::ptrdiff_t>(idx));
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
                return read_matching_string_nocase(SCN_FWD(range), "0b");
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>>
            read_hex_base_prefix(Range&& range)
            {
                return read_matching_string_nocase(SCN_FWD(range), "0x");
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>>
            read_oct_base_prefix(Range&& range)
            {
                if (auto r = read_matching_string_nocase(range, "0o")) {
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
                    return *r;
                }
                if (auto r = read_bin_base_prefix(range)) {
                    m_base = 2;
                    return *r;
                }
                if (auto r = read_oct_base_prefix(range)) {
                    m_base = 8;
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

                if (SCN_UNLIKELY(m_locale_options.thsep != 0)) {
                    auto it = ranges::begin(range);
                    for (; it != ranges::end(range); ++it) {
                        if (*it == m_locale_options.thsep) {
                            m_thsep_indices.push_back(static_cast<char>(
                                ranges::distance(ranges::begin(range), it)));
                        }
                        else if (numeric_reader_base::char_to_int(*it) >=
                                 m_base) {
                            break;
                        }
                    }
                    return it;
                }

                return read_while1_code_unit(range, [&](char_type ch) {
                    return numeric_reader_base::char_to_int(ch) < m_base;
                });
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_digits(
                Range&& range)
            {
                auto adjust_for_zero = [&](auto it) {
                    if (m_zero_parsed && it == ranges::begin(range)) {
                        m_zero_parsed = false;
                        return ranges::prev(it);
                    }
                    return it;
                };

                if constexpr (ranges::contiguous_range<Range> &&
                              ranges::sized_range<Range>) {
                    if (m_zero_parsed) {
                        return read_digits_impl(SCN_FWD(range))
                            .transform(adjust_for_zero);
                    }
                    return read_all(SCN_FWD(range));
                }
                else {
                    return read_digits_impl(SCN_FWD(range))
                        .transform(adjust_for_zero);
                }
            }

            template <typename T>
            scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>
            parse_value_impl(T& value);

            localized_number_formatting_options<CharT> m_locale_options{};
            std::string m_thsep_indices{};
            std::ptrdiff_t m_nondigit_prefix_len{0};
            numeric_base::sign m_sign{numeric_base::sign::default_sign};
            bool m_zero_parsed{false};
        };

#define SCN_DECLARE_INTEGER_READER_TEMPLATE_IMPL(CharT, IntT)           \
    extern template auto integer_reader<CharT>::parse_value_impl(IntT&) \
        ->scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>;

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
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
