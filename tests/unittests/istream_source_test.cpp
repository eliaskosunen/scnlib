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

#include <algorithm>
#include <array>
#include <limits>
#include <sstream>
#include <string>

using testing::FieldsAre;

namespace {
// Only has the bare minimum of a streambuf:
// no get area, and thus no support for putback
class string_streambuf : public std::streambuf {
public:
    explicit string_streambuf(std::string data) : m_data(std::move(data)) {}

protected:
    int_type underflow() override
    {
        if (m_pos == m_data.size()) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(m_data[m_pos]);
    }
    int_type uflow() override
    {
        auto ch = underflow();
        if (!traits_type::eq_int_type(ch, traits_type::eof())) {
            ++m_pos;
        }
        return ch;
    }

    std::string m_data;
    std::size_t m_pos{0};
};

class unbuffered_streambuf : public string_streambuf {
public:
    explicit unbuffered_streambuf(std::string data,
                                  std::streamsize available = 0)
        : string_streambuf(std::move(data)), m_available(available)
    {
    }

private:
    std::streamsize showmanyc() override
    {
        return m_available;
    }

    std::streamsize m_available;
};

// Claims more characters to be available than there are
class overpromising_streambuf : public string_streambuf {
public:
    using string_streambuf::string_streambuf;

private:
    std::streamsize showmanyc() override
    {
        return 100;
    }
    std::streamsize xsgetn(char* s, std::streamsize n) override
    {
        const auto count =
            std::min(static_cast<std::size_t>(n), m_data.size() - m_pos);
        std::copy_n(m_data.data() + m_pos, count, s);
        m_pos += count;
        return static_cast<std::streamsize>(count);
    }
};

// Claims characters to be available, but can't read them in bulk
class no_bulk_read_streambuf : public overpromising_streambuf {
public:
    using overpromising_streambuf::overpromising_streambuf;

private:
    std::streamsize xsgetn(char*, std::streamsize) override
    {
        return 0;
    }
};
}  // namespace

TEST(IstreamSourceTest, Stringstream)
{
    std::istringstream ss{"123 abc"};
    auto res = scn::scan<int, std::string>(ss, "{} {}");
    ASSERT_THAT(res, Succeeded());
    EXPECT_THAT(res->values(), FieldsAre(123, "abc"));
    EXPECT_EQ(&res->stream(), &ss);
}

TEST(IstreamSourceTest, StringstreamMultiline)
{
    std::istringstream ss{"123\n456\n789"};
    auto res1 = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res1, Succeeded());
    EXPECT_EQ(res1->value(), 123);

    auto res2 = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res2, Succeeded());
    EXPECT_EQ(res2->value(), 456);

    auto res3 = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res3, Succeeded());
    EXPECT_EQ(res3->value(), 789);
}

TEST(IstreamSourceTest, StringstreamEOF)
{
    std::istringstream ss{"42"};
    auto res1 = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res1, Succeeded());
    EXPECT_EQ(res1->value(), 42);

    auto res2 = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res2, FailedWith(scn::scan_error::end_of_input));
}

TEST(IstreamSourceTest, EmptyStringstream)
{
    std::istringstream ss{""};
    auto res = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res, FailedWith(scn::scan_error::end_of_input));
}

TEST(IstreamSourceTest, StringstreamWithWhitespace)
{
    std::istringstream ss{"  42  "};
    auto res = scn::scan<int>(ss, "{}");
    ASSERT_THAT(res, Succeeded());
    EXPECT_EQ(res->value(), 42);
}

TEST(IstreamSourceTest, StringstreamChar)
{
    std::istringstream ss{"abc"};
    auto res = scn::scan<char, char, char>(ss, "{}{}{}");
    ASSERT_THAT(res, Succeeded());
    auto [a, b, c] = res->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'b');
    EXPECT_EQ(c, 'c');
}

TEST(IstreamSourceTest, StringstreamBool)
{
    std::istringstream ss{"true false"};
    auto res = scn::scan<bool, bool>(ss, "{} {}");
    ASSERT_THAT(res, Succeeded());
    auto [a, b] = res->values();
    EXPECT_TRUE(a);
    EXPECT_FALSE(b);
}

TEST(IstreamSourceTest, StringstreamNegativeDouble)
{
    std::istringstream ss{"-3.14"};
    auto res = scn::scan<double>(ss, "{}");
    ASSERT_THAT(res, Succeeded());
    EXPECT_NEAR(res->value(), -3.14, 0.001);
}

