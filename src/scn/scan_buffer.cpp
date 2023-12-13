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
            : base(std::false_type{}), m_file(file), m_allow_eagerness(false)
        {
        }

        std::optional<char> scan_file_buffer::read_single()
        {
            SCN_EXPECT(m_file);
            auto ch = std::fgetc(m_file);
            if (SCN_UNLIKELY(ch == EOF)) {
                return std::nullopt;
            }
            return static_cast<char>(ch);
        }

        void scan_file_buffer::sync(std::ptrdiff_t position)
        {
            SCN_EXPECT(m_file);
            auto [offset, seg] = this->get_contiguous_segment();
            auto to_put_back = seg.substr(position - offset);
            for (auto ch : ranges::views::reverse(to_put_back)) {
                std::ungetc(static_cast<unsigned char>(ch), m_file);
            }
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
