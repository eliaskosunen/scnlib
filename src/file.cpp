// Copyright 2017-2019 Elias Kosunen
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

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY
#define SCN_FILE_CPP
#endif

#include <scn/detail/file.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        error byte_file::read(span<char> s)
        {
            if (s.size() == 0) {
                return {};
            }
            if (m_it != m_buffer.end()) {
                const auto n = std::min(s.ssize(), m_buffer.end() - m_it);
                std::copy(m_it, m_it + n, s.begin());
                return read(s.subspan(static_cast<size_t>(n)));
            }

            std::string buf(s.size(), '\0');
            const auto n = std::fread(&buf[0], s.size(), 1, m_file);
            std::copy(buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(n),
                      s.begin());
            if (n != s.size()) {
            }
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
