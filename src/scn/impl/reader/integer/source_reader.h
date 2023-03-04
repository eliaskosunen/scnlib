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

#include <scn/impl/reader/common.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        using int_classic_source_reader = classic_numeric_source_reader<CharT>;

        template <typename CharT>
        class int_localized_source_reader {
        public:
            using char_type = CharT;
            using string_type = std::basic_string<CharT>;
            using string_view_type = std::basic_string_view<CharT>;

            template <typename T>
            int_localized_source_reader(detail::locale_ref loc,
                                        detail::tag_type<T>)
                : int_localized_source_reader(loc, 16, std::is_signed_v<T>)
            {
            }

            int_localized_source_reader(detail::locale_ref loc,
                                        int base,
                                        bool allow_minus_sign);

            template <typename SourceRange>
            using source_read_result =
                iterator_value_result<ranges::borrowed_iterator_t<SourceRange>,
                                      string_view_type>;

            template <typename SourceRange>
            scan_expected<source_read_result<SourceRange>> read(
                SourceRange&& range)
            {
                until_callback until{m_digits, m_thsep};

                if constexpr (range_supports_nocopy<SourceRange>()) {
                    return {read_until_classic_nocopy(SCN_FWD(range), until)};
                }
                else {
                    source_reader_buffer<CharT>().clear();
                    auto r = read_until_classic_copying(
                        SCN_FWD(range),
                        back_insert(source_reader_buffer<CharT>()), until);

                    SCN_GCC_PUSH
                    SCN_GCC_IGNORE("-Wconversion")
                    return {{r.in, {source_reader_buffer<CharT>()}}};
                    SCN_GCC_POP
                }
            }

        private:
            struct until_callback {
                bool operator()(char ch) const SCN_NOEXCEPT
                {
                    const bool is_digit =
                        digits.find(ch) != std::string_view::npos;
                    if (thsep == 0) {
                        return !is_digit;
                    }
                    if (is_digit) {
                        return false;
                    }
                    return ch != thsep;
                }
                bool operator()(wchar_t wch) const SCN_NOEXCEPT
                {
                    if (static_cast<wint_t>(wch) >
                        static_cast<wint_t>(std::numeric_limits<char>::max())) {
                        return false;
                    }
                    return operator()(static_cast<char>(wch));
                }

                std::string_view digits;
                char_type thsep;
            };

            detail::locale_ref m_locale{};
            std::string_view m_digits{};
            char_type m_thsep{0};
        };

        extern template int_localized_source_reader<
            char>::int_localized_source_reader(detail::locale_ref, int, bool);
        extern template int_localized_source_reader<
            wchar_t>::int_localized_source_reader(detail::locale_ref,
                                                  int,
                                                  bool);
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
