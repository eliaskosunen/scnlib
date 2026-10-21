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

#include <array>
#include <map>
#include <set>
#include <tuple>
#include <vector>

#include <scn/ranges.h>
#include <scn/scan.h>
#include <scn/xchar.h>

namespace {
struct arrow_separated {
    std::vector<int> values;
};
}  // namespace

template <>
struct scn::scanner<arrow_separated, char> {
    constexpr scanner()
    {
        m_underlying.set_separator("->");
        m_underlying.set_brackets("<<", ">>");
    }

    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx) -> typename ParseCtx::iterator
    {
        return m_underlying.parse(pctx);
    }

    template <typename Context>
    auto scan(arrow_separated& val, Context& ctx) const
        -> scn::scan_expected<typename Context::iterator>
    {
        return m_underlying.scan(val.values, ctx);
    }

    scn::range_scanner<int, char> m_underlying;
};

TEST(RangesTest, VectorSequence)
{
    static_assert(scn::range_format_kind<std::vector<int>, char>::value ==
                  scn::range_format::sequence);

    auto result = scn::scan<std::vector<int>>("[123, 456]", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(123, 456));
}

TEST(RangesTest, Set)
{
    static_assert(scn::range_format_kind<std::set<int>, char>::value ==
                  scn::range_format::set);

    auto result = scn::scan<std::set<int>>("{123, 456}", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(123, 456));
}

TEST(RangesTest, Map)
{
    static_assert(scn::range_format_kind<std::map<int, int>, char>::value ==
                  scn::range_format::map);

    auto result = scn::scan<std::map<int, int>>("{12: 34, 56: 78}", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(),
                testing::ElementsAre(std::pair{12, 34}, std::pair{56, 78}));
}

TEST(RangesTest, VectorEmpty)
{
    auto result = scn::scan<std::vector<int>>("[]", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), ::testing::IsEmpty());
}

TEST(RangesTest, VectorSingleElement)
{
    auto result = scn::scan<std::vector<int>>("[42]", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(42));
}

TEST(RangesTest, SetEmpty)
{
    auto result = scn::scan<std::set<int>>("{}", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), ::testing::IsEmpty());
}

TEST(RangesTest, SetSingleElement)
{
    auto result = scn::scan<std::set<int>>("{999}", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(999));
}

TEST(RangesTest, MapEmpty)
{
    auto result = scn::scan<std::map<int, int>>("{}", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), ::testing::IsEmpty());
}

TEST(RangesTest, MapSingleElement)
{
    auto result = scn::scan<std::map<int, int>>("{10: 20}", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(std::pair{10, 20}));
}

TEST(RangesTest, VectorMalformedMissingCloseBracket)
{
    auto result = scn::scan<std::vector<int>>("[123, 456", "{}");
    ASSERT_THAT(result, Failed());
}

TEST(RangesTest, SetMalformedMissingCloseBrace)
{
    auto result = scn::scan<std::set<int>>("{123, 456", "{}");
    ASSERT_THAT(result, Failed());
}

TEST(RangesTest, MapMalformedMissingColon)
{
    auto result = scn::scan<std::map<int, int>>("{12 34}", "{}");
    ASSERT_THAT(result, Failed());
}

TEST(RangesTest, VectorNegativeNumbers)
{
    auto result = scn::scan<std::vector<int>>("[-1, -2, -3]", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(-1, -2, -3));
}

TEST(RangesTest, VectorDoubles)
{
    auto result = scn::scan<std::vector<double>>("[1.5, 2.5, 3.5]", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().size(), 3);
    EXPECT_NEAR(result->value()[0], 1.5, 0.001);
    EXPECT_NEAR(result->value()[1], 2.5, 0.001);
    EXPECT_NEAR(result->value()[2], 3.5, 0.001);
}

TEST(RangesTest, VectorWithExtraWhitespace)
{
    auto result = scn::scan<std::vector<int>>("[  1  ,  2  ,  3  ]", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(1, 2, 3));
}

TEST(RangesTest, SetDuplicates)
{
    auto result = scn::scan<std::set<int>>("{1, 2, 2, 3}", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(1, 2, 3));
}

