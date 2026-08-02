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

#include "../float_test_suite.h"
#include "reader_test_common.h"

#include <scn/impl.h>

#include <cmath>
#include <iomanip>

template <typename CharT, typename FloatT>
struct float_reader_interface {
    using char_type = CharT;
    using float_type = FloatT;

    static constexpr bool enabled = true;
    static constexpr bool supports_nan =
        std::numeric_limits<FloatT>::is_iec559 &&
        std::numeric_limits<FloatT>::has_quiet_NaN && !finite_math_only;
    static constexpr bool supports_inf =
        std::numeric_limits<FloatT>::is_iec559 &&
        std::numeric_limits<FloatT>::has_infinity && !finite_math_only;
    static constexpr bool supports_hex = true;

    static scn::scan_expected<void> test(std::basic_string_view<CharT> source,
                                         FloatT& parsed)
    {
        scn::impl::reader_impl_for_float<CharT> reader{};
        if (auto res = reader.read_default(source, parsed, {}); res) {
            if (*res != source.end()) {
                return scn::detail::unexpected_scan_error(
                    scn::scan_error::length_too_short,
                    "Entire input was not exhausted");
            }
            return {};
        }
        else {
            return scn::unexpected(res.error());
        }
    }
};

INSTANTIATE_TYPED_TEST_SUITE_P(FloatReaderTest,
                               FloatTestSuite,
                               float_test_suite_types<float_reader_interface>);

template <typename InterfaceT>
struct FloatValueReaderTest : testing::Test {
    using char_type = typename InterfaceT::char_type;
    using float_type = typename InterfaceT::float_type;
    using reader_type = typename InterfaceT::reader_type;
    static constexpr bool is_localized = InterfaceT::is_localized;

    void set_source(std::string_view s)
    {
        if constexpr (std::is_same_v<char_type, char>) {
            source = s;
        }
        else {
            source.resize(s.size());
            std::copy(s.begin(), s.end(), source.begin());
        }
    }

    auto read_specs(const scn::detail::format_specs& specs)
    {
        return reader.read_specs(source_as_view(), specs, value);
    }

    auto read_specs_with_locale(const scn::detail::format_specs& specs,
                                scn::detail::locale_ref loc)
    {
        return reader.read_specs_with_locale(source_as_view(), specs, value,
                                             loc);
    }

    template <typename T>
    testing::AssertionResult check_value(T expected) const
    {
        return check_floating_eq(this->value,
                                 static_cast<float_type>(expected));
    }

    std::basic_string_view<char_type> source_as_view() const
    {
        return source;
    }

    static scn::detail::format_specs make_format_specs_with_presentation(
        scn::detail::presentation_type type)
    {
        scn::detail::format_specs specs{};
        specs.type = type;
        return specs;
    }

    reader_type reader{};
    std::basic_string<char_type> source{};
    float_type value{};
};

TYPED_TEST_SUITE_P(FloatValueReaderTest);

TYPED_TEST_P(FloatValueReaderTest, PresentationScientificValueScientific)
{
    this->set_source("12.3e2");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_scientific));

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(12.3e2)));
}
TYPED_TEST_P(FloatValueReaderTest, PresentationScientificValueFixed)
{
    this->set_source("12.3");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_scientific));

    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), scn::scan_error::invalid_scanned_value);
}
TYPED_TEST_P(FloatValueReaderTest, PresentationScientificValueHexWithPrefix)
{
    this->set_source("0x1.fp3");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_scientific));

    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), scn::scan_error::invalid_scanned_value);
}
TYPED_TEST_P(FloatValueReaderTest, PresentationScientificValueHexWithoutPrefix)
{
    this->set_source("0x1.fp3");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_scientific));

    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), scn::scan_error::invalid_scanned_value);
}

TYPED_TEST_P(FloatValueReaderTest, PresentationFixedValueScientific)
{
    this->set_source("12.3e2");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_fixed));

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().begin() + 4);
    EXPECT_NE(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(12.3)));
}
TYPED_TEST_P(FloatValueReaderTest, PresentationFixedValueFixed)
{
    this->set_source("12.3");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_fixed));

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(12.3)));
}
TYPED_TEST_P(FloatValueReaderTest, PresentationFixedValueHexWithPrefix)
{
    this->set_source("0x1.fp3");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_fixed));

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().begin() + 1);
    EXPECT_NE(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(0.0)));
}
TYPED_TEST_P(FloatValueReaderTest, PresentationFixedValueHexWithoutPrefix)
{
    this->set_source("1.fp3");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_fixed));

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().begin() + 2);
    EXPECT_NE(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(1.0)));
}

