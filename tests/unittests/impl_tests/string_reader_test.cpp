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

#include "reader_test_common.h"

#include <scn/impl.h>

#include <optional>

using namespace std::string_view_literals;

TEST(StringReaderTranscodeTest, StringViewWithSameCharacterType)
{
    auto src = scn::impl::string_view_wrapper{"foo"sv};
    std::string dst;

    auto e = scn::impl::transcode_if_necessary(src, dst);
    ASSERT_TRUE(e);
    EXPECT_EQ(dst, "foo");
    EXPECT_EQ(src.view(), "foo");
}
TEST(StringReaderTranscodeTest, StringViewWithDifferentCharacterType)
{
    auto src = scn::impl::string_view_wrapper{"foo"sv};
    std::wstring dst;

    auto e = scn::impl::transcode_if_necessary(src, dst);
    ASSERT_TRUE(e);
    EXPECT_EQ(dst, L"foo");
    EXPECT_EQ(src.view(), "foo");
}

TEST(StringReaderTranscodeTest, LvalueContiguousRangeWithSameCharacterType)
{
    auto src = scn::impl::contiguous_range_factory{"foo"sv};
    std::string dst;

    auto e = scn::impl::transcode_if_necessary(src, dst);
    ASSERT_TRUE(e);
    EXPECT_EQ(dst, "foo");
    EXPECT_EQ(src.view(), "foo");
}
TEST(StringReaderTranscodeTest, LvalueContiguousRangeWithDifferentCharacterType)
{
    auto src = scn::impl::contiguous_range_factory{"foo"sv};
    std::wstring dst;

    auto e = scn::impl::transcode_if_necessary(src, dst);
    ASSERT_TRUE(e);
    EXPECT_EQ(dst, L"foo");
    EXPECT_EQ(src.view(), "foo");
}

TEST(StringReaderTranscodeTest, RvalueContiguousRangeWithSameCharacterType)
{
    auto src = scn::impl::contiguous_range_factory{"foo"sv};
    std::string dst;

    auto e = scn::impl::transcode_if_necessary(SCN_MOVE(src), dst);
    ASSERT_TRUE(e);
    EXPECT_EQ(dst, "foo");
}
TEST(StringReaderTranscodeTest, RvalueContiguousRangeWithDifferentCharacterType)
{
    auto src = scn::impl::contiguous_range_factory{"foo"sv};
    std::wstring dst;

    auto e = scn::impl::transcode_if_necessary(SCN_MOVE(src), dst);
    ASSERT_TRUE(e);
    EXPECT_EQ(dst, L"foo");
}

struct string_tag {};
struct string_view_tag {};

template <typename SourceCharT, typename DestCharT, typename DestStringTag>
struct test_type_pack {
    using source_char_type = SourceCharT;
    using source_string_type = std::basic_string<source_char_type>;

    using dest_char_type = DestCharT;
    using dest_string_type =
        std::conditional_t<std::is_same_v<DestStringTag, string_tag>,
                           std::basic_string<dest_char_type>,
                           std::basic_string_view<dest_char_type>>;

    template <template <class> class Reader>
    using reader_type = Reader<source_char_type>;

    static constexpr bool is_source_wide =
        std::is_same_v<source_char_type, wchar_t>;
    static constexpr bool is_dest_wide =
        std::is_same_v<dest_char_type, wchar_t>;

    template <typename Source>
    static source_string_type make_widened_source(Source&& s)
    {
        if constexpr (std::is_same_v<scn::ranges::range_value_t<Source>,
                                     source_char_type>) {
            return source_string_type{SCN_FWD(s)};
        }
        else {
            source_string_type str;
            scn::impl::transcode_valid_to_string(s, str);
            return str;
        }
    }

    static testing::AssertionResult check_value(const dest_string_type& val,
                                                std::string_view expected)
    {
        std::string narrowed_val;
        if constexpr (std::is_same_v<dest_char_type, char>) {
            narrowed_val = val;
        }
        else {
            scn::impl::transcode_to_string(
                std::basic_string_view<dest_char_type>(val), narrowed_val);
        }

        if (narrowed_val.size() != expected.size()) {
            return testing::AssertionFailure()
                   << "Size mismatch: " << val.size()
                   << " != " << expected.size() << " (\"" << narrowed_val
                   << "\" " << string_bytes_spelled_out(narrowed_val)
                   << " != \"" << expected << "\" "
                   << string_bytes_spelled_out(expected) << ')';
        }
        if (!std::equal(narrowed_val.begin(), narrowed_val.end(),
                        expected.begin())) {
            return testing::AssertionFailure()
                   << "Value mismatch: \"" << narrowed_val << "\" "
                   << string_bytes_spelled_out(narrowed_val) << " != \""
                   << expected << "\" " << string_bytes_spelled_out(expected);
        }

        return testing::AssertionSuccess();
    }

private:
    static std::string string_bytes_spelled_out(std::string_view s)
    {
        std::ostringstream oss;
        oss << '[' << std::hex;
        for (auto byte : s.substr(0, s.size() - 1)) {
            oss << static_cast<int>(static_cast<unsigned char>(byte)) << ", ";
        }
        oss << static_cast<int>(static_cast<unsigned char>(s.back())) << ']';
        return oss.str();
    }
};

