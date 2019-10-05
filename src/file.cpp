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

#include <cstdio>

#if SCN_POSIX
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#elif SCN_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
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

            struct stat s;
            int status = fstat(fd, &s);
            if (status == -1) {
                close(fd);
                return;
            }
            auto size = s.st_size;

            auto ptr =
                static_cast<char*>(mmap(nullptr, static_cast<size_t>(size),
                                        PROT_READ, MAP_PRIVATE, fd, 0));
            if (ptr == MAP_FAILED) {
                close(fd);
                return;
            }

            m_file.handle = fd;
            m_begin = ptr;
            m_end = m_begin + size;
#elif SCN_WINDOWS
            auto f = CreateFileA(
                filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
            if (f == INVALID_HANDLE_VALUE) {
                return;
            }

            auto size = GetFileSize(f, NULL);
            if (size == INVALID_FILE_SIZE) {
                CloseHandle(f);
                return;
            }

            auto h = CreateFileMappingA(f, NULL, PAGE_READONLY, 0, size, NULL);
            if (h == NULL) {
                CloseHandle(f);
                return;
            }

            m_file.handle = f;
            m_begin = static_cast<char*>(h);
            m_end = static_cast<char*>(h) + size;
#else
            SCN_UNUSED(filename);
#endif
        }

        SCN_FUNC void byte_mapped_file::_destruct()
        {
#if SCN_POSIX
            munmap(m_begin, static_cast<size_t>(m_end - m_begin));
            close(m_file.handle);
#elif SCN_WINDOWS
            CloseHandle(m_begin);
            CloseHandle(m_file.handle);
#endif
            *this = mapped_file{};
            SCN_ENSURE(!valid());
        }

        template <>
        SCN_FUNC expected<char> cfile_iterator<char>::operator*() const
        {
            SCN_EXPECT(valid());
            int tmp = std::fgetc(file().file());
            if (tmp == EOF) {
                if (std::feof(file().file()) != 0) {
                    return error(error::end_of_range, "EOF");
                }
                if (std::ferror(file().file()) != 0) {
                    return error(error::source_error, "fgetc error");
                }
                return error(error::unrecoverable_source_error,
                             "Unknown fgetc error");
            }
            return static_cast<char>(tmp);
        }
        template <>
        SCN_FUNC expected<wchar_t> cfile_iterator<wchar_t>::operator*() const
        {
            SCN_EXPECT(valid());
            wint_t tmp = std::fgetwc(file().file());
            if (tmp == WEOF) {
                if (std::feof(file().file()) != 0) {
                    return error(error::end_of_range, "EOF");
                }
                if (std::ferror(file().file()) != 0) {
                    return error(error::source_error, "fgetc error");
                }
                return error(error::unrecoverable_source_error,
                             "Unknown fgetc error");
            }
            return static_cast<wchar_t>(tmp);
        }
    }  // namespace detail

    template <>
    SCN_FUNC bool basic_file<char>::sync() const
    {
        return m_cache.sync([&](span<char> s) {
            for (auto it = s.rbegin(); it != s.rend(); ++it) {
                if (std::ungetc(static_cast<unsigned char>(*it), m_file) ==
                    EOF) {
                    return false;
                }
            }
            return true;
        });
    }
    template <>
    SCN_FUNC bool basic_file<wchar_t>::sync() const
    {
        return m_cache.sync([&](span<wchar_t> s) {
            for (auto it = s.rbegin(); it != s.rend(); ++it) {
                if (std::ungetwc(static_cast<wint_t>(*it), m_file) == WEOF) {
                    return false;
                }
            }
            return true;
        });
    }

    SCN_END_NAMESPACE
}  // namespace scn
