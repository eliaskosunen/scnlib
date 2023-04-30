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
#include "scn/detail/format_string_parser.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        class bool_value_reader {
        public:
            using char_type = CharT;
            using string_view_type = std::basic_string_view<char_type>;
            using iterator = ranges::iterator_t<string_view_type>;

            enum flags {
                allow_text = 1,
                allow_numeric = 2,
                use_localized_numpunct = 4,
            };

            constexpr bool_value_reader() = default;
            explicit constexpr bool_value_reader(detail::locale_ref loc)
                : m_locale(loc)
            {
            }
            explicit constexpr bool_value_reader(unsigned f,
                                                 detail::locale_ref loc = {})
                : m_flags(f), m_locale(loc)
            {
            }

            scan_expected<iterator> read(string_view_type range,
                                         bool& val) const;

        private:
            scan_expected<iterator> read_text(string_view_type range,
                                              string_view_type truename,
                                              string_view_type falsename,
                                              bool& val) const;
            scan_expected<iterator> read_numeric(string_view_type range,
                                                 bool& val) const;

            static constexpr std::basic_string_view<CharT> classic_truename()
            {
                if constexpr (std::is_same_v<CharT, char>) {
                    return "true";
                }
                else {
                    return L"true";
                }
            }

            static constexpr std::basic_string_view<CharT> classic_falsename()
            {
                if constexpr (std::is_same_v<CharT, char>) {
                    return "false";
                }
                else {
                    return L"false";
                }
            }

            unsigned m_flags{allow_text | allow_numeric};
            detail::locale_ref m_locale{};
        };

        extern template auto bool_value_reader<char>::read(std::string_view,
                                                           bool&) const
            -> scan_expected<std::string_view::iterator>;
        extern template auto bool_value_reader<wchar_t>::read(std::wstring_view,
                                                              bool&) const
            -> scan_expected<std::wstring_view::iterator>;

        extern template auto bool_value_reader<char>::read_text(
            std::string_view,
            std::string_view,
            std::string_view,
            bool&) const -> scan_expected<std::string_view::iterator>;
        extern template auto bool_value_reader<wchar_t>::read_text(
            std::wstring_view,
            std::wstring_view,
            std::wstring_view,
            bool&) const -> scan_expected<std::wstring_view::iterator>;

        extern template auto bool_value_reader<char>::read_numeric(
            std::string_view,
            bool&) const -> scan_expected<std::string_view::iterator>;
        extern template auto bool_value_reader<wchar_t>::read_numeric(
            std::wstring_view,
            bool&) const -> scan_expected<std::wstring_view::iterator>;

        template <typename CharT>
        class bool_reader
            : public reader_facade<bool_reader<CharT>, bool, CharT> {
            friend reader_facade<bool_reader<CharT>, bool, CharT>;

        public:
            bool_reader() = default;

        private:
            static void check_specs_impl(
                const detail::basic_format_specs<CharT>& specs,
                reader_error_handler& eh)
            {
                return detail::check_bool_type_specs(specs, eh);
            }

            auto make_default_classic_readers() const
            {
                return std::make_pair(
                    simple_classic_source_reader<CharT>{this->buffer},
                    bool_value_reader<CharT>{});
            }
            auto make_default_userlocale_readers(detail::locale_ref loc) const
            {
                return std::make_pair(
                    simple_localized_source_reader<CharT>{loc, this->buffer},
                    bool_value_reader<CharT>{loc});
            }

            auto make_specs_classic_readers(
                const detail::basic_format_specs<CharT>& specs) const
            {
                return std::make_pair(
                    simple_classic_source_reader<CharT>{this->buffer},
                    bool_value_reader<CharT>{make_flags(specs)});
            }
            auto make_specs_userlocale_readers(
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc) const
            {
                return std::make_pair(
                    simple_localized_source_reader<CharT>{loc, this->buffer},
                    bool_value_reader<CharT>{make_flags(specs), loc});
            }
            auto make_specs_localized_readers(
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc) const
            {
                return std::make_pair(
                    simple_localized_source_reader<CharT>{loc, this->buffer},
                    bool_value_reader<CharT>{
                        make_flags(specs) |
                            bool_value_reader<CharT>::use_localized_numpunct,
                        loc});
            }

            static constexpr unsigned make_flags(
                const detail::basic_format_specs<CharT>& specs)
            {
                SCN_GCC_COMPAT_PUSH
                SCN_GCC_COMPAT_IGNORE("-Wswitch-enum")
                switch (specs.type) {
                    case detail::presentation_type::none:
                        return bool_value_reader<CharT>::allow_text |
                               bool_value_reader<CharT>::allow_numeric;

                    case detail::presentation_type::string:
                        return bool_value_reader<CharT>::allow_text;

                    case detail::presentation_type::int_generic:
                        return bool_value_reader<CharT>::allow_numeric;

                    default:
                        SCN_EXPECT(false);
                        SCN_UNREACHABLE;
                }
                SCN_GCC_COMPAT_POP
            }
        };

        template <typename CharT>
        class reader<bool, CharT> : public bool_reader<CharT> {};
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
