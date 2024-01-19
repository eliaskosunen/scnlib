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
#include <scn/detail/input_map.h>
#include <scn/util/expected.h>
#include <scn/util/meta.h>

#include <array>
#include <cstddef>
#include <tuple>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
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
    narrow_regex_matches_type,
    wide_regex_matches_type,
    custom_type,
    last_type = custom_type
};

template <typename>
inline constexpr bool is_type_disabled = SCN_DISABLE_TYPE_CUSTOM;

template <typename T, typename CharT>
struct arg_type_constant
    : std::integral_constant<arg_type, arg_type::custom_type> {
    using type = T;
};

#define SCN_TYPE_CONSTANT(Type, C, Disabled)              \
    template <typename CharT>                             \
    struct arg_type_constant<Type, CharT>                 \
        : std::integral_constant<arg_type, arg_type::C> { \
        using type = Type;                                \
    };                                                    \
    template <>                                           \
    inline constexpr bool is_type_disabled<Type> = Disabled

SCN_TYPE_CONSTANT(signed char, schar_type, SCN_DISABLE_TYPE_SCHAR);
SCN_TYPE_CONSTANT(short, short_type, SCN_DISABLE_TYPE_SHORT);
SCN_TYPE_CONSTANT(int, int_type, SCN_DISABLE_TYPE_INT);
SCN_TYPE_CONSTANT(long, long_type, SCN_DISABLE_TYPE_LONG);
SCN_TYPE_CONSTANT(long long, llong_type, SCN_DISABLE_TYPE_LONG_LONG);
SCN_TYPE_CONSTANT(unsigned char, uchar_type, SCN_DISABLE_TYPE_UCHAR);
SCN_TYPE_CONSTANT(unsigned short, ushort_type, SCN_DISABLE_TYPE_USHORT);
SCN_TYPE_CONSTANT(unsigned int, uint_type, SCN_DISABLE_TYPE_UINT);
SCN_TYPE_CONSTANT(unsigned long, ulong_type, SCN_DISABLE_TYPE_ULONG);
SCN_TYPE_CONSTANT(unsigned long long, ullong_type, SCN_DISABLE_TYPE_ULONG_LONG);
SCN_TYPE_CONSTANT(bool, bool_type, SCN_DISABLE_TYPE_BOOL);
SCN_TYPE_CONSTANT(char, narrow_character_type, SCN_DISABLE_TYPE_CHAR);
SCN_TYPE_CONSTANT(wchar_t, wide_character_type, SCN_DISABLE_TYPE_CHAR);
SCN_TYPE_CONSTANT(char32_t, code_point_type, SCN_DISABLE_TYPE_CHAR32);
SCN_TYPE_CONSTANT(void*, pointer_type, SCN_DISABLE_TYPE_POINTER);
SCN_TYPE_CONSTANT(float, float_type, SCN_DISABLE_TYPE_FLOAT);
SCN_TYPE_CONSTANT(double, double_type, SCN_DISABLE_TYPE_DOUBLE);
SCN_TYPE_CONSTANT(long double, ldouble_type, SCN_DISABLE_TYPE_LONG_DOUBLE);
SCN_TYPE_CONSTANT(std::string_view,
                  narrow_string_view_type,
                  SCN_DISABLE_TYPE_STRING_VIEW);
SCN_TYPE_CONSTANT(std::wstring_view,
                  wide_string_view_type,
                  SCN_DISABLE_TYPE_STRING_VIEW);
SCN_TYPE_CONSTANT(std::string, narrow_string_type, SCN_DISABLE_TYPE_STRING);
SCN_TYPE_CONSTANT(std::wstring, wide_string_type, SCN_DISABLE_TYPE_STRING);
SCN_TYPE_CONSTANT(regex_matches, narrow_regex_matches_type, SCN_DISABLE_REGEX);
SCN_TYPE_CONSTANT(wregex_matches, wide_regex_matches_type, SCN_DISABLE_REGEX);