template <typename T>
T get_hexfloat_interpreted_as_decimal(std::string_view input)
{
    SCN_EXPECT(input.size() >= 2 && input[0] == '0' && input[1] == 'x');

    scn::impl::reader_impl_for_float<char> reader{};
    T value{};
    auto res = reader.read_default(input, value, {});
    SCN_ENSURE(res);
    SCN_ENSURE(res.value() == input.end());
    return value;
}

TYPED_TEST_P(FloatValueReaderTest, PresentationHexValueScientific)
{
    this->set_source("12.3e2");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_hex));

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(
        get_hexfloat_interpreted_as_decimal<typename TestFixture::float_type>(
            "0x12.3e2")));
}
TYPED_TEST_P(FloatValueReaderTest, PresentationHexValueFixed)
{
    this->set_source("12.3");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_hex));

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(
        get_hexfloat_interpreted_as_decimal<typename TestFixture::float_type>(
            "0x12.3")));
}
TYPED_TEST_P(FloatValueReaderTest, PresentationHexValueHexWithPrefix)
{
    this->set_source("0x1.fp3");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_hex));

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(0x1.fp3)));
}
TYPED_TEST_P(FloatValueReaderTest, PresentationHexValueHexWithoutPrefix)
{
    this->set_source("1.fp3");
    auto res = this->read_specs(this->make_format_specs_with_presentation(
        scn::detail::presentation_type::float_hex));

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(0x1.fp3)));
}

#if !SCN_DISABLE_LOCALE
template <typename CharT>
struct numpunct_with_comma_thsep : std::numpunct<CharT> {
    numpunct_with_comma_thsep(std::string s)
        : std::numpunct<CharT>{}, g(std::move(s))
    {
    }

    CharT do_thousands_sep() const override
    {
        return CharT{','};
    }
    std::string do_grouping() const override
    {
        return g;
    }

    std::string g;
};

template <typename CharT>
struct thsep_test_state {
    thsep_test_state(std::string grouping)
        : stdloc(std::locale::classic(),
                 new numpunct_with_comma_thsep<CharT>{std::move(grouping)}),
          locref(stdloc)
    {
    }

    scn::detail::format_specs specs{};
    std::locale stdloc;
    scn::detail::locale_ref locref;
};

TYPED_TEST_P(FloatValueReaderTest, ThousandsSeparators)
{
    if constexpr (!TestFixture::is_localized) {
        GTEST_SKIP() << "This test requires a localized reader";
    }

    this->set_source("1,234.56789");
    auto state = thsep_test_state<typename TestFixture::char_type>{"\3"};
    auto res = this->read_specs_with_locale(state.specs, state.locref);

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(1234.56789)));
}

TYPED_TEST_P(FloatValueReaderTest, ThousandsSeparatorsWithInvalidGrouping)
{
    if constexpr (!TestFixture::is_localized) {
        GTEST_SKIP() << "This test requires a localized reader";
    }

    this->set_source("12,34.56789");
    auto state = thsep_test_state<typename TestFixture::char_type>{"\3"};
    auto res = this->read_specs_with_locale(state.specs, state.locref);

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(1234.56789)));
}

TYPED_TEST_P(FloatValueReaderTest, ExoticThousandsSeparators)
{
    if constexpr (!TestFixture::is_localized) {
        GTEST_SKIP() << "This test only works with localized_interface";
    }
    if constexpr (float_kind_for<typename TestFixture::float_type> ==
                  float_kind::f16) {
        GTEST_SKIP() << "float16 can't represent the tested value";
    }

    this->set_source("1,23,45,6.789");
    auto state = thsep_test_state<typename TestFixture::char_type>{"\1\2"};
    auto res = this->read_specs_with_locale(state.specs, state.locref);

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(123456.789)));
}

