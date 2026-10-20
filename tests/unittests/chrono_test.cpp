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
#include <scn/xchar.h>

#if !SCN_DISABLE_CHRONO

#include <scn/chrono.h>

namespace {

using sys_seconds =
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;
using sys_milliseconds = std::chrono::time_point<std::chrono::system_clock,
                                                 std::chrono::milliseconds>;
using sys_microseconds = std::chrono::time_point<std::chrono::system_clock,
                                                 std::chrono::microseconds>;
using sys_nanoseconds = std::chrono::time_point<std::chrono::system_clock,
                                                std::chrono::nanoseconds>;

TEST(ChronoScanTest, ScanTmYear)
{
    auto result = scn::scan<std::tm>("2024", "{:%Y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
}

TEST(ChronoScanTest, ScanTmMonth)
{
    auto result = scn::scan<std::tm>("10", "{:%m}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_mon, 10 - 1);

    result = scn::scan<std::tm>("09", "{:%m}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_mon, 9 - 1);

    result = scn::scan<std::tm>("90", "{:%m}");
    ASSERT_THAT(result, Failed());
}

TEST(ChronoScanTest, ScanTmMDay)
{
    auto result = scn::scan<std::tm>("30", "{:%d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_mday, 30);

    result = scn::scan<std::tm>("03", "{:%d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_mday, 3);

    result = scn::scan<std::tm>("90", "{:%d}");
    ASSERT_THAT(result, Failed());
}

TEST(ChronoScanTest, ScanTmISODate)
{
    auto result = scn::scan<std::tm>("2024-08-21", "{:%Y-%m-%d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
    EXPECT_EQ(result->value().tm_mon, 8 - 1);
    EXPECT_EQ(result->value().tm_mday, 21);

    result = scn::scan<std::tm>("2024-12-25", "{:%F}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
    EXPECT_EQ(result->value().tm_mon, 12 - 1);
    EXPECT_EQ(result->value().tm_mday, 25);
}

TEST(ChronoScanTest, LiteralText)
{
    auto result = scn::scan<std::tm>("%abc", "{:%%abc}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*result->begin(), '\0');

    result = scn::scan<std::tm>("   %abc", "{:%%abc}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*result->begin(), '\0');

    result = scn::scan<std::tm>("   a    bc", "{:%na  bc}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*result->begin(), '\0');
}

TEST(ChronoScanTest, SetMultipleTimes)
{
    auto result =
        scn::scan<std::tm>("2023 2024", scn::runtime_format("{:%Y %Y}"));
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_format_string));

    result =
        scn::scan<std::tm>("2023 2024-10-01", scn::runtime_format("{:%Y %F}"));
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_format_string));
}

TEST(ChronoScanTest, Timezone)
{
    auto result = scn::scan<scn::tm_with_tz>("+0200", "{:%z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{2 * 60});

    result = scn::scan<scn::tm_with_tz>("+04:30", "{:%z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{4 * 60 + 30});

    result = scn::scan<scn::tm_with_tz>("-2:00", "{:%Ez}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{-2 * 60});

    result = scn::scan<scn::tm_with_tz>("UTC", "{:%Z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_name, "UTC");

    result = scn::scan<scn::tm_with_tz>("Europe/Helsinki", "{:%Z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_name, "Europe/Helsinki");
}

TEST(ChronoScanTest, ShortYearAndCentury)
{
    auto result = scn::scan<std::tm>("2024", "{:%C%y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);

    result = scn::scan<std::tm>("24", "{:%y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);

    result = scn::scan<std::tm>("84", "{:%y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 1984 - 1900);

    result = scn::scan<std::tm>("00", "{:%y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 2000 - 1900);

    result = scn::scan<std::tm>("68", "{:%y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 2068 - 1900);

    result = scn::scan<std::tm>("69", "{:%y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 1969 - 1900);

    result = scn::scan<std::tm>("20", "{:%C}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 2000 - 1900);

    result = scn::scan<std::tm>("20242024", scn::runtime_format("{:%Y%C%y}"));
    ASSERT_THAT(result, Failed());
}

TEST(ChronoScanTest, Time24HClock)
{
    auto result = scn::scan<std::tm>("09:30", "{:%H:%M}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_hour, 9);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("21:30", "{:%R}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_hour, 21);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("09:30:03", "{:%H:%M:%S}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_hour, 9);
    EXPECT_EQ(result->value().tm_min, 30);
    EXPECT_EQ(result->value().tm_sec, 3);

    result = scn::scan<std::tm>("21:30:03", "{:%T}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_hour, 21);
    EXPECT_EQ(result->value().tm_min, 30);
    EXPECT_EQ(result->value().tm_sec, 3);
}

TEST(ChronoScanTest, Time12HClock)
{
    auto result = scn::scan<std::tm>("09:30", "{:%I:%M}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_hour, 9);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("12:30", "{:%I:%M}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_hour, 12);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("11:30 am", "{:%I:%M %p}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_hour, 11);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("12:30 a.m.", "{:%I:%M %p}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_hour, 0);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("11:30 pm", "{:%I:%M %p}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_hour, 23);
    EXPECT_EQ(result->value().tm_min, 30);

    result = scn::scan<std::tm>("12:30 PM", "{:%I:%M %p}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_hour, 12);
    EXPECT_EQ(result->value().tm_min, 30);
}

TEST(ChronoScanTest, MonthByName)
{
    auto result = scn::scan<std::tm>("Jan", "{:%B}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_mon, 0);

    result = scn::scan<std::tm>("february", "{:%B}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_mon, 1);

    result = scn::scan<std::tm>("marc", "{:%B}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*result->begin(), 'c');
    EXPECT_EQ(result->value().tm_mon, 2);

    result = scn::scan<std::tm>("Foo", "{:%B}");
    ASSERT_THAT(result, Failed());
}

TEST(ChronoScanTest, Weekday)
{
    auto result = scn::scan<std::tm>("0", "{:%w}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_wday, 0);

    result = scn::scan<std::tm>("Mon", "{:%a}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_wday, 1);

    result = scn::scan<std::tm>("tuesday", "{:%A}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_wday, 2);

    result = scn::scan<std::tm>("Foo", "{:%a}");
    ASSERT_THAT(result, Failed());
}

TEST(ChronoScanTest, IsoWeekday)
{
    auto result = scn::scan<std::tm>("1", "{:%u}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_wday, 1);

    result = scn::scan<std::tm>("7", "{:%u}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_wday, 0);

    result = scn::scan<std::tm>("0", "{:%u}");
    ASSERT_THAT(result, Failed());
}

TEST(ChronoScanTest, DatetimeComponents)
{
    auto result = scn::scan<scn::datetime_components>(
        "2024-08-23T23:06:10+02:00", "{:%Y-%m-%dT%H:%M:%S%z}");
    ASSERT_THAT(result, Succeeded());

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
    ASSERT_THAT(result_tm, FailedWith(scn::scan_error::invalid_format_string));

    auto result_dtc =
        scn::scan<scn::datetime_components>("12:34:56.789", "{:%H:%M:%.S}");
    ASSERT_THAT(result_dtc, Succeeded());
    EXPECT_EQ(result_dtc->value().hour, 12);
    EXPECT_EQ(result_dtc->value().min, 34);
    EXPECT_EQ(result_dtc->value().sec, 56);
    EXPECT_DOUBLE_EQ(result_dtc->value().subsec.value_or(-1.0), 0.789);
}

TEST(ChronoScanTest, ChronoCalendarTypes)
{
    auto result_wd = scn::scan<scn::weekday>("Monday", "{:%a}");
    ASSERT_THAT(result_wd, Succeeded());
    EXPECT_EQ(result_wd->value().c_encoding(), 1);

    result_wd = scn::scan<scn::weekday>("2", "{:%w}");
    ASSERT_THAT(result_wd, Succeeded());
    EXPECT_EQ(result_wd->value().c_encoding(), 2);

    auto result_d = scn::scan<scn::day>("10", "{:%d}");
    ASSERT_THAT(result_d, Succeeded());
    EXPECT_EQ(static_cast<unsigned>(result_d->value()), 10);

    auto result_ymd =
        scn::scan<scn::year_month_day>("2024-08-24", "{:%Y-%m-%d}");
    ASSERT_THAT(result_ymd, Succeeded());
    EXPECT_EQ(static_cast<int>(result_ymd->value().year()), 2024);
    EXPECT_EQ(static_cast<unsigned>(result_ymd->value().month()), 8);
    EXPECT_EQ(static_cast<unsigned>(result_ymd->value().day()), 24);
}

TEST(ChronoScanTest, ChronoTimePoint)
{
    auto result = scn::scan<std::chrono::system_clock::time_point>(
        "2024-09-10 23:11:10", "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_THAT(result, Succeeded());
    auto val = std::chrono::duration_cast<std::chrono::seconds>(
        result->value().time_since_epoch());
    EXPECT_EQ(val, std::chrono::seconds{1726009870});
}

TEST(ChronoScanTest, ChronoTimePointDefaults)
{
    auto month = scn::scan<sys_seconds>("2024-03", "{:%Y-%m}");
    ASSERT_THAT(month, Succeeded());
    EXPECT_EQ(month->value().time_since_epoch(),
              std::chrono::seconds{1709251200});

    auto time = scn::scan<sys_seconds>("12:30", "{:%H:%M}");
    ASSERT_THAT(time, Succeeded());
    EXPECT_EQ(time->value().time_since_epoch(), std::chrono::seconds{45000});

    auto before_epoch =
        scn::scan<sys_seconds>("1969-12-31 23:59:59", "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_THAT(before_epoch, Succeeded());
    EXPECT_EQ(before_epoch->value().time_since_epoch(),
              std::chrono::seconds{-1});
}

TEST(ChronoScanTest, ChronoTimePointWithOffset)
{
    auto result = scn::scan<sys_seconds>("2024-09-10 23:11:10 +0200",
                                         "{:%Y-%m-%d %H:%M:%S %z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().time_since_epoch(),
              std::chrono::seconds{1726002670});
}

TEST(ChronoScanTest, ChronoTimePointFractionalSeconds)
{
    auto with_fraction = scn::scan<sys_microseconds>(
        "2026-10-04 21:00:08.964009+00", "{:%F %T%z}");
    ASSERT_THAT(with_fraction, Succeeded());
    EXPECT_TRUE(with_fraction->range().empty());
    EXPECT_EQ(with_fraction->value().time_since_epoch(),
              std::chrono::microseconds{1791147608964009});

    auto without_fraction =
        scn::scan<sys_microseconds>("2026-01-02 03:04:05+00", "{:%F %T%z}");
    ASSERT_THAT(without_fraction, Succeeded());
    EXPECT_TRUE(without_fraction->range().empty());
    EXPECT_EQ(without_fraction->value().time_since_epoch(),
              std::chrono::microseconds{1767323045000000});

    auto with_offset = scn::scan<sys_microseconds>(
        "2026-01-02 03:04:05.5+02:30", "{:%F %T%z}");
    ASSERT_THAT(with_offset, Succeeded());
    EXPECT_EQ(with_offset->value().time_since_epoch(),
              std::chrono::microseconds{1767314045500000});

    // Truncated, not off by one from floating-point rounding
    auto tiny = scn::scan<sys_microseconds>("00.000065", "{:%S}");
    ASSERT_THAT(tiny, Succeeded());
    EXPECT_EQ(tiny->value().time_since_epoch(), std::chrono::microseconds{65});

    auto subsecond = scn::scan<sys_microseconds>("00.000065", "{:%.S}");
    ASSERT_THAT(subsecond, Succeeded());
    EXPECT_EQ(subsecond->value().time_since_epoch(),
              std::chrono::microseconds{65});
}

TEST(ChronoScanTest, ChronoTimePointFractionalSecondsPrecision)
{
    auto extra_digits = scn::scan<sys_milliseconds>("05.1239", "{:%S}");
    ASSERT_THAT(extra_digits, Succeeded());
    EXPECT_TRUE(extra_digits->range().empty());
    EXPECT_EQ(extra_digits->value().time_since_epoch(),
              std::chrono::milliseconds{5123});

    auto before_epoch = scn::scan<sys_milliseconds>("1969-12-31 23:59:59.5",
                                                    "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_THAT(before_epoch, Succeeded());
    EXPECT_EQ(before_epoch->value().time_since_epoch(),
              std::chrono::milliseconds{-500});

    auto no_digits = scn::scan<sys_milliseconds>("05.x", "{:%S}");
    ASSERT_THAT(no_digits, Succeeded());
    EXPECT_EQ(no_digits->range().size(), 2u);
    EXPECT_EQ(no_digits->value().time_since_epoch(),
              std::chrono::milliseconds{5000});

    // Seconds aren't fractional when the type can't hold a fraction
    auto whole_seconds = scn::scan<sys_seconds>("05.5", "{:%S}");
    ASSERT_THAT(whole_seconds, Succeeded());
    EXPECT_EQ(whole_seconds->range().size(), 2u);
    EXPECT_EQ(whole_seconds->value().time_since_epoch(),
              std::chrono::seconds{5});
}

TEST(ChronoScanTest, ChronoTimePointOutOfRange)
{
    // The range of nanoseconds in int64_t ends at 2262-04-11 23:47:16.854775807
    auto max = scn::scan<sys_nanoseconds>("2262-04-11 23:47:16.854775807",
                                          "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_THAT(max, Succeeded());
    EXPECT_EQ(max->value().time_since_epoch(), std::chrono::nanoseconds::max());

    auto past_max = scn::scan<sys_nanoseconds>("2262-04-11 23:47:16.854775808",
                                               "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_THAT(past_max, FailedWith(scn::scan_error::value_positive_overflow));

    auto later = scn::scan<sys_nanoseconds>("2263-01-01 00:00:00",
                                            "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_THAT(later, FailedWith(scn::scan_error::value_positive_overflow));

    // The range of nanoseconds in int64_t starts at
    // 1677-09-21 00:12:43.145224192
    auto min = scn::scan<sys_nanoseconds>("1677-09-21 00:12:43.145224192",
                                          "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_THAT(min, Succeeded());
    EXPECT_EQ(min->value().time_since_epoch(), std::chrono::nanoseconds::min());

    auto past_min = scn::scan<sys_nanoseconds>("1677-09-21 00:12:43.145224191",
                                               "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_THAT(past_min, FailedWith(scn::scan_error::value_negative_overflow));

    auto just_after_min = scn::scan<sys_nanoseconds>(
        "1677-09-21 00:12:43.145224193", "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_THAT(just_after_min, Succeeded());
    EXPECT_EQ(just_after_min->value().time_since_epoch(),
              std::chrono::nanoseconds::min() + std::chrono::nanoseconds{1});

    auto earlier = scn::scan<sys_nanoseconds>("1677-01-01 00:00:00",
                                              "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_THAT(earlier, FailedWith(scn::scan_error::value_negative_overflow));

    // Not limited by the range of nanoseconds
    auto micro = scn::scan<sys_microseconds>("3000-01-01 00:00:00.5",
                                             "{:%Y-%m-%d %H:%M:%S}");
    ASSERT_THAT(micro, Succeeded());
    EXPECT_EQ(micro->value().time_since_epoch(),
              std::chrono::microseconds{32503680000500000});
}

TEST(ChronoScanTest, ChronoTimePointWithWeekday)
{
    auto result = scn::scan<std::chrono::system_clock::time_point>(
        "Tue, 10 Sep 2024 23:11:10", "{:%a, %d %b %Y %H:%M:%S}");
    ASSERT_THAT(result, Succeeded());
    auto without_weekday = scn::scan<std::chrono::system_clock::time_point>(
        "10 Sep 2024 23:11:10", "{:%d %b %Y %H:%M:%S}");
    ASSERT_THAT(without_weekday, Succeeded());
    EXPECT_EQ(result->value(), without_weekday->value());
}

TEST(ChronoScanTest, ChronoTimePointWithContradictingWeekday)
{
    // 2024-09-10 was a Tuesday
    auto result = scn::scan<std::chrono::system_clock::time_point>(
        "Wed, 10 Sep 2024 23:11:10", "{:%a, %d %b %Y %H:%M:%S}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));

    result = scn::scan<std::chrono::system_clock::time_point>("3 2024-09-10",
                                                              "{:%u %F}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));

    // Sunday is both 0 and 7. Dates before the epoch are checked too.
    result = scn::scan<std::chrono::system_clock::time_point>("0 2024-09-08",
                                                              "{:%w %F}");
    EXPECT_THAT(result, Succeeded());
    result = scn::scan<std::chrono::system_clock::time_point>("7 2024-09-08",
                                                              "{:%u %F}");
    EXPECT_THAT(result, Succeeded());
    result = scn::scan<std::chrono::system_clock::time_point>("Wed 1969-12-31",
                                                              "{:%a %F}");
    EXPECT_THAT(result, Succeeded());
    result = scn::scan<std::chrono::system_clock::time_point>("Thu 1969-12-31",
                                                              "{:%a %F}");
    EXPECT_THAT(result, Failed());

    // Not checked unless the whole date is scanned
    result = scn::scan<std::chrono::system_clock::time_point>("Wed 2024-09",
                                                              "{:%a %Y-%m}");
    EXPECT_THAT(result, Succeeded());
    result = scn::scan<std::chrono::system_clock::time_point>(
        "Wed 10 23:11:10", "{:%a %d %H:%M:%S}");
    EXPECT_THAT(result, Succeeded());

    auto ymd = scn::scan<scn::year_month_day>("Tue 2024-09-10", "{:%a %F}");
    ASSERT_THAT(ymd, Succeeded());
    ymd = scn::scan<scn::year_month_day>("Wed 2024-09-10", "{:%a %F}");
    ASSERT_THAT(ymd, FailedWith(scn::scan_error::invalid_scanned_value));

    // Types without a resolved date only get field-level checks
    auto tm = scn::scan<std::tm>("Wed 2024-09-10", "{:%a %F}");
    EXPECT_THAT(tm, Succeeded());
}

TEST(ChronoScanTest, DayOfYear)
{
    auto result = scn::scan<std::tm>("100", "{:%j}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_yday, 100 - 1);

    result = scn::scan<std::tm>("365", "{:%j}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_yday, 365 - 1);

    result = scn::scan<std::tm>("001", "{:%j}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_yday, 0);
}

TEST(ChronoScanTest, ChronoMonth)
{
    auto result = scn::scan<scn::month>("03", "{:%m}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(static_cast<unsigned>(result->value()), 3);

    result = scn::scan<scn::month>("December", "{:%B}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(static_cast<unsigned>(result->value()), 12);
}

TEST(ChronoScanTest, ChronoYear)
{
    auto result = scn::scan<scn::year>("2024", "{:%Y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(static_cast<int>(result->value()), 2024);

    result = scn::scan<scn::year>("1999", "{:%Y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(static_cast<int>(result->value()), 1999);
}

TEST(ChronoScanTest, ChronoYearMonth)
{
    auto result = scn::scan<scn::year_month>("2024-08", "{:%Y-%m}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(static_cast<int>(result->value().year()), 2024);
    EXPECT_EQ(static_cast<unsigned>(result->value().month()), 8);
}

TEST(ChronoScanTest, ChronoMonthDay)
{
    auto result = scn::scan<scn::month_day>("08-21", "{:%m-%d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(static_cast<unsigned>(result->value().month()), 8);
    EXPECT_EQ(static_cast<unsigned>(result->value().day()), 21);
}

TEST(ChronoScanTest, InvalidMonth)
{
    auto result = scn::scan<std::tm>("13", "{:%m}");
    ASSERT_THAT(result, Failed());

    result = scn::scan<std::tm>("00", "{:%m}");
    ASSERT_THAT(result, Failed());
}

TEST(ChronoScanTest, InvalidDay)
{
    auto result = scn::scan<std::tm>("00", "{:%d}");
    ASSERT_THAT(result, Failed());

    result = scn::scan<std::tm>("32", "{:%d}");
    ASSERT_THAT(result, Failed());
}

TEST(ChronoScanTest, InvalidHour)
{
    auto result = scn::scan<std::tm>("24", "{:%H}");
    ASSERT_THAT(result, Failed());

    result = scn::scan<std::tm>("99", "{:%H}");
    ASSERT_THAT(result, Failed());
}

TEST(ChronoScanTest, InvalidMinuteSecond)
{
    auto result = scn::scan<std::tm>("60", "{:%M}");
    ASSERT_THAT(result, Failed());

    result = scn::scan<std::tm>("61", "{:%S}");
    ASSERT_THAT(result, Failed());
}

TEST(ChronoScanTest, CombinedDateTime)
{
    auto result = scn::scan<std::tm>("08/21/24 14:30:45", "{:%D %T}");
    ASSERT_THAT(result, Succeeded());
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
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().year, 2024);
    EXPECT_FALSE(result->value().mon);
    EXPECT_FALSE(result->value().mday);
}

TEST(ChronoScanTest, DatetimeComponentsWithoutSubsec)
{
    auto result = scn::scan<scn::datetime_components>("56", "{:%.S}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().sec, 56);
    EXPECT_FALSE(result->value().subsec);

    // `%S` only reads a fraction for time points
    result = scn::scan<scn::datetime_components>("56.5", "{:%S}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().sec, 56);
    EXPECT_FALSE(result->value().subsec);
    EXPECT_EQ(result->range().size(), 2u);
}

TEST(ChronoScanTest, TmWithTzOffset)
{
    auto result = scn::scan<scn::tm_with_tz>("-0500", "{:%z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{-5 * 60});

    result = scn::scan<scn::tm_with_tz>("+00:00", "{:%z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{0});
}

TEST(ChronoScanTest, TmWithTzName)
{
    auto result = scn::scan<scn::tm_with_tz>("PST", "{:%Z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_name, "PST");

    result = scn::scan<scn::tm_with_tz>("America/New_York", "{:%Z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_name, "America/New_York");
}

TEST(ChronoScanTest, InvalidDayOfMonth)
{
    EXPECT_THAT(scn::scan<scn::year_month_day>("2026-02-30", "{:%F}"),
                Failed());
    EXPECT_THAT(scn::scan<scn::year_month_day>("2026-04-31", "{:%F}"),
                Failed());
    EXPECT_THAT(scn::scan<scn::year_month_day>("2026-02-29", "{:%F}"),
                Failed());
    EXPECT_THAT(scn::scan<scn::year_month_day>("1900-02-29", "{:%F}"),
                Failed());
    EXPECT_THAT(scn::scan<scn::year_month_day>("2000-02-29", "{:%F}"),
                Succeeded());
    EXPECT_THAT(scn::scan<scn::year_month_day>("2024-02-29", "{:%F}"),
                Succeeded());

    EXPECT_THAT(scn::scan<scn::month_day>("02-30", "{:%m-%d}"), Failed());
    EXPECT_THAT(scn::scan<scn::month_day>("02-29", "{:%m-%d}"), Succeeded());

    EXPECT_THAT(scn::scan<sys_seconds>("2026-02-30 21:00:00", "{:%F %T}"),
                Failed());
    EXPECT_THAT(scn::scan<sys_seconds>("2024-02-29 21:00:00", "{:%F %T}"),
                Succeeded());
    // The year defaults to 1970, which isn't a leap year
    EXPECT_THAT(scn::scan<sys_seconds>("02-29", "{:%m-%d}"), Failed());

    // std::tm only gets field-level checks
    EXPECT_THAT(scn::scan<std::tm>("2026-02-30", "{:%F}"), Succeeded());
}

TEST(ChronoScanTest, LeapSecond)
{
    EXPECT_THAT(scn::scan<sys_seconds>("2016-12-31 23:59:60", "{:%F %T}"),
                Failed());
    EXPECT_THAT(scn::scan<std::tm>("2016-12-31 23:59:60", "{:%F %T}"),
                Succeeded());
}

TEST(ChronoScanTest, EmptyInput)
{
    auto result = scn::scan<std::tm>("", "{:%Y}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
}

TEST(ChronoScanTest, MultipleTimeFields)
{
    auto result =
        scn::scan<std::tm, std::tm>("12:30 14:45", "{:%H:%M} {:%H:%M}");
    ASSERT_THAT(result, Succeeded());
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
    ASSERT_THAT(result, Succeeded());
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
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_mon, 0);

    result = scn::scan<std::tm>("Feb", "{:%b}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_mon, 1);

    result = scn::scan<std::tm>("March", "{:%B}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_mon, 2);
}

TEST(ChronoScanTest, WeekdayNames)
{
    auto result = scn::scan<std::tm>("Sun", "{:%a}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_wday, 0);

    result = scn::scan<std::tm>("Sunday", "{:%A}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_wday, 0);

    result = scn::scan<std::tm>("Thu", "{:%a}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_wday, 4);
}

TEST(ChronoScanTest, TmWithTzOffset2)
{
    auto result = scn::scan<scn::tm_with_tz>("+0530", "{:%z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{5 * 60 + 30});
}

TEST(ChronoScanTest, DatetimeComponentsComplete)
{
    auto result = scn::scan<scn::datetime_components>(
        "2024-08-23T23:06:10.123+02:00", "{:%Y-%m-%dT%H:%M:%.S%z}");
    ASSERT_THAT(result, Succeeded());
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

TEST(ChronoScanTest, DatetimeComponentsTimezoneNameWithSigns)
{
    auto result = scn::scan<scn::datetime_components>("Etc/GMT-3+x,", "{:%Z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_name, "Etc/GMT-3+x");
    EXPECT_EQ(result->range().size(), 1u);

    result = scn::scan<scn::datetime_components>("Etc/GMT-3+x", "{:%Z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_name, "Etc/GMT-3+x");
    EXPECT_TRUE(result->range().empty());
}

TEST(ChronoScanTest, DatetimeComponentsTimezoneNameEmpty)
{
    auto result = scn::scan<scn::datetime_components>("12 !", "{:%H %Z}");
    ASSERT_THAT(result, Failed());
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_scanned_value);

    result = scn::scan<scn::datetime_components>("12", "{:%H%Z}");
    ASSERT_THAT(result, Failed());
    EXPECT_EQ(result.error().code(), scn::scan_error::end_of_input);
}

TEST(ChronoScanTest, DatetimeComponentsTimezoneNameWide)
{
    auto result =
        scn::scan<scn::datetime_components>(L"Europe/Helsinki x", L"{:%Z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_name, "Europe/Helsinki");
    EXPECT_EQ(result->range().size(), 2u);
}

TEST(ChronoScanTest, DatetimeComponentsTimezoneOffset)
{
    auto result = scn::scan<scn::datetime_components>("-0800", "{:%z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(result->value().tz_offset);
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{-8 * 60});
}

TEST(ChronoScanTest, MonthDayVariations)
{
    auto result = scn::scan<scn::month_day>("01-15", "{:%m-%d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(static_cast<unsigned>(result->value().month()), 1);
    EXPECT_EQ(static_cast<unsigned>(result->value().day()), 15);

    result = scn::scan<scn::month_day>("12-31", "{:%m-%d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(static_cast<unsigned>(result->value().month()), 12);
    EXPECT_EQ(static_cast<unsigned>(result->value().day()), 31);
}

TEST(ChronoScanTest, DatetimeComponentsDayOfYear)
{
    auto result = scn::scan<scn::datetime_components>("100", "{:%j}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().yday, 100 - 1);

    result = scn::scan<scn::datetime_components>("366", "{:%j}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().yday, 366 - 1);
}

TEST(ChronoScanTest, DatetimeComponentsShortYearAndCentury)
{
    auto result = scn::scan<scn::datetime_components>("24", "{:%y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().year, 2024);

    result = scn::scan<scn::datetime_components>("99", "{:%y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().year, 1999);

    result = scn::scan<scn::datetime_components>("1999", "{:%C%y}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().year, 1999);

    result = scn::scan<scn::datetime_components>("19", "{:%C}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().year, 1900);

    auto year_result = scn::scan<scn::year>("24", "{:%y}");
    ASSERT_THAT(year_result, Succeeded());
    EXPECT_EQ(static_cast<int>(year_result->value()), 2024);
}

TEST(ChronoScanTest, WeekdayWithDate)
{
    constexpr auto input = std::string_view{"Tue, 15 Nov 1994"};

    auto dt = scn::scan<scn::datetime_components>(input, "{:%a, %d %b %Y}");
    ASSERT_THAT(dt, Succeeded());
    EXPECT_EQ(dt->value().wday, scn::Tuesday);
    EXPECT_EQ(dt->value().mday, 15);
    EXPECT_EQ(dt->value().mon, scn::November);
    EXPECT_EQ(dt->value().year, 1994);

    auto tm = scn::scan<std::tm>(input, "{:%a, %d %b %Y}");
    ASSERT_THAT(tm, Succeeded());
    EXPECT_EQ(tm->value().tm_wday, 2);
    EXPECT_EQ(tm->value().tm_mday, 15);

    auto ymd = scn::scan<scn::year_month_day>(input, "{:%a, %d %b %Y}");
    ASSERT_THAT(ymd, Succeeded());
    EXPECT_EQ(static_cast<int>(ymd->value().year()), 1994);
    EXPECT_EQ(static_cast<unsigned>(ymd->value().day()), 15);

    auto with_yday = scn::scan<scn::datetime_components>("Tue 319", "{:%a %j}");
    ASSERT_THAT(with_yday, Succeeded());
}

TEST(ChronoScanTest, MonthDayAndYearDayTogether)
{
    auto result = scn::scan<scn::datetime_components>(
        "15 319", scn::runtime_format("{:%d %j}"));
    ASSERT_THAT(result, Failed());
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string);
    EXPECT_STREQ(result.error().msg(),
                 "mday (%d/%e) and yday (%j) can't be used together");
}

TEST(ChronoScanTest, IncompatibleFieldCombinations)
{
    const auto check = [](std::string_view input, const char* format,
                          const char* message) {
        auto result = scn::scan<scn::datetime_components>(
            input, scn::runtime_format(format));
        ASSERT_THAT(result, Failed());
        EXPECT_EQ(result.error().code(),
                  scn::scan_error::invalid_format_string);
        EXPECT_STREQ(result.error().msg(), message);
    };
    check("13 PM", "{:%H %p}",
          "AM/PM (%p) can't be used with a 24-hour clock (%H)");
    check("PM", "{:%p}", "AM/PM (%p) requires a 12-hour clock hour (%I)");
    check("13 01", "{:%H %I}",
          "24-hour (%H) and 12-hour (%I) clocks can't both be in use "
          "simultaneously");
    check("2024 20", "{:%Y %C}",
          "full-year (%Y) can't be used together with century (%C) or "
          "short-year (%y)");
    check("2024 24", "{:%Y %y}",
          "full-year (%Y) can't be used together with century (%C) or "
          "short-year (%y)");
}

TEST(ChronoScanTest, CalendarTypeWithoutRequiredField)
{
    auto month = scn::scan<scn::month>("15", scn::runtime_format("{:%d}"));
    ASSERT_THAT(month, Failed());
    EXPECT_EQ(month.error().code(), scn::scan_error::invalid_format_string);

    auto ymd = scn::scan<scn::year_month_day>("2024-08",
                                              scn::runtime_format("{:%Y-%m}"));
    ASSERT_THAT(ymd, Failed());
    EXPECT_EQ(ymd.error().code(), scn::scan_error::invalid_format_string);

    auto month_by_name = scn::scan<scn::month>("Aug", "{:%b}");
    ASSERT_THAT(month_by_name, Succeeded());
    EXPECT_EQ(static_cast<unsigned>(month_by_name->value()), 8);
}

TEST(ChronoScanTest, MultiCodeUnitLiteral)
{
    auto result = scn::scan<std::tm>("2024年05月", "{:%Y年%m月}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
    EXPECT_EQ(result->value().tm_mon, 5 - 1);

    result = scn::scan<std::tm>("2024年05", scn::runtime_format("{:%Y年%m}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_mon, 5 - 1);

    result = scn::scan<std::tm>("2024年", scn::runtime_format("{:%Y年}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_year, 2024 - 1900);
}

TEST(ChronoScanTest, TwelveHourTime)
{
    auto result = scn::scan<std::tm>("01:02:03 PM", "{:%r}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tm_hour, 13);
    EXPECT_EQ(result->value().tm_min, 2);
    EXPECT_EQ(result->value().tm_sec, 3);
}

TEST(ChronoScanTest, LocaleDateAndTimeSpecifier)
{
    auto result = scn::scan<scn::datetime_components>(
        "Tue Nov 15 08:12:31 1994", "{:%c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().wday, scn::Tuesday);
    EXPECT_EQ(result->value().mon, scn::November);
    EXPECT_EQ(result->value().mday, 15);
    EXPECT_EQ(result->value().hour, 8);
    EXPECT_EQ(result->value().year, 1994);
}

TEST(ChronoScanTest, LocaleDateSpecifier)
{
    auto result = scn::scan<scn::datetime_components>("11/15/94", "{:%x}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().mon, scn::November);
    EXPECT_EQ(result->value().mday, 15);
    EXPECT_EQ(result->value().year, 1994);
}

TEST(ChronoScanTest, AlternativeIsoWeekday)
{
    auto result = scn::scan<scn::datetime_components>("5", "{:%Ou}");
    ASSERT_THAT(result, Succeeded());
    ASSERT_TRUE(result->value().wday.has_value());
    EXPECT_EQ(*result->value().wday, scn::Friday);

    result = scn::scan<scn::datetime_components>("7", "{:%Ou}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*result->value().wday, scn::Sunday);

    result = scn::scan<scn::datetime_components>("0", "{:%Ou}");
    ASSERT_THAT(result, Failed());
}

TEST(ChronoScanTest, UnimplementedSpecifiersAreNamed)
{
    const auto check = [](const char* format, const char* message) {
        auto result = scn::scan<scn::datetime_components>(
            "5", scn::runtime_format(format));
        ASSERT_THAT(result, Failed());
        EXPECT_EQ(result.error().code(),
                  scn::scan_error::invalid_format_string);
        EXPECT_STREQ(result.error().msg(), message);
    };
    check("{:%W}", "Unimplemented: %W");
    check("{:%U}", "Unimplemented: %U");
    check("{:%V}", "Unimplemented: %V");
    check("{:%OW}", "Unimplemented: %W");
    check("{:%OU}", "Unimplemented: %U");
    check("{:%OV}", "Unimplemented: %V");
    check("{:%G}", "Unimplemented: %G");
    check("{:%g}", "Unimplemented: %g");
    check("{:%Ey}", "Unimplemented: %Ey");
    check("{:%s}", "Unimplemented: %s");
    check("{:%Q}", "Unimplemented: %Q");
    check("{:%q}", "Unimplemented: %q");
}

TEST(ChronoScanTest, TimePointUnsupportedSpecifiers)
{
    auto day_of_year =
        scn::scan<sys_seconds>("2024 100", scn::runtime_format("{:%Y %j}"));
    ASSERT_THAT(day_of_year, Failed());
    EXPECT_EQ(day_of_year.error().code(),
              scn::scan_error::invalid_format_string);
    EXPECT_STREQ(day_of_year.error().msg(),
                 "Day-of-year (%j) not supported with time_point");

    auto tz_name = scn::scan<sys_seconds>("UTC", scn::runtime_format("{:%Z}"));
    ASSERT_THAT(tz_name, Failed());
    EXPECT_EQ(tz_name.error().code(), scn::scan_error::invalid_format_string);
    EXPECT_STREQ(tz_name.error().msg(),
                 "Timezone names (%Z) not supported with time_point");
}

TEST(ChronoScanTest, TimezoneOffsetRange)
{
    EXPECT_THAT(scn::scan<scn::tm_with_tz>("+0260", "{:%z}"),
                FailedWith(scn::scan_error::invalid_scanned_value));
    EXPECT_THAT(scn::scan<scn::tm_with_tz>("+02:60", "{:%z}"),
                FailedWith(scn::scan_error::invalid_scanned_value));
    EXPECT_THAT(scn::scan<scn::tm_with_tz>("-2:99", "{:%Ez}"),
                FailedWith(scn::scan_error::invalid_scanned_value));
    EXPECT_THAT(scn::scan<scn::tm_with_tz>("+2:60", "{:%Oz}"),
                FailedWith(scn::scan_error::invalid_scanned_value));
    EXPECT_THAT(scn::scan<scn::datetime_components>("+0260", "{:%z}"),
                FailedWith(scn::scan_error::invalid_scanned_value));

    auto result = scn::scan<scn::tm_with_tz>("+0259", "{:%z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{2 * 60 + 59});

    result = scn::scan<scn::tm_with_tz>("-02", "{:%z}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{-2 * 60});

    result = scn::scan<scn::tm_with_tz>("+02:30", "{:%Oz}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().tz_offset, std::chrono::minutes{2 * 60 + 30});
}

TEST(ChronoScanTest, Fuzz1)
{
    auto result = scn::scan<std::tm>("08/08/22", "{:%D}");
    ASSERT_THAT(result, Succeeded());

    result = scn::scan<std::tm>("", "{:%D}");
    ASSERT_THAT(result, Failed());

    auto str = std::string_view{""};
    result = scn::scan<std::tm>(
        scn::ranges::subrange{str.data(), str.data() + str.size()}, "{:%D}");
    ASSERT_THAT(result, Failed());
}

}  // namespace

#endif  // !SCN_DISABLE_CHRONO
