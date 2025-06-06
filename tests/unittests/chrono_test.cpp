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

#if !SCN_DISABLE_CHRONO

#include <scn/chrono.h>

namespace {

TEST(ChronoScanTest, ScanTmYear)
{
    auto result = scn::scan<std::tm>("2024", "{:%Y}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
}

TEST(ChronoScanTest, ScanTmMonth)
{
    auto result = scn::scan<std::tm>("10", "{:%m}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 10 - 1);

    result = scn::scan<std::tm>("09", "{:%m}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 9 - 1);

    result = scn::scan<std::tm>("90", "{:%m}");
    ASSERT_FALSE(result);
}

TEST(ChronoScanTest, ScanTmMDay)
{
    auto result = scn::scan<std::tm>("30", "{:%d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mday, 30);

    result = scn::scan<std::tm>("03", "{:%d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mday, 3);

    result = scn::scan<std::tm>("90", "{:%d}");
    ASSERT_FALSE(result);
}

TEST(ChronoScanTest, ScanTmISODate)
{
    auto result = scn::scan<std::tm>("2024-08-21", "{:%Y-%m-%d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
    EXPECT_EQ(result->value().tm_mon, 8 - 1);
    EXPECT_EQ(result->value().tm_mday, 21);
}

TEST(ChronoScanTest, LiteralText)
{
    auto result = scn::scan<std::tm>("%abc", "{:%%abc}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');

    result = scn::scan<std::tm>("   %abc", "{:%%abc}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');

    result = scn::scan<std::tm>("   a    bc", "{:%na  bc}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');
}

TEST(ChronoScanTest, SetMultipleTimes)
{
    auto result =
        scn::scan<std::tm>("2023 2024", scn::runtime_format("{:%Y %Y}"));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string);

    result =
        scn::scan<std::tm>("2023 2024-10-01", scn::runtime_format("{:%Y %F}"));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string);
}

TEST(ChronoScanTest, Timezone)
{
    auto result = scn::scan<scn::tm_with_tz>("+0200", "{:%z}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{2 * 60});

    result = scn::scan<scn::tm_with_tz>("+04:30", "{:%z}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{4 * 60 + 30});

    result = scn::scan<scn::tm_with_tz>("-2:00", "{:%Ez}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{-2 * 60});

    result = scn::scan<scn::tm_with_tz>("UTC", "{:%Z}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tz_name, "UTC");

    result = scn::scan<scn::tm_with_tz>("Europe/Helsinki", "{:%Z}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tz_name, "Europe/Helsinki");
}

TEST(ChronoScanTest, ShortYearAndCentury)
{
    auto result = scn::scan<std::tm>("2024", "{:%C%y}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);

    result = scn::scan<std::tm>("24", "{:%y}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);

    result = scn::scan<std::tm>("84", "{:%y}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 1984 - 1900);

    result = scn::scan<std::tm>("20", "{:%C}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2000 - 1900);

    result = scn::scan<std::tm>("20242024", scn::runtime_format("{:%Y%C%y}"));
    ASSERT_FALSE(result);
}

TEST(ChronoScanTest, Time24HClock)
{
    auto result = scn::scan<std::tm>("09:30", "{:%H:%M}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 9);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("21:30", "{:%R}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 21);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("09:30:03", "{:%H:%M:%S}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 9);
    EXPECT_EQ(result->value().tm_min, 30);
    EXPECT_EQ(result->value().tm_sec, 3);

    result = scn::scan<std::tm>("21:30:03", "{:%T}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 21);
    EXPECT_EQ(result->value().tm_min, 30);
    EXPECT_EQ(result->value().tm_sec, 3);
}

TEST(ChronoScanTest, Time12HClock)
{
    auto result = scn::scan<std::tm>("09:30", "{:%I:%M}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 9);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("12:30", "{:%I:%M}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 12);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("11:30 am", "{:%I:%M %p}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 11);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("12:30 a.m.", "{:%I:%M %p}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 0);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("11:30 pm", "{:%I:%M %p}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 23);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("12:30 PM", "{:%I:%M %p}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 12);
    EXPECT_EQ(result->value().tm_min, 30);
}

TEST(ChronoScanTest, MonthByName)
{
    auto result = scn::scan<std::tm>("Jan", "{:%B}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 0);

    result = scn::scan<std::tm>("february", "{:%B}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 1);

    result = scn::scan<std::tm>("marc", "{:%B}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), 'c');
    EXPECT_EQ(result->value().tm_mon, 2);

    result = scn::scan<std::tm>("Foo", "{:%B}");
    ASSERT_FALSE(result);
}

TEST(ChronoScanTest, Weekday)
{
    auto result = scn::scan<std::tm>("0", "{:%w}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 0);

    result = scn::scan<std::tm>("Mon", "{:%a}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 1);

    result = scn::scan<std::tm>("tuesday", "{:%A}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 2);

    result = scn::scan<std::tm>("Foo", "{:%a}");
    ASSERT_FALSE(result);
}

TEST(ChronoScanTest, DatetimeComponents)
{
    auto result = scn::scan<scn::datetime_components>(
        "2024-08-23T23:06:10+02:00", "{:%Y-%m-%dT%H:%M:%S%z}");
    ASSERT_TRUE(result);

    EXPECT_EQ(result->value().year, 2024);
    EXPECT_EQ(result->value().mon, scn::August);
    EXPECT_EQ(result->value().mday, 23);
    EXPECT_EQ(result->value().hour, 23);
    EXPECT_EQ(result->value().min, 6);
    EXPECT_EQ(result->value().sec, 10);
    EXPECT_EQ(result->value().tz_offset, std::chrono::hours{2});

    EXPECT_FALSE(result->value().subsec);
    EXPECT_FALSE(result->value().wday);
    EXPECT_FALSE(result->value().yday);
    EXPECT_FALSE(result->value().tz_name);
}

TEST(ChronoScanTest, Subsecond)
{
    auto result_tm =
        scn::scan<std::tm>("12:34:56.789", scn::runtime_format("{:%H:%M:%.S}"));
    ASSERT_FALSE(result_tm);
    EXPECT_EQ(result_tm.error().code(), scn::scan_error::invalid_format_string);

    auto result_dtc =
        scn::scan<scn::datetime_components>("12:34:56.789", "{:%H:%M:%.S}");
    ASSERT_TRUE(result_dtc);
    EXPECT_EQ(result_dtc->value().hour, 12);
    EXPECT_EQ(result_dtc->value().min, 34);
    EXPECT_EQ(result_dtc->value().sec, 56);
    EXPECT_DOUBLE_EQ(result_dtc->value().subsec.value_or(-1.0), 0.789);
}

TEST(ChronoScanTest, ChronoCalendarTypes)
{
    auto result_wd = scn::scan<scn::weekday>("Monday", "{:%a}");
    ASSERT_TRUE(result_wd);
    EXPECT_EQ(result_wd->value().c_encoding(), 1);

    result_wd = scn::scan<scn::weekday>("2", "{:%w}");
    ASSERT_TRUE(result_wd);
    EXPECT_EQ(result_wd->value().c_encoding(), 2);

    auto result_d = scn::scan<scn::day>("10", "{:%d}");
    ASSERT_TRUE(result_d);
    EXPECT_EQ(static_cast<unsigned>(result_d->value()), 10);

    auto result_ymd =
        scn::scan<scn::year_month_day>("2024-08-24", "{:%Y-%m-%d}");
    ASSERT_TRUE(result_ymd);
    EXPECT_EQ(static_cast<int>(result_ymd->value().year()), 2024);
    EXPECT_EQ(static_cast<unsigned>(result_ymd->value().month()), 8);
    EXPECT_EQ(static_cast<unsigned>(result_ymd->value().day()), 24);
}

TEST(ChronoScanTest, ChronoTimePoint)
{
    auto result = scn::scan<std::chrono::system_clock::time_point>(
        "2024-09-10 23:11:10", "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_TRUE(result);
    auto val = std::chrono::duration_cast<std::chrono::seconds>(
        result->value().time_since_epoch());

    std::tm expected_tm{};
    expected_tm.tm_sec = 10;
    expected_tm.tm_min = 11;
    expected_tm.tm_hour = 23;
    expected_tm.tm_mday = 10;
    expected_tm.tm_mon = 8;
    expected_tm.tm_year = 2024 - 1900;
    expected_tm.tm_wday = 0;
    expected_tm.tm_yday = 0;
    expected_tm.tm_isdst = -1;
    auto expected_val = std::chrono::seconds{std::mktime(&expected_tm)};

    EXPECT_EQ(val, expected_val);
}

TEST(ChronoScanTest, Fuzz1)
{
    auto result = scn::scan<std::tm>("08/08/22", "{:%D}");
    ASSERT_TRUE(result);

    result = scn::scan<std::tm>("", "{:%D}");
    ASSERT_FALSE(result);

    auto str = std::string_view{""};
    result = scn::scan<std::tm>(
        scn::ranges::subrange{str.data(), str.data() + str.size()}, "{:%D}");
    ASSERT_FALSE(result);
}

}  // namespace

#endif  // !SCN_DISABLE_CHRONO
