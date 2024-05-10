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

#include "../wrapped_gtest.h"

#include <scn/impl.h>

TEST(StringViewWrapperTest, DefaultConstructible)
{
    scn::impl::string_view_wrapper<char> svw{};
    EXPECT_EQ(svw.view(), std::string_view{});
    EXPECT_FALSE(svw.stores_allocated_string());
}

TEST(StringViewWrapperTest, ConstructibleFromStringView)
{
    scn::impl::string_view_wrapper svw{std::string_view{"abc"}};
    EXPECT_EQ(svw.view(), std::string_view{"abc"});
}
TEST(StringViewWrapperTest, ConstructibleFromLvalueString)
{
    std::string str{"def"};
    scn::impl::string_view_wrapper svw{str};
    EXPECT_EQ(svw.view(), std::string_view{"def"});
}
TEST(StringViewWrapperTest, NotConstructibleFromRvalueString)
{
    static_assert(!std::is_constructible_v<scn::impl::string_view_wrapper<char>,
                                           std::string>);
}

TEST(StringViewWrapperTest, AssignFromStringView)
{
    scn::impl::string_view_wrapper<char> svw{};
    svw.assign(std::string_view{"ghi"});
    EXPECT_EQ(svw.view(), std::string_view{"ghi"});
}
TEST(StringViewWrapperTest, AssignFromString)
{
    scn::impl::string_view_wrapper<char> svw{};
    std::string str{"jkl"};
    svw.assign(str);
    EXPECT_EQ(svw.view(), std::string_view{"jkl"});
}

TEST(ContiguousRangeFactoryTest, DefaultConstructible)
{
    scn::impl::contiguous_range_factory<char> crf{};
    EXPECT_EQ(crf.view(), std::string_view{});
    EXPECT_FALSE(crf.stores_allocated_string());
}

TEST(ContiguousRangeFactoryTest, ConstructibleFromStringView)
{
    scn::impl::contiguous_range_factory crf{std::string_view{"abc"}};
    EXPECT_EQ(crf.view(), std::string_view{"abc"});
    EXPECT_FALSE(crf.stores_allocated_string());
}
TEST(ContiguousRangeFactoryTest, ConstructibleFromLvalueString)
{
    std::string str{"def"};
    scn::impl::contiguous_range_factory crf{str};
    EXPECT_EQ(crf.view(), std::string_view{"def"});
    EXPECT_FALSE(crf.stores_allocated_string());
}
TEST(ContiguousRangeFactoryTest, ConstructibleFromRvalueString)
{
    scn::impl::contiguous_range_factory crf{std::string{"ghi"}};
    EXPECT_EQ(crf.view(), std::string_view{"ghi"});
    EXPECT_TRUE(crf.stores_allocated_string());
}

TEST(ContiguousRangeFactoryTest, MakeStringIntoAllocatedString)
{
    scn::impl::contiguous_range_factory crf{std::string{"jkl"}};
    EXPECT_TRUE(crf.stores_allocated_string());

    EXPECT_EQ(crf.make_into_allocated_string(), std::string{"jkl"});
    EXPECT_TRUE(crf.stores_allocated_string());
}
TEST(ContiguousRangeFactoryTest, MakeStringViewIntoAllocatedString)
{
    scn::impl::contiguous_range_factory crf{std::string_view{"mno"}};
    EXPECT_FALSE(crf.stores_allocated_string());

    EXPECT_EQ(crf.make_into_allocated_string(), std::string_view{"mno"});
    EXPECT_TRUE(crf.stores_allocated_string());
}

TEST(MakeContiguousBufferTest, StringViewIntoStringViewWrapper)
{
    auto buf = scn::impl::make_contiguous_buffer(std::string_view{"abc"});
    static_assert(
        std::is_same_v<decltype(buf), scn::impl::string_view_wrapper<char>>);
    EXPECT_EQ(buf.view(), "abc");
}
TEST(MakeContiguousBufferTest, LvalueStringIntoStringViewWrapper)
{
    std::string str{"def"};
    auto buf = scn::impl::make_contiguous_buffer(str);
    static_assert(
        std::is_same_v<decltype(buf), scn::impl::string_view_wrapper<char>>);
    EXPECT_EQ(buf.view(), "def");
}
TEST(MakeContiguousBufferTest, RvalueStringIntoContiguousRangeFactory)
{
    auto buf = scn::impl::make_contiguous_buffer(std::string{"ghi"});
    static_assert(std::is_same_v<decltype(buf),
                                 scn::impl::contiguous_range_factory<char>>);
    EXPECT_EQ(buf.view(), "ghi");
}
