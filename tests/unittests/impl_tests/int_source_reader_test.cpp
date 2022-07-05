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
#include <scn/impl/reader/integer/reader.h>

TEST(IntSourceReaderTest, DISABLED_Nocopy)
{
    std::string_view source{"123 456"};
    auto source_reader = scn::impl::int_classic_source_reader<char>{};
    auto result = source_reader.read(source);
    EXPECT_EQ(result.iterator, source.begin() + 3);
    EXPECT_EQ(result.value, std::string_view{"123"});
    // EXPECT_NE(result.value.data(), source_reader.get_buffer().data());
}

TEST(IntSourceReaderTest, Copying)
{
    std::istringstream ss{"123 456"};
    auto source = scn::istreambuf_view{ss};
    auto subrange = scn::istreambuf_subrange{source};

    auto source_reader = scn::impl::int_classic_source_reader<char>{};
    auto result = source_reader.read(subrange);
    EXPECT_NE(result.iterator, subrange.begin());
    EXPECT_NE(result.iterator, subrange.end());
    EXPECT_EQ(result.value, std::string_view{"123"});
    // EXPECT_EQ(result.value.data(), source_reader.get_buffer().data());
}
