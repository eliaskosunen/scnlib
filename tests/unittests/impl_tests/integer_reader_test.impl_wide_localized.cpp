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

#if !SCN_DISABLE_LOCALE

using TypeList =
    ::testing::Types<int_reader_wrapper<true, wchar_t, signed char>,
                     int_reader_wrapper<true, wchar_t, short>,
                     int_reader_wrapper<true, wchar_t, int>,
                     int_reader_wrapper<true, wchar_t, long>,
                     int_reader_wrapper<true, wchar_t, long long>,
                     int_reader_wrapper<true, wchar_t, unsigned char>,
                     int_reader_wrapper<true, wchar_t, unsigned short>,
                     int_reader_wrapper<true, wchar_t, unsigned int>,
                     int_reader_wrapper<true, wchar_t, unsigned long>,
                     int_reader_wrapper<true, wchar_t, unsigned long long>>;

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wgnu-zero-variadic-macro-arguments")

INSTANTIATE_TYPED_TEST_SUITE_P(WideLocalized, IntValueReaderTest, TypeList);

SCN_CLANG_POP

#endif  // !SCN_DISABLE_LOCALE
