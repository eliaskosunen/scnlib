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
        if (m_it != m_read.end()) {
            auto ch = *m_it;
            ++m_it;
            return ch;
        }
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
            auto ch = static_cast<char_type>(tmp);
            m_read.push_back(ch);
            m_it = m_read.end();
            return ch;
        }
        catch (const std::ios_base::failure&) {
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
#else
        return error(error::exceptions_required,
                     "Reading from a std::basic_istream without exceptions "
                     "enabled is not supported. Use FILEs instead.");
#endif
    }

    template class basic_std_istream_stream<char>;
    template class basic_std_istream_stream<wchar_t>;

    SCN_END_NAMESPACE
}  // namespace scn
