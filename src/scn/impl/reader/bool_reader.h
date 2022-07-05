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
        // TODO
        template <typename CharT>
        using bool_classic_source_reader = classic_numeric_source_reader<CharT>;

        template <typename CharT>
        class bool_value_reader {
        public:
            using char_type = CharT;
            using string_view_type = std::basic_string_view<char_type>;
            using iterator = ranges::iterator_t<string_view_type>;

            enum class flags : uint8_t {
                allow_text = 1,
                allow_numeric = 2,
                use_localized_numpunct = 4,
            };

            constexpr bool_value_reader() = default;
            explicit constexpr bool_value_reader(detail::locale_ref loc)
                : m_locale(loc)
            {
            }
            constexpr bool_value_reader(uint8_t f, detail::locale_ref loc)
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

            uint8_t m_flags{static_cast<uint8_t>(flags::allow_text) |
                            static_cast<uint8_t>(flags::allow_numeric)};
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
                // TODO
                SCN_UNUSED(specs);
                SCN_UNUSED(eh);
            }

            auto make_default_classic_readers() const
            {
                return std::make_pair(bool_classic_source_reader<CharT>{},
                                      bool_value_reader<CharT>{});
            }
            auto make_default_userlocale_readers(detail::locale_ref loc) const
            {
                return std::make_pair(
                    until_space_localized_source_reader<CharT>{loc},
                    bool_value_reader<CharT>{loc});
            }

            auto make_specs_classic_readers(
                const detail::basic_format_specs<CharT>& specs) const
            {
                // TODO: specs
                SCN_UNUSED(specs);
                return std::make_pair(bool_classic_source_reader<CharT>{},
                                      bool_value_reader<CharT>{});
            }
            auto make_specs_userlocale_readers(
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc) const
            {
                // TODO: specs
                SCN_UNUSED(specs);
                return std::make_pair(
                    until_space_localized_source_reader<CharT>{loc},
                    bool_value_reader<CharT>{loc});
            }
            auto make_specs_localized_readers(
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc) const
            {
                // TODO: specs
                SCN_UNUSED(specs);
                return std::make_pair(
                    until_space_localized_source_reader<CharT>{loc},
                    bool_value_reader<CharT>{loc});
            }
        };

        template <typename CharT>
        class reader<bool, CharT> : public bool_reader<CharT> {};
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