#undef SCN_TYPE_CONSTANT

struct custom_value_type {
    void* value;
    scan_error (*scan)(void* arg, void* pctx, void* ctx);
};

struct unscannable {};
struct unscannable_char : unscannable {};
struct unscannable_const : unscannable {};
struct unscannable_disabled : unscannable {
    unscannable_disabled() = default;

    template <typename T>
    constexpr unscannable_disabled(T&&)
    {
    }
};

struct needs_context_tag {};

template <typename Context>
struct context_tag {
    using type = Context;
};

template <typename T, typename Context>
struct custom_wrapper {
    using context_type = Context;
    T& val;
};

class arg_value {
public:
    // trivial default initialization in constexpr
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201907L && \
    SCN_STD > SCN_STD_20
    constexpr
#endif
        arg_value() = default;

    template <typename T>
    explicit constexpr arg_value(T& val) : ref_value{std::addressof(val)}
    {
    }

    template <typename T, typename Context>
    explicit constexpr arg_value(custom_wrapper<T, Context> val)
        : custom_value{static_cast<void*>(&val.val),
                       scan_custom_arg<T, Context>}
    {
    }

    arg_value(unscannable);
    arg_value(unscannable_char);
    arg_value(unscannable_const);
    arg_value(unscannable_disabled);

    union {
        void* ref_value{nullptr};
        custom_value_type custom_value;
    };

private:
    template <typename T, typename Context>
    static scan_error scan_custom_arg(void* arg, void* pctx, void* ctx)
    {
        static_assert(!is_type_disabled<T>,
                      "Scanning of custom types is disabled by "
                      "SCN_DISABLE_TYPE_CUSTOM");
        SCN_EXPECT(arg && pctx && ctx);

        using context_type = Context;
        using parse_context_type = typename context_type::parse_context_type;
        using scanner_type = typename context_type::template scanner_type<T>;

        auto s = scanner_type{};

        auto& arg_ref = *static_cast<T*>(arg);
        auto& pctx_ref = *static_cast<parse_context_type*>(pctx);
        auto& ctx_ref = *static_cast<context_type*>(ctx);

        SCN_TRY_ERR(_, s.parse(pctx_ref));
        SCN_UNUSED(_);
        SCN_TRY_ERR(it, s.scan(arg_ref, ctx_ref));
        ctx_ref.advance_to(SCN_MOVE(it));

        return {};
    }
};

template <typename CharT>
struct arg_mapper {
    using char_type = CharT;
    using other_char_type =
        std::conditional_t<std::is_same_v<char_type, char>, wchar_t, char>;

#define SCN_ARG_MAPPER(T)                                                    \
    static auto map(T& val)                                                  \
        -> std::conditional_t<is_type_disabled<T>, unscannable_disabled, T&> \
    {                                                                        \
        return val;                                                          \
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
    SCN_ARG_MAPPER(char32_t)
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
        if constexpr (std::is_same_v<char_type, char> &&
                      !is_type_disabled<char_type>) {
            return val;
        }
        else if constexpr (is_type_disabled<char_type>) {
            return unscannable_disabled{val};
        }
        else {
            SCN_UNUSED(val);
            return unscannable_char{};
        }
    }

    static decltype(auto) map(basic_regex_matches<char_type>& val)
    {
        if constexpr (is_type_disabled<char_type>) {
            return unscannable_disabled{val};
        }
        else {
            return val;
        }
    }

    static unscannable_char map(std::basic_string_view<other_char_type>&)
    {
        return {};
    }
    static unscannable_char map(basic_regex_matches<other_char_type>&)
    {
        return {};
    }

    template <typename T>
    static std::enable_if_t<std::is_constructible_v<scanner<T, char_type>>,
                            needs_context_tag>
    map(T&)
    {
        return {};
    }

