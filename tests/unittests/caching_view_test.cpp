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

#include <gtest/gtest.h>

#include <scn/detail/caching_view.h>

TEST(CachingViewTest, CompareDereferenceIncrease) {
    std::string source{"123"};
    scn::detail::basic_caching_view<std::string_view> view{source};

    auto it = view.begin();
    EXPECT_NE(it, view.end());
    EXPECT_EQ(*it, '1');
    ++it;
    EXPECT_NE(it, view.end());
    EXPECT_EQ(*it, '2');
    ++it;
    EXPECT_NE(it, view.end());
    EXPECT_EQ(*it, '3');
    ++it;
    EXPECT_EQ(it, view.end());
}

TEST(CachingViewTest, DereferenceIncreaseCompare) {
    std::string source{"123"};
    scn::detail::basic_caching_view<std::string_view> view{source};

    auto it = view.begin();
    EXPECT_EQ(*it, '1');
    ++it;
    EXPECT_NE(it, view.end());
    EXPECT_EQ(*it, '2');
    ++it;
    EXPECT_NE(it, view.end());
    EXPECT_EQ(*it, '3');
    ++it;
    EXPECT_EQ(it, view.end());
}

TEST(CachingViewTest, Loop)
{
    std::string source{"123"};
    scn::detail::basic_caching_view<std::string_view> view{source};

    std::string dest;
    for (auto ch : view) {
        dest.push_back(ch);
    }
    EXPECT_EQ(source, dest);
}
