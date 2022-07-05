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

#include <gtest/gtest.h>

#include <scn/detail/erased_range.h>
#include <scn/impl/algorithms/read_localized.h>

class LocalizedSingleCharWidenerTest
    : public testing::TestWithParam<std::locale> {
protected:
    scn::impl::localized_single_character_widener<char> narrow_widener{
        scn::detail::locale_ref{GetParam()}};
    scn::impl::localized_single_character_widener<wchar_t> identity_widener{
        scn::detail::locale_ref{GetParam()}};
};

INSTANTIATE_TEST_SUITE_P(WithClassicLocale,
                         LocalizedSingleCharWidenerTest,
                         testing::Values(std::locale::classic()));
INSTANTIATE_TEST_SUITE_P(WithEnUsUtf8Locale,
                         LocalizedSingleCharWidenerTest,
                         testing::Values(std::locale{"en_US.UTF-8"}));

TEST_P(LocalizedSingleCharWidenerTest, AlreadyWide)
{
    auto input = std::wstring_view{L"ab"};

    auto result = identity_widener(input);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->iterator, input.begin() + 1);
    EXPECT_EQ(result->value, L'a');

    result = identity_widener(input.substr(1));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->iterator, input.end());
    EXPECT_EQ(result->value, L'b');
}

TEST_P(LocalizedSingleCharWidenerTest, AsciiToWide)
{
    auto input = std::string_view{"ab"};

    auto result = narrow_widener(input);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->iterator, input.begin() + 1);
    EXPECT_EQ(result->value, L'a');

    result = narrow_widener(input.substr(1));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->iterator, input.end());
    EXPECT_EQ(result->value, L'b');
}

TEST_P(LocalizedSingleCharWidenerTest, TwoCodeUnits)
{
    auto input = std::string_view{"ä"};

    auto result = narrow_widener(input);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->iterator, input.end());
    EXPECT_EQ(result->value, L'ä');
}

namespace {
    inline std::locale classic_locale = std::locale::classic();
    inline std::locale en_us_utf8_locale = std::locale("en_US.UTF-8");

    struct classic_locale_ref {
        scn::detail::locale_ref locale{classic_locale};
    };
    struct en_us_utf8_locale_ref {
        scn::detail::locale_ref locale{en_us_utf8_locale};
    };
}  // namespace

template <typename T>
class ReadUntilLocalizedSkipTest : public testing::Test {
public:
    using locale_ref_wrapper_type = std::tuple_element_t<0, T>;
    using narrow_range_type = std::tuple_element_t<1, T>;
    using wide_range_type = std::tuple_element_t<2, T>;

    ReadUntilLocalizedSkipTest() : locale(locale_ref_wrapper_type{}.locale) {}

protected:
    template <typename CharT,
              typename InputRange,
              typename SourceStr,
              typename SourceErased,
              typename Input>
    auto& make_input_impl(SourceStr& source_str,
                          SourceErased& source_erased,
                          Input& input)
    {
        if constexpr (std::is_same_v<InputRange,
                                     std::basic_string_view<CharT>>) {
            input.emplace(source_str);
            return *input;
        }
        else {
            source_erased.emplace(scn::erase_range(source_str));
            input.emplace(*source_erased);
            return *input;
        }
    }

    template <typename Source>
    auto& make_input(Source&& src)
    {
        if constexpr (std::is_same_v<scn::ranges::range_value_t<Source>,
                                     char>) {
            string_narrow = src;
            return make_input_impl<char, narrow_range_type>(
                string_narrow, erased_narrow, input_narrow);
        }
        else {
            string_wide = src;
            return make_input_impl<wchar_t, wide_range_type>(
                string_wide, erased_wide, input_wide);
        }
    }

    std::string string_narrow{};
    std::wstring string_wide{};
    std::optional<scn::erased_range> erased_narrow{};
    std::optional<scn::werased_range> erased_wide{};

    std::optional<narrow_range_type> input_narrow{};
    std::optional<wide_range_type> input_wide{};
    scn::detail::locale_ref locale;
};

using types = testing::Types<
    std::tuple<classic_locale_ref, std::string_view, std::wstring_view>,
    std::tuple<en_us_utf8_locale_ref, std::string_view, std::wstring_view>>;

TYPED_TEST_SUITE(ReadUntilLocalizedSkipTest, types);

TYPED_TEST(ReadUntilLocalizedSkipTest, UntilSpace)
{
    {
        auto& input = this->make_input("abc def");
        auto result = scn::impl::read_until_localized_skip(
            input, this->locale, std::ctype_base::space, true);
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, std::next(input.begin(), 3));
    }
    {
        auto& input = this->make_input(L"abc def");
        auto result = scn::impl::read_until_localized_skip(
            input, this->locale, std::ctype_base::space, true);
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, std::next(input.begin(), 3));
    }
}

TYPED_TEST(ReadUntilLocalizedSkipTest, UntilSpaceNonAscii)
{
    {
        auto& input = this->make_input("åäö def");
        auto result = scn::impl::read_until_localized_skip(
            input, this->locale, std::ctype_base::space, true);
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, std::next(input.begin(), 6));
    }

    {
        auto& input = this->make_input(L"åäö def");
        auto result = scn::impl::read_until_localized_skip(
            input, this->locale, std::ctype_base::space, true);
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, std::next(input.begin(), 3));
    }
}