    template <typename T, typename Context>
    static std::enable_if_t<std::is_constructible_v<scanner<T, char_type>>,
                            custom_wrapper<T, Context>>
    map(T& val, context_tag<Context>)
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
    std::remove_reference_t<decltype(arg_mapper<CharT>().map(SCN_DECLVAL(T&)))>,
    CharT>;

template <typename T, typename CharT>
using is_scannable = std::integral_constant<
    bool,
    !std::is_base_of_v<
        unscannable,
        remove_cvref_t<decltype(arg_mapper<CharT>().map(SCN_DECLVAL(T&)))>>>;

constexpr std::size_t packed_arg_bits = 5;
static_assert((1 << packed_arg_bits) >= static_cast<int>(arg_type::last_type));
constexpr std::size_t bits_in_sz = sizeof(std::size_t) * 8;
constexpr std::size_t max_packed_args = (bits_in_sz - 1) / packed_arg_bits - 1;
constexpr std::size_t is_unpacked_bit = std::size_t{1} << (bits_in_sz - 1);

template <typename>
constexpr size_t encode_types_impl()
{
    return 0;
}
template <typename CharT, typename T, typename... Others>
constexpr size_t encode_types_impl()
{
    return static_cast<unsigned>(mapped_type_constant<T, CharT>::value) |
           (encode_types_impl<CharT, Others...>() << packed_arg_bits);
}

template <typename CharT, typename... Ts>
constexpr size_t encode_types()
{
    static_assert(sizeof...(Ts) < (1 << packed_arg_bits));
    return sizeof...(Ts) |
           (encode_types_impl<CharT, Ts...>() << packed_arg_bits);
}

template <typename Arg>
constexpr auto make_value_impl(Arg&& arg)
{
    using arg_nocvref_t = remove_cvref_t<Arg>;
    static_assert(!std::is_same_v<arg_nocvref_t, needs_context_tag>);

    constexpr bool scannable_char =
        !std::is_same_v<arg_nocvref_t, unscannable_char>;
    static_assert(scannable_char,
                  "Cannot scan an argument of an unsupported character "
                  "type (i.e. char from a wchar_t source)");

    constexpr bool scannable_const =
        !std::is_same_v<arg_nocvref_t, unscannable_const>;
    static_assert(scannable_const, "Cannot scan a const argument");

    constexpr bool scannable_disabled =
        !std::is_same_v<arg_nocvref_t, unscannable_disabled>;
    static_assert(scannable_disabled,
                  "Cannot scan an argument that has been disabled by "
                  "flag (SCN_DISABLE_TYPE_*)");

    constexpr bool scannable = !std::is_same_v<arg_nocvref_t, unscannable>;
    static_assert(
        scannable,
        "Cannot scan an argument. To make a type T scannable, provide "
        "a scn::scanner<T, CharT> specialization.");

    return arg_value{arg};
}

template <typename Context, typename T>
constexpr auto make_value(T& value)
{
    auto&& arg = arg_mapper<typename Context::char_type>().map(value);

    if constexpr (!std::is_same_v<remove_cvref_t<decltype(arg)>,
                                  needs_context_tag>) {
        return make_value_impl(SCN_FWD(arg));
    }
    else {
        return make_value_impl(arg_mapper<typename Context::char_type>().map(
            value, context_tag<Context>{}));
    }
}

template <typename... Args>
constexpr void check_scan_arg_types()
{
    static_assert(
        std::conjunction<std::is_default_constructible<Args>...>::value,
        "Scan argument types must be default constructible");
    static_assert(std::conjunction<std::is_destructible<Args>...>::value,
                  "Scan argument types must be Destructible");
    static_assert(
        !std::conjunction<std::false_type, std::is_reference<Args>...>::value,
        "Scan argument types must not be references");
}

template <typename Context, typename T>
constexpr basic_scan_arg<Context> make_arg(T& value)
{
    check_scan_arg_types<T>();

    basic_scan_arg<Context> arg;
    arg.m_type = mapped_type_constant<T, typename Context::char_type>::value;
    arg.m_value = make_value<Context>(value);
    return arg;
}

template <bool is_packed,
          typename Context,
          arg_type,
          typename T,
          typename = std::enable_if_t<is_packed>>
constexpr arg_value make_arg(T& value)
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
constexpr arg_value& get_arg_value(basic_scan_arg<Context>& arg);
}  // namespace detail

