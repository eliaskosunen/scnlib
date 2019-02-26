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

#ifndef SCN_ISTREAM_H
#define SCN_ISTREAM_H

#include "detail/core.h"
#include "detail/stream.h"
#include "detail/types.h"

#include <iosfwd>

namespace scn {
    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")

    template <typename CharT>
    class basic_std_istream_stream : public stream_base {
    public:
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
        }
        error putback(char_type ch)
        {
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
                return error(error::unrecoverable_stream_source_error,
                             e.what());
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

    private:
        source_type* m_is;
        long m_read{0};
    };

    SCN_CLANG_POP

    template <typename CharT>
    basic_std_istream_stream<CharT> make_stream(
        std::basic_istream<CharT>& s) noexcept
    {
        return s;
    }

    namespace detail {
        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wpadded")

        template <typename Stream, typename CharT>
        class stream_std_streambuf : public std::basic_streambuf<CharT> {
            using base = std::basic_streambuf<CharT>;

        public:
            using stream_type = Stream;
            using char_type = CharT;
            using traits_type = typename base::traits_type;
            using int_type = typename base::int_type;

            explicit stream_std_streambuf(Stream& s)
                : m_stream(std::addressof(s))
            {
            }

        private:
            int_type underflow() override
            {
                if (m_read) {
                    return traits_type::to_int_type(m_ch);
                }
                auto ret = m_stream->read_char();
                if (!ret) {
                    return traits_type::eof();
                }
                m_ch = ret.value();
                m_read = true;
                return traits_type::to_int_type(m_ch);
            }
            int_type uflow() override
            {
                auto ret = underflow();
                if (ret != traits_type::eof()) {
                    m_read = false;
                }
                return ret;
            }
            std::streamsize showmanyc() override
            {
                return m_read ? 1 : 0;
            }
            int_type pbackfail(int_type c = traits_type::eof()) override
            {
                if (c == traits_type::eof()) {
                    c = 0;
                }
                auto ret = m_stream->putback(traits_type::to_char_type(c));
                if (!ret) {
                    return traits_type::eof();
                }
                return traits_type::to_int_type(m_ch);
            }

            Stream* m_stream;
            char_type m_ch{};
            bool m_read{false};
        };

        SCN_CLANG_POP

        // Trick stolen from fmtlib
        template <typename CharT>
        struct test_std_stream : std::basic_istream<CharT> {
        private:
            struct null;
            // Hide all operator>> from std::basic_istream<CharT>
            void operator>>(null);
        };

        // Check for user-defined operator>>
        template <typename CharT, typename T, typename = void>
        struct is_std_streamable : std::false_type {
        };

        template <typename CharT, typename T>
        struct is_std_streamable<
            CharT,
            T,
            void_t<decltype(std::declval<test_std_stream<CharT>&>() >>
                            std::declval<T&>())>> : std::true_type {
        };
    }  // namespace detail

    template <typename CharT, typename T>
    struct value_scanner<CharT,
                         T,
                         typename std::enable_if<
                             detail::is_std_streamable<CharT, T>::value>::type>
        : public empty_parser<CharT> {
        template <typename Context>
        error scan(T& val, Context& ctx)
        {
            detail::stream_std_streambuf<typename Context::stream_type, CharT>
                streambuf(ctx.stream());
            std::basic_istream<CharT> stream(std::addressof(streambuf));

            if (!(stream >> val)) {
                ctx.stream()._set_bad();
                return error(error::unrecoverable_stream_source_error,
                             "Bad stream after reading");
            }
            return {};
        }
    };
}  // namespace scn

#endif  // SCN_ISTREAM_H

