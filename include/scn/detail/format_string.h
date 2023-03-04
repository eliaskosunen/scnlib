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

#pragma once

#include <scn/detail/format_string_parser.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename CharT>
    struct basic_runtime_format_string {
        std::basic_string_view<CharT> str;
    };

    inline basic_runtime_format_string<char> runtime(std::string_view s)
    {
        return {{s}};
    }
    inline basic_runtime_format_string<wchar_t> runtime(std::wstring_view s)
    {
        return {{s}};
    }

    namespace detail {
        struct compile_string {};

        template <typename Str>
        inline constexpr bool is_compile_string_v =
            std::is_base_of_v<compile_string, Str>;

        template <typename T, typename Ctx, typename ParseCtx>
        constexpr typename ParseCtx::iterator parse_format_specs(
            ParseCtx& parse_ctx)
        {
            using char_type = typename ParseCtx::char_type;
            using mapped_type = std::conditional_t<
                mapped_type_constant<T, char_type>::value !=
                    arg_type::custom_type,
                std::remove_reference_t<decltype(arg_mapper<char_type>().map(
                    SCN_DECLVAL(T&)))>,
                T>;
            auto s = scanner<mapped_type, char_type>{};
            return s.parse(parse_ctx)
                .transform_error([&](scan_error err) constexpr {
                    parse_ctx.on_error(err.msg());
                    return err;
                })
                .value_or(parse_ctx.end());
        }

        template <typename CharT, typename... Args>
        class format_string_checker {
        public:
            using parse_context_type = compile_parse_context<CharT>;
            static constexpr auto num_args = sizeof...(Args);

            explicit constexpr format_string_checker(
                std::basic_string_view<CharT> format_str)
                : m_parse_context(format_str, num_args, m_types),
                  m_parse_funcs{&parse_format_specs<
                      Args,
                      basic_scan_context<std::basic_string_view<CharT>, CharT>,
                      parse_context_type>...},
                  m_types{arg_type_constant<Args, CharT>::value...}
            {
            }

            constexpr void on_literal_text(const CharT*, const CharT*) const {}

            constexpr auto on_arg_id()
            {
                return m_parse_context.next_arg_id();
            }
            constexpr auto on_arg_id(std::size_t id)
            {
                m_parse_context.check_arg_id(id);
                return id;
            }

            constexpr void on_replacement_field(size_t, const CharT*) {}

            constexpr const CharT* on_format_specs(std::size_t id,
                                                   const CharT* begin,
                                                   const CharT*)
            {
                m_parse_context.advance_to(begin);
                return id < num_args ? m_parse_funcs[id](m_parse_context)
                                     : begin;
            }

            constexpr void check_args_exhausted() const
            {
                m_parse_context.check_args_exhausted(num_args);
            }

            void on_error(const char* msg) const
            {
                m_parse_context.on_error(msg);
            }

            // Only to satisfy the concept and eliminate compiler errors,
            // because errors are reported by failing to compile on_error above
            // (it's not constexpr)
            constexpr explicit operator bool() const
            {
                return true;
            }
            constexpr scan_error get_error() const
            {
                return {};
            }

        private:
            using parse_func = const CharT* (*)(parse_context_type&);

            parse_context_type m_parse_context;
            parse_func m_parse_funcs[num_args > 0 ? num_args : 1];
            arg_type m_types[num_args > 0 ? num_args : 1];
        };

        template <typename... Args, typename Str>
        auto check_format_string(const Str&)
            -> std::enable_if_t<!is_compile_string_v<Str>>
        {
            // TODO: SCN_ENFORE_COMPILE_STRING?
#if 0  // SCN_ENFORE_COMPILE_STRING
                static_assert(dependent_false<Str>::value,
                          "SCN_ENFORCE_COMPILE_STRING requires all format "
                          "strings to use SCN_STRING.");
#endif
        }

        template <typename... Args, typename Str>
        auto check_format_string(Str format_str)
            -> std::enable_if_t<is_compile_string_v<Str>>
        {
            using char_type = typename Str::char_type;

            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wconversion")
            constexpr auto s = std::basic_string_view<char_type>{format_str};
            SCN_GCC_POP

            using checker =
                format_string_checker<char_type, remove_cvref_t<Args>...>;
            constexpr bool invalid_format =
                (parse_format_string<true>(s, checker(s)), true);
            SCN_UNUSED(invalid_format);
        }

        template <typename CharT, std::size_t N>
        constexpr std::basic_string_view<CharT> compile_string_to_view(
            const CharT (&s)[N])
        {
            return {s, N - 1};
        }
        template <typename CharT>
        constexpr std::basic_string_view<CharT> compile_string_to_view(
            std::basic_string_view<CharT> s)
        {
            return s;
        }
    }  // namespace detail

#define SCN_STRING_IMPL(s, base, expl)                                       \
    [] {                                                                     \
        struct SCN_COMPILE_STRING : base {                                   \
            using char_type = ::scn::detail::remove_cvref_t<decltype(s[0])>; \
            SCN_MAYBE_UNUSED constexpr expl                                  \
            operator ::std::basic_string_view<char_type>() const             \
            {                                                                \
                return ::scn::detail::compile_string_to_view<char_type>(s);  \
            }                                                                \
        };                                                                   \
        return SCN_COMPILE_STRING{};                                         \
    }()

#define SCN_STRING(s) SCN_STRING_IMPL(s, ::scn::detail::compile_string, )

    template <typename CharT, typename... Args>
    class basic_format_string {
    public:
        template <
            typename S,
            typename = std::enable_if_t<
                std::is_convertible_v<const S&, std::basic_string_view<CharT>>>>
        SCN_CONSTEVAL basic_format_string(const S& s) : m_str(s)
        {
#if SCN_HAS_CONSTEVAL
            using checker =
                detail::format_string_checker<CharT,
                                              detail::remove_cvref_t<Args>...>;
            const auto e = detail::parse_format_string<true>(m_str, checker(s));
            SCN_UNUSED(e);
#else
            detail::check_format_string<Args...>(s);
#endif
        }

        basic_format_string(basic_runtime_format_string<CharT> r) : m_str(r.str)
        {
        }

        operator std::basic_string_view<CharT>() const
        {
            return m_str;
        }
        std::basic_string_view<CharT> get() const
        {
            return m_str;
        }

    private:
        std::basic_string_view<CharT> m_str;
    };

    SCN_END_NAMESPACE
}  // namespace scn
