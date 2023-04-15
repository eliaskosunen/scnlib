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

#include "../test_common.h"
#include "scn/util/meta.h"

#include <scn/impl/reader/number_preparer.h>

template <typename CharT>
class NumberPreparerTest : public testing::Test {
protected:
    using char_type = CharT;

    template <typename Source>
    void widen_source(Source&& s)
    {
        if constexpr (std::is_same_v<char_type, char>) {
            widened_source = std::string{SCN_FWD(s)};
        }
        else if constexpr (std::is_same_v<
                               scn::detail::remove_cvref_t<
                                   scn::ranges::range_value_t<Source>>,
                               wchar_t>) {
            widened_source = std::wstring{SCN_FWD(s)};
        }
        else {
            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wconversion")
            auto sv = std::string_view{s};
            widened_source = std::wstring(sv.size(), L'\0');
            std::copy(sv.begin(), sv.end(), widened_source->begin());
            SCN_GCC_POP
        }
    }

    template <template <class> class Preparer, typename Source>
    auto make_preparer(Source&& s)
    {
        widen_source(SCN_FWD(s));
        return Preparer<char_type>{*widened_source};
    }

    std::basic_string_view<CharT> get_widened_source_view() const
    {
        return *widened_source;
    }

    std::optional<std::basic_string<CharT>> widened_source;
};

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wgnu-zero-variadic-macro-arguments")

using type_list = testing::Types<char, wchar_t>;
TYPED_TEST_SUITE(NumberPreparerTest, type_list);

SCN_CLANG_POP

TYPED_TEST(NumberPreparerTest, IntWithoutThsep)
{
    auto preparer =
        this->template make_preparer<scn::impl::int_preparer>("123456");
    preparer.prepare_without_thsep();

    auto res = preparer.check_grouping_and_get_end_iterator(
        "", preparer.get_output().end());
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->get_widened_source_view().end());
}

TYPED_TEST(NumberPreparerTest, IntWithOneCorrectThsep)
{
    auto preparer =
        this->template make_preparer<scn::impl::int_preparer>("123,456");
    preparer.prepare_with_thsep(',');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\3", preparer.get_output().end());
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->get_widened_source_view().end());
}

TYPED_TEST(NumberPreparerTest, IntWithTwoCorrectThseps)
{
    auto preparer =
        this->template make_preparer<scn::impl::int_preparer>("123,456,789");
    preparer.prepare_with_thsep(',');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\3", preparer.get_output().end());
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->get_widened_source_view().end());
}

TYPED_TEST(NumberPreparerTest, IntWithIncorrectThsep)
{
    auto preparer =
        this->template make_preparer<scn::impl::int_preparer>("123456,789");
    preparer.prepare_with_thsep(',');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\3", preparer.get_output().end());
    EXPECT_FALSE(res);
}

TYPED_TEST(NumberPreparerTest, IntWithExoticThsepGrouping) {
    auto preparer =
        this->template make_preparer<scn::impl::int_preparer>("1,23,45,6");
    preparer.prepare_with_thsep(',');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\1\2", preparer.get_output().end());
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->get_widened_source_view().end());
}

TYPED_TEST(NumberPreparerTest, FloatWithoutThsep)
{
    auto preparer =
        this->template make_preparer<scn::impl::float_preparer>("123.456");
    preparer.prepare_without_thsep(typename TestFixture::char_type{'.'});

    auto res = preparer.check_grouping_and_get_end_iterator(
        "", preparer.get_output().end());
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->get_widened_source_view().end());
}

TYPED_TEST(NumberPreparerTest, FloatWithOneCorrectThsepAndNoDecimalPoint)
{
    auto preparer =
        this->template make_preparer<scn::impl::float_preparer>("123,456");
    preparer.prepare_with_thsep(',', '.');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\3", preparer.get_output().end());
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->get_widened_source_view().end());
}

TYPED_TEST(NumberPreparerTest, FloatWithOneCorrectThsep)
{
    auto preparer =
        this->template make_preparer<scn::impl::float_preparer>("123,456.789");
    preparer.prepare_with_thsep(',', '.');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\3", preparer.get_output().end());
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->get_widened_source_view().end());
}

TYPED_TEST(NumberPreparerTest, FloatWithTwoCorrectThseps)
{
    auto preparer = this->template make_preparer<scn::impl::float_preparer>(
        "123,456,789.123");
    preparer.prepare_with_thsep(',', '.');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\3", preparer.get_output().end());
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), this->get_widened_source_view().end());
}

TYPED_TEST(NumberPreparerTest, FloatWithIncorrectThsep)
{
    auto preparer = this->template make_preparer<scn::impl::float_preparer>(
        "123456,789.123");
    preparer.prepare_with_thsep(',', '.');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\3", preparer.get_output().end());
    EXPECT_FALSE(res);
}

TYPED_TEST(NumberPreparerTest, FloatWithThsepInDecimal)
{
    auto preparer = this->template make_preparer<scn::impl::float_preparer>(
        "123,456.789,123");
    preparer.prepare_with_thsep(',', '.');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\3", preparer.get_output().end());
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), this->get_widened_source_view().begin() + 11);
    EXPECT_EQ(*res.value(), ',');
}

TYPED_TEST(NumberPreparerTest, FloatInfWithoutThsep)
{
    auto preparer =
        this->template make_preparer<scn::impl::float_preparer>("inf");
    preparer.prepare_without_thsep('.');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\3", preparer.get_output().end());
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), this->get_widened_source_view().end());
}
TYPED_TEST(NumberPreparerTest, FloatInfWithThsep)
{
    auto preparer =
        this->template make_preparer<scn::impl::float_preparer>("inf");
    preparer.prepare_with_thsep(',', '.');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\3", preparer.get_output().end());
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), this->get_widened_source_view().end());
}
TYPED_TEST(NumberPreparerTest, FloatInfWithThsepAndSign)
{
    auto preparer =
        this->template make_preparer<scn::impl::float_preparer>("+inf");
    preparer.prepare_with_thsep(',', '.');

    auto res = preparer.check_grouping_and_get_end_iterator(
        "\3", preparer.get_output().end());
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), this->get_widened_source_view().end());
}
