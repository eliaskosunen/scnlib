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

#pragma once

#include <scn/scan.h>

#if defined(SCN_MODULE) && defined(SCN_IMPORT_STD)
import std;
#include <time.h>
#else
#include <chrono>
#include <cmath>
#include <ctime>
#include <ratio>
#endif

#if !SCN_DISABLE_CHRONO

namespace scn {
SCN_BEGIN_NAMESPACE

/**
 * \defgroup chrono Dates and times
 *
 * \brief Scanning of `<chrono>` and `<ctime>` types.
 *
 * In header `<scn/chrono.h>`.
 * Not available if `SCN_DISABLE_CHRONO` is `1`.
 *
 * The following types can be scanned:
 *
 *  * `std::tm`
 *  * `scn::tm_with_tz`: a `std::tm`, with a timezone offset and name
 *  * `scn::datetime_components`
 *  * `std::chrono::time_point<std::chrono::system_clock, Duration>`
 *  * `scn::weekday`, `scn::day`, `scn::month`, `scn::year`,
 *    `scn::month_day`, `scn::year_month`, and `scn::year_month_day`.
 *    These are aliases to the types with the same names in `std::chrono`,
 *    if the standard library supports them (`__cpp_lib_chrono >= 201907L`),
 *    and minimal fallback types otherwise.
 *
 * \section chrono-spec Format specifiers
 *
 * The format specifier of a chrono type is required, and its syntax is
 * similar to `strftime` and `std::format`:
 *
 * \code
 * chrono-format-spec  ::= ['L'] conversion-spec [chrono-specs]
 * chrono-specs        ::= (conversion-spec | literal-char)...
 * conversion-spec     ::= '%' [modifier] type | '%' '.' [modifier] 'S'
 * modifier            ::= 'E' | 'O'
 * \endcode
 *
 * Preceding whitespace is skipped before the first field.
 * A whitespace character in the format specifier skips any amount of
 * whitespace (including none) in the input.
 * Any other literal character must match exactly.
 *
 * Numeric fields accept between one and the maximum number of digits,
 * unless otherwise noted.
 * Field values are range-checked (e.g. `%%H` accepts `0` to `23`).
 * When scanning a `month_day`, `year_month_day`, or `time_point`, the day is
 * also checked to exist in its month (e.g. February 30th is an error),
 * like `std::chrono::parse` does.
 * When the year, month, and day are all scanned into a `year_month_day` or
 * a `time_point`, a scanned weekday must match the date, or it's an error.
 * With other types, the weekday is not checked.
 * `std::tm`, `tm_with_tz`, and `datetime_components` only get the
 * field-level checks.
 * A field can't be set twice.
 *
 * <table>
 * <caption id="chrono-spec-table">
 * Conversion specifiers
 * </caption>
 * <tr><th>Specifier</th> <th>Meaning</th></tr>
 * <tr><td>`%%`</td> <td>A literal `%`</td></tr>
 * <tr><td>`%%n`, `%%t`</td> <td>Whitespace, like a space</td></tr>
 * <tr><td>`%%Y`</td> <td>Year, exactly four digits</td></tr>
 * <tr><td>`%%y`</td> <td>Year within a century, two digits.
 * Without `%%C`, `69`-`99` means 1969-1999, and `00`-`68` means 2000-2068.
 * </td></tr>
 * <tr><td>`%%C`</td> <td>Century, two digits.
 * Without `%%y`, the year is the first year of the century.</td></tr>
 * <tr><td>`%%b`, `%%B`, `%%h`</td> <td>Month name, full or abbreviated,
 * case-insensitive</td></tr>
 * <tr><td>`%%m`</td> <td>Month, `1`-`12`</td></tr>
 * <tr><td>`%%d`, `%%e`</td> <td>Day of the month, `1`-`31`</td></tr>
 * <tr><td>`%%j`</td> <td>Day of the year, `1`-`366`</td></tr>
 * <tr><td>`%%a`, `%%A`</td> <td>Weekday name, full or abbreviated,
 * case-insensitive</td></tr>
 * <tr><td>`%%w`</td> <td>Weekday, `0`-`6`, Sunday is `0`</td></tr>
 * <tr><td>`%%u`</td> <td>ISO weekday, `1`-`7`, Monday is `1`</td></tr>
 * <tr><td>`%%H`, `%%k`</td> <td>Hour (24-hour clock), `0`-`23`</td></tr>
 * <tr><td>`%%I`, `%%l`</td> <td>Hour (12-hour clock), `1`-`12`</td></tr>
 * <tr><td>`%%p`, `%%P`</td> <td>`AM`, `PM`, `A.M.`, or `P.M.`,
 * case-insensitive. Requires `%%I`.</td></tr>
 * <tr><td>`%%M`</td> <td>Minute, `0`-`59`</td></tr>
 * <tr><td>`%%S`</td> <td>Second, `0`-`60`.
 * With a `time_point` more precise than seconds, also reads an optional
 * fractional part, like `%.S`, and like `std::chrono::parse` does.</td></tr>
 * <tr><td>`%.S`</td> <td>Second, with an optional fractional part
 * (like `05` or `05.25`).
 * Digits beyond nanoseconds, or beyond the precision of a `time_point`,
 * are truncated.
 * Not supported by `std::tm` and `scn::tm_with_tz`.</td></tr>
 * <tr><td>`%%z`</td> <td>Timezone offset from UTC,
 * `[+|-]hh[[:]mm]`. `%%Ez` and `%%Oz` accept `[+|-]h[h][:mm]`.
 * With `std::tm`, requires `tm_gmtoff` to be available.</td></tr>
 * <tr><td>`%%Z`</td> <td>Timezone name, consisting of ASCII letters,
 * digits, and `-+_/`. Not supported by `std::tm`.
 * Can't be used together with `%%z`.</td></tr>
 * <tr><td>`%%D`</td> <td>Equivalent to `%%m/%%d/%%y`</td></tr>
 * <tr><td>`%%F`</td> <td>Equivalent to `%%Y-%%m-%%d`</td></tr>
 * <tr><td>`%%R`</td> <td>Equivalent to `%%H:%%M`</td></tr>
 * <tr><td>`%%T`</td> <td>Equivalent to `%%H:%%M:%%S`</td></tr>
 * <tr><td>`%%c`</td> <td>Date and time, `%%a %%b %%d %%H:%%M:%%S %%Y`
 * (localized with `L`)</td></tr>
 * <tr><td>`%%x`</td> <td>Date, `%%m/%%d/%%y` (localized with `L`)</td></tr>
 * <tr><td>`%%X`</td> <td>Time, `%%H:%%M:%%S` (localized with `L`)</td></tr>
 * <tr><td>`%%r`</td> <td>12-hour time, `%%I:%%M:%%S %%p`
 * (localized with `L`)</td></tr>
 * </table>
 *
 * `%%G`, `%%g`, `%%U`, `%%W`, `%%V`, `%%s`, `%%Q`, `%%q`, and `%%Ey` are
 * recognized, but not implemented, and cause an error.
 *
 * The `L` flag makes the names (`%%a`, `%%b`), and the composite specifiers
 * `%%c`, `%%x`, `%%X`, and `%%r` follow the locale passed to `scn::scan`.
 * Together with `E` or `O` (e.g. `%.ES`), it also makes the decimal
 * separator of a fractional second follow the locale.
 * The `E` and `O` modifiers select the locale's alternative representation,
 * and, except for `%%Ez` and `%%Oz`, only have an effect together with `L`.
 *
 * \code{.cpp}
 * auto result = scn::scan<std::tm>("2024-03-04 12:30", "{:%F %R}");
 * // result->value().tm_year == 124
 *
 * auto date = scn::scan<scn::year_month_day>("04.03.2024", "{:%d.%m.%Y}");
 * \endcode
 *
 * The types `scn::weekday`, `scn::day`, `scn::month`, etc. require the
 * corresponding fields to be present in the format string.
 */

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L

using weekday = std::chrono::weekday;
using day = std::chrono::day;
using month = std::chrono::month;
using year = std::chrono::year;

using month_day = std::chrono::month_day;
using year_month = std::chrono::year_month;
using year_month_day = std::chrono::year_month_day;

using std::chrono::Friday;
using std::chrono::Monday;
using std::chrono::Saturday;
using std::chrono::Sunday;
using std::chrono::Thursday;
using std::chrono::Tuesday;
using std::chrono::Wednesday;

using std::chrono::April;
using std::chrono::August;
using std::chrono::December;
using std::chrono::February;
using std::chrono::January;
using std::chrono::July;
using std::chrono::June;
using std::chrono::March;
using std::chrono::May;
using std::chrono::November;
using std::chrono::October;
using std::chrono::September;

#else

// Fallbacks

struct weekday {
    weekday() = default;
    constexpr explicit weekday(unsigned wd) noexcept
        : m_value(static_cast<unsigned char>(wd != 7 ? wd : 0))
    {
    }
    constexpr unsigned c_encoding() const noexcept
    {
        return m_value;
    }

