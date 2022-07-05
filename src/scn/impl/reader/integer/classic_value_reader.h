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

#include <scn/detail/scanner.h>
#include <scn/impl/reader/common.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        /**
         * ValueReader:
         * auto read(sv, T&) -> scan_expected<iterator<sv>>
         */

        template <typename CharT>
        class int_classic_value_reader {
        public:
            using char_type = CharT;
            using string_type = std::basic_string<CharT>;
            using string_view_type = std::basic_string_view<CharT>;

            enum options_type : uint8_t {
                // ' option -> accept thsep (',')
                allow_thsep = 1,
                // 'u' option -> don't allow sign
                only_unsigned = 2,
                // Allow base prefix (e.g. 0B or 0x)
                allow_base_prefix = 4,
            };

            template <typename T>
            explicit int_classic_value_reader(detail::tag_type<T>)
                : m_options(get_default_options<T>()), m_base(0)
            {
            }

            int_classic_value_reader(uint8_t opt, int8_t b)
                : m_options(opt), m_base(b)
            {
            }

            template <typename T>
            static constexpr uint8_t get_default_options()
            {
                return 0;
            }

            template <typename T>
            scan_expected<ranges::iterator_t<string_view_type>> read(
                string_view_type source,
                T& value);

        private:
            template <typename T>
            struct int_reader_state;

            enum sign_type { plus_sign, minus_sign };

            scan_expected<sign_type> parse_sign_signed(
                string_view_type& source);
            scan_error parse_sign_unsigned(string_view_type& source);

            enum class base_prefix_state : uint8_t {
                // Base not determined
                base_not_determined = 0,
                // Determined base from "0_" prefix
                base_determined_from_prefix,
                // Determined base from "0" prefix
                base_determined_from_zero_prefix,
                // Found '0'
                zero_parsed,
            };
            struct base_prefix_result {
                typename string_view_type::iterator iterator;
                base_prefix_state state;
                int8_t parsed_base{0};
            };

            base_prefix_result parse_base_prefix(string_view_type source);

            enum class determine_base_result {
                zero_parsed,
                keep_parsing,
            };

            scan_expected<determine_base_result> parse_and_determine_base(
                string_view_type& source);

            SCN_NODISCARD scan_error
            check_thousands_separators(const std::string&) const;

            template <typename T>
            std::pair<bool, scan_error> do_single_char(
                CharT ch,
                int_reader_state<T>& state);
            template <typename T>
            std::pair<bool, scan_error> do_single_char_with_thsep(
                int_reader_state<T>& state,
                typename string_view_type::iterator it,
                typename string_view_type::iterator& after_last_thsep_it,
                std::string&);

            template <typename T>
            scan_expected<ranges::iterator_t<string_view_type>>
            do_read(string_view_type source, T& value, sign_type sign);

            uint8_t m_options;
            // 0 = detect_base
            // else, [2,36]
            int8_t m_base{0};
        };

#define SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, IntT) \
    extern template auto int_classic_value_reader<CharT>::read(         \
        string_view_type, IntT&)                                        \
        ->scan_expected<ranges::iterator_t<string_view_type>>;

#define SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE(CharT)                  \
    SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, signed char)    \
    SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, short)          \
    SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, int)            \
    SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, long)           \
    SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, long long)      \
    SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned char)  \
    SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned short) \
    SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned int)   \
    SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned long)  \
    SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT,                 \
                                                       unsigned long long)

        SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE(char)
        SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE(wchar_t)

#undef SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE
#undef SCN_DECLARE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL

    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
