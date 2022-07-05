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

#include <scn/detail/istream_range.h>
#include <scn/impl/algorithms/common.h>

TEST(CopyTest, ContiguousSourceAndDest)
{
    auto input = std::string_view{"123"};
    auto output = std::string(3, '\0');

    auto [in, out] = scn::impl::copy(input, output);
    EXPECT_EQ(in, input.end());
    EXPECT_EQ(out, output.end());
    EXPECT_EQ(output, "123");
}

TEST(CopyTest, ContiguousSource_OutputDest)
{
    auto input = std::string_view{"123"};
    auto output = std::string(8, '\0');

    auto [in, out] = scn::impl::copy(
        input, scn::ranges::subrange<std::string::iterator,
                                     scn::ranges_std::unreachable_sentinel_t>{
                   output.begin(), scn::ranges_std::unreachable_sentinel});
    EXPECT_EQ(in, input.end());
    auto out_sv = std::string_view{output.data(),
                                   static_cast<size_t>(out - output.begin())};
    EXPECT_EQ(out_sv, "123");
}

TEST(CopyTest, NullDest)
{
    auto input = std::string_view{"123"};
    auto output = scn::impl::null_output_range<char>{};

    auto [in, out] = scn::impl::copy(input, output);
    EXPECT_EQ(in, input.end());
    EXPECT_NE(out, output.end());
}

TEST(CopyTest, IstreambufSubrangeSource_ContiguousDest)
{
    auto input_ss = std::istringstream{"123"};
    auto input_source = scn::istreambuf_view{input_ss};
    auto input = scn::istreambuf_subrange{input_source};
    auto output = std::string(3, '\0');

    auto [in, out] = scn::impl::copy(input, output);
    EXPECT_EQ(in, input.end());
    EXPECT_EQ(out, output.end());
    EXPECT_EQ(output, "123");
}
TEST(CopyTest, IstreambufSubrangeSource_OutputDest)
{
    auto input_ss = std::istringstream{"123"};
    auto input_source = scn::istreambuf_view{input_ss};
    auto input = scn::istreambuf_subrange{input_source};

    auto output_sink = std::string{};
    auto output = scn::impl::back_insert(output_sink);

    auto [in, out] = scn::impl::copy(input, output);
    EXPECT_EQ(in, input.end());
    EXPECT_NE(out, output.end());
    EXPECT_EQ(output_sink, "123");
}

TEST(CopyTest, IstreambufViewSource_ContiguousDest)
{
    auto input_ss = std::istringstream{"123 "};
    auto input = scn::istreambuf_view{input_ss};
    auto output = std::string(3, '\0');

    auto [in, out] = scn::impl::copy(input, output);
    EXPECT_NE(in, input.end());
    EXPECT_EQ(out, output.end());
    EXPECT_EQ(output, "123");

    scn::ranges::fill(output, '\0');
    auto [in2, out2] =
        scn::impl::copy(scn::ranges::subrange{in, input.end()}, output);
    EXPECT_EQ(in2, input.end());
    EXPECT_NE(out2, output.end());
    EXPECT_STREQ(output.c_str(), " ");
}
TEST(CopyTest, IstreambufViewSource_OutputDest)
{
    auto input_ss = std::istringstream{"123 "};
    auto input = scn::istreambuf_view{input_ss};

    auto output_sink = std::string{};
    auto output = scn::impl::back_insert(output_sink);

    auto [in, out] = scn::impl::copy(input, output);
    EXPECT_EQ(in, input.end());
    EXPECT_NE(out, output.end());
    EXPECT_EQ(output_sink, "123 ");

    output_sink.clear();
    output = scn::impl::back_insert(output_sink);
    std::ignore =
        scn::impl::copy(scn::ranges::subrange{in, input.end()}, output);
}
