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

#include <scn/istream.h>

#include "wrapped_gtest.h"

#include <sstream>

using testing::FieldsAre;

TEST(IstreamSourceTest, Stringstream)
{
    std::istringstream ss{"123 abc"};
    auto res = scn::scan<int, std::string>(ss, "{} {}");
    ASSERT_TRUE(res);
    EXPECT_THAT(res->values(), FieldsAre(123, "abc"));
}
