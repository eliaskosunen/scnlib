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

#include <scn/detail/scan_buffer.h>

#include <scn/detail/ranges.h>

#include <cstdio>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename CharT>
        std::optional<CharT> basic_scan_file_buffer<CharT>::read_single()
        {
            SCN_EXPECT(m_file);
            if constexpr (std::is_same_v<CharT, char>) {
                auto ch = std::fgetc(m_file);
                if (SCN_UNLIKELY(ch == EOF)) {
                    return std::nullopt;
                }
                return static_cast<char>(ch);
            }
            else {
                auto ch = std::fgetwc(m_file);
                if (SCN_UNLIKELY(ch == WEOF)) {
                    return std::nullopt;
                }
                return static_cast<wchar_t>(ch);
            }
        }

        template <typename CharT>
        void basic_scan_file_buffer<CharT>::sync(std::ptrdiff_t position)
        {
            SCN_EXPECT(m_file);
            auto buf = this->get_contiguous_segment().substr(position);
            for (auto ch : ranges::views::reverse(buf)) {
                if constexpr (std::is_same_v<CharT, char>) {
                    std::ungetc(static_cast<unsigned char>(ch), m_file);
                }
                else {
                    std::ungetwc(static_cast<wint_t>(ch), m_file);
                }
            }
        }

        template auto basic_scan_file_buffer<char>::read_single()
            -> std::optional<char>;
        // template auto basic_scan_file_buffer<wchar_t>::read_single()
        //     -> std::optional<wchar_t>;

        template void basic_scan_file_buffer<char>::sync(std::ptrdiff_t);
        // template void basic_scan_file_buffer<wchar_t>::sync(std::ptrdiff_t);
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
