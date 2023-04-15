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

#include <scn/impl/reader/integer/source_reader.h>
#include <scn/impl/reader/integer/value_reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        class int_reader_factory_base {
        protected:
            int_reader_factory_base(
                const detail::basic_format_specs<CharT>& specs)
                : m_specs(specs)
            {
            }

            constexpr unsigned get_options() const SCN_NOEXCEPT
            {
                using opt_t = int_value_reader_base::options_type;
                unsigned opt{};
                if (m_specs.thsep) {
                    opt |= opt_t::allow_thsep;
                }
                if (m_specs.type ==
                    detail::presentation_type::int_unsigned_decimal) {
                    opt |= opt_t::only_unsigned;
                }
                if (m_specs.type !=
                    detail::presentation_type::int_arbitrary_base) {
                    opt |= opt_t::allow_base_prefix;
                }
                return opt;
            }

            const detail::basic_format_specs<CharT>& m_specs;
        };

        template <typename CharT>
        class int_classic_reader_factory
            : private int_reader_factory_base<CharT> {
        public:
            int_classic_reader_factory(
                std::basic_string<CharT>& buffer,
                const detail::basic_format_specs<CharT>& s)
                : int_reader_factory_base<CharT>(s), m_buffer(buffer)
            {
            }

            auto make() const
            {
                return std::make_pair(
                    simple_classic_source_reader<CharT>{m_buffer},
                    make_value_reader());
            }

            template <typename T>
            auto make_with_locale(detail::locale_ref loc) const
            {
                return std::make_pair(
                    int_localized_source_reader<CharT>{m_buffer, loc,
                                                       detail::tag_type<T>{}},
                    make_value_reader());
            }

        private:
            constexpr int_classic_value_reader<CharT> make_value_reader() const
                SCN_NOEXCEPT
            {
                return {this->get_options(), this->m_specs.get_base(0)};
            }

            std::basic_string<CharT>& m_buffer;
        };

        template <typename CharT, typename T>
        class int_localized_reader_factory
            : private int_reader_factory_base<CharT> {
        public:
            int_localized_reader_factory(
                std::basic_string<CharT>& buffer,
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc)
                : int_reader_factory_base<CharT>(specs),
                  m_buffer(buffer),
                  m_loc(loc)
            {
            }

            auto make() const SCN_NOEXCEPT
            {
                return std::make_pair(
                    make_source_reader(),
                    int_localized_value_reader<CharT>{
                        m_loc, this->get_options(), this->m_specs.get_base(0)});
            }

        private:
            constexpr int_localized_source_reader<CharT> make_source_reader()
                const SCN_NOEXCEPT
            {
                const int base = this->m_specs.get_base(16);
                const bool allow_sign =
                    std::is_signed_v<T> &&
                    this->m_specs.type !=
                        detail::presentation_type::int_unsigned_decimal;
                return int_localized_source_reader<CharT>{m_buffer, m_loc, base,
                                                          allow_sign};
            }

            std::basic_string<CharT>& m_buffer;
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
                    simple_classic_source_reader<CharT>{this->buffer},
                    int_classic_value_reader<CharT>{detail::tag_type<T>{}});
            }
            auto make_default_userlocale_readers(detail::locale_ref loc) const
            {
                return std::make_pair(
                    int_localized_source_reader<CharT>{this->buffer, loc,
                                                       detail::tag_type<T>{}},
                    int_classic_value_reader<CharT>{detail::tag_type<T>{}});
            }

            auto make_specs_classic_readers(
                const detail::basic_format_specs<CharT>& specs) const
            {
                return int_classic_reader_factory<CharT>(this->buffer, specs)
                    .make();
            }
            auto make_specs_userlocale_readers(
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc) const
            {
                return int_classic_reader_factory<CharT>(this->buffer, specs)
                    .template make_with_locale<T>(loc);
            }
            auto make_specs_localized_readers(
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc) const
            {
                return int_localized_reader_factory<CharT, T>(this->buffer,
                                                              specs, loc)
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
