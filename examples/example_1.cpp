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
    if (const auto result =
            scn::prompt<int>("What's your favorite number? ", "{}")) {
        // `result` is true, we have a successful read.
        // Read the single parsed value with result->value()
        std::printf("%d, interesting\n", result->value());
    }
    else {
        std::puts("Well, never mind then.");
    }
}
