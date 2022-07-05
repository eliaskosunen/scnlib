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

    template <typename Visitor, typename Ctx>
    constexpr decltype(auto) visit_scan_arg(Visitor&& vis,
                                            basic_scan_arg<Ctx>& arg)
    {
#define SCN_VISIT_CAST(Type) vis(*static_cast<Type*>(arg.m_value.ref_value))

        switch (arg.m_type) {
            case detail::arg_type::schar_type:
                return SCN_VISIT_CAST(signed char);
            case detail::arg_type::short_type:
                return SCN_VISIT_CAST(short);
            case detail::arg_type::int_type:
                return SCN_VISIT_CAST(int);
            case detail::arg_type::long_type:
                return SCN_VISIT_CAST(long);
            case detail::arg_type::llong_type:
                return SCN_VISIT_CAST(long long);
            case detail::arg_type::uchar_type:
                return SCN_VISIT_CAST(unsigned char);
            case detail::arg_type::ushort_type:
                return SCN_VISIT_CAST(unsigned short);
            case detail::arg_type::uint_type:
                return SCN_VISIT_CAST(unsigned);
            case detail::arg_type::ulong_type:
                return SCN_VISIT_CAST(unsigned long);
            case detail::arg_type::ullong_type:
                return SCN_VISIT_CAST(unsigned long long);
            case detail::arg_type::pointer_type:
                return SCN_VISIT_CAST(void*);
            case detail::arg_type::bool_type:
                return SCN_VISIT_CAST(bool);
            case detail::arg_type::narrow_character_type:
                return SCN_VISIT_CAST(char);
            case detail::arg_type::wide_character_type:
                return SCN_VISIT_CAST(wchar_t);
            case detail::arg_type::code_point_type:
                return SCN_VISIT_CAST(code_point);
            case detail::arg_type::float_type:
                return SCN_VISIT_CAST(float);
            case detail::arg_type::double_type:
                return SCN_VISIT_CAST(double);
            case detail::arg_type::ldouble_type:
                return SCN_VISIT_CAST(long double);
            case detail::arg_type::narrow_string_view_type:
                return SCN_VISIT_CAST(std::string_view);
            case detail::arg_type::narrow_string_type:
                return SCN_VISIT_CAST(std::string);
            case detail::arg_type::wide_string_view_type:
                return SCN_VISIT_CAST(std::wstring_view);
            case detail::arg_type::wide_string_type:
                return SCN_VISIT_CAST(std::wstring);
            case detail::arg_type::custom_type:
                return vis(typename basic_scan_arg<Ctx>::handle(
                    arg.m_value.custom_value));
                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wcovered-switch-default")
            case detail::arg_type::none_type:
            default:
                auto val = detail::monostate{};
                return vis(val);
                SCN_CLANG_POP
        }

#undef SCN_VISIT_CAST

        SCN_ENSURE(false);
        SCN_UNREACHABLE;
    }

    SCN_END_NAMESPACE
}  // namespace scn
