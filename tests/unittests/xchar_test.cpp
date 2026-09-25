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

#include "test_common.h"

#include <scn/xchar.h>

TEST(XCharTest, ScanWideInt)
{
    auto result = scn::scan<int>(L"42", L"{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 42);
}

TEST(XCharTest, ScanWideMultipleValues)
{
    auto result = scn::scan<int, int>(L"123 456", L"{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
}

TEST(XCharTest, ScanWideString)
{
    auto result = scn::scan<std::wstring>(L"hello world", L"{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), L"hello");
}

TEST(XCharTest, ScanWideDouble)
{
    auto result = scn::scan<double>(L"3.14159", L"{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_NEAR(result->value(), 3.14159, 0.00001);
}

TEST(XCharTest, ScanWideWithDefault)
{
    auto result = scn::scan<int>(L"789", L"{}", {123});
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 789);
}

TEST(XCharTest, ScanWideWithDefaultFail)
{
    auto result = scn::scan<int>(L"notanumber", L"{}", {999});
    ASSERT_THAT(result, Failed());
}

TEST(XCharTest, ScanValueWide)
{
    auto result = scn::scan_value<int>(L"555");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 555);
}

TEST(XCharTest, ScanValueWideWithDefault)
{
    auto result = scn::scan_value<int>(L"777", 111);
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 777);
}

TEST(XCharTest, ScanValueWideWithDefaultFail)
{
    auto result = scn::scan_value<int>(L"xyz", 222);
    ASSERT_THAT(result, Failed());
}
