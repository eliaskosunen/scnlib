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

TEST(ChronoScanTest, DayOfYear)
{
    auto result = scn::scan<std::tm>("100", "{:%j}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_yday, 100 - 1);

    result = scn::scan<std::tm>("365", "{:%j}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_yday, 365 - 1);

    result = scn::scan<std::tm>("001", "{:%j}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_yday, 0);
}

TEST(ChronoScanTest, ChronoMonth)
{
    auto result = scn::scan<scn::month>("03", "{:%m}");
    ASSERT_TRUE(result);
    EXPECT_EQ(static_cast<unsigned>(result->value()), 3);

    result = scn::scan<scn::month>("December", "{:%B}");
    ASSERT_TRUE(result);
    EXPECT_EQ(static_cast<unsigned>(result->value()), 12);
}

TEST(ChronoScanTest, ChronoYear)
{
    auto result = scn::scan<scn::year>("2024", "{:%Y}");
    ASSERT_TRUE(result);
    EXPECT_EQ(static_cast<int>(result->value()), 2024);

    result = scn::scan<scn::year>("1999", "{:%Y}");
    ASSERT_TRUE(result);
    EXPECT_EQ(static_cast<int>(result->value()), 1999);
}

TEST(ChronoScanTest, ChronoYearMonth)
{
    auto result = scn::scan<scn::year_month>("2024-08", "{:%Y-%m}");
    ASSERT_TRUE(result);
    EXPECT_EQ(static_cast<int>(result->value().year()), 2024);
    EXPECT_EQ(static_cast<unsigned>(result->value().month()), 8);
}

TEST(ChronoScanTest, ChronoMonthDay)
{
    auto result = scn::scan<scn::month_day>("08-21", "{:%m-%d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(static_cast<unsigned>(result->value().month()), 8);
    EXPECT_EQ(static_cast<unsigned>(result->value().day()), 21);
}

TEST(ChronoScanTest, InvalidMonth)
{
    auto result = scn::scan<std::tm>("13", "{:%m}");
    ASSERT_FALSE(result);

    result = scn::scan<std::tm>("00", "{:%m}");
    ASSERT_FALSE(result);
}

TEST(ChronoScanTest, InvalidDay)
{
    auto result = scn::scan<std::tm>("00", "{:%d}");
    ASSERT_FALSE(result);

    result = scn::scan<std::tm>("32", "{:%d}");
    ASSERT_FALSE(result);
}

TEST(ChronoScanTest, InvalidHour)
{
    auto result = scn::scan<std::tm>("24", "{:%H}");
    ASSERT_FALSE(result);

    result = scn::scan<std::tm>("99", "{:%H}");
    ASSERT_FALSE(result);
}

TEST(ChronoScanTest, InvalidMinuteSecond)
{
    auto result = scn::scan<std::tm>("60", "{:%M}");
    ASSERT_FALSE(result);

    result = scn::scan<std::tm>("61", "{:%S}");
    ASSERT_FALSE(result);
}

TEST(ChronoScanTest, CombinedDateTime)
{
    auto result = scn::scan<std::tm>("08/21/24 14:30:45", "{:%D %T}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 7);
    EXPECT_EQ(result->value().tm_mday, 21);
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
    EXPECT_EQ(result->value().tm_hour, 14);
    EXPECT_EQ(result->value().tm_min, 30);
    EXPECT_EQ(result->value().tm_sec, 45);
}

TEST(ChronoScanTest, DatetimeComponentsMinimal)
{
    auto result = scn::scan<scn::datetime_components>("2024", "{:%Y}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().year, 2024);
    EXPECT_FALSE(result->value().mon);
    EXPECT_FALSE(result->value().mday);
}

TEST(ChronoScanTest, DatetimeComponentsWithSubsec)
{
    auto result = scn::scan<scn::datetime_components>("56.123456", "{:%.S}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().sec, 56);
    EXPECT_TRUE(result->value().subsec);
    EXPECT_NEAR(result->value().subsec.value(), 0.123456, 0.000001);
}

TEST(ChronoScanTest, TmWithTzOffset)
{
    auto result = scn::scan<scn::tm_with_tz>("-0500", "{:%z}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{-5 * 60});

    result = scn::scan<scn::tm_with_tz>("+00:00", "{:%z}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{0});
}

TEST(ChronoScanTest, TmWithTzName)
{
    auto result = scn::scan<scn::tm_with_tz>("PST", "{:%Z}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tz_name, "PST");

    result = scn::scan<scn::tm_with_tz>("America/New_York", "{:%Z}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tz_name, "America/New_York");
}

TEST(ChronoScanTest, LeapYear)
{
    auto result = scn::scan<std::tm>("2024-02-29", "{:%Y-%m-%d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
    EXPECT_EQ(result->value().tm_mon, 1);
    EXPECT_EQ(result->value().tm_mday, 29);
}

TEST(ChronoScanTest, EmptyInput)
{
    auto result = scn::scan<std::tm>("", "{:%Y}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::end_of_input);
}

TEST(ChronoScanTest, MultipleTimeFields)
{
    auto result =
        scn::scan<std::tm, std::tm>("12:30 14:45", "{:%H:%M} {:%H:%M}");
    ASSERT_TRUE(result);
    auto [t1, t2] = result->values();
    EXPECT_EQ(t1.tm_hour, 12);
    EXPECT_EQ(t1.tm_min, 30);
    EXPECT_EQ(t2.tm_hour, 14);
    EXPECT_EQ(t2.tm_min, 45);
}

TEST(ChronoScanTest, DateAndTime)
{
    auto result =
        scn::scan<std::tm>("2024-01-15 09:45:30", "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
    EXPECT_EQ(result->value().tm_mon, 0);
    EXPECT_EQ(result->value().tm_mday, 15);
    EXPECT_EQ(result->value().tm_hour, 9);
    EXPECT_EQ(result->value().tm_min, 45);
    EXPECT_EQ(result->value().tm_sec, 30);
}

TEST(ChronoScanTest, MonthNames)
{
    auto result = scn::scan<std::tm>("Jan", "{:%b}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 0);

    result = scn::scan<std::tm>("Feb", "{:%b}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 1);

    result = scn::scan<std::tm>("March", "{:%B}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 2);
}

TEST(ChronoScanTest, WeekdayNames)
{
    auto result = scn::scan<std::tm>("Sun", "{:%a}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 0);

    result = scn::scan<std::tm>("Sunday", "{:%A}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 0);

    result = scn::scan<std::tm>("Thu", "{:%a}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 4);
}

TEST(ChronoScanTest, DateFormats)
{
    auto result = scn::scan<std::tm>("12/31/99", "{:%D}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 11);
    EXPECT_EQ(result->value().tm_mday, 31);

    result = scn::scan<std::tm>("2024-12-25", "{:%F}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
    EXPECT_EQ(result->value().tm_mon, 11);
    EXPECT_EQ(result->value().tm_mday, 25);
}

TEST(ChronoScanTest, TimeFormats)
{
    auto result = scn::scan<std::tm>("14:30", "{:%R}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 14);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("14:30:45", "{:%T}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 14);
    EXPECT_EQ(result->value().tm_min, 30);
    EXPECT_EQ(result->value().tm_sec, 45);
}

TEST(ChronoScanTest, TmWithTzOffset2)
{
    auto result = scn::scan<scn::tm_with_tz>("+0530", "{:%z}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{5 * 60 + 30});
}

TEST(ChronoScanTest, DatetimeComponentsComplete)
{
    auto result = scn::scan<scn::datetime_components>(
        "2024-08-23T23:06:10.123+02:00", "{:%Y-%m-%dT%H:%M:%.S%z}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().year, 2024);
    EXPECT_EQ(result->value().mon, scn::August);
    EXPECT_EQ(result->value().mday, 23);
    EXPECT_EQ(result->value().hour, 23);
    EXPECT_EQ(result->value().min, 6);
    EXPECT_EQ(result->value().sec, 10);
    EXPECT_TRUE(result->value().subsec);
    EXPECT_NEAR(result->value().subsec.value(), 0.123, 0.001);
    EXPECT_EQ(result->value().tz_offset, std::chrono::hours{2});
}

TEST(ChronoScanTest, DatetimeComponentsYearOnly)
{
    auto result = scn::scan<scn::datetime_components>("2023", "{:%Y}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().year, 2023);
    EXPECT_FALSE(result->value().mon);
}

TEST(ChronoScanTest, DatetimeComponentsMonthOnly)
{
    auto result = scn::scan<scn::datetime_components>("05", "{:%m}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->value().mon);
    EXPECT_EQ(result->value().mon, scn::May);
}

TEST(ChronoScanTest, DatetimeComponentsHourMinute)
{
    auto result = scn::scan<scn::datetime_components>("15:45", "{:%H:%M}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().hour, 15);
    EXPECT_EQ(result->value().min, 45);
}

TEST(ChronoScanTest, DatetimeComponentsSecondOnly)
{
    auto result = scn::scan<scn::datetime_components>("45", "{:%S}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().sec, 45);
}

TEST(ChronoScanTest, DatetimeComponentsTmCentury)
{
    auto result = scn::scan<std::tm>("21", "{:%C}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2100 - 1900);
}

TEST(ChronoScanTest, DatetimeComponentsTmShortYear)
{
    auto result = scn::scan<std::tm>("50", "{:%y}");
    ASSERT_TRUE(result);
    // 50 means 1950 if year >= 69, otherwise 2000+year
    // So 50 should be 2050
    EXPECT_EQ(result->value().tm_year, 2050 - 1900);

    result = scn::scan<std::tm>("00", "{:%y}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2000 - 1900);
}

TEST(ChronoScanTest, DatetimeComponentsTimezoneName)
{
    auto result = scn::scan<scn::datetime_components>("GMT", "{:%Z}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->value().tz_name);
    EXPECT_EQ(result->value().tz_name, "GMT");
}

TEST(ChronoScanTest, DatetimeComponentsTimezoneOffset)
{
    auto result = scn::scan<scn::datetime_components>("-0800", "{:%z}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->value().tz_offset);
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{-8 * 60});
}

TEST(ChronoScanTest, DateTimeSeparator)
{
    auto result =
        scn::scan<std::tm>("2024-01-01 12:00:00", "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
    EXPECT_EQ(result->value().tm_mon, 0);
    EXPECT_EQ(result->value().tm_mday, 1);
    EXPECT_EQ(result->value().tm_hour, 12);
}

TEST(ChronoScanTest, MonthDayVariations)
{
    auto result = scn::scan<scn::month_day>("01-15", "{:%m-%d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(static_cast<unsigned>(result->value().month()), 1);
    EXPECT_EQ(static_cast<unsigned>(result->value().day()), 15);

    result = scn::scan<scn::month_day>("12-31", "{:%m-%d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(static_cast<unsigned>(result->value().month()), 12);
    EXPECT_EQ(static_cast<unsigned>(result->value().day()), 31);
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
