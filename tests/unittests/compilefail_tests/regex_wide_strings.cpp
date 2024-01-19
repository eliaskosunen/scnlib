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

#include <scn/regex.h>
#include <scn/scan.h>
#include <scn/xchar.h>

int main()
{
#if !SCN_REGEX_SUPPORTS_WIDE_STRINGS
    // build error: Regex backend doesn't support wide strings as input
    auto result = scn::scan<std::wstring>(L"42", SCN_STRING(L"{:/[a-z]+/}"));
    return result && result->value() == L"42";
#else
    static_assert(false, "Regex backend doesn't support wide strings as input");
#endif
}
