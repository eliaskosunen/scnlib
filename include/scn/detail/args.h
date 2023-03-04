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

/**
 * @file scn/detail/args.h
 *
 * This file is quite directly ported from {fmt} (fmt/core.h and fmt/format.h).
 */

#include <scn/detail/error.h>
#include <scn/detail/unicode.h>
#include <scn/util/meta.h>

#include <array>
#include <cstddef>
#include <string>
#include <tuple>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        struct monostate {};

        enum class arg_type {
            none_type,
            schar_type,
            short_type,
            int_type,
            long_type,
            llong_type,
            uchar_type,
            ushort_type,
            uint_type,
            ulong_type,
            ullong_type,
            bool_type,
            narrow_character_type,
            wide_character_type,
            code_point_type,
            pointer_type,
            float_type,
            double_type,
            ldouble_type,
            narrow_string_view_type,
            wide_string_view_type,
            narrow_string_type,
            wide_string_type,
            custom_type,
            last_type = custom_type
        };

        template <typename T, typename CharT>
        struct arg_type_constant
            : std::integral_constant<arg_type, arg_type::custom_type> {
            using type = T;
        };

#define SCN_TYPE_CONSTANT(Type, C)                        \
    template <typename CharT>                             \
    struct arg_type_constant<Type, CharT>                 \
        : std::integral_constant<arg_type, arg_type::C> { \
        using type = Type;                                \
    }

        SCN_TYPE_CONSTANT(signed char, schar_type);
        SCN_TYPE_CONSTANT(short, short_type);
        SCN_TYPE_CONSTANT(int, int_type);
        SCN_TYPE_CONSTANT(long, long_type);
        SCN_TYPE_CONSTANT(long long, llong_type);
        SCN_TYPE_CONSTANT(unsigned char, uchar_type);
        SCN_TYPE_CONSTANT(unsigned short, ushort_type);
        SCN_TYPE_CONSTANT(unsigned int, uint_type);
        SCN_TYPE_CONSTANT(unsigned long, ulong_type);
        SCN_TYPE_CONSTANT(unsigned long long, ullong_type);
        SCN_TYPE_CONSTANT(bool, bool_type);
        SCN_TYPE_CONSTANT(char, narrow_character_type);
        SCN_TYPE_CONSTANT(wchar_t, wide_character_type);
        SCN_TYPE_CONSTANT(code_point, code_point_type);
        SCN_TYPE_CONSTANT(void*, pointer_type);
        SCN_TYPE_CONSTANT(float, float_type);
        SCN_TYPE_CONSTANT(double, double_type);
        SCN_TYPE_CONSTANT(long double, ldouble_type);
        SCN_TYPE_CONSTANT(std::string_view, narrow_string_view_type);
        SCN_TYPE_CONSTANT(std::wstring_view, wide_string_view_type);
        SCN_TYPE_CONSTANT(std::string, narrow_string_type);
        SCN_TYPE_CONSTANT(std::wstring, wide_string_type);