    friend constexpr bool operator==(const weekday& lhs, const weekday& rhs)
    {
        return lhs.m_value == rhs.m_value;
    }

private:
    unsigned char m_value;
};

inline constexpr weekday Sunday{0};
inline constexpr weekday Monday{1};
inline constexpr weekday Tuesday{2};
inline constexpr weekday Wednesday{3};
inline constexpr weekday Thursday{4};
inline constexpr weekday Friday{5};
inline constexpr weekday Saturday{6};

struct day {
    day() = default;
    constexpr explicit day(unsigned d) noexcept
        : m_value(static_cast<unsigned char>(d))
    {
    }
    constexpr explicit operator unsigned() const noexcept
    {
        return m_value;
    }

    friend constexpr bool operator==(const day& lhs, const day& rhs)
    {
        return lhs.m_value == rhs.m_value;
    }

private:
    unsigned char m_value;
};

struct month {
    month() = default;
    constexpr explicit month(unsigned d) noexcept
        : m_value(static_cast<unsigned char>(d))
    {
    }
    constexpr explicit operator unsigned() const noexcept
    {
        return m_value;
    }

    friend constexpr bool operator==(const month& lhs, const month& rhs)
    {
        return lhs.m_value == rhs.m_value;
    }

private:
    unsigned char m_value;
};

inline constexpr month January{1};
inline constexpr month February{2};
inline constexpr month March{3};
inline constexpr month April{4};
inline constexpr month May{5};
inline constexpr month June{6};
inline constexpr month July{7};
inline constexpr month August{8};
inline constexpr month September{9};
inline constexpr month October{10};
inline constexpr month November{11};
inline constexpr month December{12};

struct year {
    year() = default;
    constexpr explicit year(int d) noexcept : m_value(d) {}
    constexpr explicit operator int() const noexcept
    {
        return m_value;
    }

    friend constexpr bool operator==(const year& lhs, const year& rhs)
    {
        return lhs.m_value == rhs.m_value;
    }

private:
    int m_value;
};

struct month_day {
    month_day() = default;
    constexpr month_day(const scn::month& m, const scn::day& d) noexcept
        : m_month(m), m_day(d)
    {
    }
    constexpr scn::month month() const noexcept
    {
        return m_month;
    }
    constexpr scn::day day() const noexcept
    {
        return m_day;
    }

    friend constexpr bool operator==(const month_day& lhs, const month_day& rhs)
    {
        return lhs.m_month == rhs.m_month && lhs.m_day == rhs.m_day;
    }

private:
    scn::month m_month;
    scn::day m_day;
};

struct year_month {
    year_month() = default;
    constexpr year_month(const scn::year& y, const scn::month& m) noexcept
        : m_year(y), m_month(m)
    {
    }
    constexpr scn::year year() const noexcept
    {
        return m_year;
    }
    constexpr scn::month month() const noexcept
    {
        return m_month;
    }

    friend constexpr bool operator==(const year_month& lhs,
                                     const year_month& rhs)
    {
        return lhs.m_year == rhs.m_year && lhs.m_month == rhs.m_month;
    }

private:
    scn::year m_year;
    scn::month m_month;
};

struct year_month_day {
    year_month_day() = default;
    constexpr year_month_day(const scn::year& y,
                             const scn::month& m,
                             const scn::day& d) noexcept
        : m_year(y), m_month(m), m_day(d)
    {
    }
    constexpr scn::year year() const noexcept
    {
        return m_year;
    }
    constexpr scn::month month() const noexcept
    {
        return m_month;
    }
    constexpr scn::day day() const noexcept
    {
        return m_day;
    }

    friend constexpr bool operator==(const year_month_day& lhs,
                                     const year_month_day& rhs)
    {
        return lhs.m_year == rhs.m_year && lhs.m_month == rhs.m_month &&
               lhs.m_day == rhs.m_day;
    }

private:
    scn::year m_year;
    scn::month m_month;
    scn::day m_day;
};

#endif  // __cpp_lib_chrono >= 201907

namespace detail {
template <typename T>
using tm_gmtoff = decltype(T::tm_gmtoff);

template <typename T>
void assign_gmtoff(T& tm, std::chrono::seconds val)
{
    static_assert(std::is_same_v<T, std::tm>);
    if constexpr (mp_valid<tm_gmtoff, T>::value) {
        tm.tm_gmtoff = static_cast<tm_gmtoff<T>>(val.count());
    }
    else {
        static_assert(dependent_false<T>::value,
                      "std::tm has no tm_gmtoff: check before calling");
    }
}
}  // namespace detail

/**
 * An alternative to `std::tm`,
 * with support for subsecond precision, and
 * clear distinction between the value `0` and an unset field.
 *
 * \ingroup chrono
 */
struct datetime_components {
    /// Fractions of a second, [0.0-1.0)
    std::optional<double> subsec;
    /// Seconds, [0-60]
    std::optional<signed char> sec;
    /// Minutes, [0-59]
    std::optional<signed char> min;
    /// Hours, [0-23]
    std::optional<signed char> hour;
    /// Day (in a month), [1-31]
    std::optional<signed char> mday;
    /// Month (strongly typed)
    std::optional<month> mon;
    /// Year, without an offset
    /// Note, `std::tm` stores years since 1900
    std::optional<int> year;
    /// Day of week (strongly typed)
    std::optional<weekday> wday;
    /// Day of year (offset from Jan 1st), [0-365]
    std::optional<short> yday;
    /// Timezone offset from UTC
    std::optional<std::chrono::minutes> tz_offset;
    /// Timezone name
    std::optional<std::string> tz_name;

