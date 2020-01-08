// Copyright 2017-2019 Elias Kosunen
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

#include <scn/scn.h>

#include <iostream>

#if SCN_HAS_INCLUDE(<string_view>) && SCN_STD >= SCN_STD_17
#include <string_view>

using namespace std::string_view_literals;  // For sv

int main()
{
    auto str = "Hello world!"sv;

    std::string word;
    scn::scan(str, "{}", word);

    std::cout << word << '\n';  // Will output "Hello"
    std::cout << str << '\n';   // Will output " world!"
}
#else
int main() {
    std::cout << "No string_view :(\n";
}
#endif