template <typename Visitor, typename Ctx>
constexpr decltype(auto) visit_scan_arg(Visitor&& vis,
                                        basic_scan_arg<Ctx>& arg);

/**
 * Type-erased scanning argument.
 *
 * Contains a pointer to the value contained in a `scan_arg_store`.
 */
template <typename Context>
class basic_scan_arg {
public:
    /**
     * Enables scanning of a user-defined type.
     *
     * Contains a pointer to the value contained in a `scan_arg_store`, and
     * a callback for parsing the format string, and scanning the value.
     *
     * \see scn::visit_scan_arg
     */
    class handle {
    public:
        /**
         * Parse the format string in `parse_ctx`, and scan the value from
         * `ctx`.
         *
         * \return Any error returned by the scanner
         */
        scan_error scan(typename Context::parse_context_type& parse_ctx,
                        Context& ctx) const
        {
            return m_custom.scan(m_custom.value, &parse_ctx, &ctx);
        }

    private:
        explicit handle(detail::custom_value_type custom) : m_custom(custom) {}

        template <typename Visitor, typename C>
        friend constexpr decltype(auto) visit_scan_arg(Visitor&& vis,
                                                       basic_scan_arg<C>& arg);

        detail::custom_value_type m_custom;
    };

    /// Construct a `basic_scan_arg` which doesn't contain an argument.
    constexpr basic_scan_arg() = default;

    /**
     * @return `true` if `*this` contains an argument
     */
    constexpr explicit operator bool() const SCN_NOEXCEPT
    {
        return m_type != detail::arg_type::none_type;
    }

    SCN_NODISCARD constexpr detail::arg_type type() const
    {
        return m_type;
    }

    SCN_NODISCARD constexpr detail::arg_value& value()
    {
        return m_value;
    }
    SCN_NODISCARD constexpr const detail::arg_value& value() const
    {
        return m_value;
    }

private:
    template <typename ContextType, typename T>
    friend constexpr basic_scan_arg<ContextType> detail::make_arg(T& value);

    template <typename C>
    friend constexpr detail::arg_value& detail::get_arg_value(
        basic_scan_arg<C>& arg);

    template <typename Visitor, typename C>
    friend constexpr decltype(auto) visit_scan_arg(Visitor&& vis,
                                                   basic_scan_arg<C>& arg);

    friend class basic_scan_args<Context>;

    detail::arg_value m_value{};
    detail::arg_type m_type{detail::arg_type::none_type};
};

namespace detail {
template <typename Context>
constexpr arg_value& get_arg_value(basic_scan_arg<Context>& arg)
{
    return arg.m_value;
}

template <typename Context, std::size_t NumArgs>
struct scan_arg_store_base {
protected:
    static constexpr std::size_t num_args = NumArgs;
    static constexpr bool is_packed = num_args <= detail::max_packed_args;

    using value_type = std::
        conditional_t<is_packed, detail::arg_value, basic_scan_arg<Context>>;
    using value_array_type = std::array<value_type, num_args>;
};
}  // namespace detail

/**
 * A tuple of scanning arguments, stored by value.
 *
 * Implicitly convertible to `basic_scan_args`,
 * to be passed to type-erased along to type-erased scanning functions,
 * like `scn::vscan`.
 */
