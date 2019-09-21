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

#if SCN_POSIX
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        SCN_FUNC byte_mapped_file::byte_mapped_file(const char* filename)
        {
#if SCN_POSIX
            int fd = open(filename, O_RDONLY);
            if (fd == -1) {
                return;
            }
            m_file.handle = fd;

            struct stat s;
            int status = fstat(fd, &s);
            if (status == -1) {
                return;
            }
            auto size = s.st_size;

            auto ptr =
                static_cast<char*>(mmap(nullptr, static_cast<size_t>(size),
                                        PROT_READ, MAP_PRIVATE, fd, 0));
            if (ptr == MAP_FAILED) {
                m_file = file_handle::invalid();
                return;
            }

            m_begin = ptr;
            m_end = m_begin + size;
#elif SCN_WINDOWS

#else
            SCN_UNUSED(filename);
#endif
        }

        SCN_FUNC void byte_mapped_file::_destruct()
        {
            munmap(m_begin, static_cast<size_t>(m_end - m_begin));
            *this = mapped_file{};
            SCN_ENSURE(!valid());
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
