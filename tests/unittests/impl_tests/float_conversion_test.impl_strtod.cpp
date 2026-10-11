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

#include "float_conversion_test.h"

#include <clocale>

template <bool Localized, typename CharT, typename FloatT>
struct convert_strtod_interface
    : float_conversion_interface_base<
          Localized,
          scn::impl::float_conversion::convert_strtod_traits,
          CharT,
          FloatT> {};

INSTANTIATE_TYPED_TEST_SUITE_P(
    ClassicStrtod,
    FloatTestSuite,
    float_test_suite_types<make_classic_float_conversion_interface<
        convert_strtod_interface>::template type>);

INSTANTIATE_TYPED_TEST_SUITE_P(
    LocalizedStrtod,
    FloatTestSuite,
    float_test_suite_types<make_localized_float_conversion_interface<
        convert_strtod_interface>::template type>);

namespace {
// Sets LC_NUMERIC to a locale with a comma as the decimal point,
// and restores the previous one on destruction
class comma_decimal_point_c_locale {
public:
    comma_decimal_point_c_locale()
        : m_previous(std::setlocale(LC_NUMERIC, nullptr))
    {
        for (const char* name :
             {"fi_FI.UTF-8", "de_DE.UTF-8", "fr_FR.UTF-8", "ru_RU.UTF-8"}) {
            if (std::setlocale(LC_NUMERIC, name) != nullptr) {
                if (std::string_view{std::localeconv()->decimal_point} == ",") {
                    m_set = true;
                    return;
                }
            }
        }
        std::setlocale(LC_NUMERIC, m_previous.c_str());
    }

    comma_decimal_point_c_locale(const comma_decimal_point_c_locale&) = delete;
    comma_decimal_point_c_locale& operator=(
        const comma_decimal_point_c_locale&) = delete;

    ~comma_decimal_point_c_locale()
    {
        std::setlocale(LC_NUMERIC, m_previous.c_str());
    }

    bool is_set() const
    {
        return m_set;
    }

private:
    std::string m_previous;
    bool m_set{false};
};

template <typename CharT, typename FloatT>
void test_strtod_with_comma_decimal_point()
{
    using traits = scn::impl::float_conversion::convert_strtod_traits;
    if constexpr (traits::enabled<CharT, FloatT>) {
        const auto convert = [](std::basic_string_view<CharT> source,
                                scn::detail::presentation_type type)
            -> scn::scan_expected<FloatT> {
            scn::detail::format_specs specs{};
            specs.type = type;
            FloatT value{};
            auto converter =
                scn::impl::float_conversion::basic_convert_float<CharT,
                                                                 traits>{};
            SCN_TRY(it, converter.convert_specs(source, value, specs, {}));
            if (it != source.end()) {
                return scn::detail::unexpected_scan_error(
                    scn::scan_error::invalid_scanned_value,
                    "Not all of the input was read");
            }
            return value;
        };

        const auto expect_value = [](const scn::scan_expected<FloatT>& result,
                                     FloatT expected) {
            ASSERT_THAT(result, Succeeded());
            EXPECT_TRUE(check_floating_eq(*result, expected));
        };

        using scn::detail::presentation_type;
        const auto widen = [](std::string_view str) {
            return std::basic_string<CharT>(str.begin(), str.end());
        };

        expect_value(convert(widen("3.25"), presentation_type::none),
                     FloatT{3.25});
        expect_value(convert(widen("-3.25e1"), presentation_type::none),
                     FloatT{-32.5});
        expect_value(convert(widen("0x1.8p1"), presentation_type::none),
                     FloatT{3.0});
        expect_value(convert(widen("1.8p1"), presentation_type::float_hex),
                     FloatT{3.0});
    }
    else {
        GTEST_SKIP() << "The strtod fallback isn't used for this type";
    }
}
}  // namespace

// The strtod fallback uses the decimal point of the global C locale.
// It's only enabled with SCN_FORCE_STRTOD_FALLBACK on regular platforms.
TEST(StrtodFallbackTest, CommaDecimalPointInCLocale)
{
    const comma_decimal_point_c_locale locale{};
    if (!locale.is_set()) {
        GTEST_SKIP() << "No locale with a comma as the decimal point";
    }

    test_strtod_with_comma_decimal_point<char, float>();
    test_strtod_with_comma_decimal_point<char, double>();
    test_strtod_with_comma_decimal_point<char, long double>();
    test_strtod_with_comma_decimal_point<wchar_t, float>();
    test_strtod_with_comma_decimal_point<wchar_t, double>();
    test_strtod_with_comma_decimal_point<wchar_t, long double>();
}
