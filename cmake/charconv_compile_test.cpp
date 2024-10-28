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

#include <charconv>
#include <cstdio>
#include <string_view>

int main() {
    std::string_view input("3.14");
    double value{};
    auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), value);
    if (ec != std::errc{}) {
        std::fprintf(stderr, "std::from_chars failed with error code %d\n", static_cast<int>(ec));
        return 1;
    }
    if (ptr != input.data() + input.size()) {
        std::fprintf(stderr, "std::from_chars failed with invalid pointer: %p, expected %p\n", ptr, input.data() + input.size());
        return 1;
    }
    return 0;
}
