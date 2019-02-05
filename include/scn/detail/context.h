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

#ifndef SCN_DETAIL_CONTEXT_H
#define SCN_DETAIL_CONTEXT_H

#include "args.h"
#include "core.h"
#include "locale.h"

namespace scn {
    namespace detail {
        template <typename Stream, typename Context>
        class context_base {
        public:
            using stream_type = Stream;
            using char_type = typename stream_type::char_type;
            using parse_context_type = basic_parse_context<char_type>;
            using locale_type = basic_locale_ref<char_type>;

            SCN_CONSTEXPR14 parse_context_type& parse_context() noexcept
            {
                return m_parse_ctx;
            }
            SCN_CONSTEXPR14 stream_type& stream() noexcept
            {
                return *m_stream;
            }
            basic_arg<Context> arg(unsigned id) const
            {
                return m_args.get(id);
            }
            locale_type locale() const
            {
                return m_locale;
            }

            detail::error_handler error_handler()
            {
                return m_parse_ctx.error_handler();
            }

            void on_error(error e)
            {
                m_parse_ctx.on_error(e);
            }
            void on_error(const char* e)
            {
                m_parse_ctx.on_error(e);
            }

        protected:
#if SCN_CLANG >= SCN_COMPILER(3, 9, 0)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif
            context_base(stream_type& s,
                         basic_string_view<char_type> f,
                         basic_args<Context> args,
                         locale_type l = locale_type())
                : m_stream(std::addressof(s)),
                  m_parse_ctx(f),
                  m_args(std::move(args)),
                  m_locale(std::move(l))
            {
            }
#if SCN_CLANG >= SCN_COMPILER(3, 9, 0)
#pragma clang diagnostic pop
#endif

            using arg_type = basic_arg<Context>;

            result<arg_type> do_get_arg(unsigned id)
            {
                auto a = m_args.get(id);
                if (!a && !m_args.get(id - 1)) {
                    return make_error(error::invalid_argument);
                }
                return a;
            }

            result<arg_type> arg(unsigned id)
            {
                return m_parse_ctx.check_arg_id(id) ? do_get_arg(id)
                                                    : arg_type();
            }

        private:
            stream_type* m_stream;
            parse_context_type m_parse_ctx;
            basic_args<Context> m_args;
            locale_type m_locale;
        };
    }  // namespace detail

    /// Scanning context.
    template <typename Stream>
    class basic_context
        : public detail::context_base<Stream, basic_context<Stream>> {
        using base = detail::context_base<Stream, basic_context<Stream>>;

    public:
        using stream_type = typename base::stream_type;
        using char_type = typename base::char_type;
        using locale_type = typename base::locale_type;

        using arg_type = typename base::arg_type;

        /// basic_value_scanner to use with a specific `T`
        template <typename T>
        using value_scanner_type = basic_value_scanner<char_type, T>;

        basic_context(stream_type& s,
                      basic_string_view<char_type> f,
                      basic_args<basic_context> args,
                      locale_type l = locale_type())
            : base(s, f, std::move(args), std::move(l))
        {
        }

        result<arg_type> next_arg()
        {
            return this->do_get_arg(this->parse_context().next_arg_id());
        }
        result<arg_type> arg(unsigned id)
        {
            return this->do_get_arg(id);
        }

        result<arg_type> arg(basic_string_view<char_type> name);

    private:
        detail::arg_map<basic_context> m_map;

        using base::arg;
    };

    namespace detail {
        template <typename CharT>
        struct dummy_stream {
            using char_type = CharT;
        };
        template <typename CharT>
        struct dummy_context {
            using type = basic_context<dummy_stream<CharT>>;
        };

        template <typename CharT>
        struct named_arg_base {
            using context_type = typename dummy_context<CharT>::type;
            using arg_type = basic_arg<context_type>;
            using storage_type =
                typename std::aligned_storage<sizeof(arg_type),
                                              alignof(arg_type)>::type;

            named_arg_base(basic_string_view<CharT> n) : name(n) {}

            template <typename Context>
            basic_arg<Context> deserialize()
            {
                basic_arg<Context> arg;
                std::memcpy(&arg, &data, sizeof(arg_type));
                return arg;
            }

            basic_string_view<CharT> name;
            storage_type data;
        };

        template <typename T, typename CharT>
        struct named_arg : named_arg_base<CharT> {
            using base = named_arg_base<CharT>;

            named_arg(basic_string_view<CharT> name, T& val)
                : base(name), value(std::addressof(val))
            {
            }

            T* value;
        };
    }  // namespace detail

    template <typename T>
    detail::named_arg<T, char> arg(string_view name, T& arg)
    {
        return {name, std::addressof(arg)};
    }
    template <typename T>
    detail::named_arg<T, wchar_t> arg(wstring_view name, T& arg)
    {
        return {name, std::addressof(arg)};
    }

    template <typename S, typename T, typename Char>
    void arg(S, detail::named_arg<T, Char>) = delete;
}  // namespace scn

#endif  // SCN_DETAIL_CONTEXT_H
