// Copyright 2017 Elias Kosunen
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

#include "reader.h"

#include <istream>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename WrappedRange>
        class range_streambuf
            : public std::basic_streambuf<typename WrappedRange::char_type> {
            using base = std::basic_streambuf<typename WrappedRange::char_type>;

        public:
            using range_type = WrappedRange;
            using char_type = typename WrappedRange::char_type;
            using traits_type = typename base::traits_type;
            using int_type = typename base::int_type;

            explicit range_streambuf(range_type& r) : m_range(std::addressof(r))
            {
            }

        private:
            int_type underflow() override
            {
                // already read
                if (!traits_type::eq_int_type(m_ch, traits_type::eof())) {
                    return m_ch;
                }

                auto ret = read_char(*m_range);
                if (!ret) {
                    // error
                    // m_ch is already eof
                    return traits_type::eof();
                }
                m_ch = traits_type::to_int_type(ret.value());
                return m_ch;
            }
            int_type uflow() override
            {
                auto ret = underflow();
                if (ret != traits_type::eof()) {
                    m_ch = traits_type::eof();
                }
                return ret;
            }
            std::streamsize showmanyc() override
            {
                return traits_type::eq_int_type(m_ch, traits_type::eof()) ? 0
                                                                          : 1;
            }
            int_type pbackfail(int_type) override
            {
                auto e = putback_n(*m_range, 1);
                if (!e) {
                    return traits_type::eof();
                }
                return traits_type::to_int_type(0);
            }

            range_type* m_range;
            int_type m_ch{traits_type::eof()};
        };

        // Trick stolen from {fmt}
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
        : public empty_parser {
        template <typename Context>
        error scan(T& val, Context& ctx)
        {
            detail::range_streambuf<typename Context::range_type> streambuf(
                ctx.range());
            std::basic_istream<CharT> stream(std::addressof(streambuf));

            if (!(stream >> val)) {
                if (stream.eof()) {
                    return error(error::end_of_range, "EOF");
                }
                if (stream.bad()) {
                    return error(error::unrecoverable_source_error,
                                 "Bad std::istream after reading");
                }
                return error(error::invalid_scanned_value,
                             "Failed to read with std::istream");
            }
            return {};
        }
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_ISTREAM_H