TYPED_TEST_P(FloatValueReaderTest, ExoticThousandsSeparatorsWithInvalidGrouping)
{
    if (!TestFixture::is_localized) {
        GTEST_SKIP() << "This test only works with localized_interface";
    }

    this->set_source("1,234.56789");
    auto state = thsep_test_state<typename TestFixture::char_type>{"\1\2"};
    auto res = this->read_specs_with_locale(state.specs, state.locref);

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(1234.56789)));
}

template <typename CharT>
struct numpunct_with_comma_decimal_separator : std::numpunct<CharT> {
    numpunct_with_comma_decimal_separator() = default;

    CharT do_decimal_point() const override
    {
        return CharT{','};
    }
};

template <typename CharT>
struct decimal_comma_test_state {
    decimal_comma_test_state()
        : stdloc(std::locale::classic(),
                 new numpunct_with_comma_decimal_separator<CharT>{}),
          locref(stdloc)
    {
    }

    std::locale stdloc;
    scn::detail::locale_ref locref;
};

TYPED_TEST_P(FloatValueReaderTest, LocalizedDecimalSeparator)
{
    if (!TestFixture::is_localized) {
        GTEST_SKIP() << "This test only works with localized_interface";
    }

    this->set_source("3,14");
    auto state = decimal_comma_test_state<typename TestFixture::char_type>{};
    auto res = this->read_specs_with_locale({}, state.locref);

    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->source_as_view().end());
    EXPECT_TRUE(this->check_value(SCN_FLOAT_CONSTANT(3.14)));
}
#endif  // !SCN_DISABLE_LOCALE

#if SCN_DISABLE_LOCALE
REGISTER_TYPED_TEST_SUITE_P(FloatValueReaderTest,
                            PresentationScientificValueScientific,
                            PresentationScientificValueFixed,
                            PresentationScientificValueHexWithPrefix,
                            PresentationScientificValueHexWithoutPrefix,
                            PresentationFixedValueScientific,
                            PresentationFixedValueFixed,
                            PresentationFixedValueHexWithPrefix,
                            PresentationFixedValueHexWithoutPrefix,
                            PresentationHexValueScientific,
                            PresentationHexValueFixed,
                            PresentationHexValueHexWithPrefix,
                            PresentationHexValueHexWithoutPrefix);
#else
REGISTER_TYPED_TEST_SUITE_P(FloatValueReaderTest,
                            PresentationScientificValueScientific,
                            PresentationScientificValueFixed,
                            PresentationScientificValueHexWithPrefix,
                            PresentationScientificValueHexWithoutPrefix,
                            PresentationFixedValueScientific,
                            PresentationFixedValueFixed,
                            PresentationFixedValueHexWithPrefix,
                            PresentationFixedValueHexWithoutPrefix,
                            PresentationHexValueScientific,
                            PresentationHexValueFixed,
                            PresentationHexValueHexWithPrefix,
                            PresentationHexValueHexWithoutPrefix,
                            ThousandsSeparators,
                            ThousandsSeparatorsWithInvalidGrouping,
                            ExoticThousandsSeparators,
                            ExoticThousandsSeparatorsWithInvalidGrouping,
                            LocalizedDecimalSeparator);
#endif

template <bool Localized, typename CharT, typename FloatT>
struct float_value_reader_interface {
    using char_type = CharT;
    using float_type = FloatT;
    using reader_type = reader_wrapper<Localized,
                                       char_type,
                                       float_type,
                                       scn::impl::reader_impl_for_float>;
    static constexpr bool is_localized = Localized;
    static constexpr bool enabled = true;
};

template <typename CharT, typename FloatT>
using classic_value_reader_interface =
    float_value_reader_interface<false, CharT, FloatT>;
template <typename CharT, typename FloatT>
using localized_value_reader_interface =
    float_value_reader_interface<true, CharT, FloatT>;

INSTANTIATE_TYPED_TEST_SUITE_P(
    ClassicFloatValueReaderTest,
    FloatValueReaderTest,
    float_test_suite_types<classic_value_reader_interface>);
INSTANTIATE_TYPED_TEST_SUITE_P(
    LocalizedFloatValueReaderTest,
    FloatValueReaderTest,
    float_test_suite_types<localized_value_reader_interface>);
