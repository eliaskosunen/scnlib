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

#include <scn/scan.h>

#include <array>
#include <deque>
#include <string>
#include <utility>

// gcc 7 exceeds its maximum template instantiation depth with this many
// arguments
#if !SCN_GCC || SCN_GCC >= SCN_COMPILER(8, 0, 0)
namespace {
template <std::size_t N>
constexpr auto make_repeated_ints_format()
{
    std::array<char, N * 3> result{};
    for (std::size_t i = 0; i < N; ++i) {
        result[i * 3] = '{';
        result[i * 3 + 1] = '}';
        result[i * 3 + 2] = ' ';
    }
    return result;
}
template <std::size_t N>
inline constexpr auto repeated_ints_format = make_repeated_ints_format<N>();

template <std::size_t N>
std::string make_repeated_ints_input()
{
    std::string result;
    for (std::size_t i = 0; i < N; ++i) {
        result += std::to_string(i) + ' ';
    }
    return result;
}

template <std::size_t... I>
void check_many_arguments(std::index_sequence<I...>)
{
    constexpr auto n = sizeof...(I);
    const auto input = make_repeated_ints_input<n>();
    constexpr auto format = std::string_view{
        repeated_ints_format<n>.data(), repeated_ints_format<n>.size() - 1};

    auto compiletime = scn::scan<decltype((void)I, int{})...>(input, format);
    ASSERT_THAT(compiletime, Succeeded()) << n;
    EXPECT_EQ(compiletime->values(), std::tuple{static_cast<int>(I)...});

    auto runtime = scn::scan<decltype((void)I, int{})...>(
        input, scn::runtime_format(format));
    ASSERT_THAT(runtime, Succeeded()) << n;
    EXPECT_EQ(runtime->values(), std::tuple{static_cast<int>(I)...});

    // Format string with one too few replacement fields
    auto missing = scn::scan<decltype((void)I, int{})...>(
        input, scn::runtime_format(format.substr(0, format.size() - 3)));
    ASSERT_THAT(missing, FailedWith(scn::scan_error::invalid_format_string))
        << n;
}
}  // namespace
#endif

