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
#include <scn/util/expected.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
template <typename T, typename ParseCtx>
constexpr scan_expected<typename ParseCtx::iterator>
scanner_parse_for_builtin_type(ParseCtx& pctx, format_specs& specs);

template <typename T, typename Context>
scan_expected<typename Context::iterator>
scanner_scan_for_builtin_type(T& val, Context& ctx, const format_specs& specs);
}  // namespace detail

/**
 * `scanner` specialization for all built-in types
 *
 * \ingroup ctx
 */
template <typename T, typename CharT>
struct scanner<T,
               CharT,
               std::enable_if_t<detail::arg_type_constant<T, CharT>::value !=
                                    detail::arg_type::custom_type &&
                                !detail::is_type_disabled<T>>> {
public:
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx)
        -> scan_expected<typename ParseCtx::iterator>
    {
        return detail::scanner_parse_for_builtin_type<T>(pctx, m_specs);
    }

    template <typename Context>
    scan_expected<typename Context::iterator> scan(T& val, Context& ctx) const
    {
        return detail::scanner_scan_for_builtin_type(val, ctx, m_specs);
    }

    constexpr auto& _format_specs()
    {
        return m_specs;
    }

private:
    detail::format_specs m_specs;
};

namespace detail {
template <typename T, typename ParseCtx>
constexpr scan_expected<typename ParseCtx::iterator>
scanner_parse_for_builtin_type(ParseCtx& pctx, format_specs& specs)
{
    using char_type = typename ParseCtx::char_type;

    auto begin = pctx.begin();
    const auto end = pctx.end();
    if (begin == end) {
        return begin;
    }

    using handler_type = detail::specs_setter;
    const auto type = detail::arg_type_constant<T, char_type>::value;
    auto checker =
        detail::specs_checker<handler_type>(handler_type(specs), type);

    const auto it =
        detail::parse_format_specs(to_address(begin), to_address(end), checker);
    if (auto e = checker.get_error(); SCN_UNLIKELY(!e)) {
        return unexpected(e);
    }

    switch (type) {
        case detail::arg_type::none_type:
            SCN_FALLTHROUGH;
        case detail::arg_type::custom_type:
            SCN_ENSURE(false);
            break;

        case detail::arg_type::bool_type:
            detail::check_bool_type_specs(specs, checker);
            break;

        case detail::arg_type::schar_type:
        case detail::arg_type::short_type:
        case detail::arg_type::int_type:
        case detail::arg_type::long_type:
        case detail::arg_type::llong_type:
        case detail::arg_type::uchar_type:
        case detail::arg_type::ushort_type:
        case detail::arg_type::uint_type:
        case detail::arg_type::ulong_type:
        case detail::arg_type::ullong_type:
            detail::check_int_type_specs(specs, checker);
            break;

        case detail::arg_type::narrow_character_type:
        case detail::arg_type::wide_character_type:
        case detail::arg_type::code_point_type:
            detail::check_char_type_specs(specs, checker);
            break;

        case detail::arg_type::float_type:
        case detail::arg_type::double_type:
        case detail::arg_type::ldouble_type:
            detail::check_float_type_specs(specs, checker);
            break;

        case detail::arg_type::narrow_string_type:
        case detail::arg_type::narrow_string_view_type:
        case detail::arg_type::wide_string_type:
        case detail::arg_type::wide_string_view_type:
            detail::check_string_type_specs(specs, checker);
            break;

        case detail::arg_type::pointer_type:
            detail::check_pointer_type_specs(specs, checker);
            break;

        case detail::arg_type::narrow_regex_matches_type:
        case detail::arg_type::wide_regex_matches_type:
            detail::check_regex_type_specs(specs, checker);
            break;

            SCN_CLANG_PUSH
            SCN_CLANG_IGNORE("-Wcovered-switch-default")

        default:
            SCN_ENSURE(false);
            SCN_UNREACHABLE;

            SCN_CLANG_POP
    }

    if (auto e = checker.get_error(); SCN_UNLIKELY(!e)) {
        return unexpected(e);
    }

    return {it};
}
}  // namespace detail

/**
 * Type for discarding any scanned value.
 * Example:
 *
 * \code{.cpp}
 * auto r = scn::scan<scn::discard<int>>("42", "{}");
 * // r.has_value() == true
 * // decltype(r->value()) is scn::discard<int>
 * \endcode
 *
 * \ingroup format-string
 */
template <typename T>
struct discard {
    constexpr discard() = default;

    constexpr discard(const T&) SCN_NOEXCEPT {}
    constexpr discard(T&&) SCN_NOEXCEPT {}

    constexpr discard& operator=(const T&) SCN_NOEXCEPT
    {
        return *this;
    }
    constexpr discard& operator=(T&&) SCN_NOEXCEPT
    {
        return *this;
    }
};

template <typename T, typename CharT>
struct scanner<discard<T>, CharT> : public scanner<T, CharT> {
    template <typename Context>
    auto scan(discard<T>&, Context& ctx) const
    {
        T val{};
        return scanner<T, CharT>::scan(val, ctx);
    }
};

SCN_END_NAMESPACE
}  // namespace scn
