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

#ifndef SCN_DETAIL_FILE_H
#define SCN_DETAIL_FILE_H

#include "range.h"

#include <memory>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        struct file_handle {
#if SCN_WINDOWS
            using handle_type = void*;
#else
            using handle_type = int;
#endif

            static constexpr file_handle invalid()
            {
#if SCN_WINDOWS
                return {nullptr};
#else
                return {-1};
#endif
            }

            handle_type handle;
        };

        class byte_mapped_file {
        public:
            using iterator = const char*;
            using sentinel = const char*;

            byte_mapped_file() = default;
            byte_mapped_file(const char* filename);

            byte_mapped_file(const byte_mapped_file&) = delete;
            byte_mapped_file& operator=(const byte_mapped_file&) = delete;

            byte_mapped_file(byte_mapped_file&& o)
                : m_file(o.m_file), m_begin(o.m_begin), m_end(o.m_end)
            {
                o.m_file = file_handle::invalid();
                o.m_begin = nullptr;
                o.m_end = nullptr;

                SCN_ENSURE(!o.valid());
                SCN_ENSURE(valid());
            }
            byte_mapped_file& operator=(byte_mapped_file&& o)
            {
                if (valid()) {
                    _destruct();
                }
                m_file = o.m_file;
                m_begin = o.m_begin;
                m_end = o.m_end;

                o.m_file = file_handle::invalid();
                o.m_begin = nullptr;
                o.m_end = nullptr;

                SCN_ENSURE(!o.valid());
                SCN_ENSURE(valid());
                return *this;
            }

            ~byte_mapped_file()
            {
                if (valid()) {
                    _destruct();
                }
            }

            bool valid() const
            {
                return m_file.handle != file_handle::invalid().handle;
            }

            iterator begin() const
            {
                return m_begin;
            }
            sentinel end() const
            {
                return m_end;
            }

        private:
            void _destruct();

            file_handle m_file{file_handle::invalid()};
            char* m_begin{nullptr};
            char* m_end{nullptr};
        };
    }  // namespace detail

    template <typename CharT>
    class basic_mapped_file : public detail::byte_mapped_file {
    public:
        using iterator = const CharT*;
        using sentinel = const CharT*;

        using byte_mapped_file::byte_mapped_file;

        // embrace the UB
        iterator begin() const
        {
            return reinterpret_cast<iterator>(byte_mapped_file::begin());
        }
        sentinel end() const
        {
            return reinterpret_cast<sentinel>(byte_mapped_file::end());
        }
    };

    using mapped_file = basic_mapped_file<char>;
    using wmapped_file = basic_mapped_file<wchar_t>;

    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_FILE_CPP)
#include "file.cpp"
#endif

#endif  // SCN_DETAIL_FILE_H
