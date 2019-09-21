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
        class byte_file {
        public:
            using fileptr = FILE*;

            byte_file(fileptr f) : m_file(f) {}

            byte_file(const byte_file&) = delete;
            byte_file& operator=(const byte_file&) = delete;

            byte_file(byte_file&& o) : m_file(o.m_file)
            {
                const auto n = std::distance(o.m_buffer.begin(), o.m_it);
                m_buffer = std::move(o.m_buffer);
                m_it = m_buffer.begin() + n;

                o.m_file = nullptr;
                o.m_buffer.clear();
            }
            byte_file& operator=(byte_file&&) = delete;

            ~byte_file()
            {
                flush();
            }

            fileptr& get()
            {
                return m_file;
            }
            fileptr get() const
            {
                return m_file;
            }

            error read(span<char>);
            expected<char> read()
            {
                char ch;
                auto e = read(make_span(&ch, 1));
                if (!e) {
                    return e;
                }
                return {ch};
            }

            void putback(std::ptrdiff_t);

            void flush();

            fileptr m_file;
            std::string m_buffer;
            std::string::iterator m_it;
        };

        class byte_file_iterator {
        public:
            using value_type = expected<char>;
            using reference = expected<char>&;
            using pointer = expected<char>*;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::bidirectional_iterator_tag;

            byte_file_iterator() = default;
            byte_file_iterator(byte_file& f) : m_file(std::addressof(f)) {}

            expected<char> operator*()
            {
                if (!m_latest) {
                    return m_latest.error();
                }
                if (m_latest && m_latest.value() == EOF) {
                    _read_next();
                }
                return static_cast<char>(m_latest.value());
            }

            byte_file_iterator& operator++()
            {
                if (m_latest) {
                    _read_next();
                }
                return *this;
            }
            byte_file_iterator& operator--()
            {
                m_file->putback(1);
                _refresh();
                return *this;
            }

            bool operator==(const byte_file_iterator& o) const
            {
                return _is_sentinel() == o._is_sentinel();
            }
            bool operator!=(const byte_file_iterator& o) const
            {
                return !(operator==(o));
            }

        private:
            void _read_next()
            {
                auto r = m_file->read();
                if (!r) {
                    m_latest = r.error();
                }
                m_latest = static_cast<int>(r.value());
            }

            void _refresh()
            {
                m_latest = *(m_file->m_it);
            }

            bool _is_sentinel() const
            {
                if (!m_file) {
                    return true;
                }
                if (!m_latest) {
                    return m_latest.error() == error::end_of_stream;
                }
                return false;
            }

            byte_file* m_file;
            expected<int> m_latest{EOF};
        };
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_FILE_CPP)
#include "file.cpp"
#endif

#endif  // SCN_DETAIL_FILE_H
