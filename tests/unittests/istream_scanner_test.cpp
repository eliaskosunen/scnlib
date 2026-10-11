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

#include <scn/istream.h>

#include "test_common.h"

#if !SCN_DISABLE_IOSTREAM

struct has_istream_operator {
    int i{};

    friend std::istream& operator>>(std::istream& is, has_istream_operator& val)
    {
        return is >> val.i;
    }
};
template <typename CharT>
struct scn::scanner<has_istream_operator, CharT>
    : public scn::basic_istream_scanner<CharT> {};

namespace {
// Reads two characters, and puts them back in various ways
struct uses_putback {
    std::string read{};
    bool putback_other_failed{false};

    friend std::istream& operator>>(std::istream& is, uses_putback& val)
    {
        char ch{};
        is.get(ch);
        is.putback(ch);
        is.get(ch);
        is.unget();
        // peek + unget: puts back nothing, since nothing was extracted
        is.peek();

        char first{}, second{};
        is.get(first);
        is.get(second);
        is.unget();
        is.unget();

        // Putting back a different character than was read fails
        is.get(ch);
        is.putback('x');
        val.putback_other_failed = is.bad();
        is.clear();

        is.get(first);
        is.get(second);
        val.read = {first, second};
        return is;
    }
};

struct unget_first {
    bool unget_failed{false};
    char ch{};

    friend std::istream& operator>>(std::istream& is, unget_first& val)
    {
        is.unget();
        val.unget_failed = is.bad();
        is.clear();
        return is >> val.ch;
    }
};
}  // namespace

template <typename CharT>
struct scn::scanner<uses_putback, CharT>
    : public scn::basic_istream_scanner<CharT> {};

template <typename CharT>
struct scn::scanner<unget_first, CharT>
    : public scn::basic_istream_scanner<CharT> {};

#if !SCN_DISABLE_LOCALE

namespace {
struct comma_decimal_point : std::numpunct<char> {
    char do_decimal_point() const override
    {
        return ',';
    }
};

struct reads_decimal_point {
    char decimal_point{};

    friend std::istream& operator>>(std::istream& is, reads_decimal_point& val)
    {
        val.decimal_point =
            std::use_facet<std::numpunct<char>>(is.getloc()).decimal_point();
        return is;
    }
};
}  // namespace

template <typename CharT>
struct scn::scanner<reads_decimal_point, CharT>
    : public scn::basic_istream_scanner<CharT> {};

#endif

TEST(IstreamScannerTest, HasIstreamOperator)
{
    auto result = scn::scan<has_istream_operator>("42", "{}");
    ASSERT_THAT(result, Succeeded());
    const auto& [val] = result->values();
    EXPECT_EQ(val.i, 42);
}
TEST(IstreamScannerTest, OtherValues)
{
    auto result = scn::scan<has_istream_operator, has_istream_operator,
                            has_istream_operator>("123 456 789", "{} {} {}");
    ASSERT_THAT(result, Succeeded());
    const auto& [a, b, c] = result->values();
    EXPECT_EQ(a.i, 123);
    EXPECT_EQ(b.i, 456);
    EXPECT_EQ(c.i, 789);
}

TEST(IstreamScannerTest, EmptyInput)
{
    auto result = scn::scan<has_istream_operator>("", "{}");
    ASSERT_THAT(result, Failed());
}

TEST(IstreamScannerTest, InvalidInput)
{
    auto result = scn::scan<has_istream_operator>("notanumber", "{}");
    ASSERT_THAT(result, Failed());
}

TEST(IstreamScannerTest, WithLiteral)
{
    auto result = scn::scan<has_istream_operator>("value=42", "value={}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().i, 42);
}

TEST(IstreamScannerTest, NegativeValue)
{
    auto result = scn::scan<has_istream_operator>("-123", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().i, -123);
}

TEST(IstreamScannerTest, WithTrailingContent)
{
    auto result = scn::scan<has_istream_operator>("42 extra", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().i, 42);
    EXPECT_STREQ(result->begin(), " extra");
}

TEST(IstreamScannerTest, Putback)
{
    auto result = scn::scan<uses_putback>("abc", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(result->value().putback_other_failed);
    EXPECT_EQ(result->value().read, "bc");
    EXPECT_STREQ(result->begin(), "");
}

TEST(IstreamScannerTest, PutbackPastBeginningFails)
{
    auto result = scn::scan<int, unget_first>("1 ab", "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [i, val] = result->values();
    EXPECT_EQ(i, 1);
    EXPECT_TRUE(val.unget_failed);
    EXPECT_EQ(val.ch, 'a');
    EXPECT_STREQ(result->begin(), "b");
}

#if !SCN_DISABLE_LOCALE

TEST(IstreamScannerTest, ImbuesScanLocale)
{
    auto loc = std::locale(std::locale::classic(), new comma_decimal_point);
    auto result = scn::scan<reads_decimal_point>(loc, "x", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().decimal_point, ',');
}

TEST(IstreamScannerTest, NoLocaleKeepsDefault)
{
    auto result = scn::scan<reads_decimal_point>("x", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().decimal_point, '.');
}

#endif

#endif
