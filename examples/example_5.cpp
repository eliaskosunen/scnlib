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
    std::puts("Write two integers:");

    if (auto result = scn::scan<int>(stdin, "{}")) {
        if (auto second_result = scn::scan<int>(result->file(), "{}")) {
            std::printf("Two integers: %d %d\n", result->value(),
                        second_result->value());
            return 0;
        }

        std::string buf{};
        buf.resize(256);
        std::ignore = std::fgets(buf.data(), 255, stdin);

        std::printf("First integer: %d, rest of the line: %s", result->value(),
                    buf.c_str());
        return 0;
    }

    std::string buf{};
    buf.resize(256);
    std::ignore = std::fgets(buf.data(), 255, stdin);

    std::printf("Entire line: %s", buf.c_str());
}