#undef SCN_TYPE_CONSTANT

        template <typename Context>
        struct custom_value_type {
            using parse_context = typename Context::parse_context_type;

            void* value;
            scan_error (*scan)(void* arg, parse_context& pctx, Context& ctx);
        };

        struct unscannable {};
        struct unscannable_char : unscannable {};
        struct unscannable_const : unscannable {};

        template <typename T>
        struct custom_wrapper {
            T& val;
        };

        template <typename Context>
        class arg_value {
        public:
            using char_type = typename Context::char_type;

            constexpr arg_value() = default;

            template <typename T>
            explicit constexpr arg_value(T& val)
                : ref_value{std::addressof(val)}
            {
            }

            template <typename T>
            explicit constexpr arg_value(custom_wrapper<T> val)
                : custom_value{static_cast<void*>(&val.val),
                               scan_custom_arg<
                                   T,
                                   typename Context::template scanner_type<T>>}
            {
            }

            arg_value(unscannable);
            arg_value(unscannable_char);
            arg_value(unscannable_const);

            union {
                void* ref_value{nullptr};
                custom_value_type<Context> custom_value;
            };

        private:
            template <typename T, typename Scanner>
            static scan_error scan_custom_arg(
                void* arg,
                typename Context::parse_context_type& pctx,
                Context& ctx)
            {
                auto s = Scanner{};

                auto r = s.parse(pctx)
                             .and_then([&](auto) {
                                 return s.scan(*static_cast<T*>(arg), ctx);
                             })
                             .transform([&](auto&& it) SCN_NOEXCEPT {
                                 ctx.advance_to(SCN_MOVE(it));
                             });

                if (!r) {
                    return r.error();
                }
                return {};
            }
        };

        template <typename CharT>
        struct arg_mapper {
            using char_type = CharT;
            using other_char_type = std::
                conditional_t<std::is_same_v<char_type, char>, wchar_t, char>;

#define SCN_ARG_MAPPER(T) \
    static T& map(T& val) \
    {                     \
        return val;       \
    }
            SCN_ARG_MAPPER(signed char)
            SCN_ARG_MAPPER(short)
            SCN_ARG_MAPPER(int)
            SCN_ARG_MAPPER(long)
            SCN_ARG_MAPPER(long long)
            SCN_ARG_MAPPER(unsigned char)
            SCN_ARG_MAPPER(unsigned short)
            SCN_ARG_MAPPER(unsigned)
            SCN_ARG_MAPPER(unsigned long)
            SCN_ARG_MAPPER(unsigned long long)
            SCN_ARG_MAPPER(wchar_t)
            SCN_ARG_MAPPER(code_point)
            SCN_ARG_MAPPER(bool)
            SCN_ARG_MAPPER(void*)
            SCN_ARG_MAPPER(float)
            SCN_ARG_MAPPER(double)
            SCN_ARG_MAPPER(long double)

            SCN_ARG_MAPPER(std::basic_string_view<char_type>)
            SCN_ARG_MAPPER(std::string)
            SCN_ARG_MAPPER(std::wstring)

#undef SCN_ARG_MAPPER

            static decltype(auto) map(char& val)
            {
                if constexpr (std::is_same_v<char_type, char>) {
                    return val;
                }
                else {
                    SCN_UNUSED(val);
                    return unscannable_char{};
                }
            }
            static unscannable_char map(
                std::basic_string_view<other_char_type>&)
            {
                return {};
            }

            template <typename T>
            static std::enable_if_t<
                std::is_constructible_v<scanner<T, char_type>>,
                custom_wrapper<T>>
            map(T& val)
            {
                return {val};
            }

            static unscannable map(...)
            {
                return {};
            }
        };

        template <typename T, typename CharT>
        using mapped_type_constant = arg_type_constant<
            std::remove_reference_t<decltype(arg_mapper<CharT>().map(
                SCN_DECLVAL(T&)))>,
            CharT>;

        template <typename T, typename CharT>
        using is_scannable = std::integral_constant<
            bool,
            !std::is_base_of_v<unscannable,
                               remove_cvref_t<decltype(arg_mapper<CharT>().map(
                                   SCN_DECLVAL(T&)))>>>;

        constexpr std::size_t packed_arg_bits = 5;
        static_assert((1 << packed_arg_bits) >=
                      static_cast<int>(arg_type::last_type));
        constexpr std::size_t bits_in_sz = sizeof(std::size_t) * 8;
        constexpr std::size_t max_packed_args =
            (bits_in_sz - 1) / packed_arg_bits - 1;
        constexpr std::size_t is_unpacked_bit = std::size_t{1}
                                                << (bits_in_sz - 1);

        template <typename>
        constexpr size_t encode_types_impl()
        {
            return 0;
        }
        template <typename Context, typename T, typename... Others>
        constexpr size_t encode_types_impl()
        {
            return static_cast<unsigned>(
                       mapped_type_constant<
                           T, typename Context::char_type>::value) |
                   (encode_types_impl<Context, Others...>() << packed_arg_bits);
        }

        template <typename Context, typename... Ts>
        constexpr size_t encode_types()
        {
            static_assert(sizeof...(Ts) < (1 << packed_arg_bits));
            return sizeof...(Ts) |
                   (encode_types_impl<Context, Ts...>() << packed_arg_bits);
        }

        template <typename Context, typename T>
        constexpr auto make_value(T& value)
        {
            auto&& arg = arg_mapper<typename Context::char_type>().map(value);

            constexpr bool scannable_char =
                !std::is_same_v<remove_cvref_t<decltype(arg)>,
                                unscannable_char>;
            static_assert(scannable_char,
                          "Cannot scan an argument of an unsupported character "
                          "type (char from a wchar_t source)");

            constexpr bool scannable_const =
                !std::is_same_v<remove_cvref_t<decltype(arg)>,
                                unscannable_const>;
            static_assert(scannable_const, "Cannot scan a const argument");

            constexpr bool scannable =
                !std::is_same_v<remove_cvref_t<decltype(arg)>, unscannable>;
            static_assert(
                scannable,
                "Cannot scan an argument. To make a type T scannable, provide "
                "a scn::scanner<T, CharT> specialization.");

            return arg_value<Context>{arg};
        }

        template <typename... Args>
        constexpr void check_scan_arg_types()
        {
            static_assert(
                std::conjunction<std::is_default_constructible<Args>...>::value,
                "Scan argument types must be default constructible");
            static_assert(
                std::conjunction<std::is_destructible<Args>...>::value,
                "Scan argument types must be Destructible");
            static_assert(!std::conjunction<std::false_type,
                                            std::is_reference<Args>...>::value,
                          "Scan argument types must not be references");
        }

        template <typename Context, typename T>
        constexpr basic_scan_arg<Context> make_arg(T& value)
        {
            check_scan_arg_types<T>();

            basic_scan_arg<Context> arg;
            arg.m_type =
                mapped_type_constant<T, typename Context::char_type>::value;
            arg.m_value = make_value<Context>(value);
            return arg;
        }

        template <bool is_packed,
                  typename Context,
                  arg_type,
                  typename T,
                  typename = std::enable_if_t<is_packed>>
        constexpr arg_value<Context> make_arg(T& value)
        {
            return make_value<Context>(value);
        }
        template <bool is_packed,
                  typename Context,
                  arg_type,
                  typename T,
                  typename = std::enable_if_t<!is_packed>>
        constexpr basic_scan_arg<Context> make_arg(T&& value)
        {
            return make_arg<Context>(SCN_FWD(value));
        }

        template <typename Context>
        constexpr arg_value<Context>& get_arg_value(
            basic_scan_arg<Context>& arg);
    }  // namespace detail

    template <typename Visitor, typename Ctx>
    constexpr decltype(auto) visit_scan_arg(Visitor&& vis,
                                            basic_scan_arg<Ctx>& arg);

    template <typename Context>
    class basic_scan_arg {
    public:
        class handle {
        public:
            explicit handle(detail::custom_value_type<Context> custom)
                : m_custom(custom)
            {
            }

            scan_error scan(typename Context::parse_context_type& parse_ctx,
                            Context& ctx) const
            {
                return m_custom.scan(m_custom.value, parse_ctx, ctx);
            }

        private:
            detail::custom_value_type<Context> m_custom;
        };

        constexpr basic_scan_arg() = default;

        constexpr explicit operator bool() const SCN_NOEXCEPT
        {
            return m_type != detail::arg_type::none_type;
        }

        constexpr detail::arg_type type() const
        {
            return m_type;
        }

        constexpr detail::arg_value<Context>& value()
        {
            return m_value;
        }
        constexpr const detail::arg_value<Context>& value() const
        {
            return m_value;
        }

    private:
        template <typename ContextType, typename T>
        friend constexpr basic_scan_arg<ContextType> detail::make_arg(T& value);

        template <typename C>
        friend constexpr detail::arg_value<C>& detail::get_arg_value(
            basic_scan_arg<C>& arg);

        template <typename Visitor, typename C>
        friend constexpr decltype(auto) visit_scan_arg(Visitor&& vis,
                                                       basic_scan_arg<C>& arg);

        friend class basic_scan_args<Context>;

        detail::arg_value<Context> m_value{};
        detail::arg_type m_type{detail::arg_type::none_type};
    };

    namespace detail {
        template <typename Context>
        constexpr arg_value<Context>& get_arg_value(
            basic_scan_arg<Context>& arg)
        {
            return arg.m_value;
        }

        template <typename Context, std::size_t NumArgs>
        struct scan_arg_store_base {
        protected:
            static constexpr std::size_t num_args = NumArgs;
            static constexpr bool is_packed =
                num_args <= detail::max_packed_args;

            using value_type = std::conditional_t<is_packed,
                                                  detail::arg_value<Context>,
                                                  basic_scan_arg<Context>>;
            using value_array_type = std::array<value_type, num_args>;
        };
    }  // namespace detail

    template <typename Context, typename... Args>
    class scan_arg_store
        : public detail::scan_arg_store_base<Context, sizeof...(Args)> {
        using base = detail::scan_arg_store_base<Context, sizeof...(Args)>;

    public:
        constexpr scan_arg_store() : scan_arg_store(std::tuple<Args...>{}) {}
        constexpr explicit scan_arg_store(std::tuple<Args...>&& a)
            : m_args{std::move(a)},
              m_data{std::apply(make_data_array<Args...>, m_args)}
        {
        }

        std::tuple<Args...>& args()
        {
            return m_args;
        }

    private:
        template <typename... A>
        static constexpr typename base::value_array_type make_data_array(
            A&... args)
        {
            return {detail::make_arg<base::is_packed, Context,
                                     detail::mapped_type_constant<
                                         detail::remove_cvref_t<A>,
                                         typename Context::char_type>::value>(
                args)...};
        }

        constexpr detail::arg_value<Context>& get_value_at(std::size_t i)
        {
            if constexpr (base::is_packed) {
                return m_data[i];
            }
            else {
                return detail::get_arg_value(m_data[i]);
            }
        }

        std::tuple<Args...> m_args;
        typename base::value_array_type m_data;

        friend class basic_scan_args<Context>;

        static constexpr size_t desc =
            base::is_packed ? detail::encode_types<Context, Args...>()
                            : detail::is_unpacked_bit | base::num_args;
    };

    template <typename Context, typename... Args>
    constexpr auto make_scan_args() -> scan_arg_store<Context, Args...>
    {
        detail::check_scan_arg_types<Args...>();
        return {};
    }
    template <typename Context, typename... Args>
    constexpr auto make_scan_args(std::tuple<Args...>&& values)
    {
        detail::check_scan_arg_types<Args...>();
        return scan_arg_store<Context, Args...>{SCN_MOVE(values)};
    }

    template <typename Context>
    class basic_scan_args {
    public:
        constexpr basic_scan_args() = default;

        template <typename... Args>
        constexpr basic_scan_args(scan_arg_store<Context, Args...>& store)
            : basic_scan_args{scan_arg_store<Context, Args...>::desc,
                              store.m_data.data()}
        {
        }

        SCN_NODISCARD constexpr basic_scan_arg<Context> get(
            std::size_t id) const
        {
            if (!is_packed()) {
                if (id < max_size()) {
                    return m_args[id];
                }
                return {};
            }

            if (id >= detail::max_packed_args) {
                return {};
            }

            const auto t = type(id);
            if (t == detail::arg_type::none_type) {
                return {};
            }

            basic_scan_arg<Context> arg;
            arg.m_type = t;
            arg.m_value = m_values[id];
            return arg;
        }

        SCN_NODISCARD constexpr std::size_t max_size() const
        {
            return is_packed() ? detail::max_packed_args
                               : (m_desc & ~detail::is_unpacked_bit);
        }

        SCN_NODISCARD constexpr std::size_t size() const
        {
            if (!is_packed()) {
                return max_size();
            }

            return static_cast<std::size_t>(
                m_desc & ((1 << detail::packed_arg_bits) - 1));
        }

    private:
        constexpr basic_scan_args(size_t desc,
                                  detail::arg_value<Context>* values)
            : m_desc{desc}, m_values{values}
        {
        }
        constexpr basic_scan_args(size_t desc,
                                  basic_scan_args<Context>* args)
            : m_desc{desc}, m_args{args}
        {
        }

        SCN_NODISCARD constexpr bool is_packed() const
        {
            return (m_desc & detail::is_unpacked_bit) == 0;
        }

        SCN_NODISCARD constexpr detail::arg_type type(std::size_t index) const
        {
            // First (0th) index is size, types start after that
            const auto shift = (index + 1) * detail::packed_arg_bits;
            const std::size_t mask = (1 << detail::packed_arg_bits) - 1;
            return static_cast<detail::arg_type>((m_desc >> shift) & mask);
        }

        size_t m_desc{0};
        union {
            detail::arg_value<Context>* m_values;
            basic_scan_arg<Context>* m_args{nullptr};
        };
    };

    SCN_END_NAMESPACE
}  // namespace scn
