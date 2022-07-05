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

#include "int_reader_test.h"

using TypeList = ::testing::Types<
    test_type_pack<localized_reader_interface, char, signed char>,
    test_type_pack<localized_reader_interface, char, short>,
    test_type_pack<localized_reader_interface, char, int>,
    test_type_pack<localized_reader_interface, char, long>,
    test_type_pack<localized_reader_interface, char, long long>,
    test_type_pack<localized_reader_interface, char, unsigned char>,
    test_type_pack<localized_reader_interface, char, unsigned short>,
    test_type_pack<localized_reader_interface, char, unsigned int>,
    test_type_pack<localized_reader_interface, char, unsigned long>,
    test_type_pack<localized_reader_interface, char, unsigned long long>>;

INSTANTIATE_TYPED_TEST_SUITE_P(NarrowLocalized, IntValueReaderTest, TypeList);
