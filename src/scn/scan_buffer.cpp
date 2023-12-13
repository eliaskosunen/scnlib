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
        scan_file_buffer::scan_file_buffer(std::FILE* file)
            : base(base::non_contiguous_tag{}), m_file(file)
        {
        }

        bool scan_file_buffer::fill()
        {
            SCN_EXPECT(m_file);
            if (auto prev = m_latest) {
                this->m_putback_buffer.push_back(*prev);
            }
            auto ch = std::fgetc(m_file);
            if (SCN_UNLIKELY(ch == EOF)) {
                return false;
            }
            this->m_current_view =
                std::basic_string_view<char_type>{&*m_latest, 1};
            return true;
        }

        void scan_file_buffer::sync(std::ptrdiff_t position)
        {
            SCN_EXPECT(m_file);
            auto former_segment = this->get_segment_starting_at(position);
            auto latter_segment = this->get_segment_starting_at(
                static_cast<std::ptrdiff_t>(position + former_segment.size()));
            for (auto ch : ranges::views::reverse(latter_segment)) {
                std::ungetc(static_cast<unsigned char>(ch), m_file);
            }
            for (auto ch : ranges::views::reverse(former_segment)) {
                std::ungetc(static_cast<unsigned char>(ch), m_file);
            }
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