TEST(IstreamSourceTest, StringstreamMixedTypes)
{
    std::istringstream ss{"42 hello 3.14 true"};
    auto res = scn::scan<int, std::string, double, bool>(ss, "{} {} {} {}");
    ASSERT_THAT(res, Succeeded());
    auto [i, s, d, b] = res->values();
    EXPECT_EQ(i, 42);
    EXPECT_EQ(s, "hello");
    EXPECT_NEAR(d, 3.14, 0.001);
    EXPECT_TRUE(b);
}

TEST(IstreamSourceTest, NullStreambuf)
{
    std::istream stream{nullptr};
    auto result = scn::scan<int>(stream, "{}");
    EXPECT_THAT(result, Failed());
}

TEST(IstreamSourceTest, StreambufWithHugeAvailableCount)
{
    unbuffered_streambuf buf{"12 34",
                             std::numeric_limits<std::streamsize>::max()};
    std::istream stream{&buf};
    auto result = scn::scan<int, int>(stream, "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 12);
    EXPECT_EQ(b, 34);
}

TEST(IstreamSourceTest, StreambufWithHugeAvailableCountLongInput)
{
    const std::string word(10000, 'a');
    unbuffered_streambuf buf{word + " 34",
                             std::numeric_limits<std::streamsize>::max()};
    std::istream stream{&buf};
    auto result = scn::scan<std::string, int>(stream, "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [str, num] = result->values();
    EXPECT_EQ(str, word);
    EXPECT_EQ(num, 34);
}

TEST(IstreamSourceTest, StreambufWithoutPutbackConsecutiveScans)
{
    unbuffered_streambuf buf{"12 34"};
    std::istream stream{&buf};

    auto first = scn::scan<int>(stream, "{}");
    ASSERT_THAT(first, Succeeded());
    EXPECT_EQ(first->value(), 12);

    auto second = scn::scan<int>(stream, "{}");
    ASSERT_THAT(second, Succeeded());
    EXPECT_EQ(second->value(), 34);
}

TEST(IstreamSourceTest, StreambufWithoutPutbackFailedScan)
{
    unbuffered_streambuf buf{"12 x tail"};
    std::istream stream{&buf};

    // The characters read by the failed scan can't be put back,
    // and that is reported instead of the actual error
    auto result = scn::scan<int, int>(stream, "{} {}");
    ASSERT_THAT(result, Failed());
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_source_state);
    EXPECT_STREQ(result.error().msg(), "Failed to sync with underlying source");
}

TEST(IstreamSourceTest, StreambufOverpromisingAvailableCharacters)
{
    overpromising_streambuf buf{"12 34"};
    std::istream stream{&buf};
    auto result = scn::scan<int, int>(stream, "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 12);
    EXPECT_EQ(b, 34);
}

TEST(IstreamSourceTest, StreambufWithoutBulkReads)
{
    no_bulk_read_streambuf buf{"12 34"};
    std::istream stream{&buf};
    auto result = scn::scan<int, int>(stream, "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 12);
    EXPECT_EQ(b, 34);
}

TEST(IstreamSourceTest, SequentialSingleFieldsWithSpecs)
{
    // "{:...}" takes the single-field fast path, "{0:...}" never does.
    // Both must leave the stream at the same position after each scan.
    constexpr std::array<const char*, 2> field_prefixes{"{:", "{0:"};
    for (auto field_prefix : field_prefixes) {
        std::istringstream ss{"ff 12abc  777 xyz"};
        auto format = [&](std::string_view specs) {
            return std::string{field_prefix} + std::string{specs} + "}";
        };

        auto hex = scn::scan<int>(ss, scn::runtime_format(format("x")));
        ASSERT_THAT(hex, Succeeded()) << field_prefix;
        EXPECT_EQ(hex->value(), 0xff);

        auto dec = scn::scan<int>(ss, scn::runtime_format(format(".3d")));
        ASSERT_THAT(dec, Succeeded()) << field_prefix;
        EXPECT_EQ(dec->value(), 12);

        auto str =
            scn::scan<std::string>(ss, scn::runtime_format(format("[a-c]")));
        ASSERT_THAT(str, Succeeded()) << field_prefix;
        EXPECT_EQ(str->value(), "abc");

        auto oct = scn::scan<int>(ss, scn::runtime_format(format("o")));
        ASSERT_THAT(oct, Succeeded()) << field_prefix;
        EXPECT_EQ(oct->value(), 0777);

        auto failed = scn::scan<int>(ss, scn::runtime_format(format("d")));
        EXPECT_THAT(failed, FailedWith(scn::scan_error::invalid_scanned_value))
            << field_prefix;

        std::string rest;
        std::getline(ss, rest);
        EXPECT_EQ(rest, " xyz") << field_prefix;
    }
}

#endif
