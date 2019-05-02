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
#define SCN_STREAM_CPP
#endif

#include <scn/detail/stream.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

namespace scn {
    SCN_BEGIN_NAMESPACE

    SCN_FUNC auto basic_cstdio_stream<char>::read_char() -> expected<char_type>
    {
        auto ret = std::fgetc(m_file);
        if (ret == EOF) {
            if (std::ferror(m_file) != 0) {
                return error(error::stream_source_error, std::strerror(errno));
            }
            if (std::feof(m_file) != 0) {
                return error(error::end_of_stream, "EOF");
            }
            return error(error::unrecoverable_stream_source_error,
                         "Unknown error");
        }
        m_read.push_back(static_cast<char_type>(ret));
        return static_cast<char_type>(ret);
    }
    SCN_FUNC error basic_cstdio_stream<char>::putback(char_type ch) noexcept
    {
        SCN_EXPECT(!m_read.empty());
        if (std::ungetc(ch, m_file) == EOF) {
            return error(error::unrecoverable_stream_source_error,
                         std::strerror(errno));
        }
        m_read.pop_back();
        return {};
    }
    SCN_FUNC error basic_cstdio_stream<char>::roll_back() noexcept
    {
        if (m_read.empty()) {
            return {};
        }
        for (auto it = m_read.rbegin(); it != m_read.rend(); ++it) {
            if (std::ungetc(*it, m_file) == EOF) {
                return error(error::unrecoverable_stream_source_error,
                             std::strerror(errno));
            }
        }
        m_read.clear();
        return {};
    }

    SCN_FUNC auto basic_cstdio_stream<wchar_t>::read_char() -> expected<char_type>
    {
        auto ret = std::fgetwc(m_file);
        if (ret == WEOF) {
            if (std::ferror(m_file) != 0) {
                return error(error::stream_source_error, std::strerror(errno));
            }
            if (std::feof(m_file) != 0) {
                return error(error::end_of_stream, "EOF");
            }
            return error(error::unrecoverable_stream_source_error,
                         "Unknown error");
        }
        m_read.push_back(static_cast<char_type>(ret));
        return static_cast<char_type>(ret);
    }
    SCN_FUNC error basic_cstdio_stream<wchar_t>::putback(char_type ch) noexcept
    {
        SCN_EXPECT(!m_read.empty());
        if (std::ungetwc(std::char_traits<char_type>::to_int_type(ch),
                         m_file) == WEOF) {
            return error(error::unrecoverable_stream_source_error,
                         std::strerror(errno));
        }
        m_read.pop_back();
        return {};
    }
    SCN_FUNC error basic_cstdio_stream<wchar_t>::roll_back() noexcept
    {
        if (m_read.empty()) {
            return {};
        }
        for (auto it = m_read.rbegin(); it != m_read.rend(); ++it) {
            if (std::ungetwc(std::char_traits<char_type>::to_int_type(*it),
                             m_file) == WEOF) {
                return error(error::unrecoverable_stream_source_error,
                             std::strerror(errno));
            }
        }
        m_read.clear();
        return {};
    }

    SCN_END_NAMESPACE
}  // namespace scn

