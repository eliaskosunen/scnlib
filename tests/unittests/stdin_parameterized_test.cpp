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

#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
std::string to_hex(std::string_view val)
{
    std::ostringstream oss;
    bool first{true};
    for (char ch : val) {
        if (!first) {
            oss << ' ';
        }
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(ch);
        first = false;
    }
    return oss.str();
}

std::string to_hex(int val)
{
    return to_hex(std::to_string(val));
}

template <typename T>
int do_scan(std::string_view format)
{
    auto result = scn::input<T>(scn::runtime_format(format));
    if (result) {
        std::cout << to_hex(result->value());
    } else {
        std::cerr << "Error: " << result.error().msg() << std::endl;
    }
    std::cout << std::endl;

    if (auto leftovers = scn::input<std::string>("{:[^\n]}"); leftovers) {
        std::cout << to_hex(leftovers->value());
    }
    std::cout << std::endl;

    return result ? 0 : 1;
}
}  // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "argc must be 3, got %d", argc);
        return -1;
    }

    auto type = scn::scan_int<int>(argv[1]);
    if (!type) {
        std::fputs("Invalid type parameter", stderr);
        return -1;
    }

    if (type->value() == 0) {
        return do_scan<std::string>(argv[2]);
    }
    if (type->value() == 1) {
        return do_scan<int>(argv[2]);
    }
    std::fprintf(stderr, "Invalid value for the type parameter (got %d)",
                 type->value());
    return -1;
}