    /**
     * Returns a `std::tm` object corresponding to `*this`.
     * Unset fields have a value of `0`, with `tm_isdst` having a value of `-1`.
     * The fields `subsec` and `tz_name` are discarded.
     * `tz_offset` is set to `tm_gmtoff`, if it's available.
     *
     * \pre `year`, if set, must be at least `INT_MIN + 1900`, so that
     * `tm_year` doesn't overflow. Scanned values always satisfy this.
     */
    SCN_NODISCARD std::tm to_tm() const
    {
        SCN_UNUSED(subsec);
        std::tm t{};
        t.tm_sec = static_cast<int>(sec.value_or(0));
        t.tm_min = static_cast<int>(min.value_or(0));
        t.tm_hour = static_cast<int>(hour.value_or(0));
        t.tm_mday = static_cast<int>(mday.value_or(0));
        t.tm_mon =
            static_cast<int>(static_cast<unsigned>(mon.value_or(January)) - 1);
        t.tm_year = year.value_or(1900) - 1900;
        t.tm_wday = static_cast<int>(wday.value_or(Sunday).c_encoding());
        t.tm_yday = static_cast<int>(yday.value_or(0));
        t.tm_isdst = -1;
        if constexpr (detail::mp_valid<detail::tm_gmtoff, std::tm>::value) {
            detail::assign_gmtoff(
                t, std::chrono::duration_cast<std::chrono::seconds>(
                       tz_offset.value_or(std::chrono::minutes{0})));
        }
        return t;
    }
};

/**
 * A `std::tm`, with a timezone offset (`%%z`) and name (`%%Z`).
 *
 * \ingroup chrono
 */
struct tm_with_tz : public std::tm {
    std::optional<std::chrono::minutes> tz_offset;
    std::optional<std::string> tz_name;
};

namespace detail {
enum class numeric_system {
    standard,
    alternative_e,  // 'E'
    alternative_o,  // 'O'
};

// Whether `%S` also reads a fractional part, like `std::chrono::parse` does
// when the scanned type is more precise than seconds
enum class seconds_format {
    integer,
    fractional,
};

// Days since 1970-01-01 in the proleptic Gregorian calendar, see
// http://howardhinnant.github.io/date_algorithms.html#days_from_civil
constexpr std::int64_t days_from_civil(std::int64_t y, unsigned m, unsigned d)
{
    y -= m <= 2 ? 1 : 0;
    const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
    const auto year_of_era = static_cast<unsigned>(y - era * 400);
    const unsigned day_of_year =
        (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned day_of_era =
        year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
    return era * 146097 + static_cast<std::int64_t>(day_of_era) - 719468;
}

// Whether day `d` exists in month `m` (in [1, 12]) of year `y`.
// Without a year, February 29th is valid, like in `std::chrono::month_day`.
constexpr bool is_valid_day_of_month(std::optional<int> y,
                                     unsigned m,
                                     unsigned d)
{
    constexpr unsigned char days_in_month[] = {31, 29, 31, 30, 31, 30,
                                               31, 31, 30, 31, 30, 31};
    if (m == 2 && d == 29 && y) {
        return (*y % 4 == 0 && *y % 100 != 0) || *y % 400 == 0;
    }
    return d <= days_in_month[m - 1];
}

// Day of the week (Sunday is 0) of the day `days` days after 1970-01-01
constexpr unsigned weekday_from_days(std::int64_t days)
{
    // 1970-01-01 was a Thursday
    return static_cast<unsigned>((days % 7 + 11) % 7);
}

// Whether the scanned weekday, if any, agrees with the scanned date.
// Only checked if the whole date was scanned.
inline bool is_weekday_consistent(const datetime_components& dt)
{
    if (!dt.wday || !dt.year || !dt.mon || !dt.mday) {
        return true;
    }
    const auto days = days_from_civil(*dt.year, static_cast<unsigned>(*dt.mon),
                                      static_cast<unsigned>(*dt.mday));
    return dt.wday->c_encoding() == weekday_from_days(days);
}

// `sec` seconds and `ns` nanoseconds (non-negative) as a `Duration`,
// rounded down.
// Unlike `std::chrono::floor`, fails instead of overflowing,
// e.g. with `nanoseconds` beyond the year 2262.
template <typename Duration>
scan_expected<Duration> floor_to_duration(std::int64_t sec, std::int64_t ns)
{
    using limits = std::numeric_limits<std::int64_t>;
    using ticks_per_second =
        std::ratio_divide<std::ratio<1>, typename Duration::period>;
    // At most `std::nano::den`, since `Duration` is at most as precise as
    // nanoseconds
    constexpr std::int64_t num = ticks_per_second::num;
    constexpr std::int64_t den = ticks_per_second::den;
    constexpr std::int64_t ns_per_sec = std::nano::den;
    // Keeps `fraction_ticks` below from overflowing
    static_assert(den <=
                  (limits::max() - ns_per_sec * ns_per_sec) / ns_per_sec);

    const auto overflow = [](bool negative) {
        return unexpected_scan_error(negative
                                         ? scan_error::value_negative_overflow
                                         : scan_error::value_positive_overflow,
                                     "time_point out of range");
    };

    sec += ns / ns_per_sec;
    ns %= ns_per_sec;
    if (sec > limits::max() / num) {
        return overflow(false);
    }
    if (sec < limits::min() / num) {
        // `limits::min() / num` is rounded toward zero, so the second below
        // it can still contain representable instants, e.g.
        // `nanoseconds::min()`. `sec * num` would overflow, so count
        // from the next second, and subtract the rest of this one.
        if (sec + 1 != limits::min() / num) {
            return overflow(true);
        }
        ++sec;
        ns -= ns_per_sec;
    }

    // ticks = floor((sec + ns / ns_per_sec) * num / den),
    // with the division done in two parts, so that nothing overflows.
    // `ns` is negative only after the adjustment above.
    const auto scaled_sec = sec * num;
    auto ticks = scaled_sec / den;
    auto remainder = scaled_sec % den;
    if (remainder < 0) {
        --ticks;
        remainder += den;
    }
    const auto fraction_numerator = remainder * ns_per_sec + ns * num;
    auto fraction_ticks = fraction_numerator / (den * ns_per_sec);
    if (fraction_numerator % (den * ns_per_sec) < 0) {
        --fraction_ticks;
    }
    if (fraction_ticks >= 0) {
        if (ticks > limits::max() - fraction_ticks) {
            return overflow(false);
        }
    }
    else if (ticks < limits::min() - fraction_ticks) {
        return overflow(true);
    }
    ticks += fraction_ticks;

    using rep = typename Duration::rep;
    if constexpr (std::is_unsigned_v<rep>) {
        if (ticks < 0) {
            return overflow(true);
        }
    }
    else if constexpr (sizeof(rep) < sizeof(std::int64_t)) {
        if (ticks <
            static_cast<std::int64_t>(std::numeric_limits<rep>::min())) {
            return overflow(true);
        }
    }
    if constexpr (sizeof(rep) < sizeof(std::int64_t)) {
        if (ticks >
            static_cast<std::int64_t>(std::numeric_limits<rep>::max())) {
            return overflow(false);
        }
    }
    return Duration{static_cast<rep>(ticks)};
}

// The fields of `dt` are in UTC, offset by `tz_offset`, if it's set.
// Unset fields default to 1970-01-01 00:00:00.
template <typename Duration>
scan_expected<Duration> time_since_unix_epoch(const datetime_components& dt)
{
    static_assert(std::is_integral_v<typename Duration::rep>);
    static_assert(Duration::period::type::den <= std::nano::den);

    // Rejected by the format string checker
    SCN_EXPECT(!dt.tz_name && !dt.yday);

    const auto days = days_from_civil(
        dt.year.value_or(1970), static_cast<unsigned>(dt.mon.value_or(January)),
        static_cast<unsigned>(dt.mday.value_or(1)));
    auto time =
        std::chrono::seconds{days * 86400 + dt.hour.value_or(0) * 3600 +
                             dt.min.value_or(0) * 60 + dt.sec.value_or(0)};
    if (dt.tz_offset) {
        time -= *dt.tz_offset;
    }

    // A fraction doesn't affect the result if `Duration` is seconds or larger
    std::int64_t subsec_in_ns = 0;
    if (dt.subsec) {
        // Rounded, because truncating would turn e.g. 0.000065 into
        // 64999 ns, and then into 64 us
        subsec_in_ns =
            std::llround(*dt.subsec * static_cast<double>(std::nano::den));
    }
    return floor_to_duration<Duration>(time.count(), subsec_in_ns);
}

template <typename CharT, typename Handler>
constexpr const CharT* parse_chrono_format_specs(const CharT* begin,
                                                 const CharT* end,
                                                 Handler&& handler)
{
    if (begin == end || *begin == CharT{'}'}) {
        handler.on_error("chrono format specs can't be empty");
        return begin;
    }

    auto p = begin;
    if (*p == CharT{'L'}) {
        handler.on_localized();
        begin = ++p;
        if (p == end) {
            handler.on_error("chrono format specs can't be empty");
            return p;
        }
    }

    if (p == end || *p != CharT{'%'}) {
        handler.on_error(
            "chrono format spec must start with a conversion specifier (%...)");
        return p;
    }

    while (p != end) {
        auto ch = *p;
        if (ch == CharT{'}'}) {
            break;
        }
        if (ch != CharT{'%'}) {
            if (ch == 0x20 || (ch >= 0x09 && ch <= 0x0d)) {
                // Simple ASCII space
                handler.on_text(begin, p);
                handler.on_whitespace();
                begin = p += 1;
                continue;
            }
            const auto cp_len =
                detail::code_point_length_by_starting_code_unit(ch);
            if (cp_len == 0) {
                handler.on_error("Invalid literal character");
                return p;
            }
            if (cp_len > 1) {
                // Multi code unit code point,
                // possible space
                auto cp_start_p = p;
                CharT buffer[4] = {};
                for (std::size_t i = 0; i < cp_len; ++i) {
                    if (p == end) {
                        handler.on_error("Invalid literal character");
                        return p;
                    }
                    buffer[i] = *p;
                    ++p;
                }
                if (is_cp_space(decode_code_point_exhaustive(
                        std::basic_string_view<CharT>{buffer, cp_len}))) {
                    handler.on_text(begin, cp_start_p);
                    handler.on_whitespace();
                    begin = p;
                }
                continue;
            }
            ++p;
            continue;
        }
        if (begin != p) {
            handler.on_text(begin, p);
        }
        ++p;  // Consume '%'
        if (p == end) {
            handler.on_error("Unexpected end of chrono format string");
            return p;
        }

        ch = *p;
        ++p;

        switch (ch) {
            case CharT{'%'}:
                handler.on_text(&ch, &ch + 1);
                break;
            case CharT{'n'}:
            case CharT{'t'}:
                handler.on_whitespace();
                break;
            // Year
            case CharT{'Y'}:
                handler.on_full_year();
                break;
            case CharT{'y'}:
                handler.on_short_year();
                break;
            case CharT{'C'}:
                handler.on_century();
                break;
            case CharT{'G'}:
                handler.on_iso_week_based_year();
                break;
            case CharT{'g'}:
                handler.on_iso_week_based_short_year();
                break;
            // Month
            case CharT{'b'}:
            case CharT{'B'}:
            case CharT{'h'}:
                handler.on_month_name();
                break;
            case CharT{'m'}:
                handler.on_dec_month();
                break;
            // Week
            case CharT{'U'}:
                handler.on_dec0_week_of_year();
                break;
            case CharT{'W'}:
                handler.on_dec1_week_of_year();
                break;
            case CharT{'V'}:
                handler.on_iso_week_of_year();
                break;
            // Day of year
            case CharT{'j'}:
                handler.on_day_of_year();
                break;
            // Day of month
            case CharT{'d'}:
            case CharT{'e'}:
                handler.on_day_of_month();
                break;
            // Day of week
            case CharT{'a'}:
            case CharT{'A'}:
                handler.on_weekday_name();
                break;
            case CharT{'w'}:
                handler.on_dec0_weekday();
                break;
            case CharT{'u'}:
                handler.on_dec1_weekday();
                break;
            // Hour
            case CharT{'H'}:
            case CharT{'k'}:
                handler.on_24_hour();
                break;
            case CharT{'I'}:
            case CharT{'l'}:
                handler.on_12_hour();
                break;
            // Minute
            case CharT{'M'}:
                handler.on_minute();
                break;
            // Second
            case CharT{'S'}:
                handler.on_second();
                break;
            // Subsecond
            case CharT{'.'}: {
                // p already increased before the switch
                if (p == end) {
                    handler.on_error("Unexpected end of chrono format string");
                    return p;
                }
                bool use_alternate = false;
                if (*p == CharT{'E'} || *p == CharT{'O'}) {
                    ++p;
                    if (p == end) {
                        handler.on_error(
                            "Unexpected end of chrono format string");
                        return p;
                    }
                    use_alternate = true;
                }
                if (*p != CharT{'S'}) {
                    handler.on_error(
                        "Expected `S` after `%.` in format string");
                    return p;
                }
                ++p;
                handler.on_subsecond(use_alternate
                                         ? numeric_system::alternative_e
                                         : numeric_system::standard);
                break;
            }
            // Timezones
            case CharT{'z'}:
                handler.on_tz_offset();
                break;
            case CharT{'Z'}:
                handler.on_tz_name();
                break;
            // Other
            case CharT{'c'}:
                handler.on_loc_datetime();
                break;
            case CharT{'x'}:
                handler.on_loc_date();
                break;
            case CharT{'X'}:
                handler.on_loc_time();
                break;
            case CharT{'D'}:
                handler.on_us_date();
                break;
            case CharT{'F'}:
                handler.on_iso_date();
                break;
            case CharT{'r'}:
                handler.on_loc_12_hour_time();
                break;
            case CharT{'R'}:
                handler.on_24_hour_time();
                break;
            case CharT{'T'}:
                handler.on_iso_time();
                break;
            case CharT{'p'}:
            case CharT{'P'}:
                handler.on_am_pm();
                break;
            case CharT{'s'}:
                handler.on_epoch_offset();
                break;
            case CharT{'Q'}:
                handler.on_duration_tick_count();
                break;
            case CharT{'q'}:
                handler.on_duration_suffix();
                break;
            // 'E'
            case CharT{'E'}: {
                if (p == end) {
                    handler.on_error("Unexpected end of chrono format string");
                    return p;
                }
                ch = *p;
                ++p;

                switch (ch) {
                    case CharT{'c'}:
                        handler.on_loc_datetime(numeric_system::alternative_e);
                        break;
                    case CharT{'C'}:
                        handler.on_century(numeric_system::alternative_e);
                        break;
                    case CharT{'x'}:
                        handler.on_loc_date(numeric_system::alternative_e);
                        break;
                    case CharT{'X'}:
                        handler.on_loc_time(numeric_system::alternative_e);
                        break;
                    case CharT{'y'}:
                        handler.on_loc_offset_year();
                        break;
                    case CharT{'Y'}:
                        handler.on_full_year(numeric_system::alternative_e);
                        break;
                    case CharT{'z'}:
                        handler.on_tz_offset(numeric_system::alternative_e);
                        break;
                    default:
                        handler.on_error(
                            "Invalid character following 'E' in chrono format "
                            "string");
                        return p;
                }
                break;
            }
            // 'O'
            case CharT{'O'}: {
                if (p == end) {
                    handler.on_error("Unexpected end of chrono format string");
                    return p;
                }
                ch = *p;
                ++p;

                switch (ch) {
                    case CharT{'d'}:
                    case CharT{'e'}:
                        handler.on_day_of_month(numeric_system::alternative_o);
                        break;
                    case CharT{'H'}:
                    case CharT{'k'}:
                        handler.on_24_hour(numeric_system::alternative_o);
                        break;
                    case CharT{'I'}:
                    case CharT{'l'}:
                        handler.on_12_hour(numeric_system::alternative_o);
                        break;
                    case CharT{'m'}:
                        handler.on_dec_month(numeric_system::alternative_o);
                        break;
                    case CharT{'M'}:
                        handler.on_minute(numeric_system::alternative_o);
                        break;
                    case CharT{'S'}:
                        handler.on_second(numeric_system::alternative_o);
                        break;
                    case CharT{'U'}:
                        handler.on_dec0_week_of_year(
                            numeric_system::alternative_o);
                        break;
                    case CharT{'V'}:
                        handler.on_iso_week_of_year(
                            numeric_system::alternative_o);
                        break;
                    case CharT{'w'}:
                        handler.on_dec0_weekday(numeric_system::alternative_o);
                        break;
                    case CharT{'W'}:
                        handler.on_dec1_week_of_year(
                            numeric_system::alternative_o);
                        break;
                    case CharT{'u'}:
                        handler.on_dec1_weekday(numeric_system::alternative_o);
                        break;
                    case CharT{'y'}:
                        handler.on_short_year(numeric_system::alternative_o);
                        break;
                    case CharT{'z'}:
                        handler.on_tz_offset(numeric_system::alternative_o);
                        break;
                    default:
                        handler.on_error(
                            "Invalid character following 'O' in chrono format "
                            "string");
                        return p;
                }
                break;
            }
            default:
                handler.on_error("Invalid character in chrono format string");
                return p;
        }
        begin = p;

        if (!handler.get_error()) {
            return p;
        }
    }
    if (begin != p) {
        handler.on_text(begin, p);
    }
    handler.verify();
    return p;
}

struct setter_state {
    unsigned localized : 1;
    unsigned subsec_set : 1;
    unsigned sec_set : 1;
    unsigned min_set : 1;
    unsigned hour24_set : 1;
    unsigned hour12_set : 1;
    unsigned mday_set : 1;
    unsigned mon_set : 1;
    unsigned full_year_set : 1;
    unsigned century_set : 1;
    unsigned short_year_set : 1;
    unsigned wday_set : 1;
    unsigned yday_set : 1;
    unsigned tzoff_set : 1;
    unsigned tzname_set : 1;
    unsigned am_pm_set : 1;
    unsigned epoch_ticks_set : 1;
    unsigned duration_ticks_set : 1;
    unsigned duration_suffix_set : 1;
    unsigned is_pm : 1;
    unsigned char short_year_value{0};
    unsigned char century_value{0};

    constexpr setter_state()
        : localized(0),
          subsec_set(0),
          sec_set(0),
          min_set(0),
          hour24_set(0),
          hour12_set(0),
          mday_set(0),
          mon_set(0),
          full_year_set(0),
          century_set(0),
          short_year_set(0),
          wday_set(0),
          yday_set(0),
          tzoff_set(0),
          tzname_set(0),
          am_pm_set(0),
          epoch_ticks_set(0),
          duration_ticks_set(0),
          duration_suffix_set(0),
          is_pm(0)
    {
    }

#define SCN_CHRONO_SETTER_STATE_SET(field)                        \
    template <typename Handler>                                   \
    constexpr void set_##field(Handler& handler)                  \
    {                                                             \
        if (field##_set) {                                        \
            handler.on_error(#field "-field set multiple times"); \
        }                                                         \
        field##_set = 1;                                          \
    }

    SCN_CHRONO_SETTER_STATE_SET(subsec)
    SCN_CHRONO_SETTER_STATE_SET(sec)
    SCN_CHRONO_SETTER_STATE_SET(min)
    SCN_CHRONO_SETTER_STATE_SET(hour24)
    SCN_CHRONO_SETTER_STATE_SET(hour12)
    SCN_CHRONO_SETTER_STATE_SET(mday)
    SCN_CHRONO_SETTER_STATE_SET(mon)
    SCN_CHRONO_SETTER_STATE_SET(full_year)
    SCN_CHRONO_SETTER_STATE_SET(century)
    SCN_CHRONO_SETTER_STATE_SET(short_year)
    SCN_CHRONO_SETTER_STATE_SET(wday)
    SCN_CHRONO_SETTER_STATE_SET(yday)
    SCN_CHRONO_SETTER_STATE_SET(tzoff)
    SCN_CHRONO_SETTER_STATE_SET(tzname)
    SCN_CHRONO_SETTER_STATE_SET(am_pm)
    SCN_CHRONO_SETTER_STATE_SET(epoch_ticks)
    SCN_CHRONO_SETTER_STATE_SET(duration_ticks)
    SCN_CHRONO_SETTER_STATE_SET(duration_suffix)

#undef SCN_CHRONO_SETTER_STATE_SET

    template <typename Handler>
    constexpr void verify(Handler& handler) const
    {
        if (hour24_set && hour12_set) {
            // clang-format off
            return handler.on_error("24-hour (%H) and 12-hour (%I) clocks can't both be in use simultaneously");
            // clang-format on
        }
        if (am_pm_set && hour24_set) {
            // clang-format off
            return handler.on_error("AM/PM (%p) can't be used with a 24-hour clock (%H)");
            // clang-format on
        }
        if (am_pm_set && !hour12_set) {
            // clang-format off
            return handler.on_error("AM/PM (%p) requires a 12-hour clock hour (%I)");
            // clang-format on
        }
        if (full_year_set && (century_set || short_year_set)) {
            // clang-format off
            return handler.on_error("full-year (%Y) can't be used together with century (%C) or short-year (%y)");
            // clang-format on
        }
        if (tzoff_set && tzname_set) {
            // clang-format off
            return handler.on_error("tzoff (%z) can't be used together with tzname (%Z)");
            // clang-format on
        }
        if (mday_set && yday_set) {
            // clang-format off
            return handler.on_error("mday (%d/%e) and yday (%j) can't be used together");
            // clang-format on
        }
    }

    template <typename Hour>
    constexpr void handle_am_pm(Hour& hour)
    {
        SCN_EXPECT(hour12_set);
        SCN_EXPECT(hour <= 12);
        if (is_pm) {
            // PM
            if (hour == 12) {
                // 12:xx PM -> 12:xx, no-op
            }
            else {
                hour = static_cast<Hour>(hour + Hour{12});
            }
        }
        else {
            // AM
            if (hour == 12) {
                // 12:xx AM -> 00:xx
                hour = Hour{0};
            }
            else {
                // no-op
            }
        }
    }

    template <typename Year>
    constexpr void handle_short_year_and_century(Year& year, Year offset)
    {
        SCN_EXPECT(!full_year_set);
        if (short_year_set && century_set) {
            year = century_value * 100 + short_year_value - offset;
        }
        else if (short_year_set) {
            if (short_year_value >= 69) {
                year = 1900 + short_year_value - offset;
            }
            else {
                year = 2000 + short_year_value - offset;
            }
        }
        else if (century_set) {
            year = 100 * century_value - offset;
        }
    }
};

namespace field_tags {

struct subsec {};
struct sec {};
struct min {};
struct hour {};
struct mday {};
struct mon {};
struct year {};
struct wday {};
struct yday {};
struct tzoff {};
struct tzname {};
struct duration {};

}  // namespace field_tags

template <typename T, typename Field>
struct always_supports_field;

template <typename Field>
struct always_supports_field<std::tm, Field> : std::true_type {};
template <>
struct always_supports_field<std::tm, field_tags::subsec> : std::false_type {};
template <>
struct always_supports_field<std::tm, field_tags::tzoff>
    : mp_valid<tm_gmtoff, std::tm> {};
template <>
struct always_supports_field<std::tm, field_tags::tzname> : std::false_type {};

template <typename Field>
struct always_supports_field<tm_with_tz, Field>
    : always_supports_field<std::tm, Field> {};
template <>
struct always_supports_field<tm_with_tz, field_tags::tzoff> : std::true_type {};
template <>
struct always_supports_field<tm_with_tz, field_tags::tzname> : std::true_type {
};

template <typename Field>
struct always_supports_field<datetime_components, Field> : std::true_type {};

// Format string checking for `time_point`: `datetime_components`, minus the
// fields that can never produce a valid time_point
struct time_point_fields {};

template <typename Field>
struct always_supports_field<time_point_fields, Field>
    : always_supports_field<datetime_components, Field> {};
template <>
struct always_supports_field<time_point_fields, field_tags::yday>
    : std::false_type {};
template <>
struct always_supports_field<time_point_fields, field_tags::tzname>
    : std::false_type {};

// Calendar component types are scanned through `datetime_components`,
// so they accept any field it does: fields not needed by the type are scanned
// and discarded
template <typename Field>
struct always_supports_field<weekday, Field>
    : always_supports_field<datetime_components, Field> {};
template <typename Field>
struct always_supports_field<day, Field>
    : always_supports_field<datetime_components, Field> {};
template <typename Field>
struct always_supports_field<month, Field>
    : always_supports_field<datetime_components, Field> {};
template <typename Field>
struct always_supports_field<year, Field>
    : always_supports_field<datetime_components, Field> {};
template <typename Field>
struct always_supports_field<year_month, Field>
    : always_supports_field<datetime_components, Field> {};
template <typename Field>
struct always_supports_field<month_day, Field>
    : always_supports_field<datetime_components, Field> {};
template <typename Field>
struct always_supports_field<year_month_day, Field>
    : always_supports_field<datetime_components, Field> {};

template <typename T>
struct always_supports_field<T, field_tags::duration> : std::false_type {};

template <typename T, typename Field>
struct always_requires_field : std::false_type {};

template <>
struct always_requires_field<weekday, field_tags::wday> : std::true_type {};
template <>
struct always_requires_field<day, field_tags::mday> : std::true_type {};
template <>
struct always_requires_field<month, field_tags::mon> : std::true_type {};
template <>
struct always_requires_field<year, field_tags::year> : std::true_type {};
template <>
struct always_requires_field<year_month, field_tags::year> : std::true_type {};
template <>
struct always_requires_field<year_month, field_tags::mon> : std::true_type {};
template <>
struct always_requires_field<month_day, field_tags::mon> : std::true_type {};
template <>
struct always_requires_field<month_day, field_tags::mday> : std::true_type {};
template <>
struct always_requires_field<year_month_day, field_tags::year>
    : std::true_type {};
template <>
struct always_requires_field<year_month_day, field_tags::mon> : std::true_type {
};
template <>
struct always_requires_field<year_month_day, field_tags::mday>
    : std::true_type {};

template <typename Derived>
struct null_chrono_spec_handler {
    constexpr void unsupported()
    {
        static_cast<Derived*>(this)->unsupported();
    }

    constexpr void on_localized()
    {
        unsupported();
    }

    constexpr void on_full_year(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_short_year(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_century(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_iso_week_based_year()
    {
        unsupported();
    }
    constexpr void on_iso_week_based_short_year()
    {
        unsupported();
    }
    constexpr void on_loc_offset_year()
    {
        unsupported();
    }

    constexpr void on_month_name()
    {
        unsupported();
    }
    constexpr void on_dec_month(numeric_system = numeric_system::standard)
    {
        unsupported();
    }

    constexpr void on_dec0_week_of_year(
        numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_dec1_week_of_year(
        numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_iso_week_of_year(
        numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_day_of_year()
    {
        unsupported();
    }
    constexpr void on_day_of_month(numeric_system = numeric_system::standard)
    {
        unsupported();
    }

    constexpr void on_weekday_name()
    {
        unsupported();
    }
    constexpr void on_dec0_weekday(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_dec1_weekday(numeric_system = numeric_system::standard)
    {
        unsupported();
    }

    constexpr void on_24_hour(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_12_hour(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_minute(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_second(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_subsecond(numeric_system = numeric_system::standard)
    {
        unsupported();
    }

    constexpr void on_tz_offset(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_tz_name()
    {
        unsupported();
    }

    constexpr void on_loc_datetime(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_loc_date(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_loc_time(numeric_system = numeric_system::standard)
    {
        unsupported();
    }
    constexpr void on_us_date()
    {
        unsupported();
    }
    constexpr void on_iso_date()
    {
        unsupported();
    }
    constexpr void on_loc_12_hour_time()
    {
        unsupported();
    }
    constexpr void on_24_hour_time()
    {
        unsupported();
    }
    constexpr void on_iso_time()
    {
        unsupported();
    }
    constexpr void on_am_pm()
    {
        unsupported();
    }

    constexpr void on_epoch_offset()
    {
        unsupported();
    }
    constexpr void on_duration_tick_count()
    {
        unsupported();
    }
    constexpr void on_duration_suffix()
    {
        unsupported();
    }

    constexpr void verify()
    {
        unsupported();
    }
};

template <typename T>
struct tm_format_checker {
    template <typename CharT>
    constexpr void on_text(const CharT*, const CharT*)
    {
    }
    constexpr void on_whitespace() {}

    constexpr void on_localized()
    {
        if constexpr (SCN_DISABLE_LOCALE) {
            on_error("'L' specifier invalid when SCN_DISABLE_LOCALE is on");
        }
        st.localized = 1;
    }

    constexpr void on_full_year(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::year>::value) {
            on_error("Years not supported with this type");
        }
        st.set_full_year(*this);
    }
    constexpr void on_short_year(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::year>::value) {
            on_error("Years not supported with this type");
        }
        st.set_short_year(*this);
    }
    constexpr void on_century(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::year>::value) {
            on_error("Years not supported with this type");
        }
        st.set_century(*this);
    }
    constexpr void on_iso_week_based_year()
    {
        if (!always_supports_field<T, field_tags::year>::value) {
            on_error("Years not supported with this type");
        }
        on_error("Unimplemented: %G");
    }
    constexpr void on_iso_week_based_short_year()
    {
        if (!always_supports_field<T, field_tags::year>::value) {
            on_error("Years not supported with this type");
        }
        on_error("Unimplemented: %g");
    }
    constexpr void on_loc_offset_year()
    {
        if (!always_supports_field<T, field_tags::year>::value) {
            on_error("Years not supported with this type");
        }
        on_error("Unimplemented: %Ey");
    }

    constexpr void on_month_name()
    {
        if (!always_supports_field<T, field_tags::mon>::value) {
            on_error("Months not supported with this type");
        }
        st.set_mon(*this);
    }
    constexpr void on_dec_month(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::mon>::value) {
            on_error("Months not supported with this type");
        }
        st.set_mon(*this);
    }

    constexpr void on_dec0_week_of_year(
        numeric_system = numeric_system::standard)
    {
        on_error("Unimplemented: %U");
    }
    constexpr void on_dec1_week_of_year(
        numeric_system = numeric_system::standard)
    {
        on_error("Unimplemented: %W");
    }
    constexpr void on_iso_week_of_year(
        numeric_system = numeric_system::standard)
    {
        on_error("Unimplemented: %V");
    }
    constexpr void on_day_of_year()
    {
        if (!always_supports_field<T, field_tags::yday>::value) {
            if constexpr (std::is_same_v<T, time_point_fields>) {
                on_error("Day-of-year (%j) not supported with time_point");
            }
            else {
                on_error("Day-of-year not supported with this type");
            }
        }
        st.set_yday(*this);
    }
    constexpr void on_day_of_month(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::mday>::value) {
            on_error("Day-of-month not supported with this type");
        }
        st.set_mday(*this);
    }

    constexpr void on_weekday_name()
    {
        if (!always_supports_field<T, field_tags::wday>::value) {
            on_error("Day-of-week not supported with this type");
        }
        st.set_wday(*this);
    }
    constexpr void on_dec0_weekday(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::wday>::value) {
            on_error("Day-of-week not supported with this type");
        }
        st.set_wday(*this);
    }
    constexpr void on_dec1_weekday(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::wday>::value) {
            on_error("Day-of-week not supported with this type");
        }
        st.set_wday(*this);
    }

    constexpr void on_24_hour(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::hour>::value) {
            on_error("Hours not supported with this type");
        }
        st.set_hour24(*this);
    }
    constexpr void on_12_hour(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::hour>::value) {
            on_error("Hours not supported with this type");
        }
        st.set_hour12(*this);
    }
    constexpr void on_minute(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::min>::value) {
            on_error("Minutes not supported with this type");
        }
        st.set_min(*this);
    }
    constexpr void on_second(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::sec>::value) {
            on_error("Seconds not supported with this type");
        }
        st.set_sec(*this);
    }
    constexpr void on_subsecond(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::subsec>::value) {
            on_error("Sub-seconds not supported with this type");
        }
        st.set_subsec(*this);
    }

    constexpr void on_tz_offset(numeric_system = numeric_system::standard)
    {
        if (!always_supports_field<T, field_tags::tzoff>::value) {
            on_error("Timezone offsets not supported with this type");
        }
        st.set_tzoff(*this);
    }
    constexpr void on_tz_name()
    {
        if (!always_supports_field<T, field_tags::tzname>::value) {
            if constexpr (std::is_same_v<T, time_point_fields>) {
                on_error("Timezone names (%Z) not supported with time_point");
            }
            else {
                on_error("Timezone names not supported with this type");
            }
        }
        st.set_tzname(*this);
    }

    // The fields set by these must match the ones set by `tm_reader`

    constexpr void on_loc_datetime(
        numeric_system sys = numeric_system::standard)
    {
        if (st.localized) {
            on_loc_date(sys);
        }
        else {
            // %a %b %d ... %Y, unlike %x
            on_weekday_name();
            on_month_name();
            on_day_of_month();
            on_full_year();
        }
        on_loc_time(sys);
    }
    constexpr void on_loc_date(numeric_system sys = numeric_system::standard)
    {
        if (st.localized) {
            on_full_year(sys);
        }
        else {
            on_short_year(sys);
        }
        on_dec_month(sys);
        on_day_of_month(sys);
    }
    constexpr void on_loc_time(numeric_system sys = numeric_system::standard)
    {
        on_24_hour(sys);
        on_minute(sys);
        on_second(sys);
    }
    constexpr void on_us_date()
    {
        on_dec_month();
        on_day_of_month();
        on_short_year();
    }
    constexpr void on_iso_date()
    {
        on_full_year();
        on_dec_month();
        on_day_of_month();
    }
    constexpr void on_loc_12_hour_time()
    {
        if (st.localized) {
            on_24_hour();
            on_minute();
            on_second();
            return;
        }
        on_12_hour();
        on_minute();
        on_second();
        on_am_pm();
    }
    constexpr void on_24_hour_time()
    {
        on_24_hour();
        on_minute();
    }
    constexpr void on_iso_time()
    {
        on_24_hour();
        on_minute();
        on_second();
    }
    constexpr void on_am_pm()
    {
        if (!always_supports_field<T, field_tags::hour>::value) {
            on_error("AM/PM not supported with this type");
        }
        st.set_am_pm(*this);
    }

    constexpr void on_epoch_offset()
    {
        on_error("Unimplemented: %s");
    }
    constexpr void on_duration_tick_count()
    {
        on_error("Unimplemented: %Q");
    }
    constexpr void on_duration_suffix()
    {
        on_error("Unimplemented: %q");
    }

    constexpr void verify()
    {
        if (always_requires_field<T, field_tags::subsec>::value &&
            !st.subsec_set) {
            on_error("Sub-seconds not set by the format string");
        }
        if (always_requires_field<T, field_tags::sec>::value && !st.sec_set) {
            on_error("Seconds not set by the format string");
        }
        if (always_requires_field<T, field_tags::min>::value && !st.min_set) {
            on_error("Minutes not set by the format string");
        }
        if (always_requires_field<T, field_tags::hour>::value &&
            !st.hour24_set && !st.hour12_set) {
            on_error("Hours not set by the format string");
        }
        if (always_requires_field<T, field_tags::mday>::value && !st.mday_set) {
            on_error("Day not set by the format string");
        }
        if (always_requires_field<T, field_tags::mon>::value && !st.mon_set) {
            on_error("Month not set by the format string");
        }
        if (always_requires_field<T, field_tags::year>::value &&
            !st.full_year_set && !st.century_set && !st.short_year_set) {
            on_error("Year not set by the format string");
        }
        if (always_requires_field<T, field_tags::wday>::value && !st.wday_set) {
            on_error("Day-of-week not set by the format string");
        }
        if (always_requires_field<T, field_tags::yday>::value && !st.yday_set) {
            on_error("Day-of-year not set by the format string");
        }
        if (always_requires_field<T, field_tags::tzoff>::value &&
            !st.tzoff_set) {
            on_error("Timezone offset not set by the format string");
        }
        if (always_requires_field<T, field_tags::tzname>::value &&
            !st.tzname_set) {
            on_error("Timezone name not set by the format string");
        }
        if (always_requires_field<T, field_tags::duration>::value &&
            !st.duration_ticks_set) {
            on_error("Duration tick count not set by the format string");
        }

        st.verify(*this);
    }

    SCN_NODISCARD constexpr scan_expected<void> get_error() const
    {
        return err;
    }
    constexpr void on_error(const char* msg)
    {
        if (err.has_value()) {
            invalid_format_string();
            err = unexpected(detail::handle_error(
                scan_error{scan_error::invalid_format_string, msg}));
        }
    }

    scan_expected<void> err{};
    setter_state st{};
};

template <typename T, typename CharT, typename ParseCtx>
constexpr auto chrono_parse_impl(ParseCtx& pctx,
                                 std::basic_string_view<CharT>& fmt_str) ->
    typename ParseCtx::iterator
{
    auto it = pctx.begin();
    auto end = pctx.end();
    if (it == end || *it == CharT{'}'}) {
        pctx.on_error(
            "Format string without specifiers is not valid for this type");
        return it;
    }

    auto checker = detail::tm_format_checker<T>{};
    end = detail::parse_chrono_format_specs(it, end, checker);
    if (end != it) {
        fmt_str = detail::make_string_view_from_pointers(it, end);
    }
    if (auto e = checker.get_error(); SCN_UNLIKELY(!e)) {
        SCN_EXPECT(e.error().code() == scan_error::invalid_format_string);
        pctx.on_error(e.error().msg());
    }
    return end;
}

template <typename CharT, typename T, typename Context>
SCN_PUBLIC auto chrono_scan_impl(std::basic_string_view<CharT> fmt_str,
                                 T& t,
                                 Context& ctx,
                                 seconds_format sec_format)
    -> scan_expected<typename Context::iterator>;

extern template SCN_PUBLIC auto chrono_scan_impl(std::string_view,
                                                 std::tm&,
                                                 scan_context&,
                                                 seconds_format)
    -> scan_expected<scan_context::iterator>;
extern template SCN_PUBLIC auto chrono_scan_impl(std::string_view,
                                                 tm_with_tz&,
                                                 scan_context&,
                                                 seconds_format)
    -> scan_expected<scan_context::iterator>;
extern template SCN_PUBLIC auto chrono_scan_impl(std::string_view,
                                                 datetime_components&,
                                                 scan_context&,
                                                 seconds_format)
    -> scan_expected<scan_context::iterator>;

extern template SCN_PUBLIC auto chrono_scan_impl(std::wstring_view,
                                                 std::tm&,
                                                 wscan_context&,
                                                 seconds_format)
    -> scan_expected<wscan_context::iterator>;
extern template SCN_PUBLIC auto chrono_scan_impl(std::wstring_view,
                                                 tm_with_tz&,
                                                 wscan_context&,
                                                 seconds_format)
    -> scan_expected<wscan_context::iterator>;
extern template SCN_PUBLIC auto chrono_scan_impl(std::wstring_view,
                                                 datetime_components&,
                                                 wscan_context&,
                                                 seconds_format)
    -> scan_expected<wscan_context::iterator>;

template <typename CharT, typename T>
struct chrono_datetime_scanner {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx) -> typename ParseCtx::iterator
    {
        return detail::chrono_parse_impl<T, CharT>(pctx, m_fmt_str);
    }

    template <typename Context>
    auto scan(T& t, Context& ctx) const
        -> scan_expected<typename Context::iterator>
    {
        return detail::chrono_scan_impl(m_fmt_str, t, ctx,
                                        seconds_format::integer);
    }

protected:
    std::basic_string_view<CharT> m_fmt_str{};
};

}  // namespace detail

template <typename CharT>
struct scanner<std::tm, CharT>
    : public detail::chrono_datetime_scanner<CharT, std::tm> {};

template <typename CharT>
struct scanner<tm_with_tz, CharT>
    : public detail::chrono_datetime_scanner<CharT, tm_with_tz> {};

template <typename CharT>
struct scanner<datetime_components, CharT>
    : public detail::chrono_datetime_scanner<CharT, datetime_components> {};

namespace detail {

// Scans a `T` through `datetime_components`.
// The format string is checked against the fields required by `T`.
template <typename CharT, typename T>
struct chrono_component_scanner
    : protected chrono_datetime_scanner<CharT, datetime_components> {
    template <typename ParseCtx>
    constexpr typename ParseCtx::iterator parse(ParseCtx& pctx)
    {
        if (pctx.begin() == pctx.end() || *pctx.begin() == CharT{'}'}) {
            pctx.on_error("Default format not supported for this type");
        }
        return detail::chrono_parse_impl<T, CharT>(pctx, this->m_fmt_str);
    }
};

}  // namespace detail

template <typename CharT>
struct scanner<weekday, CharT>
    : public detail::chrono_component_scanner<CharT, weekday> {
    template <typename Context>
    scan_expected<typename Context::iterator> scan(weekday& wd,
                                                   Context& ctx) const
    {
        datetime_components dt{};
        auto r =
            detail::chrono_component_scanner<CharT, weekday>::scan(dt, ctx);
        if (!r) {
            return unexpected(r.error());
        }
        SCN_EXPECT(dt.wday);
        wd = *dt.wday;
        return *r;
    }
};

template <typename CharT>
struct scanner<day, CharT>
    : public detail::chrono_component_scanner<CharT, day> {
    template <typename Context>
    scan_expected<typename Context::iterator> scan(day& d, Context& ctx) const
    {
        datetime_components dt{};
        auto r = detail::chrono_component_scanner<CharT, day>::scan(dt, ctx);
        if (!r) {
            return unexpected(r.error());
        }
        SCN_EXPECT(dt.mday);
        d = day{static_cast<unsigned>(*dt.mday)};
        return *r;
    }
};

template <typename CharT>
struct scanner<month, CharT>
    : public detail::chrono_component_scanner<CharT, month> {
    template <typename Context>
    scan_expected<typename Context::iterator> scan(month& m, Context& ctx) const
    {
        datetime_components dt{};
        auto r = detail::chrono_component_scanner<CharT, month>::scan(dt, ctx);
        if (!r) {
            return unexpected(r.error());
        }
        SCN_EXPECT(dt.mon);
        m = *dt.mon;
        return *r;
    }
};

template <typename CharT>
struct scanner<year, CharT>
    : public detail::chrono_component_scanner<CharT, year> {
    template <typename Context>
    scan_expected<typename Context::iterator> scan(year& y, Context& ctx) const
    {
        datetime_components dt{};
        auto r = detail::chrono_component_scanner<CharT, year>::scan(dt, ctx);
        if (!r) {
            return unexpected(r.error());
        }
        SCN_EXPECT(dt.year);
        y = year{*dt.year};
        return *r;
    }
};

template <typename CharT>
struct scanner<month_day, CharT>
    : public detail::chrono_component_scanner<CharT, month_day> {
    template <typename Context>
    scan_expected<typename Context::iterator> scan(month_day& md,
                                                   Context& ctx) const
    {
        datetime_components dt{};
        auto r =
            detail::chrono_component_scanner<CharT, month_day>::scan(dt, ctx);
        if (!r) {
            return unexpected(r.error());
        }
        SCN_EXPECT(dt.mon);
        SCN_EXPECT(dt.mday);
        if (!detail::is_valid_day_of_month(std::nullopt,
                                           static_cast<unsigned>(*dt.mon),
                                           static_cast<unsigned>(*dt.mday))) {
            return unexpected(scan_error{scan_error::invalid_scanned_value,
                                         "Invalid day of month"});
        }
        md = month_day{month{static_cast<unsigned>(*dt.mon)},
                       day{static_cast<unsigned>(*dt.mday)}};
        return *r;
    }
};

template <typename CharT>
struct scanner<year_month, CharT>
    : public detail::chrono_component_scanner<CharT, year_month> {
    template <typename Context>
    scan_expected<typename Context::iterator> scan(year_month& ym,
                                                   Context& ctx) const
    {
        datetime_components dt{};
        auto r =
            detail::chrono_component_scanner<CharT, year_month>::scan(dt, ctx);
        if (!r) {
            return unexpected(r.error());
        }
        SCN_EXPECT(dt.year);
        SCN_EXPECT(dt.mon);
        ym = year_month{year{*dt.year}, month{static_cast<unsigned>(*dt.mon)}};
        return *r;
    }
};

template <typename CharT>
struct scanner<year_month_day, CharT>
    : public detail::chrono_component_scanner<CharT, year_month_day> {
    template <typename Context>
    scan_expected<typename Context::iterator> scan(year_month_day& ymd,
                                                   Context& ctx) const
    {
        datetime_components dt{};
        auto r = detail::chrono_component_scanner<CharT, year_month_day>::scan(
            dt, ctx);
        if (!r) {
            return unexpected(r.error());
        }
        SCN_EXPECT(dt.year);
        SCN_EXPECT(dt.mon);
        SCN_EXPECT(dt.mday);
        if (!detail::is_valid_day_of_month(dt.year,
                                           static_cast<unsigned>(*dt.mon),
                                           static_cast<unsigned>(*dt.mday))) {
            return unexpected(scan_error{scan_error::invalid_scanned_value,
                                         "Invalid day of month"});
        }
        if (!detail::is_weekday_consistent(dt)) {
            return unexpected(scan_error{scan_error::invalid_scanned_value,
                                         "Weekday doesn't match the date"});
        }
        ymd = year_month_day{year{*dt.year},
                             month{static_cast<unsigned>(*dt.mon)},
                             day{static_cast<unsigned>(*dt.mday)}};
        return *r;
    }
};

/**
 * Scans a `std::chrono::time_point<std::chrono::system_clock>`.
 *
 * The scanned date and time are interpreted as UTC, unless a timezone offset
 * is scanned with `%%z`, in which case it's applied.
 * Unset fields default to 1970-01-01 00:00:00.
 * Invalid dates (e.g. February 30th) and leap seconds (second `60`) are
 * errors.
 * Timezone names (`%%Z`) and days of the year (`%%j`) are not supported,
 * and are format string errors.
 * If `Duration` is more precise than seconds, `%%S` also reads a fractional
 * part.
 *
 * \ingroup chrono
 */
template <typename CharT, typename Duration>
struct scanner<std::chrono::time_point<std::chrono::system_clock, Duration>,
               CharT>
    : public detail::chrono_component_scanner<CharT,
                                              detail::time_point_fields> {
    using time_point_type =
        std::chrono::time_point<std::chrono::system_clock, Duration>;

    template <typename Context>
    scan_expected<typename Context::iterator> scan(time_point_type& tp,
                                                   Context& ctx) const
    {
        constexpr auto sec_format =
            std::ratio_less_v<typename Duration::period, std::ratio<1>>
                ? detail::seconds_format::fractional
                : detail::seconds_format::integer;
        datetime_components dt{};
        auto r = detail::chrono_scan_impl(this->m_fmt_str, dt, ctx, sec_format);
        if (!r) {
            return unexpected(r.error());
        }
        if (!detail::is_valid_day_of_month(
                dt.year.value_or(1970),
                static_cast<unsigned>(dt.mon.value_or(January)),
                static_cast<unsigned>(dt.mday.value_or(1)))) {
            return unexpected(scan_error{scan_error::invalid_scanned_value,
                                         "Invalid day of month"});
        }
        if (!detail::is_weekday_consistent(dt)) {
            return unexpected(scan_error{scan_error::invalid_scanned_value,
                                         "Weekday doesn't match the date"});
        }
        if (dt.sec == 60) {
            return unexpected(scan_error{
                scan_error::invalid_scanned_value,
                "Leap seconds can't be represented by system_clock"});
        }
        auto tp_value = detail::time_since_unix_epoch<Duration>(dt);
        if (!tp_value) {
            return unexpected(tp_value.error());
        }
        tp = time_point_type{*tp_value};
        return *r;
    }
};

SCN_END_NAMESPACE
}  // namespace scn

#endif  // !SCN_DISABLE_CHRONO
