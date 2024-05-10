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

#include <scn/impl.h>

int identity_fn(int a)
{
    return a;
}

TEST(FunctionRefTest, Test)
{
    EXPECT_EQ(scn::impl::function_ref<int(int)>(identity_fn)(42), 42);

    EXPECT_EQ(scn::impl::function_ref<int(int)>([](int a) { return a; })(42),
              42);

    int n = 42;
    EXPECT_EQ(scn::impl::function_ref<int()>([&]() { return n; })(), 42);
}
