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

#include <scn/impl/reader/float/value_reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename T, typename CharT>
        class float_reader
            : public reader_facade<float_reader<T, CharT>, T, CharT> {
            friend reader_facade<float_reader<T, CharT>, T, CharT>;

        public:
            float_reader() = default;

            static constexpr uint8_t get_presentation_flags(
                const detail::basic_format_specs<CharT>& specs)
            {
                SCN_GCC_COMPAT_PUSH
                SCN_GCC_COMPAT_IGNORE("-Wswitch-enum")
                switch (specs.type) {
                    case detail::presentation_type::float_fixed:
                        return float_value_reader_base::allow_fixed;
                    case detail::presentation_type::float_scientific:
                        return float_value_reader_base::allow_scientific;
                    case detail::presentation_type::float_hex:
                        return float_value_reader_base::allow_hex;
                    case detail::presentation_type::float_general:
                        return static_cast<uint8_t>(
                            float_value_reader_base::allow_scientific |
                            float_value_reader_base::allow_fixed);
                    case detail::presentation_type::none:
                        return static_cast<uint8_t>(
                            float_value_reader_base::allow_scientific |
                            float_value_reader_base::allow_fixed |
                            float_value_reader_base::allow_hex);
                    default:
                        SCN_EXPECT(false);
                        SCN_UNREACHABLE;
                }
                SCN_GCC_COMPAT_POP  // -Wswitch-enum
            }

        private:
            static void check_specs_impl(
                const detail::basic_format_specs<CharT>& specs,
                reader_error_handler& eh)
            {
                detail::check_float_type_specs(specs, eh);
            }

            auto make_default_classic_readers() const
            {
                return std::make_pair(classic_numeric_source_reader<CharT>{},
                                      float_classic_value_reader<CharT>{});
            }
            auto make_default_userlocale_readers(detail::locale_ref loc) const
            {
                return std::make_pair(
                    until_space_localized_source_reader<CharT>{loc},
                    float_classic_value_reader<CharT>{});
            }

            auto make_specs_classic_readers(
                const detail::basic_format_specs<CharT>& specs) const
            {
                const auto flags = get_presentation_flags(specs);

                return std::make_pair(classic_numeric_source_reader<CharT>{},
                                      float_classic_value_reader<CharT>{flags});
            }
            auto make_specs_userlocale_readers(
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc) const
            {
                const auto flags = get_presentation_flags(specs);

                return std::make_pair(
                    until_space_localized_source_reader<CharT>{loc},
                    float_classic_value_reader<CharT>{flags});
            }
            auto make_specs_localized_readers(
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc) const
            {
                const auto flags = get_presentation_flags(specs);

                return std::make_pair(
                    until_space_localized_source_reader<CharT>{loc},
                    float_localized_value_reader<CharT>{flags, loc});
            }
        };

        template <typename CharT>
        class reader<float, CharT> : public float_reader<float, CharT> {};
        template <typename CharT>
        class reader<double, CharT> : public float_reader<double, CharT> {};
        template <typename CharT>
        class reader<long double, CharT>
            : public float_reader<long double, CharT> {};
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
