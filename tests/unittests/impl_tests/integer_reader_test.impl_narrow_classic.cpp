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

#include "integer_reader_test.h"

using TypeList =
    ::testing::Types<reader_wrapper<false, char, signed char>,
                     reader_wrapper<false, char, short>,
                     reader_wrapper<false, char, int>,
                     reader_wrapper<false, char, long>,
                     reader_wrapper<false, char, long long>,
                     reader_wrapper<false, char, unsigned char>,
                     reader_wrapper<false, char, unsigned short>,
                     reader_wrapper<false, char, unsigned int>,
                     reader_wrapper<false, char, unsigned long>,
                     reader_wrapper<false, char, unsigned long long>>;

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wgnu-zero-variadic-macro-arguments")

INSTANTIATE_TYPED_TEST_SUITE_P(NarrowClassic, IntValueReaderTest, TypeList);

SCN_CLANG_POP
