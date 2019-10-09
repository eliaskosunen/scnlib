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

#ifndef SCN_DETAIL_ARGS_H
#define SCN_DETAIL_ARGS_H

#include "parse_context.h"
#include "small_vector.h"
#include "util.h"

#include <cstring>

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename Context>
    class basic_arg;
    template <typename Context>
    class basic_args;

    template <typename CharT, typename T, typename Enable = void>
    struct scanner;

    /**
     * \ingroup convenience_scan_types
     *
     * Allows reading an rvalue.
     * Stores an rvalue and returns an lvalue reference to it via `operator()`.
     * Create one with \ref temp.
     */
    template <typename T>
    struct temporary {
        temporary(T&& val) : value(std::move(val)) {}

        T& operator()() &&
        {
            return value;
        }

        T value;
    };
    /**
     * \ingroup convenience_scan_types
     *
     * Factory function for \ref temporary.
     */
    template <typename T>
    temporary<T> temp(T&& val)
    {
        return {std::move(val)};
    }

    namespace detail {
        enum type {
            none_type = 0,
            // signed integer
            short_type,
            int_type,
            long_type,
            long_long_type,
            // unsigned integer
            ushort_type,
            uint_type,
            ulong_type,
            ulong_long_type,
            // other integral types
            bool_type,
            char_type,
            last_integer_type = char_type,
            // floats
            float_type,
            double_type,
            long_double_type,
            last_numeric_type = long_double_type,
            // other
            buffer_type,
            string_type,
            string_view_type,

            custom_type
        };

        SCN_CONSTEXPR bool is_integral(type t) noexcept
        {
            return t > none_type && t <= last_integer_type;
        }
        SCN_CONSTEXPR bool is_arithmetic(type t) noexcept
        {
            return t > none_type && t <= last_numeric_type;
        }

        struct custom_value {
            // using scan_type = error (*)(void*, Context&, ParseCtx&);

            void* value;
            void (*scan)();
        };

        template <typename Context, typename ParseCtx, typename T>
        error scan_custom_arg(void* arg, Context& ctx, ParseCtx& pctx)
        {
            SCN_EXPECT(arg != nullptr);

            typename Context::template scanner_type<T> s;
            auto err = pctx.parse(s);
            if (!err) {
                return err;
            }
            return s.scan(*static_cast<T*>(arg), ctx);
        }

        struct monostate {
        };

        template <typename ParseCtx>
        struct parse_ctx_tag {
        };

        template <typename Context>
        class value {
        public:
            using char_type = typename Context::char_type;
            using arg_type = typename Context::arg_type;

            SCN_CONSTEXPR value() noexcept : m_empty{} {}

            template <typename T>
            value(T& val) noexcept : m_value(std::addressof(val))
            {
            }

            template <typename ParseCtx, typename T>
            value(parse_ctx_tag<ParseCtx>, T& val)
                : m_custom(
                      custom_value{std::addressof(val),
                                   reinterpret_cast<void (*)()>(
                                       &scan_custom_arg<Context, ParseCtx, T>)})
            {
            }

            template <typename T>
            T& get_as() noexcept
            {
                return *static_cast<T*>(m_value);
            }
            template <typename T>
            const T& get_as() const noexcept
            {
                return *static_cast<const T*>(m_value);
            }

            custom_value& get_custom() noexcept
            {
                return m_custom;
            }
            const custom_value& get_custom() const noexcept
            {
                return m_custom;
            }

        private:
            union {
                monostate m_empty;
                void* m_value;
                custom_value m_custom;
            };
        };

        template <typename Context, typename T, type Type>
        struct init {
            T* val;
            static const type type_tag = Type;

            SCN_CONSTEXPR init(T& v) : val(std::addressof(v)) {}
            template <typename ParseCtx>
            SCN_CONSTEXPR14 value<Context> get()
            {
                SCN_EXPECT(val != nullptr);
                return value<Context>(*val);
            }
        };
        template <typename Context, typename T>
        struct init<Context, T, custom_type> {
            T* val;
            static const type type_tag = custom_type;

            SCN_CONSTEXPR init(T& v) : val(std::addressof(v)) {}
            template <typename ParseCtx>
            SCN_CONSTEXPR14 value<Context> get()
            {
                SCN_EXPECT(val != nullptr);
                return value<Context>(parse_ctx_tag<ParseCtx>(), *val);
            }
        };

        template <typename Context, typename ParseCtx, typename T>
        SCN_CONSTEXPR14 typename Context::arg_type make_arg(T& value);

#define SCN_MAKE_VALUE(Tag, Type)                                           \
    template <typename C>                                                   \
    SCN_CONSTEXPR init<C, Type, Tag> make_value(Type& val, priority_tag<1>) \
    {                                                                       \
        return val;                                                         \
    }

        SCN_MAKE_VALUE(short_type, short)
        SCN_MAKE_VALUE(int_type, int)
        SCN_MAKE_VALUE(long_type, long)
        SCN_MAKE_VALUE(long_long_type, long long)

        SCN_MAKE_VALUE(ushort_type, unsigned short)
        SCN_MAKE_VALUE(uint_type, unsigned)
        SCN_MAKE_VALUE(ulong_type, unsigned long)
        SCN_MAKE_VALUE(ulong_long_type, unsigned long long)

        SCN_MAKE_VALUE(bool_type, bool)

        SCN_MAKE_VALUE(float_type, float)
        SCN_MAKE_VALUE(double_type, double)
        SCN_MAKE_VALUE(long_double_type, long double)

        SCN_MAKE_VALUE(buffer_type, span<typename C::char_type>)
        SCN_MAKE_VALUE(string_type, std::basic_string<typename C::char_type>)
        SCN_MAKE_VALUE(string_view_type,
                       basic_string_view<typename C::char_type>)

        template <typename C>
        init<C, typename C::char_type, char_type> make_value(
            typename C::char_type& val,
            priority_tag<1>)
        {
            return val;
        }

        template <typename T, typename Char, typename Enable = void>
        struct convert_to_int
            : std::integral_constant<bool,
                                     !std::is_arithmetic<T>::value &&
                                         std::is_convertible<T, int>::value> {
        };
        template <typename C, typename T>
        inline auto make_value(T& val, priority_tag<1>) ->
            typename std::enable_if<
                std::is_enum<T>::value &&
                    convert_to_int<T, typename C::char_type>::value,
                init<C, int, int_type>>::type
        {
            return static_cast<int>(val);
        }

        template <typename C, typename T>
        inline auto make_value(T& val, priority_tag<0>)
            -> init<C, T, custom_type>
        {
            return val;
        }

        enum : size_t { max_packed_args = sizeof(size_t) * 8 / 5 };
        enum : size_t {
            is_unpacked_bit = size_t{1} << (sizeof(size_t) * 8 - 1)
        };
    }  // namespace detail

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")

    /// Type-erased scanning argument.
    template <typename Context>
    class SCN_TRIVIAL_ABI basic_arg {
    public:
        using char_type = typename Context::char_type;

        class handle {
        public:
            explicit handle(detail::custom_value custom)
                : m_custom(std::move(custom))
            {
            }

            template <typename ParseCtx>
            error scan(Context& ctx, ParseCtx& pctx)
            {
                return reinterpret_cast<error (*)(void*, Context&, ParseCtx&)>(
                    m_custom.scan)(m_custom.value, ctx, pctx);
            }

        private:
            detail::custom_value m_custom;
        };

        SCN_CONSTEXPR basic_arg() = default;

        explicit operator bool() const noexcept
        {
            return m_type != detail::none_type;
        }

        detail::type type() const
        {
            return type;
        }
        bool is_integral() const
        {
            return detail::is_integral(m_type);
        }
        bool is_arithmetic() const
        {
            return detail::is_arithmetic(m_type);
        }

    private:
        SCN_CONSTEXPR basic_arg(detail::value<Context> v, detail::type t)
            : m_value(v), m_type(t)
        {
        }

        template <typename ContextType, typename ParseCtx, typename T>
        friend SCN_CONSTEXPR14 typename ContextType::arg_type detail::make_arg(
            T& value);

        template <typename Ctx, typename Visitor>
        friend SCN_CONSTEXPR14 error visit_arg(Visitor&& vis,
                                               typename Ctx::arg_type& arg);

        friend class basic_args<Context>;

        detail::value<Context> m_value{};
        detail::type m_type{detail::none_type};
    };

    SCN_CLANG_POP

    template <typename Context, typename Visitor>
    SCN_CONSTEXPR14 error visit_arg(Visitor&& vis,
                                    typename Context::arg_type& arg)
    {
        using char_type = typename Context::char_type;
        switch (arg.m_type) {
            case detail::none_type:
                break;

            case detail::short_type:
                return vis(arg.m_value.template get_as<short>());
            case detail::int_type:
                return vis(arg.m_value.template get_as<int>());
            case detail::long_type:
                return vis(arg.m_value.template get_as<long>());
            case detail::long_long_type:
                return vis(arg.m_value.template get_as<long long>());

            case detail::ushort_type:
                return vis(arg.m_value.template get_as<unsigned short>());
            case detail::uint_type:
                return vis(arg.m_value.template get_as<unsigned int>());
            case detail::ulong_type:
                return vis(arg.m_value.template get_as<unsigned long>());
            case detail::ulong_long_type:
                return vis(arg.m_value.template get_as<unsigned long long>());

            case detail::bool_type:
                return vis(arg.m_value.template get_as<bool>());
            case detail::char_type:
                return vis(arg.m_value.template get_as<char_type>());

            case detail::float_type:
                return vis(arg.m_value.template get_as<float>());
            case detail::double_type:
                return vis(arg.m_value.template get_as<double>());
            case detail::long_double_type:
                return vis(arg.m_value.template get_as<long double>());

            case detail::buffer_type:
                return vis(arg.m_value.template get_as<span<char_type>>());
            case detail::string_type:
                return vis(
                    arg.m_value
                        .template get_as<std::basic_string<char_type>>());
            case detail::string_view_type:
                return vis(
                    arg.m_value
                        .template get_as<basic_string_view<char_type>>());

            case detail::custom_type:
                return vis(typename Context::arg_type::handle(
                    arg.m_value.get_custom()));

                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wcovered-switch-default")
            default:
                return vis(detail::monostate{});
                SCN_CLANG_POP
        }
        SCN_UNREACHABLE;
    }

    namespace detail {
        template <typename Context, typename T>
        struct get_type {
            using value_type = decltype(make_value<Context>(
                std::declval<typename std::remove_reference<
                    typename std::remove_cv<T>::type>::type&>(),
                std::declval<priority_tag<1>>()));
            static const type value = value_type::type_tag;
        };

        template <typename Context>
        SCN_CONSTEXPR size_t get_types()
        {
            return 0;
        }
        template <typename Context, typename Arg, typename... Args>
        SCN_CONSTEXPR size_t get_types()
        {
            return static_cast<size_t>(get_type<Context, Arg>::value) |
                   (get_types<Context, Args...>() << 5);
        }

        template <typename Context, typename ParseCtx, typename T>
        SCN_CONSTEXPR14 typename Context::arg_type make_arg(T& value)
        {
            typename Context::arg_type arg;
            arg.m_type = get_type<Context, T>::value;
            arg.m_value = make_value<Context>(value, priority_tag<1>{})
                              .template get<ParseCtx>();
            return arg;
        }

        template <bool Packed, typename Context, typename ParseCtx, typename T>
        inline auto make_arg(T& v) ->
            typename std::enable_if<Packed, value<Context>>::type
        {
            return make_value<Context>(v, priority_tag<1>{})
                .template get<ParseCtx>();
        }
        template <bool Packed, typename Context, typename ParseCtx, typename T>
        inline auto make_arg(T& v) ->
            typename std::enable_if<!Packed, typename Context::arg_type>::type
        {
            return make_arg<Context, ParseCtx>(v);
        }
    }  // namespace detail

    template <typename Context, typename... Args>
    class arg_store {
        static SCN_CONSTEXPR const size_t num_args = sizeof...(Args);
        static const bool is_packed = num_args < detail::max_packed_args;

        friend class basic_args<Context>;

        static SCN_CONSTEXPR size_t get_types()
        {
            return is_packed ? detail::get_types<Context, Args...>()
                             : detail::is_unpacked_bit | num_args;
        }

    public:
        static SCN_CONSTEXPR size_t types = get_types();
        using arg_type = typename Context::arg_type;

        using value_type = typename std::
            conditional<is_packed, detail::value<Context>, arg_type>::type;
        static SCN_CONSTEXPR size_t data_size =
            num_args + (is_packed && num_args != 0 ? 0 : 1);

        template <typename ParseCtx>
        arg_store(detail::parse_ctx_tag<ParseCtx>, Args&... a)
            : m_data{{detail::make_arg<is_packed, Context, ParseCtx>(a)...}}
        {
        }

        span<value_type> data()
        {
            return make_span(m_data.data(),
                             static_cast<std::ptrdiff_t>(m_data.size()));
        }

    private:
        detail::array<value_type, data_size> m_data;
    };

    template <typename Context, typename ParseCtx, typename... Args>
    typename Context::template arg_store_type<Args...> make_args(Args&... args)
    {
        return {detail::parse_ctx_tag<ParseCtx>(), args...};
    }

    template <typename Context>
    class basic_args {
    public:
        using arg_type = typename Context::arg_type;

        basic_args() = default;

        template <typename... Args>
        basic_args(arg_store<Context, Args...>& store) : m_types(store.types)
        {
            set_data(store.m_data.data());
        }

        basic_args(span<arg_type> args)
            : m_types(detail::is_unpacked_bit | args.size())
        {
            set_data(args.data());
        }

        arg_type get(std::ptrdiff_t i) const
        {
            return do_get(i);
        }

        bool check_id(std::ptrdiff_t i) const
        {
            if (!is_packed()) {
                return static_cast<size_t>(i) <
                       (m_types &
                        ~static_cast<size_t>(detail::is_unpacked_bit));
            }
            return type(i) != detail::none_type;
        }

        size_t max_size() const
        {
            return is_packed()
                       ? static_cast<size_t>(detail::max_packed_args)
                       : m_types &
                             ~static_cast<size_t>(detail::is_unpacked_bit);
        }

    private:
        size_t m_types{0};
        union {
            detail::value<Context>* m_values;
            arg_type* m_args;
        };

        bool is_packed() const
        {
            return (m_types & detail::is_unpacked_bit) == 0;
        }

        typename detail::type type(std::ptrdiff_t i) const
        {
            size_t shift = static_cast<size_t>(i) * 5;
            return static_cast<typename detail::type>(
                (m_types & (size_t{0x1f} << shift)) >> shift);
        }

        void set_data(detail::value<Context>* values)
        {
            m_values = values;
        }
        void set_data(arg_type* args)
        {
            m_args = args;
        }

        arg_type do_get(std::ptrdiff_t i) const
        {
            SCN_EXPECT(i >= 0);
            if (!is_packed()) {
                auto num_args = static_cast<std::ptrdiff_t>(max_size());
                if (SCN_LIKELY(i < num_args)) {
                    return m_args[i];
                }
                return {};
            }

            SCN_EXPECT(m_values);
            if (SCN_UNLIKELY(
                    i > static_cast<std::ptrdiff_t>(detail::max_packed_args))) {
                return {};
            }

            arg_type arg;
            arg.m_type = type(i);
            if (arg.m_type == detail::none_type) {
                return {};
            }
            arg.m_value = m_values[i];
            return arg;
        }
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_ARGS_H
