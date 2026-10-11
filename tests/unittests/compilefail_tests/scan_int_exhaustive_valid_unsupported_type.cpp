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

#include <scn/scan.h>

#if SCN_HAS_INT128
using unsupported_type = scn::int128;
#else
using unsupported_type = char16_t;
#endif

int main()
{
    // build error: no matching function
    auto value = scn::scan_int_exhaustive_valid<unsupported_type>("123");
    return value != 123;
}
