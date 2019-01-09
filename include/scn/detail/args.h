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

#ifndef SCN_ARGS_H
#define SCN_ARGS_H

#include "core.h"

namespace scn {
    template <typename Context>
    class basic_arg {
    public:
        using char_type = typename Context::char_type;

        template <typename T>
        explicit basic_arg(T& val) : m_value{std::addressof(val), custom_arg<T>}
        {
        }

        error visit(Context& ctx)
        {
            return m_value.scan(m_value.value, ctx);
        }

    private:
        template <typename T>
        static error custom_arg(void* arg, Context& ctx)
        {
            typename Context::template value_scanner_type<T> s;
            auto err = s.parse(ctx);
            if (!err) {
                return err;
            }
            return s.scan(*static_cast<T*>(arg), ctx);
        }

        detail::custom_value<Context> m_value;
    };

    template <typename Context>
    class basic_args;

    template <typename Context, typename... Args>
    class arg_store {
    public:
        arg_store(Args&... a) : m_data{{basic_arg<Context>(a)...}} {}

        span<basic_arg<Context>> data()
        {
            return make_span(m_data.data(),
                             static_cast<std::ptrdiff_t>(m_data.size()));
        }

    private:
        static SCN_CONSTEXPR const size_t size = sizeof...(Args);

        std::array<basic_arg<Context>, size> m_data;
    };

    template <typename Context, typename... Args>
    arg_store<Context, Args...> make_args(Args&... args)
    {
        return arg_store<Context, Args...>(args...);
    }

    template <typename Context>
    class basic_args {
    public:
        basic_args() = default;

        template <typename... Args>
        basic_args(arg_store<Context, Args...>& store) : m_args(store.data())
        {
        }

        basic_args(span<basic_arg<Context>> args) : m_args(args) {}

        span<basic_arg<Context>> get() const
        {
            return m_args;
        }

        error visit(Context& ctx)
        {
            auto& pctx = ctx.parse_context();
            auto arg = m_args.begin();
            for (auto it = pctx.begin(); it != pctx.end();
                 it = pctx.begin()) {
                if (ctx.locale().is_space(*it)) {
                    // Skip whitespace from format string and from stream
                    auto ret = parse_whitespace(ctx);
                    if (!ret) {
                        auto rb = ctx.stream().roll_back();
                        if (!rb) {
                            return rb;
                        }
                        return ret;
                    }
                }
                else if (*it != ctx.locale().widen('{')) {
                    // Check for any non-specifier {foo} characters
                    auto ret = ctx.stream().read_char();
                    if (!ret || ret.value() != *it) {
                        auto rb = ctx.stream().roll_back();
                        if (!rb) {
                            return rb;
                        }
                        if (!ret) {
                            return ret.get_error();
                        }
                        else {
                            return error::invalid_scanned_value;
                        }
                    }
                    pctx.advance();
                }
                else {
                    // Scan argument
                    if (arg == m_args.end()) {
                        return error::invalid_format_string;
                    }
                    auto ret = arg->visit(ctx);
                    if (!ret) {
                        auto rb = ctx.stream().roll_back();
                        if (!rb) {
                            return rb;
                        }
                        return ret;
                    }
                    ++arg;
                    pctx.advance();
                }
            }
            auto srb = ctx.stream().set_roll_back();
            if (!srb) {
                return srb;
            }
            return {};
        }

    private:
        span<basic_arg<Context>> m_args;
    };
}  // namespace scn

#endif  // SCN_ARGS_H

