// Copyright 2017-2018 Elias Kosunen
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

#ifndef SCN_ISTREAM_H
#define SCN_ISTREAM_H

#include "scn/core.h"
#include "scn/stream.h"

#include <istream>

namespace scn {
    template <typename CharT>
    struct basic_std_istream_stream {
        using char_type = CharT;
        using source_type = std::basic_istream<char_type>;
        using traits = typename source_type::traits_type;

        basic_std_istream_stream(source_type& is) : m_is(std::addressof(is)) {}

        result<char_type> read_char()
        {
            try {
                auto tmp = m_is->get();
                if (tmp == traits::eof()) {
                    if (m_is->bad()) {
                        return error::unrecoverable_stream_source_error;
                    }
                    if (m_is->fail()) {
                        return error::stream_source_error;
                    }
                    return error::end_of_stream;
                }
                ++m_read;
                return static_cast<char_type>(tmp);
            }
            catch (const std::ios_base::failure& e) {
                if (m_is->bad()) {
                    return error::unrecoverable_stream_source_error;
                }
                if (m_is->fail()) {
                    return error::stream_source_error;
                }
                return error::end_of_stream;
            }
        }
        error putback(char_type ch)
        {
            assert(m_read > 0);
            try {
                m_is->putback(ch);
                if (m_is->fail()) {
                    return error::unrecoverable_stream_source_error;
                }
                --m_read;
                return {};
            }
            catch (const std::ios_base::failure& e) {
                return error::unrecoverable_stream_source_error;
            }
        }

        error set_roll_back()
        {
            m_read = 0;
            return {};
        }
        error roll_back()
        {
            assert(m_read >= 0);
            if (m_read == 0) {
                return {};
            }
            try {
                m_is->seekg(-m_read, std::ios_base::cur);
                if (m_is->bad()) {
                    return error::unrecoverable_stream_source_error;
                }
                if (m_is->fail()) {
                    return error::stream_source_error;
                }
                m_read = 0;
                return {};
            }
            catch (const std::ios_base::failure& e) {
                if (m_is->bad()) {
                    return error::unrecoverable_stream_source_error;
                }
                return error::stream_source_error;
            }
        }

    private:
        source_type* m_is;
        long m_read{0};
    };
}  // namespace scn

#endif  // SCN_ISTREAM_H

