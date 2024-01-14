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

#include <scn/detail/args.h>

namespace scn {
SCN_BEGIN_NAMESPACE

/**
 * Visit a `basic_scan_arg` with `Visitor`.
 * Calls `vis` with the value stored in `arg`.
 * If no value is contained in `arg`, calls `vis` with a `monostate`.
 *
 * \return `vis(x)`, where `x` is either a reference to the value contained
 * in `arg`, or a `basic_scan_arg::handle`.
 */
template <typename Visitor, typename Ctx>
constexpr decltype(auto) visit_scan_arg(Visitor&& vis, basic_scan_arg<Ctx>& arg)
{
#define SCN_VISIT(Type)                                         \
    if constexpr (!detail::is_type_disabled<Type>) {            \
        return vis(*static_cast<Type*>(arg.m_value.ref_value)); \
    }                                                           \
    else {                                                      \
        return vis(monostate_val);                              \
    }

    monostate monostate_val{};

    switch (arg.m_type) {
        case detail::arg_type::schar_type:
            SCN_VISIT(signed char);
        case detail::arg_type::short_type:
            SCN_VISIT(short);
        case detail::arg_type::int_type:
            SCN_VISIT(int);
        case detail::arg_type::long_type:
            SCN_VISIT(long);
        case detail::arg_type::llong_type:
            SCN_VISIT(long long);
        case detail::arg_type::uchar_type:
            SCN_VISIT(unsigned char);
        case detail::arg_type::ushort_type:
            SCN_VISIT(unsigned short);
        case detail::arg_type::uint_type:
            SCN_VISIT(unsigned);
        case detail::arg_type::ulong_type:
            SCN_VISIT(unsigned long);
        case detail::arg_type::ullong_type:
            SCN_VISIT(unsigned long long);
        case detail::arg_type::pointer_type:
            SCN_VISIT(void*);
        case detail::arg_type::bool_type:
            SCN_VISIT(bool);
        case detail::arg_type::narrow_character_type:
            SCN_VISIT(char);
        case detail::arg_type::wide_character_type:
            SCN_VISIT(wchar_t);
        case detail::arg_type::code_point_type:
            SCN_VISIT(char32_t);
        case detail::arg_type::float_type:
            SCN_VISIT(float);
        case detail::arg_type::double_type:
            SCN_VISIT(double);
        case detail::arg_type::ldouble_type:
            SCN_VISIT(long double);
        case detail::arg_type::narrow_string_view_type:
            SCN_VISIT(std::string_view);
        case detail::arg_type::narrow_string_type:
            SCN_VISIT(std::string);
        case detail::arg_type::wide_string_view_type:
            SCN_VISIT(std::wstring_view);
        case detail::arg_type::wide_string_type:
            SCN_VISIT(std::wstring);
        case detail::arg_type::narrow_regex_matches_type:
            SCN_VISIT(regex_matches);
        case detail::arg_type::wide_regex_matches_type:
            SCN_VISIT(wregex_matches);

        case detail::arg_type::custom_type:
#if !SCN_DISABLE_TYPE_CUSTOM
            return vis(
                typename Ctx::arg_type::handle(arg.m_value.custom_value));
#else
            return vis(monostate_val);
#endif

            SCN_CLANG_PUSH
            SCN_CLANG_IGNORE("-Wcovered-switch-default")

            SCN_UNLIKELY_ATTR
        case detail::arg_type::none_type:
        default: {
            return vis(monostate_val);
        }

            SCN_CLANG_POP
    }

#undef SCN_VISIT

    SCN_ENSURE(false);
    SCN_UNREACHABLE;
}  // namespace scn

SCN_END_NAMESPACE
}  // namespace scn
