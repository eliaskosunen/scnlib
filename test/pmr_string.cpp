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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "test.h"

#if SCN_STD >= SCN_STD_17 && SCN_HAS_INCLUDE(<memory_resource>)
#include <memory_resource>

#if defined(__cpp_lib_memory_resource) &&  \
    __cpp_lib_memory_resource >= 201603 && \
    (SCN_GCC < SCN_COMPILER(9, 0, 0) || SCN_GCC >= SCN_COMPILER(10, 0, 0))
TEST_CASE("pmr string")
{
    std::pmr::string str{};

    auto ret = do_scan<char>("str", "{}", str);
    CHECK(ret);
    CHECK(str == "str");
}
#endif
#endif
