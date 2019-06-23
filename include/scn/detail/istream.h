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

#ifndef SCN_DETAIL_ISTREAM_H
#define SCN_DETAIL_ISTREAM_H

#include "stream.h"
#include "types.h"

#include <istream>

namespace scn {
    SCN_BEGIN_NAMESPACE
    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")

    template <typename CharT>
    class basic_std_istream_stream : public stream_base {
    public:
        using char_type = CharT;
        using source_type = std::basic_istream<char_type>;
        using traits = typename source_type::traits_type;

        basic_std_istream_stream(source_type& is)
            : m_is(std::addressof(is)), m_read{}, m_it(m_read.begin())
        {
        }

        expected<char_type> read_char();

        error putback(char_type)
        {
            SCN_EXPECT(m_it != m_read.begin());
            --m_it;
            return {};
        }

        error set_roll_back()
        {
            m_read.clear();
            m_it = m_read.end();
            return {};
        }
        error roll_back()
        {
            m_it = m_read.begin();
            return {};
        }

    private:
        using buffer_type = detail::small_vector<char_type, 32>;
        using iterator = typename buffer_type::iterator;

        source_type* m_is;
        buffer_type m_read{};
        iterator m_it;
    };

    SCN_CLANG_POP

    template <typename CharT>
    basic_std_istream_stream<CharT> make_stream(
        std::basic_istream<CharT>& is) noexcept
    {
        return {is};
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
    struct scanner<CharT,
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
    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_ISTREAM_CPP)
#include "istream.cpp"
#endif

#endif  // SCN_DETAIL_ISTREAM_H