TEST(RangesTest, MapMultipleEntries)
{
    auto result = scn::scan<std::map<int, int>>("{1: 10, 2: 20, 3: 30}", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().size(), 3);
    EXPECT_EQ(result->value()[1], 10);
    EXPECT_EQ(result->value()[2], 20);
    EXPECT_EQ(result->value()[3], 30);
}

TEST(RangesTest, VectorMalformedNoOpenBracket)
{
    auto result = scn::scan<std::vector<int>>("123, 456]", "{}");
    ASSERT_THAT(result, Failed());
}

TEST(RangesTest, SetMalformedNoOpenBrace)
{
    auto result = scn::scan<std::set<int>>("123, 456}", "{}");
    ASSERT_THAT(result, Failed());
}

TEST(RangesTest, VectorInvalidElementType)
{
    auto result = scn::scan<std::vector<int>>("[abc, def]", "{}");
    ASSERT_THAT(result, Failed());
}

TEST(RangesTest, WideVector)
{
    auto result = scn::scan<std::vector<int>>(L"[1, 2, 3]", L"{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(RangesTest, WideSet)
{
    auto result = scn::scan<std::set<int>>(L"{3, 1}", L"{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(1, 3));
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(RangesTest, WideMap)
{
    auto result = scn::scan<std::map<int, int>>(L"{1: 2}", L"{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(std::pair{1, 2}));
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(RangesTest, WcharVectorFromNarrowSource)
{
    auto result = scn::scan<std::vector<wchar_t>>("[65, 66]",
                                                  scn::runtime_format("{:d}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(), testing::ElementsAre(L'A', L'B'));
    EXPECT_THAT(result->range(), IsEmptyRange());

    for (auto fmt : {"{}", "{:c}"}) {
        EXPECT_THAT(
            scn::scan<std::vector<wchar_t>>("[A, B]", scn::runtime_format(fmt)),
            FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
}

// Each element type needs its scanner instantiated in the library
TEST(RangesTest, VectorOfEachBuiltinType)
{
    auto bools = scn::scan<std::vector<bool>>("[1, 0]", "{}");
    ASSERT_THAT(bools, Succeeded());
    EXPECT_THAT(bools->value(), testing::ElementsAre(true, false));

    auto code_points = scn::scan<std::vector<char32_t>>(L"[a]", L"{}");
    ASSERT_THAT(code_points, Succeeded());
    EXPECT_THAT(code_points->value(), testing::ElementsAre(U'a'));

    auto pointers = scn::scan<std::vector<void*>>("[0x10]", "{}");
    ASSERT_THAT(pointers, Succeeded());
    EXPECT_THAT(pointers->value(),
                testing::ElementsAre(reinterpret_cast<void*>(0x10)));

#if SCN_HAS_INT128
    auto int128s = scn::scan<std::vector<scn::int128>>("[-2]", "{}");
    ASSERT_THAT(int128s, Succeeded());
    EXPECT_THAT(int128s->value(), testing::ElementsAre(scn::int128{-2}));
#endif
#if SCN_HAS_STD_F32
    auto float32s = scn::scan<std::vector<std::float32_t>>("[1.5]", "{}");
    ASSERT_THAT(float32s, Succeeded());
    EXPECT_THAT(float32s->value(), testing::ElementsAre(1.5f32));
#endif
}

TEST(RangesTest, Tuple)
{
    auto result = scn::scan<std::tuple<int, double>>("(1, 2.5) x", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(std::get<0>(result->value()), 1);
    EXPECT_DOUBLE_EQ(std::get<1>(result->value()), 2.5);
    EXPECT_STREQ(result->begin(), " x");
}

TEST(RangesTest, WideTuple)
{
    auto result = scn::scan<std::tuple<int, double>>(L"(1, 2.5)", L"{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(std::get<0>(result->value()), 1);
    EXPECT_DOUBLE_EQ(std::get<1>(result->value()), 2.5);
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(RangesTest, MultiCharacterSeparatorAndBrackets)
{
    auto result = scn::scan<arrow_separated>("<<1->2>>", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value().values, testing::ElementsAre(1, 2));
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(RangesTest, MultiCharacterSeparatorAndBracketsTruncated)
{
    constexpr std::array<std::string_view, 3> inputs{"<", "<<1-", "<<1->2>"};
    for (std::string_view input : inputs) {
        auto result = scn::scan<arrow_separated>(input, "{}");
        EXPECT_THAT(result, Failed()) << input;
    }
}