template <typename Context, typename... Args>
class scan_arg_store
    : public detail::scan_arg_store_base<Context, sizeof...(Args)> {
    using base = detail::scan_arg_store_base<Context, sizeof...(Args)>;

    using char_type = typename Context::char_type;

public:
    std::tuple<Args...>& args()
    {
        return m_args;
    }

private:
    constexpr scan_arg_store() : scan_arg_store(std::tuple<Args...>{}) {}

    constexpr explicit scan_arg_store(std::tuple<Args...>&& a)
        : m_args{std::move(a)},
          m_data{std::apply(make_data_array<Args...>, m_args)}
    {
    }

    template <typename Ctx, typename... A>
    friend constexpr auto make_scan_args();
    template <typename Ctx, typename... A>
    friend constexpr auto make_scan_args(std::tuple<A...>&& values);

    template <typename... A>
    static constexpr typename base::value_array_type make_data_array(A&... args)
    {
        return {
            detail::make_arg<base::is_packed, Context,
                             detail::mapped_type_constant<
                                 detail::remove_cvref_t<A>, char_type>::value>(
                args)...};
    }

    constexpr detail::arg_value& get_value_at(std::size_t i)
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
        base::is_packed ? detail::encode_types<char_type, Args...>()
                        : detail::is_unpacked_bit | base::num_args;
};

/**
 * Constructs a `scan_arg_store` object, associated with `Context`,
 * that contains value-initialized values of types `Args...`.
 */
template <typename Context = scan_context, typename... Args>
constexpr auto make_scan_args()
{
    detail::check_scan_arg_types<Args...>();

    return scan_arg_store<Context, Args...>{};
}
/**
 * Constructs a `scan_arg_store` object, associated with `Context`,
 * that contains `values`.
 */
template <typename Context = scan_context, typename... Args>
constexpr auto make_scan_args(std::tuple<Args...>&& values)
{
    detail::check_scan_arg_types<Args...>();

    return scan_arg_store<Context, Args...>{SCN_MOVE(values)};
}

/**
 * A view over a collection of scanning arguments (`scan_arg_store`).
 *
 * Passed to `scn::vscan`, where it's automatically constructed from a
 * `scan_arg_store`.
 */
template <typename Context>
class basic_scan_args {
public:
    /// Construct a view over no arguments
    constexpr basic_scan_args() = default;

    /**
     * Construct a view over `store`.
     *
     * Intentionally not `explicit`.
     */
    template <typename... Args>
    constexpr /*implicit*/ basic_scan_args(
        scan_arg_store<Context, Args...>& store)
        : basic_scan_args{scan_arg_store<Context, Args...>::desc,
                          store.m_data.data()}
    {
    }

    /**
     * \return `basic_scan_arg` at index `id`. Empty `basic_scan_arg` if
     * there's no argument at index `id`.
     */
    SCN_NODISCARD constexpr basic_scan_arg<Context> get(std::size_t id) const
    {
        if (SCN_UNLIKELY(!is_packed())) {
            if (SCN_LIKELY(id < max_size())) {
                return m_args[id];
            }
            return {};
        }

        if (SCN_UNLIKELY(id >= detail::max_packed_args)) {
            return {};
        }

        const auto t = type(id);
        if (SCN_UNLIKELY(t == detail::arg_type::none_type)) {
            return {};
        }

        basic_scan_arg<Context> arg;
        arg.m_type = t;
        arg.m_value = m_values[id];
        return arg;
    }

    /**
     * \return Number of arguments in `*this`.
     */
    SCN_NODISCARD constexpr std::size_t size() const
    {
        if (SCN_UNLIKELY(!is_packed())) {
            return max_size();
        }

        return static_cast<std::size_t>(m_desc &
                                        ((1 << detail::packed_arg_bits) - 1));
    }

private:
    constexpr basic_scan_args(size_t desc, detail::arg_value* values)
        : m_desc{desc}, m_values{values}
    {
    }
    constexpr basic_scan_args(size_t desc, basic_scan_args<Context>* args)
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

    SCN_NODISCARD constexpr std::size_t max_size() const
    {
        return SCN_LIKELY(is_packed()) ? detail::max_packed_args
                                       : (m_desc & ~detail::is_unpacked_bit);
    }

    size_t m_desc{0};
    union {
        detail::arg_value* m_values;
        basic_scan_arg<Context>* m_args{nullptr};
    };
};

SCN_END_NAMESPACE
}  // namespace scn
