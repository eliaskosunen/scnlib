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

int main()
{
    std::string source{
        "1 2 3 4 5 6 7 8 9 11 22 33 44 55 66 77 88 99 111 222 333 444 555 666 "
        "777 888 999"};
    auto range = scn::scan_map_input_range(source);
    std::vector<int> integers;

    while (true) {
        auto [result, i] = scn::scan<int>(range, "{}");
        if (!result) {
            break;
        }
        integers.push_back(i);
        range = result.range();
    }

    for (auto i : integers) {
        std::printf("%d\n", i);
    }
}
