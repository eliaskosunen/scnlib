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
#define SCN_ISTREAM_CPP
#endif

#include <scn/istream.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename CharT>
    auto basic_std_istream_stream<CharT>::read_char() -> expected<char_type>
    {
#if SCN_HAS_EXCEPTIONS
        try {
            auto tmp = m_is->get();
            if (tmp == traits::eof()) {
                if (m_is->bad()) {
                    _set_bad();
                    return error(error::unrecoverable_stream_source_error,
                                 "Bad underlying stream");
                }
                if (m_is->eof()) {
                    return error(error::end_of_stream, "EOF");
                }
                return error(error::stream_source_error, "Unknown error");
            }
            ++m_read;
            return static_cast<char_type>(tmp);
        }
        catch (const std::ios_base::failure& e) {
            if (m_is->bad()) {
                _set_bad();
                return error(error::unrecoverable_stream_source_error,
                             e.what());
            }
            if (m_is->eof()) {
                return error(error::end_of_stream, e.what());
            }
            return error(error::stream_source_error, e.what());
        }
#else
        return error(error::exceptions_required,
                     "Reading from a std::basic_istream without exceptions "
                     "enabled is not supported. Use FILEs instead.");
#endif
    }

    template <typename CharT>
    error basic_std_istream_stream<CharT>::putback(char_type ch)
    {
#if SCN_HAS_EXCEPTIONS
        assert(m_read > 0);
        try {
            m_is->putback(ch);
            if (m_is->fail()) {
                _set_bad();
                return error(error::unrecoverable_stream_source_error,
                             "Putback failed");
            }
            --m_read;
            return {};
        }
        catch (const std::ios_base::failure& e) {
            _set_bad();
            return error(error::unrecoverable_stream_source_error, e.what());
        }
#else
        SCN_UNUSED(ch);
        return error(error::exceptions_required,
                     "Reading from a std::basic_istream without exceptions "
                     "enabled is not supported. Use FILEs instead.");
#endif
    }

    template <typename CharT>
    error basic_std_istream_stream<CharT>::roll_back()
    {
        assert(m_read >= 0);
        if (m_read == 0) {
            return {};
        }
        for (auto i = 0; i < m_read; ++i) {
            if (m_is->rdbuf()->sungetc() == traits::eof()) {
                _set_bad();
                return error(error::unrecoverable_stream_source_error,
                             "ungetc failed");
            }
        }
        m_read = 0;
        return {};
    }

    template class basic_std_istream_stream<char>;
    template class basic_std_istream_stream<wchar_t>;

    SCN_END_NAMESPACE
}  // namespace scn
