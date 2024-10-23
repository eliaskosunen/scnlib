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

#include "wrapped_gtest.h"

#include <scn/scan.h>

using ::testing::Test;

TEST(ArgsTest, ArgTypeMapping)
{
    static_assert(scn::detail::mapped_type_constant<int, char>::value ==
                  scn::detail::arg_type::int_type);
    static_assert(scn::detail::mapped_type_constant<scn::detail::dummy_type,
                                                    char>::value ==
                  scn::detail::arg_type::custom_type);

    // narrow context, narrow char -> valid
    static_assert(scn::detail::mapped_type_constant<char, char>::value ==
                  scn::detail::arg_type::narrow_character_type);
    static_assert(scn::detail::mapped_type_constant<std::string, char>::value ==
                  scn::detail::arg_type::narrow_string_type);
    static_assert(
        scn::detail::mapped_type_constant<std::string_view, char>::value ==
        scn::detail::arg_type::narrow_string_view_type);

    // wide context, wide char -> valid
    static_assert(scn::detail::mapped_type_constant<wchar_t, wchar_t>::value ==
                  scn::detail::arg_type::wide_character_type);
    static_assert(
        scn::detail::mapped_type_constant<std::wstring, wchar_t>::value ==
        scn::detail::arg_type::wide_string_type);
    static_assert(
        scn::detail::mapped_type_constant<std::wstring_view, wchar_t>::value ==
        scn::detail::arg_type::wide_string_view_type);

    // narrow context, wide char -> valid for chars and strings, invalid for
    // string_views
    static_assert(scn::detail::mapped_type_constant<wchar_t, char>::value ==
                  scn::detail::arg_type::wide_character_type);
    static_assert(
        scn::detail::mapped_type_constant<std::wstring, char>::value ==
        scn::detail::arg_type::wide_string_type);
    static_assert(
        std::is_same_v<
            scn::detail::mapped_type_constant<std::wstring_view, char>::type,
            scn::detail::unscannable_char>);

    // wide context, narrow char -> invalid for chars and string_views, valid
    // for strings
    static_assert(
        std::is_same_v<scn::detail::mapped_type_constant<char, wchar_t>::type,
                       scn::detail::unscannable_char>);
    static_assert(
        scn::detail::mapped_type_constant<std::string, wchar_t>::value ==
        scn::detail::arg_type::narrow_string_type);
    static_assert(
        std::is_same_v<
            scn::detail::mapped_type_constant<std::string_view, wchar_t>::type,
            scn::detail::unscannable_char>);
}

TEST(ArgsTest, ArgStore)
{
    std::tuple<int, double> args_tuple{};
    auto store = scn::make_scan_args(args_tuple);
    auto args = scn::basic_scan_args{store};

    EXPECT_EQ(scn::detail::get_arg_type(args.get(0)),
              scn::detail::arg_type::int_type);
    EXPECT_EQ(scn::detail::get_arg_type(args.get(1)),
              scn::detail::arg_type::double_type);

    *static_cast<int*>(scn::detail::get_arg_value(args.get(0)).ref_value) = 42;

    EXPECT_EQ(std::get<0>(args_tuple), 42);
    EXPECT_DOUBLE_EQ(std::get<1>(args_tuple), 0.0);
}