TEST(ScanTest, MultipleValues)
{
    auto result = scn::scan<int, int>("123 456", "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
}

TEST(ScanTest, StringValue)
{
    auto result = scn::scan<std::string>("abc def", "abc {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(std::get<0>(result->values()).c_str(), "def");
}

TEST(ScanTest, ResultUse)
{
    auto source = std::string_view{"123 456"};
    auto result = scn::scan<int>(source, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(std::get<0>(result->values()), 123);

    auto result2 = scn::scan<int>(
        scn::ranges::subrange{result->begin(), source.end()}, "{}");
    ASSERT_THAT(result2, Succeeded());
    EXPECT_EQ(std::get<0>(result2->values()), 456);
}

TEST(ScanTest, IntValue)
{
    auto result = scn::scan_value<int>("123");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);
}

TEST(ScanTest, Discard)
{
    auto result =
        scn::scan<int, scn::discard<int>, int>("123 456 789", "{} {} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, _, b] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 789);
}

TEST(ScanTest, BoolNumeric)
{
    auto result = scn::scan<bool>("1", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(result->value());
}
TEST(ScanTest, BoolText)
{
    auto result = scn::scan<bool>("true", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(result->value());
}

TEST(ScanTest, DefaultValueSuccess)
{
    auto result = scn::scan<int>("42", "{}", {123});
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 42);
}
TEST(ScanTest, DefaultValueFail)
{
    auto result = scn::scan<int>("foobar", "{}", {123});
    ASSERT_THAT(result, Failed());
}
TEST(ScanTest, DefaultValueString)
{
    std::string initial_string;
    initial_string.reserve(256);
    const auto addr = initial_string.data();

    auto result =
        scn::scan<std::string>("foobar", "{}", {std::move(initial_string)});
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "foobar");
    EXPECT_EQ(result->value().data(), addr);
    EXPECT_GE(result->value().capacity(), 256);
}

TEST(ScanTest, NumberedArguments)
{
    auto result = scn::scan<int, int>("123 456", "{0} {1}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
}

TEST(ScanTest, NumberedArgumentsSwapped)
{
    auto result = scn::scan<int, int>("123 456", "{1} {0}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 456);
    EXPECT_EQ(b, 123);
}

TEST(ScanTest, NumberedArgumentsRepeatedSingleArg)
{
    auto result = scn::scan<int>("123 456", scn::runtime_format("{0} {0}"));
    ASSERT_THAT(result, Failed());
}

TEST(ScanTest, NumberedArgumentsRepeatedDoubleArg)
{
    auto result =
        scn::scan<int, int>("123 456", scn::runtime_format("{0} {0}"));
    ASSERT_THAT(result, Failed());
}

TEST(ScanTest, NumberedArgumentsOutOfRange)
{
    auto result = scn::scan<int>("123 456", scn::runtime_format("{1}"));
    ASSERT_THAT(result, Failed());
}

TEST(ScanTest, FuzzerFailStringInput)
{
    auto result = scn::scan<std::string>("]]\360\n", "{}");
    ASSERT_THAT(result, Failed());
}
TEST(ScanTest, FuzzerFailDequeInput)
{
    using namespace std::string_view_literals;
    auto in = "]\360\n"sv;
    std::deque<char> rng{};
    std::copy(in.begin(), in.end(), std::back_inserter(rng));

    auto result = scn::scan<std::string>(rng, "{}");
    ASSERT_THAT(result, Failed());
}

TEST(ScanTest, DeconstructedTimestamp)
{
    auto res = scn::scan<int, int, int, int, int, double>(
        "2024-03-23T09:20:33.576864", "{:4}-{:2}-{:2}T{:2}:{:2}:{}");
    ASSERT_THAT(res, Succeeded());
    EXPECT_EQ(std::get<0>(res->values()), 2024);
    EXPECT_EQ(std::get<1>(res->values()), 3);
    EXPECT_EQ(std::get<2>(res->values()), 23);
    EXPECT_EQ(std::get<3>(res->values()), 9);
    EXPECT_EQ(std::get<4>(res->values()), 20);
    EXPECT_DOUBLE_EQ(std::get<5>(res->values()), 33.576864);
}
TEST(ScanTest, DeconstructedTimestamp2)
{
    auto res = scn::scan<int, int, int, int, int>("2024-03-23T09:20:33.576864",
                                                  "{:4}-{:2}-{:2}T{:2}:{:2}:");
    ASSERT_THAT(res, Succeeded());
    EXPECT_EQ(std::get<0>(res->values()), 2024);
    EXPECT_EQ(std::get<1>(res->values()), 3);
    EXPECT_EQ(std::get<2>(res->values()), 23);
    EXPECT_EQ(std::get<3>(res->values()), 9);
    EXPECT_EQ(std::get<4>(res->values()), 20);
    EXPECT_STREQ(res->range().data(), "33.576864");
}

TEST(ScanTest, LotsOfArguments)
{
    auto res = scn::scan<int, int, int, int, int, int, int, double>(
        "1 2 3 4 5 6 7 8.9", "{} {} {} {} {} {} {} {}");
    ASSERT_THAT(res, Succeeded());
    auto [a1, a2, a3, a4, a5, a6, a7, a8] = res->values();
    EXPECT_EQ(a1, 1);
    EXPECT_EQ(a2, 2);
    EXPECT_EQ(a3, 3);
    EXPECT_EQ(a4, 4);
    EXPECT_EQ(a5, 5);
    EXPECT_EQ(a6, 6);
    EXPECT_EQ(a7, 7);
    EXPECT_DOUBLE_EQ(a8, 8.9);
}
TEST(ScanTest, EvenMoreArguments)
{
    auto res = scn::scan<int, int, int, int, int, int, int, int, int, int, int,
                         int, int, int, int, int, int, int, int, int, int, int,
                         int, int, int, int, int, int, int, int, int, int, int>(
        "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 "
        "27 28 29 30 31 32 33",
        "{} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} "
        "{} {} {} {} {} {} {} {} {} {}");
    ASSERT_THAT(res, Succeeded());
}

#if !SCN_GCC || SCN_GCC >= SCN_COMPILER(8, 0, 0)
TEST(ScanTest, SixtyFourOrMoreArguments)
{
    check_many_arguments(std::make_index_sequence<63>{});
    check_many_arguments(std::make_index_sequence<64>{});
    check_many_arguments(std::make_index_sequence<65>{});
    check_many_arguments(std::make_index_sequence<71>{});
    check_many_arguments(std::make_index_sequence<72>{});
    check_many_arguments(std::make_index_sequence<73>{});
    check_many_arguments(std::make_index_sequence<80>{});
}
#endif

TEST(ScanTest, DoubleNewline)
{
    auto res = scn::scan<int>("1\n\n", "{}\n\n");
    ASSERT_THAT(res, Succeeded());
    EXPECT_EQ(res->value(), 1);
    EXPECT_EQ(res->begin(), res->end());
}
TEST(ScanTest, DoubleNewline2)
{
    auto res = scn::scan<int, int>("1\n\n2", "{}\n\n{}");
    ASSERT_THAT(res, Succeeded());
    auto [a, b] = res->values();
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
    EXPECT_EQ(res->begin(), res->end());
}

TEST(ScanTest, Pointer)
{
    auto res =
        scn::scan<void*, const void*>("0xdeadbeef 0XABBAABBA", "{} {:p}");
    ASSERT_THAT(res, Succeeded());
    auto [a, b] = res->values();
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(a), 0xdeadbeef);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(b), 0xABBAABBA);
}

TEST(ScanTest, ScanValueWithDefaultSuccess)
{
    auto result = scn::scan_value<int>("789", 123);
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 789);
}

TEST(ScanTest, ScanValueWithDefaultFailure)
{
    auto result = scn::scan_value<int>("notanumber", 999);
    ASSERT_THAT(result, Failed());
}

TEST(ScanTest, DiscardFloat)
{
    auto result = scn::scan<double, scn::discard<double>, double>("1.5 2.5 3.5",
                                                                  "{} {} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, _, b] = result->values();
    EXPECT_DOUBLE_EQ(a, 1.5);
    EXPECT_DOUBLE_EQ(b, 3.5);
}

TEST(ScanTest, DiscardString)
{
    auto result =
        scn::scan<std::string, scn::discard<std::string>, std::string>(
            "hello world bye", "{} {} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, _, b] = result->values();
    EXPECT_EQ(a, "hello");
    EXPECT_EQ(b, "bye");
}

TEST(ScanTest, DiscardChar)
{
    auto result = scn::scan<char, scn::discard<char>, char>("abc", "{}{}{}");
    ASSERT_THAT(result, Succeeded());
    auto [a, _, b] = result->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'c');
}

TEST(ScanTest, EmptySource)
{
    auto result = scn::scan<int>("", "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
}

TEST(ScanTest, MixedTypes)
{
    auto result = scn::scan<int, double, std::string, char>("42 3.14 hello w",
                                                            "{} {} {} {}");
    ASSERT_THAT(result, Succeeded());
    auto [i, d, s, c] = result->values();
    EXPECT_EQ(i, 42);
    EXPECT_DOUBLE_EQ(d, 3.14);
    EXPECT_EQ(s, "hello");
    EXPECT_EQ(c, 'w');
}

TEST(ScanTest, WithLiterals)
{
    auto result = scn::scan<int, int>("value=42,next=99", "value={},next={}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 42);
    EXPECT_EQ(b, 99);
}

TEST(ScanTest, TrailingContent)
{
    auto result = scn::scan<int>("42 extra stuff", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 42);
    EXPECT_STREQ(result->begin(), " extra stuff");
}

TEST(ScanTest, ConsecutiveScans)
{
    std::string_view source = "10 20 30";
    auto r1 = scn::scan<int>(source, "{}");
    ASSERT_THAT(r1, Succeeded());
    EXPECT_EQ(r1->value(), 10);

    auto r2 = scn::scan<int>(r1->range(), "{}");
    ASSERT_THAT(r2, Succeeded());
    EXPECT_EQ(r2->value(), 20);

    auto r3 = scn::scan<int>(r2->range(), "{}");
    ASSERT_THAT(r3, Succeeded());
    EXPECT_EQ(r3->value(), 30);
}

TEST(ScanTest, FloatAndInt)
{
    auto result = scn::scan<float, int>("2.5 100", "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [f, i] = result->values();
    EXPECT_FLOAT_EQ(f, 2.5f);
    EXPECT_EQ(i, 100);
}

TEST(ScanTest, LongDouble)
{
    auto result = scn::scan<long double>("123.456789", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->value(),
                testing::DistanceFrom(123.456789L, testing::Lt(0.000001L)));
}

TEST(ScanTest, BoolThenInt)
{
    auto result = scn::scan<bool, int>("true 42", "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [b, i] = result->values();
    EXPECT_TRUE(b);
    EXPECT_EQ(i, 42);
}

TEST(ScanTest, TrailingWhitespaceInFormatAtEndOfInput)
{
    auto result = scn::scan<int>("123", "{} ");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(ScanTest, MissingFieldAtEndOfInputIsEndOfInput)
{
    auto result = scn::scan<int, int, int>("123 456", "{} {} {}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
}

TEST(ScanTest, LiteralAtEndOfInputIsEndOfInput)
{
    auto result = scn::scan<int>("123", "{},");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
}

TEST(ScanTest, LiteralMismatch)
{
    auto result = scn::scan<int>("123;", "{},");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_literal));
}
