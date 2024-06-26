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

#include <scn/ranges.h>
#include <scn/scan.h>

#include <map>
#include <vector>

int main()
{
    std::string source{R"([{1: 2, 3: 4}, {5: 6}])"};

    // Use monadic interface of scn::expected (return type of scn::scan)
    scn::scan<std::vector<std::map<int, int>>>(source, "{}")
        .transform([](auto result) {
            const auto& data = result.value();
            const auto& a = *(data[0].begin());
            const auto& b = *std::next(data[0].begin());
            const auto& c = *(data[1].begin());

            std::printf(R"([{%d: %d, %d: %d}, {%d: %d}])", a.first, a.second,
                        b.first, b.second, c.first, c.second);
        })
        .transform_error([](scn::scan_error) { std::puts("failure"); });
}