template <typename T>
class StringWordReaderTest : public testing::Test {
protected:
    using pack = T;

    static auto make_reader()
    {
        return typename T::template reader_type<scn::impl::word_reader_impl>{};
    }

    template <typename Source>
    auto set_source(Source&& s)
    {
        widened_source = T::make_widened_source(SCN_FWD(s));
        return std::basic_string_view<typename T::source_char_type>{
            *widened_source};
    }

    auto read()
    {
        typename T::dest_string_type val{};
        auto ret = make_reader().read(
            std::basic_string_view<typename T::source_char_type>{
                *widened_source},
            val);
        return std::make_pair(ret, val);
    }

    static auto check_value(const typename T::dest_string_type& val,
                            std::string_view expected)
    {
        return T::check_value(val, expected);
    }

    std::optional<typename T::source_string_type> widened_source{std::nullopt};
};

using type_list =
    testing::Types<test_type_pack<char, char, string_tag>,
                   test_type_pack<char, char, string_view_tag>,
                   test_type_pack<char, wchar_t, string_tag>,

                   test_type_pack<wchar_t, char, string_tag>,
                   test_type_pack<wchar_t, wchar_t, string_tag>,
                   test_type_pack<wchar_t, wchar_t, string_view_tag>>;

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wgnu-zero-variadic-macro-arguments")

TYPED_TEST_SUITE(StringWordReaderTest, type_list);

SCN_CLANG_POP

TYPED_TEST(StringWordReaderTest, All)
{
    auto src = this->set_source("foo"sv);
    auto [ret, val] = this->read();

    ASSERT_TRUE(ret);
    EXPECT_EQ(*ret, src.end());
    EXPECT_TRUE(this->check_value(val, "foo"sv));
}

TYPED_TEST(StringWordReaderTest, Word)
{
    auto src = this->set_source("foo bar"sv);
    auto [ret, val] = this->read();

    ASSERT_TRUE(ret);
    EXPECT_EQ(*ret, src.begin() + 3);
    EXPECT_TRUE(this->check_value(val, "foo"sv));
}

TEST(StringCharacterReaderTest, NonTakeWidthInput)
{
    auto src = "foo"sv;
    std::string val{};
    auto ret = scn::impl::character_reader_impl<char>{}.read(src, val);

    ASSERT_FALSE(ret);
}

TEST(StringCharacterReaderTest, StringWithSameWidth)
{
    auto src = scn::impl::take_width("foo"sv, 3);
    std::string val{};
    auto ret = scn::impl::character_reader_impl<char>{}.read(src, val);

    ASSERT_TRUE(ret);
    EXPECT_EQ(val, "foo");
}
TEST(StringCharacterReaderTest, StringViewWithSameWidth)
{
    auto src = scn::impl::take_width("foo"sv, 3);
    std::string_view val{};
    auto ret = scn::impl::character_reader_impl<char>{}.read(src, val);

    ASSERT_TRUE(ret);
    EXPECT_EQ(val, "foo");
}

TEST(StringCharacterReaderTest, StringWithMoreWidth)
{
    auto src = scn::impl::take_width("foo"sv, 6);
    std::string val{};
    auto ret = scn::impl::character_reader_impl<char>{}.read(src, val);

    ASSERT_TRUE(ret);
    EXPECT_EQ(val, "foo");
}
TEST(StringCharacterReaderTest, StringViewWithMoreWidth)
{
    auto src = scn::impl::take_width("foo"sv, 6);
    std::string_view val{};
    auto ret = scn::impl::character_reader_impl<char>{}.read(src, val);

    ASSERT_TRUE(ret);
    EXPECT_EQ(val, "foo");
}

