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

// Reports the layout of types as seen through the headers, like in the
// compiled library, so module_test.cpp can compare it to the one seen through
// `import scn;`.

#include <scn/scan.h>

#include <cstddef>
#include <string_view>

using expected_type = scn::scan_expected<std::ptrdiff_t>;
using result_type = decltype(scn::scan<int>(std::string_view{}, "{}"));

std::size_t header_sizeof_scan_expected()
{
    return sizeof(expected_type);
}
std::size_t header_alignof_scan_expected()
{
    return alignof(expected_type);
}
std::size_t header_sizeof_scan_result()
{
    return sizeof(result_type);
}
std::size_t header_alignof_scan_result()
{
    return alignof(result_type);
}
