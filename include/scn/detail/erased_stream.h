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

#ifndef SCN_DETAIL_ERASED_STREAM_H
#define SCN_DETAIL_ERASED_STREAM_H

#include "stream.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")

    namespace detail {
        template <typename CharT>
        class erased_stream_base {
        public:
            using char_type = CharT;

            erased_stream_base(const erased_stream_base&) = delete;
            erased_stream_base& operator=(const erased_stream_base&) = delete;
            erased_stream_base(erased_stream_base&&) = default;
            erased_stream_base& operator=(erased_stream_base&&) = default;
            virtual ~erased_stream_base() = default;

            virtual expected<char_type> read_char() = 0;
            virtual error putback(char_type) = 0;

            virtual error set_roll_back() = 0;
            virtual error roll_back() = 0;

        protected:
            erased_stream_base() = default;
        };

        template <typename CharT>
        class erased_sized_stream_base {
        public:
            using char_type = CharT;

            erased_sized_stream_base(const erased_sized_stream_base&) = delete;
            erased_sized_stream_base& operator=(
                const erased_sized_stream_base&) = delete;
            erased_sized_stream_base(erased_sized_stream_base&&) = default;
            erased_sized_stream_base& operator=(erased_sized_stream_base&&) =
                default;
            virtual ~erased_sized_stream_base() = default;

            virtual void read_sized(span<CharT> s) = 0;

            virtual void putback_n(size_t n) = 0;

            virtual size_t chars_to_read() const = 0;

            virtual void skip(size_t n) = 0;
            virtual void skip_all() = 0;

        protected:
            erased_sized_stream_base() = default;
        };

        template <typename Stream>
        class erased_stream_impl
            : public erased_stream_base<typename Stream::char_type> {
            using base = erased_stream_base<typename Stream::char_type>;

        public:
            using char_type = typename base::char_type;

            erased_stream_impl(Stream s) : m_stream(std::move(s)) {}

            expected<char_type> read_char() override
            {
                SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                return m_stream.read_char();
                SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
            }
            error putback(char_type ch) override
            {
                SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                return m_stream.putback(ch);
                SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
            }

            error set_roll_back() override
            {
                return m_stream.set_roll_back();
            }
            error roll_back() override
            {
                SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                return m_stream.roll_back();
                SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
            }

            Stream& get()
            {
                return m_stream;
            }
            const Stream& get() const
            {
                return m_stream;
            }

        private:
            Stream m_stream;
        };

        template <typename Stream>
        class erased_sized_stream_impl
            : public erased_sized_stream_base<typename Stream::char_type> {
            using base = erased_sized_stream_base<typename Stream::char_type>;

        public:
            using char_type = typename base::char_type;

            erased_sized_stream_impl(Stream& s) : m_stream(std::addressof(s)) {}

            void read_sized(span<char_type> s) override
            {
                m_stream->read_sized(s);
            }

            void putback_n(size_t n) override
            {
                m_stream->putback_n(n);
            }

            size_t chars_to_read() const override
            {
                return m_stream->chars_to_read();
            }

            void skip(size_t n) override
            {
                m_stream->skip(n);
            }
            void skip_all() override
            {
                m_stream->skip_all();
            }

            Stream& get()
            {
                return *m_stream;
            }
            const Stream& get() const
            {
                return *m_stream;
            }

        private:
            Stream* m_stream;
        };
    }  // namespace detail

    template <typename CharT>
    class erased_stream : public stream_base {
    public:
        using char_type = CharT;
        template <typename Stream>
        erased_stream(Stream s)
            : m_stream(detail::make_unique<detail::erased_stream_impl<Stream>>(
                  std::move(s)))
        {
        }

        expected<char_type> read_char()
        {
            return m_stream->read_char();
        }
        error putback(char_type ch)
        {
            return m_stream->putback(ch);
        }

        error set_roll_back()
        {
            return m_stream->set_roll_back();
        }
        error roll_back()
        {
            return m_stream->roll_back();
        }

        detail::erased_stream_base<CharT>& get()
        {
            return *m_stream;
        }
        const detail::erased_stream_base<CharT>& get() const
        {
            return *m_stream;
        }

        template <typename Stream>
        detail::erased_stream_impl<Stream>& get_as()
        {
            return static_cast<detail::erased_stream_impl<Stream>&>(*m_stream);
        }
        template <typename Stream>
        const detail::erased_stream_impl<Stream>& get_as() const
        {
            return static_cast<const detail::erased_stream_impl<Stream>&>(
                *m_stream);
        }

    private:
        detail::unique_ptr<detail::erased_stream_base<CharT>> m_stream;
    };

    template <typename CharT>
    class erased_sized_stream : public erased_stream<CharT> {
        using base = erased_stream<CharT>;

    public:
        using char_type = CharT;
        using is_sized_stream = std::true_type;

        template <typename Stream>
        erased_sized_stream(Stream s)
            : base(std::move(s)),
              m_stream(
                  detail::make_unique<detail::erased_sized_stream_impl<Stream>>(
                      base::template get_as<Stream>().get()))
        {
        }

        void read_sized(span<char_type> s)
        {
            m_stream->read_sized(s);
        }

        void putback_n(size_t n)
        {
            m_stream->putback_n(n);
        }

        size_t chars_to_read() const
        {
            return m_stream->chars_to_read();
        }

        void skip(size_t n)
        {
            m_stream->skip(n);
        }
        void skip_all()
        {
            m_stream->skip_all();
        }

        detail::erased_sized_stream_base<CharT>& get_sized()
        {
            return *m_stream;
        }
        const detail::erased_sized_stream_base<CharT>& get_sized() const
        {
            return *m_stream;
        }

    private:
        detail::unique_ptr<detail::erased_sized_stream_base<CharT>> m_stream;
    };

    SCN_CLANG_POP

    template <typename Stream,
              typename std::enable_if<!is_sized_stream<
                  typename std::remove_reference<Stream>::type>::value>::type* =
                  nullptr>
    erased_stream<typename Stream::char_type> erase_stream(Stream&& s)
    {
        return erased_stream<typename Stream::char_type>(
            std::forward<Stream>(s));
    }
    template <typename Stream,
              typename std::enable_if<is_sized_stream<
                  typename std::remove_reference<Stream>::type>::value>::type* =
                  nullptr>
    erased_sized_stream<typename Stream::char_type> erase_stream(Stream&& s)
    {
        return erased_sized_stream<typename Stream::char_type>(
            std::forward<Stream>(s));
    }

    template <typename CharT>
    erased_stream<CharT> erase_stream(erased_stream<CharT> s) = delete;
    template <typename CharT>
    erased_sized_stream<CharT> erase_stream(erased_sized_stream<CharT> s) =
        delete;

    template <typename... Args>
    auto make_erased_stream(Args&&... a)
        -> decltype(erase_stream(make_stream(std::forward<Args>(a)...)))
    {
        auto stream = make_stream(std::forward<Args>(a)...);
        return erase_stream(std::move(stream));
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_ERASED_STREAM_H
