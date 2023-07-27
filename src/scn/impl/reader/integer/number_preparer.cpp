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

#include <scn/impl/reader/number_preparer.h>
#include "scn/impl/util/ascii_ctype.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        void number_preparer_base<CharT>::thsep_checker::start(iterator begin)
        {
            input = input.substr(
                static_cast<size_t>(ranges::distance(input.begin(), begin)));
        }

        template <typename CharT>
        void number_preparer_base<CharT>::thsep_checker::mark(iterator it)
        {
            indices.push_back(
                static_cast<char>(ranges::distance(input.begin(), it)));
        }

        template <typename CharT>
        void number_preparer_base<CharT>::thsep_checker::end(iterator it)
        {
            auto n = static_cast<size_t>(ranges::distance(input.begin(), it));
            if (n > input.size()) {
                return;
            }
            input = input.substr(0, n);
        }

        template <typename CharT>
        void number_preparer_base<CharT>::thsep_checker::transform_indices(
            std::ptrdiff_t last_thsep_index) const
        {
            for (auto thsep_it = indices.rbegin(); thsep_it != indices.rend();
                 ++thsep_it) {
                const auto tmp = *thsep_it;
                *thsep_it = static_cast<char>(last_thsep_index - tmp - 1);
                last_thsep_index = static_cast<std::ptrdiff_t>(tmp);
            }
            indices.insert(indices.begin(),
                           static_cast<char>(last_thsep_index));
        }

        template <typename CharT>
        bool number_preparer_base<CharT>::thsep_checker::check(
            std::string_view grouping,
            iterator input_end_it) const
        {
            if (input.begin() >= input_end_it && input.end() <= input_end_it) {
                transform_indices(
                    ranges::distance(input.begin(), input_end_it));
            }
            else {
                transform_indices(static_cast<std::ptrdiff_t>(input.size()));
            }

            auto thsep_it = indices.rbegin();
            for (auto grouping_it = grouping.begin();
                 grouping_it != grouping.end() &&
                 thsep_it != indices.rend() - 1;
                 ++grouping_it, (void)++thsep_it) {
                if (*thsep_it != *grouping_it) {
                    return false;
                }
            }

            for (; thsep_it < indices.rend() - 1; ++thsep_it) {
                if (*thsep_it != grouping.back()) {
                    return false;
                }
            }
            if (thsep_it == indices.rend() - 1) {
                if (*thsep_it > grouping.back()) {
                    return false;
                }
            }

            return true;
        }

        template <typename CharT>
        auto number_preparer_base<CharT>::get_input_end_iterator(
            iterator output_it) const -> iterator
        {
            const auto diff = ranges::distance(output_it, get_output().end());
            return m_input.end() - diff;
        }

        template auto number_preparer_base<char>::get_input_end_iterator(
            std::string_view::iterator) const -> std::string_view::iterator;
        template auto number_preparer_base<wchar_t>::get_input_end_iterator(
            std::wstring_view::iterator) const -> std::wstring_view::iterator;

        template <typename CharT>
        scan_error number_preparer_base<CharT>::check_thsep_grouping(
            std::string_view grouping,
            iterator output_end_it) const
        {
            SCN_EXPECT(m_checker.has_any());

            if (!m_checker.check(grouping,
                                 get_input_end_iterator(output_end_it))) {
                return {scan_error::invalid_scanned_value,
                        "Invalid thousands separator grouping"};
            }

            return {};
        }

        template <typename CharT>
        auto int_preparer<CharT>::check_grouping_and_get_end_iterator(
            std::string_view grouping,
            iterator output_it) const -> scan_expected<iterator>
        {
            if (this->m_checker.has_any()) {
                if (auto e = this->check_thsep_grouping(grouping, output_it);
                    !e) {
                    return unexpected(e);
                }
            }

            return this->get_input_end_iterator(output_it);
        }

        namespace {
            template <typename CharT>
            size_t get_first_character_index(
                std::basic_string_view<CharT> input)
            {
                if (input[0] == CharT{'+'} || input[0] == CharT{'-'}) {
                    return 1;
                }

                return 0;
            }

            template <typename Ch>
            constexpr bool is_int_char(Ch ch)
            {
                return ch == Ch{'+'} || ch == Ch{'-'} ||
                       (ch >= Ch{'0'} && ch <= Ch{'9'}) ||
                       (ch >= Ch{'A'} && ch <= Ch{'Z'}) ||
                       (ch >= Ch{'a'} && ch <= Ch{'z'});
            }
        }  // namespace

        template <typename CharT>
        void int_preparer<CharT>::prepare_with_thsep(CharT thsep)
        {
            auto i = get_first_character_index(this->m_input);
            this->m_checker.start(this->m_input.begin() + i);

            for (; i < this->m_input.size(); ++i) {
                if (this->m_input[i] == thsep) {
                    this->m_checker.mark(this->m_input.begin() + i);
                    continue;
                }

                if (is_int_char(this->m_input[i])) {
                    this->m_output.push_back(this->m_input[i]);
                    continue;
                }

                break;
            }

            this->m_input = this->m_input.substr(0, i);
            this->m_checker.end(this->m_input.begin() + i);
        }

        template <typename CharT>
        void int_preparer<CharT>::prepare_without_thsep()
        {
            this->m_checker.start(this->m_input.begin());
            this->m_checker.end(this->m_input.end());
        }

        template void int_preparer<char>::prepare_with_thsep(char);
        template void int_preparer<wchar_t>::prepare_with_thsep(wchar_t);

        template void int_preparer<char>::prepare_without_thsep();
        template void int_preparer<wchar_t>::prepare_without_thsep();

        template auto int_preparer<char>::check_grouping_and_get_end_iterator(
            std::string_view,
            std::string_view::iterator) const
            -> scan_expected<std::string_view::iterator>;
        template auto
            int_preparer<wchar_t>::check_grouping_and_get_end_iterator(
                std::string_view,
                std::wstring_view::iterator) const
            -> scan_expected<std::wstring_view::iterator>;

        namespace {
            template <typename CharT>
            bool copy_ascii_lowercase(std::basic_string_view<CharT> input,
                                      span<char> output)
            {
                SCN_EXPECT(input.size() >= output.size());

                for (std::size_t i = 0; i < output.size(); ++i) {
                    if constexpr (!std::is_same_v<CharT, char>) {
                        if (!is_ascii_char(input[i])) {
                            output[i] = 0;
                            continue;
                        }
                    }
                    output[i] = static_cast<char>(input[i]);
                    output[i] |= 0x20;
                }
                return true;
            }

            template <typename CharT>
            std::ptrdiff_t is_input_inf(std::basic_string_view<CharT> input)
            {
                if (input.size() < 3) {
                    return 0;
                }

                char buf[9]{0};
                if (input.size() >= 8) {
                    if (copy_ascii_lowercase(input, {buf, 8})) {
                        if (std::strcmp(buf, "infinity") == 0) {
                            return 8;
                        }
                    }
                }
                else {
                    if (!copy_ascii_lowercase(input, {buf, 3})) {
                        return 0;
                    }
                }

                if (std::strncmp(buf, "inf", 3) == 0) {
                    return 3;
                }
                return 0;
            }

            template <typename CharT>
            std::ptrdiff_t is_input_nan(std::basic_string_view<CharT> input)
            {
                if (input.size() < 3) {
                    return 0;
                }

                char buf[4]{0};
                if (!copy_ascii_lowercase(input, {buf, 3})) {
                    return 0;
                }
                if (std::strcmp(buf, "nan") != 0) {
                    return 0;
                }

                if (input.size() <= 4 || input[3] != CharT{'('}) {
                    return 3;
                }
                return static_cast<std::ptrdiff_t>(input.find(')')) + 1;
            }

            bool is_float_char(char ch)
            {
                return std::string_view{"+-0123456789abcdefpxABCDEFPX"}.find(
                           ch) != std::string_view::npos;
            }
            bool is_float_char(wchar_t ch)
            {
                return is_ascii_char(ch) &&
                       is_float_char(static_cast<char>(ch));
            }

            template <typename CharT>
            auto prepare_float_without_thsep_nocopy(
                std::basic_string_view<CharT> input) ->
                typename std::basic_string_view<CharT>::iterator
            {
                if (auto n = is_input_inf(input); n != 0) {
                    return input.begin() + n;
                }
                if (auto n = is_input_nan(input); n != 0) {
                    return input.begin() + n;
                }

                for (auto it = input.begin(); it != input.end(); ++it) {
                    if (*it == CharT{'.'}) {
                        continue;
                    }
                    if (is_float_char(*it)) {
                        continue;
                    }
                    return it;
                }
                return input.end();
            }
        }  // namespace

        template <typename CharT>
        void float_preparer<CharT>::prepare_without_thsep(CharT decimal_point)
        {
            const auto idx = get_first_character_index(this->m_input);
            const auto stripped_input = this->m_input.substr(idx);
            this->m_checker.start(stripped_input.begin());

            if (decimal_point == CharT{'.'}) {
                auto it = prepare_float_without_thsep_nocopy(stripped_input);
                this->m_input = this->m_input.substr(
                    0, static_cast<size_t>(
                           ranges::distance(this->m_input.begin(), it)));
                this->m_checker.end(it);
                return;
            }

            if (auto n = is_input_inf(stripped_input); n != 0) {
                this->m_checker.end(stripped_input.begin() + n);
                this->m_input =
                    this->m_input.substr(0, static_cast<size_t>(n) + idx);
                return;
            }
            if (auto n = is_input_nan(stripped_input); n != 0) {
                this->m_checker.end(stripped_input.begin() + n);
                this->m_input =
                    this->m_input.substr(0, static_cast<size_t>(n) + idx);
                return;
            }

            auto it = stripped_input.begin();
            for (; it != stripped_input.end(); ++it) {
                if (*it == decimal_point) {
                    this->m_output.push_back(CharT{'.'});
                    continue;
                }
                if (is_float_char(*it)) {
                    this->m_output.push_back(*it);
                    continue;
                }
                break;
            }

            this->m_checker.end(it);
            this->m_input =
                this->m_input.substr(0, static_cast<size_t>(ranges::distance(
                                            this->m_input.begin(), it)) +
                                            idx);
        }

        template <typename CharT>
        void float_preparer<CharT>::prepare_with_thsep(CharT thsep,
                                                       CharT decimal_point)
        {
            auto i = get_first_character_index(this->m_input);
            const auto stripped_input = this->m_input.substr(i);
            this->m_checker.start(stripped_input.begin());

            if (auto n = is_input_inf(stripped_input); n != 0) {
                this->m_checker.end(stripped_input.begin() + n);
                this->m_input =
                    this->m_input.substr(0, static_cast<size_t>(n) + i);
                return;
            }
            if (auto n = is_input_nan(stripped_input); n != 0) {
                this->m_checker.end(stripped_input.begin() + n);
                this->m_input =
                    this->m_input.substr(0, static_cast<size_t>(n) + i);
                return;
            }

            for (; i < this->m_input.size(); ++i) {
                if (this->m_input[i] == decimal_point) {
                    if (m_decimal_point_input_index != -1) {
                        break;
                    }
                    this->m_output.push_back(CharT{'.'});
                    m_decimal_point_input_index =
                        static_cast<std::ptrdiff_t>(i);
                    this->m_checker.end(this->m_input.begin() + i);
                    continue;
                }

                if (this->m_input[i] == thsep) {
                    if (m_decimal_point_input_index == -1) {
                        this->m_checker.mark(this->m_input.begin() + i);
                        continue;
                    }
                    break;
                }

                if (is_float_char(this->m_input[i])) {
                    this->m_output.push_back(this->m_input[i]);
                    continue;
                }

                break;
            }

            this->m_checker.end(this->m_input.begin() + i);
            this->m_input = this->m_input.substr(0, i);
        }

        template <typename CharT>
        auto float_preparer<CharT>::check_grouping_and_get_end_iterator(
            std::string_view grouping,
            iterator output_it) const -> scan_expected<iterator>
        {
            if (this->m_checker.has_any()) {
                if (auto e = this->check_thsep_grouping(grouping, output_it);
                    !e) {
                    return unexpected(e);
                }
            }

            return this->get_input_end_iterator(output_it);
        }

        template void float_preparer<char>::prepare_without_thsep(char);
        template void float_preparer<wchar_t>::prepare_without_thsep(wchar_t);

        template void float_preparer<char>::prepare_with_thsep(char, char);
        template void float_preparer<wchar_t>::prepare_with_thsep(wchar_t,
                                                                  wchar_t);

        template auto float_preparer<char>::check_grouping_and_get_end_iterator(
            std::string_view,
            std::string_view::iterator) const
            -> scan_expected<std::string_view::iterator>;
        template auto
            float_preparer<wchar_t>::check_grouping_and_get_end_iterator(
                std::string_view,
                std::wstring_view::iterator) const
            -> scan_expected<std::wstring_view::iterator>;
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