TEST(StringCharacterReaderTest, StringWithLessWidth)
{
    auto src = scn::impl::take_width("foobar"sv, 3);
    std::string val{};
    auto ret = scn::impl::character_reader_impl<char>{}.read(src, val);

    ASSERT_TRUE(ret);
    EXPECT_EQ(val, "foo");
}
TEST(StringCharacterReaderTest, StringViewWithLessWidth)
{
    auto src = scn::impl::take_width("foobar"sv, 3);
    std::string_view val{};
    auto ret = scn::impl::character_reader_impl<char>{}.read(src, val);

    ASSERT_TRUE(ret);
    EXPECT_EQ(val, "foo");
}

template <typename T>
class StringCharacterSetReaderTest : public testing::Test {
protected:
    using pack = T;
    using specs_type = scn::detail::format_specs;

    specs_type make_specs_from_set(std::string_view f)
    {
        SCN_EXPECT(f.front() == '[');

        SCN_GCC_PUSH
        SCN_GCC_IGNORE("-Wconversion")
        scn::impl::transcode_to_string(f, tmp_specs_str);
        auto widened_sv = std::wstring_view{tmp_specs_str};
        SCN_GCC_POP

        specs_type specs{};
        scn::detail::specs_setter handler{specs};

        auto [b, e] = [&]() {
            if constexpr (std::is_same_v<typename T::source_char_type, char>) {
                return std::make_pair(f.data(), f.data() + f.size());
            }
            else {
                return std::make_pair(widened_sv.data(),
                                      widened_sv.data() + widened_sv.size());
            }
        }();

        auto s = scn::detail::parse_presentation_set(b, e, handler);
        SCN_EXPECT(s.size() > 2);
        handler.on_character_set_string(s);
        SCN_EXPECT(b == e);

        return specs;
    }

    static auto make_reader()
    {
        return typename T::template reader_type<
            scn::impl::character_set_reader_impl>{};
    }

    template <typename Source>
    auto set_source(Source&& s)
    {
        widened_source = T::make_widened_source(SCN_FWD(s));
        return std::basic_string_view<typename T::source_char_type>{
            *widened_source};
    }

    auto read(const specs_type& specs)
    {
        typename T::dest_string_type val{};
        auto ret = make_reader().read(
            std::basic_string_view<typename T::source_char_type>{
                *widened_source},
            specs, val);
        return std::make_pair(ret, val);
    }

    static auto check_value(const typename T::dest_string_type& val,
                            std::string_view expected)
    {
        return T::check_value(val, expected);
    }

    std::optional<typename T::source_string_type> widened_source{std::nullopt};
    std::wstring tmp_specs_str{};
};

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wgnu-zero-variadic-macro-arguments")

TYPED_TEST_SUITE(StringCharacterSetReaderTest, type_list);

SCN_CLANG_POP

TYPED_TEST(StringCharacterSetReaderTest, MatchEmpty)
{
    auto src = this->set_source("123"sv);
    auto [ret, val] = this->read(this->make_specs_from_set("[a-z]"));

    ASSERT_FALSE(ret);
    SCN_UNUSED(src);
}

TYPED_TEST(StringCharacterSetReaderTest, LiteralAbc)
{
    auto src = this->set_source("abc123"sv);
    auto [ret, val] = this->read(this->make_specs_from_set("[abc]"));

    ASSERT_TRUE(ret);
    EXPECT_EQ(*ret, src.begin() + 3);
    EXPECT_TRUE(this->check_value(val, "abc"));
}
TYPED_TEST(StringCharacterSetReaderTest, LiteralAToC)
{
    auto src = this->set_source("abc123"sv);
    auto [ret, val] = this->read(this->make_specs_from_set("[a-c]"));

    ASSERT_TRUE(ret);
    EXPECT_EQ(*ret, src.begin() + 3);
    EXPECT_TRUE(this->check_value(val, "abc"));
}

TYPED_TEST(StringCharacterSetReaderTest, LiteralAWithDiaeresis)
{
    auto src = this->set_source("äa"sv);
    auto [ret, val] = this->read(this->make_specs_from_set("[ä]"));

    ASSERT_TRUE(ret);
    EXPECT_NE(*ret, src.end());
    EXPECT_TRUE(this->check_value(val, "ä"));
}
TYPED_TEST(StringCharacterSetReaderTest, MultipleLiteralNonAsciiCharacters)
{
    auto src = this->set_source("öäa"sv);
    auto [ret, val] = this->read(this->make_specs_from_set("[äö]"));

    ASSERT_TRUE(ret);
    EXPECT_NE(*ret, src.end());
    EXPECT_TRUE(this->check_value(val, "öä"));
}
