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

#include <scn/scn.h>
#include <sstream>
#include <vector>

template <typename T>
void run(scn::string_view source)
{
    auto result = scn::make_result(source);
    T val{};
    while (true) {
        result = scn::scan_default(result.range(), val);
        if (!result) {
            break;
        }
    }
}

template <typename T>
void roundtrip(scn::string_view data)
{
    if (data.size() < sizeof(T)) {
        return;
    }
    T original_value{};
    std::memcpy(&original_value, data.data(), sizeof(T));

    std::ostringstream oss;
    oss << original_value;
    auto source = std::move(oss).str();

    T value{};
    auto result = scn::scan_default(source, value);
    if (!result) {
        throw std::runtime_error("Failed to read");
    }
    if (value != original_value) {
        throw std::runtime_error("Roundtrip failure");
    }
    if (!result.range().empty()) {
        throw std::runtime_error("Unparsed input");
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    scn::string_view source(reinterpret_cast<const char*>(data), size);

    run<char>(source);
    run<int>(source);
    run<long long>(source);
    run<unsigned>(source);
    run<double>(source);
    run<std::string>(source);
    run<scn::string_view>(source);

    roundtrip<int>(source);
    roundtrip<long long>(source);
    roundtrip<unsigned>(source);
    // need to fix float roundtripping at some point
    //roundtrip<double>(source);

    if (size != 0) {
        auto result = scn::make_result(source);
        std::string str{};
        while (!result.range().empty()) {
            auto f =
                scn::string_view{result.range().data(),
                                 static_cast<size_t>(result.range().size())};
            result = scn::scan(result.range(), f, str);
            if (!result) {
                break;
            }
        }
    }

    return 0;
}
