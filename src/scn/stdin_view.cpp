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

#include <scn/detail/stdin_view.h>

#include <cstdio>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        void stdin_manager::sync_now(stdin_iterator& begin)
        {
            auto buf = std::string_view{m_putback_buffer}.substr(
                begin.m_current_index);
            for (char ch : ranges::views::reverse(buf)) {
                std::ungetc(static_cast<unsigned char>(ch), stdin);
            }
            m_putback_buffer.clear();
            if (m_end_index > 0) {
                m_end_index -= begin.m_current_index;
            }
            begin.m_current_index = 0;
            SCN_ENSURE(m_end_index >= begin.m_current_index ||
                       m_end_index == -1);
        }

        auto stdin_manager::extract_char() const -> std::optional<char>
        {
            auto ch = std::fgetc(stdin);
            if (ch == EOF) {
                return std::nullopt;
            }
            return static_cast<char>(ch);
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
