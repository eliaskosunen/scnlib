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

#include <scn/impl/reader/integer/classic_value_reader.h>
#include <scn/impl/reader/integer/localized_value_reader.h>
#include <scn/impl/reader/integer/source_reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        class int_classic_reader_factory {
        public:
            int_classic_reader_factory(
                const detail::basic_format_specs<CharT>& s)
                : m_specs(s)
            {
            }

            auto make() const
            {
                return std::make_pair(int_classic_source_reader<CharT>{},
                                      make_value_reader());
            }

            template <typename T>
            auto make_with_locale(detail::locale_ref loc) const
            {
                return std::make_pair(
                    int_localized_source_reader<CharT>{loc,
                                                       detail::tag_type<T>{}},
                    make_value_reader());
            }

        private:
            constexpr uint8_t get_options() const SCN_NOEXCEPT
            {
                using options =
                    typename int_classic_value_reader<CharT>::options_type;
                uint8_t opt{};
                if (m_specs.thsep) {
                    opt |= options::allow_thsep;
                }
                if (m_specs.type ==
                    detail::presentation_type::int_unsigned_decimal) {
                    opt |= options::only_unsigned;
                }
                if (m_specs.type !=
                    detail::presentation_type::int_arbitrary_base) {
                    opt |= options::allow_base_prefix;
                }
                return opt;
            }

            constexpr int_classic_value_reader<CharT> make_value_reader() const
                SCN_NOEXCEPT
            {
                return {get_options(),
                        static_cast<int8_t>(m_specs.get_base(0))};
            }

            const detail::basic_format_specs<CharT>& m_specs;
        };

        template <typename CharT, typename T>
        class int_localized_reader_factory {
        public:
            int_localized_reader_factory(
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc)
                : m_specs(specs), m_loc(loc)
            {
            }

            auto make() const SCN_NOEXCEPT
            {
                return std::make_pair(
                    make_source_reader(),
                    int_localized_value_reader<CharT>{
                        m_loc, static_cast<int8_t>(m_specs.get_base(0))});
            }

        private:
            constexpr int_localized_source_reader<CharT> make_source_reader()
                const SCN_NOEXCEPT
            {
                const int base = m_specs.get_base(16);
                const bool allow_sign =
                    std::is_signed_v<T> &&
                    m_specs.type !=
                        detail::presentation_type::int_unsigned_decimal;
                return int_localized_source_reader<CharT>{m_loc, base,
                                                          allow_sign};
            }

            const detail::basic_format_specs<CharT>& m_specs;
            detail::locale_ref m_loc;
        };

        template <typename T, typename CharT>
        class int_reader
            : public reader_facade<int_reader<T, CharT>, T, CharT> {
            friend reader_facade<int_reader<T, CharT>, T, CharT>;

        public:
            int_reader() = default;

        private:
            static void check_specs_impl(
                const detail::basic_format_specs<CharT>& specs,
                reader_error_handler& eh)
            {
                detail::check_int_type_specs(specs, eh);
            }

            auto make_default_classic_readers() const
            {
                return std::make_pair(
                    int_classic_source_reader<CharT>{},
                    int_classic_value_reader<CharT>{detail::tag_type<T>{}});
            }
            auto make_default_userlocale_readers(detail::locale_ref loc) const
            {
                return std::make_pair(
                    int_localized_source_reader<CharT>{loc,
                                                       detail::tag_type<T>{}},
                    int_classic_value_reader<CharT>{detail::tag_type<T>{}});
            }

            auto make_specs_classic_readers(
                const detail::basic_format_specs<CharT>& specs) const
            {
                return int_classic_reader_factory<CharT>(specs).make();
            }
            auto make_specs_userlocale_readers(
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc) const
            {
                return int_classic_reader_factory<CharT>(specs)
                    .template make_with_locale<T>(loc);
            }
            auto make_specs_localized_readers(
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc) const
            {
                return int_localized_reader_factory<CharT, T>(specs, loc)
                    .make();
            }
        };

        template <typename T>
        inline constexpr bool is_int_reader_type =
            std::is_integral_v<T> && !std::is_same_v<T, char> &&
            !std::is_same_v<T, wchar_t> && !std::is_same_v<T, bool>;

        template <typename T, typename CharT>
        class reader<T, CharT, std::enable_if_t<is_int_reader_type<T>>>
            : public int_reader<T, CharT> {};
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
