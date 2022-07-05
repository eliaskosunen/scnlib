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
        struct float_value_reader_base {
            enum options_type : uint8_t {
                allow_hex = 1,
                allow_scientific = 2,
                allow_fixed = 4,
                allow_thsep = 8
            };

            float_value_reader_base() = default;
            explicit float_value_reader_base(uint8_t opt) : m_options(opt) {}

            uint8_t m_options{allow_hex | allow_scientific | allow_fixed};
        };

        template <typename CharT>
        class float_classic_value_reader : public float_value_reader_base {
        public:
            using char_type = CharT;
            using string_view_type = std::basic_string_view<CharT>;

            template <typename T>
            class cstd_impl;
            template <typename T>
            class from_chars_impl;
            template <typename T>
            class fast_float_impl;

            float_classic_value_reader() = default;

            explicit float_classic_value_reader(uint8_t opt)
                : float_value_reader_base(opt)
            {
            }

            template <typename T>
            scan_expected<ranges::iterator_t<string_view_type>> read(
                string_view_type source,
                T& value);

        private:
            friend struct float_classic_value_reader_fast_float_impl_base;
        };

        template <typename CharT>
        class float_localized_value_reader : public float_value_reader_base {
        public:
            using char_type = CharT;
            using string_view_type = std::basic_string_view<CharT>;

            explicit float_localized_value_reader(detail::locale_ref loc)
                : float_value_reader_base(), m_locale(loc)
            {
            }
            float_localized_value_reader(uint8_t flags, detail::locale_ref loc)
                : float_value_reader_base(flags), m_locale(loc)
            {
            }

            template <typename T>
            auto read(string_view_type source, T& value)
                -> scan_expected<ranges::iterator_t<string_view_type>>;

        private:
            detail::locale_ref m_locale{};
        };

        extern template auto float_classic_value_reader<char>::read(
            string_view_type,
            float&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        extern template auto float_classic_value_reader<char>::read(
            string_view_type,
            double&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        extern template auto float_classic_value_reader<char>::read(
            string_view_type,
            long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
        extern template auto float_classic_value_reader<wchar_t>::read(
            string_view_type,
            float&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        extern template auto float_classic_value_reader<wchar_t>::read(
            string_view_type,
            double&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        extern template auto float_classic_value_reader<wchar_t>::read(
            string_view_type,
            long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;

        extern template auto float_localized_value_reader<char>::read(
            string_view_type,
            float&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        extern template auto float_localized_value_reader<char>::read(
            string_view_type,
            double&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        extern template auto float_localized_value_reader<char>::read(
            string_view_type,
            long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
        extern template auto float_localized_value_reader<wchar_t>::read(
            string_view_type,
            float&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        extern template auto float_localized_value_reader<wchar_t>::read(
            string_view_type,
            double&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        extern template auto float_localized_value_reader<wchar_t>::read(
            string_view_type,
            long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;

    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
