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
        class number_preparer_base {
        public:
            std::basic_string_view<CharT> get_output() const
            {
                if (m_output.empty()) {
                    return m_input;
                }
                return m_output;
            }

            using iterator = typename std::basic_string_view<CharT>::iterator;
            iterator get_input_end_iterator(iterator output_end) const;

        protected:
            number_preparer_base(std::basic_string_view<CharT> input)
                : m_input(input), m_checker{input}
            {
            }

            scan_error check_thsep_grouping(std::string_view grouping,
                                            iterator output_end_it) const;

            struct thsep_checker {
                void start(iterator);
                void mark(iterator);
                void end(iterator);

                bool check(std::string_view, iterator) const;

                bool has_any() const
                {
                    return !indices.empty();
                }

                std::basic_string_view<CharT> input;
                mutable std::string indices{};

            private:
                void transform_indices(std::ptrdiff_t) const;
            };

            std::basic_string_view<CharT> m_input;
            std::basic_string<CharT> m_output;
            thsep_checker m_checker{};
        };

        extern template auto number_preparer_base<char>::get_input_end_iterator(
            std::string_view::iterator) const -> std::string_view::iterator;
        extern template auto number_preparer_base<
            wchar_t>::get_input_end_iterator(std::wstring_view::iterator) const
            -> std::wstring_view::iterator;

        template <typename CharT>
        class int_preparer : public number_preparer_base<CharT> {
        public:
            int_preparer(std::basic_string_view<CharT> input)
                : number_preparer_base<CharT>(input)
            {
            }

            void prepare_with_thsep(CharT thsep);
            void prepare_without_thsep();

            using iterator = typename number_preparer_base<CharT>::iterator;
            scan_expected<iterator> check_grouping_and_get_end_iterator(
                std::string_view grouping,
                iterator output_it) const;
        };

        extern template void int_preparer<char>::prepare_with_thsep(char);
        extern template void int_preparer<wchar_t>::prepare_with_thsep(wchar_t);

        extern template void int_preparer<char>::prepare_without_thsep();
        extern template void int_preparer<wchar_t>::prepare_without_thsep();

        extern template auto
            int_preparer<char>::check_grouping_and_get_end_iterator(
                std::string_view,
                std::string_view::iterator) const
            -> scan_expected<std::string_view::iterator>;
        extern template auto
            int_preparer<wchar_t>::check_grouping_and_get_end_iterator(
                std::string_view,
                std::wstring_view::iterator) const
            -> scan_expected<std::wstring_view::iterator>;

        template <typename CharT>
        class float_preparer : public number_preparer_base<CharT> {
        public:
            float_preparer(std::basic_string_view<CharT> input)
                : number_preparer_base<CharT>(input)
            {
            }

            void prepare_without_thsep(CharT decimal_point);
            void prepare_with_thsep(CharT thsep, CharT decimal_point);

            using iterator = typename number_preparer_base<CharT>::iterator;
            scan_expected<iterator> check_grouping_and_get_end_iterator(
                std::string_view grouping,
                iterator output_it) const;

        private:
            std::ptrdiff_t m_decimal_point_input_index{-1};
        };

        extern template void float_preparer<char>::prepare_without_thsep(char);
        extern template void float_preparer<wchar_t>::prepare_without_thsep(
            wchar_t);

        extern template void float_preparer<char>::prepare_with_thsep(char,
                                                                      char);
        extern template void float_preparer<wchar_t>::prepare_with_thsep(
            wchar_t,
            wchar_t);

        extern template auto
            float_preparer<char>::check_grouping_and_get_end_iterator(
                std::string_view,
                std::string_view::iterator) const
            -> scan_expected<std::string_view::iterator>;
        extern template auto
            float_preparer<wchar_t>::check_grouping_and_get_end_iterator(
                std::string_view,
                std::wstring_view::iterator) const
            -> scan_expected<std::wstring_view::iterator>;
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
