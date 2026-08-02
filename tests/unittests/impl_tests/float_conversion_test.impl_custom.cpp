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

#include "float_conversion_test.h"

template <bool Localized, typename CharT, typename FloatT>
struct convert_custom_interface
    : float_conversion_interface_base<
          Localized,
          scn::impl::float_conversion::convert_custom_traits,
          CharT,
          FloatT> {};

INSTANTIATE_TYPED_TEST_SUITE_P(
    ClassicCustom,
    FloatTestSuite,
    float_test_suite_types<make_classic_float_conversion_interface<
        convert_custom_interface>::template type>);

INSTANTIATE_TYPED_TEST_SUITE_P(
    LocalizedCustom,
    FloatTestSuite,
    float_test_suite_types<make_localized_float_conversion_interface<
        convert_custom_interface>::template type>);
