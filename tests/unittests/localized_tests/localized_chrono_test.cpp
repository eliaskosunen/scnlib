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

#include <scn/scan.h>

#if !SCN_DISABLE_LOCALE && !SCN_DISABLE_CHRONO

#include <scn/chrono.h>

namespace {

std::optional<std::locale> make_locale(const std::string& name)
{
    try {
        return std::locale(name);
    }
    catch (...) {
        return std::nullopt;
    }
}

std::locale make_one_of_locale(const std::vector<std::string>& names)
{
    for (const auto& name : names) {
        if (auto l = make_locale(name)) {
            return *l;
        }
    }
    std::fprintf(stderr, "%s locale required for scn_localized_tests",
                 names.front().c_str());
    std::fflush(stderr);
    std::abort();
}

const auto en_locale =
    make_one_of_locale({"en_US.UTF-8", "en_US.utf8", "en-US.UTF-8",
                        "en-US.utf8", "en_US", "en-US"});

const auto fi_locale =
    make_one_of_locale({"fi_FI.UTF-8", "fi_FI.utf8", "fi-FI.UTF-8",
                        "fi-FI.utf8", "fi_FI", "fi-FI"});

TEST(LocalizedChronoTest, Date)
{
    auto result =
        scn::scan<std::tm>(std::locale::classic(), "10/17/2020", "{:L%x}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2020 - 1900);
    EXPECT_EQ(result->value().tm_mon, 10 - 1);
    EXPECT_EQ(result->value().tm_mday, 17);

    result =
        scn::scan<std::tm>(std::locale::classic(), "10/17/2020", "{:L%Ex}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2020 - 1900);
    EXPECT_EQ(result->value().tm_mon, 10 - 1);
    EXPECT_EQ(result->value().tm_mday, 17);

#if !SCN_STDLIB_LIBCPP
    result = scn::scan<std::tm>(fi_locale, "17.10.2020", "{:L%x}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2020 - 1900);
    EXPECT_EQ(result->value().tm_mon, 10 - 1);
    EXPECT_EQ(result->value().tm_mday, 17);

    result = scn::scan<std::tm>(fi_locale, "17.10.2020", "{:L%Ex}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2020 - 1900);
    EXPECT_EQ(result->value().tm_mon, 10 - 1);
    EXPECT_EQ(result->value().tm_mday, 17);
#endif
}

TEST(LocalizedChronoTest, Time)
{
    auto result =
        scn::scan<std::tm>(std::locale::classic(), "04:41:13", "{:L%X}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 4);
    EXPECT_EQ(result->value().tm_min, 41);
    EXPECT_EQ(result->value().tm_sec, 13);

    result = scn::scan<std::tm>(std::locale::classic(), "04:41:13", "{:L%EX}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 4);
    EXPECT_EQ(result->value().tm_min, 41);
    EXPECT_EQ(result->value().tm_sec, 13);

    constexpr auto& source =
#if SCN_STDLIB_LIBCPP
        "04:41:13"
#else
        "04.41.13"
#endif
        ;

    result = scn::scan<std::tm>(fi_locale, source, "{:L%X}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 4);
    EXPECT_EQ(result->value().tm_min, 41);
    EXPECT_EQ(result->value().tm_sec, 13);

    result = scn::scan<std::tm>(fi_locale, source, "{:L%EX}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 4);
    EXPECT_EQ(result->value().tm_min, 41);
    EXPECT_EQ(result->value().tm_sec, 13);
}

TEST(LocalizedChronoTest, Time12Hour)
{
    // libstdc++ supports %r starting from v12
#if SCN_STDLIB_LIBCPP || SCN_STDLIB_GLIBCXX >= 12
    auto result =
        scn::scan<std::tm>(std::locale::classic(), "04:41:13 PM", "{:L%r}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 16);
    EXPECT_EQ(result->value().tm_min, 41);
    EXPECT_EQ(result->value().tm_sec, 13);

    result = scn::scan<std::tm>(std::locale::classic(), "04:41:13 PM",
                                scn::runtime_format("{:L%Er}"));
    EXPECT_FALSE(result);
#endif

    // %r doesn't have a definition in fi_FI.UTF-8 in Linux
#if SCN_APPLE
    result = scn::scan<std::tm>(fi_locale, "04:41:13 pm", "{:L%r}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_hour, 16);
    EXPECT_EQ(result->value().tm_min, 41);
    EXPECT_EQ(result->value().tm_sec, 13);

    result = scn::scan<std::tm>(fi_locale, "04:41:13 pm",
                                scn::runtime_format("{:L%Er}"));
    EXPECT_FALSE(result);
#endif
}

TEST(LocalizedChronoTest, MonthName)
{
    auto result =
        scn::scan<std::tm>(std::locale::classic(), "February", "{:L%b}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 1);

    result = scn::scan<std::tm>(std::locale::classic(), "February",
                                scn::runtime_format("{:L%Ob}"));
    EXPECT_FALSE(result);

#if !SCN_STDLIB_LIBCPP
    result = scn::scan<std::tm>(fi_locale, "helmikuu", "{:L%b}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 1);

    result = scn::scan<std::tm>(fi_locale, "helmikuu",
                                scn::runtime_format("{:L%Ob}"));
    EXPECT_FALSE(result);
#endif
}

TEST(LocalizedChronoTest, MonthDec)
{
    auto result = scn::scan<std::tm>(std::locale::classic(), "2", "{:L%m}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 1);

// libstdc++ pre-v12 requires two digits for %Om
#if !SCN_STDLIB_LIBCPP && SCN_STDLIB_GLIBCXX >= 12
    result = scn::scan<std::tm>(std::locale::classic(), "2", "{:L%Om}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 1);
#endif

    result = scn::scan<std::tm>(fi_locale, "2", "{:L%m}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 1);

#if !SCN_STDLIB_LIBCPP && SCN_STDLIB_GLIBCXX >= 12
    result = scn::scan<std::tm>(fi_locale, "2", "{:L%Om}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mon, 1);
#endif
}

TEST(LocalizedChronoTest, WeekdayName)
{
    auto result =
        scn::scan<std::tm>(std::locale::classic(), "Monday", "{:L%a}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 1);

    result = scn::scan<std::tm>(std::locale::classic(), "Monday",
                                scn::runtime_format("{:L%Oa}"));
    EXPECT_FALSE(result);

#if !SCN_STDLIB_LIBCPP
    result = scn::scan<std::tm>(fi_locale, "maanantai", "{:L%a}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 1);

    result = scn::scan<std::tm>(fi_locale, "maanantai",
                                scn::runtime_format("{:L%Oa}"));
    EXPECT_FALSE(result);
#endif
}

TEST(LocalizedChronoTest, WeekdayDec)
{
    auto result = scn::scan<std::tm>(std::locale::classic(), "1", "{:L%w}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 1);

// libstdc++ pre-v12 requires two digits for %Ow
#if !SCN_STDLIB_LIBCPP && SCN_STDLIB_GLIBCXX >= 12
    result = scn::scan<std::tm>(std::locale::classic(), "1", "{:L%Ow}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 1);
#endif

    result = scn::scan<std::tm>(fi_locale, "1", "{:L%w}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 1);

#if !SCN_STDLIB_LIBCPP && SCN_STDLIB_GLIBCXX >= 12
    result = scn::scan<std::tm>(fi_locale, "1", "{:L%Ow}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_wday, 1);
#endif
}

TEST(LocalizedChronoTest, MonthDayDec)
{
    auto result = scn::scan<std::tm>(std::locale::classic(), "1", "{:L%d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mday, 1);

// libstdc++ pre-v12 requires two digits for %Od
#if !SCN_STDLIB_LIBCPP && SCN_STDLIB_GLIBCXX >= 12
    result = scn::scan<std::tm>(std::locale::classic(), "1", "{:L%Od}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mday, 1);
#endif

    result = scn::scan<std::tm>(fi_locale, "1", "{:L%d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mday, 1);

#if !SCN_STDLIB_LIBCPP && SCN_STDLIB_GLIBCXX >= 12
    result = scn::scan<std::tm>(fi_locale, "1", "{:L%Od}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_mday, 1);
#endif
}

#if !SCN_STDLIB_GLIBCXX
// "%c" doesn't work in libstdc++
TEST(LocalizedChronoTest, Datetime)
{
    auto result = scn::scan<std::tm>(std::locale::classic(),
                                     "Sun Oct 17 04:41:13 2020", "{:L%c}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2020 - 1900);
    EXPECT_EQ(result->value().tm_mon, 10 - 1);
    EXPECT_EQ(result->value().tm_mday, 17);
    EXPECT_EQ(result->value().tm_wday, 0);
    EXPECT_EQ(result->value().tm_hour, 4);
    EXPECT_EQ(result->value().tm_min, 41);
    EXPECT_EQ(result->value().tm_sec, 13);

    result = scn::scan<std::tm>(std::locale::classic(),
                                "Sun Oct 17 04:41:13 2020", "{:L%Ec}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2020 - 1900);
    EXPECT_EQ(result->value().tm_mon, 10 - 1);
    EXPECT_EQ(result->value().tm_mday, 17);
    EXPECT_EQ(result->value().tm_wday, 0);
    EXPECT_EQ(result->value().tm_hour, 4);
    EXPECT_EQ(result->value().tm_min, 41);
    EXPECT_EQ(result->value().tm_sec, 13);

    // "%c" doesn't work with non-"C" locales in libc++, either
#if 0
    result = scn::scan<std::tm>(fi_locale, "sunnuntai 17 lokakuu 04:41:13 2020",
                                "{:L%c}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2020 - 1900);
    EXPECT_EQ(result->value().tm_mon, 10 - 1);
    EXPECT_EQ(result->value().tm_mday, 17);
    EXPECT_EQ(result->value().tm_wday, 0);
    EXPECT_EQ(result->value().tm_hour, 4);
    EXPECT_EQ(result->value().tm_min, 41);
    EXPECT_EQ(result->value().tm_sec, 13);

    result = scn::scan<std::tm>(fi_locale, "sunnuntai 17 lokakuu 04:41:13 2020",
                                "{:L%Ec}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().tm_year, 2020 - 1900);
    EXPECT_EQ(result->value().tm_mon, 10 - 1);
    EXPECT_EQ(result->value().tm_mday, 17);
    EXPECT_EQ(result->value().tm_wday, 0);
    EXPECT_EQ(result->value().tm_hour, 4);
    EXPECT_EQ(result->value().tm_min, 41);
    EXPECT_EQ(result->value().tm_sec, 13);
#endif
}
#endif

}  // namespace

#endif  // !SCN_DISABLE_LOCALE && !SCN_DISABLE_CHRONO
