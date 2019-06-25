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

    template <typename T>
    struct temporary {
        temporary(T&& val) : value(std::move(val)) {}

        T& operator()() &&
        {
            return value;
        }

        T value;
    };
    template <typename T>
    temporary<T> temp(T&& val)
    {
        return {std::move(val)};
    }

    namespace detail {
        template <typename CharT>
        struct named_arg_base;

        template <typename T, typename CharT>
        struct named_arg;

        enum type {
            none_type = 0,
            named_arg_type,
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
            custom_type
        };

        SCN_CONSTEXPR bool is_integral(type t) noexcept
        {
            SCN_EXPECT(t != named_arg_type);
            return t > none_type && t <= last_integer_type;
        }
        SCN_CONSTEXPR bool is_arithmetic(type t) noexcept
        {
            SCN_EXPECT(t != named_arg_type);
            return t > none_type && t <= last_numeric_type;
        }

        template <typename Context>
        struct custom_value {
            using fn_type = error (*)(void*, Context&);

            void* value;
            fn_type scan;
        };

        template <typename Context, typename T>
        error scan_custom_arg(void* arg, Context& ctx)
        {
            SCN_EXPECT(arg != nullptr);

            typename Context::template scanner_type<T> s;
            auto err = ctx.parse_context().parse(s, ctx);
            if (!err) {
                return err;
            }
            return s.scan(*static_cast<T*>(arg), ctx);
        }

        struct monostate {
        };

        template <typename Context>
        class value {
        public:
            using char_type = typename Context::char_type;
            using arg_type = typename Context::arg_type;

            union {
                monostate empty_value;

                short* short_value;
                int* int_value;
                long* long_value;
                long long* long_long_value;

                unsigned short* ushort_value;
                unsigned* uint_value;
                unsigned long* ulong_value;
                unsigned long long* ulong_long_value;

                bool* bool_value;
                char_type* char_value;

                float* float_value;
                double* double_value;
                long double* long_double_value;

                span<char_type>* buffer_value;
                std::basic_string<char_type>* string_value;
                custom_value<Context> custom;

                void* pointer;
            };

            SCN_CONSTEXPR value() : empty_value{} {}

            value(short& val) : short_value(&val) {}
            value(int& val) : int_value(&val) {}
            value(long& val) : long_value(&val) {}
            value(long long& val) : long_long_value(&val) {}

            value(unsigned short& val) : ushort_value(&val) {}
            value(unsigned& val) : uint_value(&val) {}
            value(unsigned long& val) : ulong_value(&val) {}
            value(unsigned long long& val) : ulong_long_value(&val) {}

            value(bool& val) : bool_value(&val) {}
            value(char_type& val) : char_value(&val) {}

            value(float& val) : float_value(&val) {}
            value(double& val) : double_value(&val) {}
            value(long double& val) : long_double_value(&val) {}

            value(span<char_type>& val) : buffer_value(&val) {}
            value(std::basic_string<char_type>& val) : string_value(&val) {}

            value(void* val) : pointer(val) {}

            template <typename T>
            value(T& val)
                : custom(custom_value<Context>{std::addressof(val),
                                               scan_custom_arg<Context, T>})
            {
            }

            named_arg_base<char_type>& as_named_arg()
            {
                SCN_EXPECT(pointer != nullptr);
                return *static_cast<named_arg_base<char_type>*>(pointer);
            }
        };

        template <typename Context, typename T, type Type>
        struct init {
            T* val;
            static const type type_tag = Type;

            SCN_CONSTEXPR init(T& v) : val(std::addressof(v)) {}
            SCN_CONSTEXPR14 operator value<Context>()
            {
                SCN_EXPECT(val != nullptr);
                return value<Context>(*val);
            }
        };

        template <typename Context, typename T>
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

        template <typename C>
        init<C, typename C::char_type, char_type> make_value(
            typename C::char_type& val,
            priority_tag<1>)
        {
            return val;
        }

        template <typename C, typename T>
        init<C, void*, named_arg_type> make_value(
            named_arg<T, typename C::char_type>& val,
            priority_tag<1>)
        {
            auto arg = make_arg<C>(val.value);
            std::memcpy(val.data, &arg, sizeof(arg));
            return static_cast<void*>(&val);
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

        template <typename Context>
        class arg_map;
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
            explicit handle(detail::custom_value<Context> custom)
                : m_custom(std::move(custom))
            {
            }

            error scan(Context& ctx)
            {
                return m_custom.scan(m_custom.value, ctx);
            }

        private:
            detail::custom_value<Context> m_custom;
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

        template <typename ContextType, typename T>
        friend SCN_CONSTEXPR14 typename ContextType::arg_type detail::make_arg(
            T& value);

        template <typename Ctx, typename Visitor>
        friend SCN_CONSTEXPR14 error visit_arg(Visitor&& vis,
                                               typename Ctx::arg_type& arg);

        friend class basic_args<Context>;
        friend class detail::arg_map<Context>;

        detail::value<Context> m_value{};
        detail::type m_type{detail::none_type};
    };

    SCN_CLANG_POP

    template <typename Context, typename Visitor>
    SCN_CONSTEXPR14 error visit_arg(Visitor&& vis,
                                    typename Context::arg_type& arg)
    {
        SCN_EXPECT(arg.m_type != detail::named_arg_type);
        switch (arg.m_type) {
            case detail::none_type:
            case detail::named_arg_type:
                break;

            case detail::short_type:
                return vis(*arg.m_value.short_value);
            case detail::int_type:
                return vis(*arg.m_value.int_value);
            case detail::long_type:
                return vis(*arg.m_value.long_value);
            case detail::long_long_type:
                return vis(*arg.m_value.long_long_value);

            case detail::ushort_type:
                return vis(*arg.m_value.ushort_value);
            case detail::uint_type:
                return vis(*arg.m_value.uint_value);
            case detail::ulong_type:
                return vis(*arg.m_value.ulong_value);
            case detail::ulong_long_type:
                return vis(*arg.m_value.ulong_long_value);

            case detail::bool_type:
                return vis(*arg.m_value.bool_value);
            case detail::char_type:
                return vis(*arg.m_value.char_value);

            case detail::float_type:
                return vis(*arg.m_value.float_value);
            case detail::double_type:
                return vis(*arg.m_value.double_value);
            case detail::long_double_type:
                return vis(*arg.m_value.long_double_value);

            case detail::buffer_type:
                return vis(*arg.m_value.buffer_value);
            case detail::string_type:
                return vis(*arg.m_value.string_value);

            case detail::custom_type:
                return vis(
                    typename Context::arg_type::handle(arg.m_value.custom));

                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wcovered-switch-default")
            default:
                return vis(detail::monostate{});
                SCN_CLANG_POP
        }
        SCN_UNREACHABLE;
    }

    namespace detail {
        template <typename Context>
        class arg_map {
        public:
            using char_type = typename Context::char_type;

            arg_map() = default;

            void init(const basic_args<Context>& args)
            {
                if (!m_args.empty()) {
                    return;
                }

                m_args.resize(args.max_size());
                if (args.is_packed()) {
                    for (size_t i = 0;; ++i) {
                        switch (args.type(i)) {
                            case none_type:
                                return;
                            case named_arg_type:
                                push_back(args.m_values[i]);
                                break;
                        }
                    }
                }
                for (size_t i = 0;; ++i) {
                    switch (args.m_args[i].m_type) {
                        case none_type:
                            return;
                        case named_arg_type:
                            push_back(args.m_args[i].m_value);
                            break;
                    }
                }
            }

            typename Context::arg_type find(
                basic_string_view<char_type> name) const
            {
                SCN_EXPECT(!m_args.empty());
                // Use for instead of find_if to avoid including <algorithm>
                for (auto& e : m_args) {
                    if (e.name == name) {
                        return e.arg;
                    }
                }
                return {};
            }

        private:
            struct entry {
                basic_string_view<char_type> name;
                typename Context::arg_type arg;
            };

            void push_back(value<Context> val)
            {
                const named_arg_base<char_type>& named = val.as_named_arg();
                m_args.emplace_back(named.name,
                                    named.template deserialize<Context>());
                SCN_ENSURE(!m_args.empty());
            }

            detail::small_vector<entry, 2> m_args;
        };

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

        template <typename Context, typename T>
        SCN_CONSTEXPR14 typename Context::arg_type make_arg(T& value)
        {
            typename Context::arg_type arg;
            arg.m_type = get_type<Context, T>::value;
            arg.m_value = make_value<Context>(value, priority_tag<1>{});
            return arg;
        }

        template <bool Packed, typename Context, typename T>
        inline auto make_arg(T& v) ->
            typename std::enable_if<Packed, value<Context>>::type
        {
            return make_value<Context>(v, priority_tag<1>{});
        }
        template <bool Packed, typename Context, typename T>
        inline auto make_arg(T& v) ->
            typename std::enable_if<!Packed, typename Context::arg_type>::type
        {
            return typename Context::arg_type(v);
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

        arg_store(Args&... a)
            : m_data{{detail::make_arg<is_packed, Context>(a)...}}
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

    template <typename Context, typename... Args>
    typename Context::template arg_store_type<Args...> make_args(Args&... args)
    {
        return {args...};
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

        arg_type get(size_t i) const
        {
            auto arg = do_get(i);
            if (arg.m_type == detail::named_arg_type) {
                arg =
                    arg.m_value.as_named_arg().template deserialize<Context>();
            }
            return arg;
        }

        bool check_id(size_t i) const
        {
            if (!is_packed()) {
                return i < (m_types &
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

        typename detail::type type(size_t i) const
        {
            size_t shift = i * 5;
            return static_cast<typename detail::type>(
                (m_types & (size_t{0x1f} << shift)) >> shift);
        }

        friend class detail::arg_map<Context>;

        void set_data(detail::value<Context>* values)
        {
            m_values = values;
        }
        void set_data(arg_type* args)
        {
            m_args = args;
        }

        arg_type do_get(size_t i) const
        {
            if (!is_packed()) {
                auto num_args = max_size();
                if (SCN_LIKELY(i < num_args)) {
                    return m_args[i];
                }
                return {};
            }

            if (SCN_UNLIKELY(i > detail::max_packed_args))
                return {};

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
