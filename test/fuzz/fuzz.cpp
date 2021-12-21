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

template <typename T, typename C>
void run(scn::basic_string_view<C> source)
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

#define SCN_FUZZ_RUN_BASIC(source, Char)                      \
    do {                                                      \
        run<Char>(source);                                    \
        run<bool>(source);                                    \
        run<short>(source);                                   \
        run<int>(source);                                     \
        run<long>(source);                                    \
        run<long long>(source);                               \
        run<unsigned short>(source);                          \
        run<unsigned int>(source);                            \
        run<unsigned long>(source);                           \
        run<unsigned long long>(source);                      \
        run<float>(source);                                   \
        run<double>(source);                                  \
        run<long double>(source);                             \
        run<std::basic_string<Char>>(source);                 \
        run<scn::basic_string_view<Char>>(source);            \
    } while (false)

#define SCN_FUZZ_RUN_ROUNDTRIP(source, Char)                      \
    do {                                                      \
        run<Char>(source);                                    \
        run<bool>(source);                                    \
        run<short>(source);                                   \
        run<int>(source);                                     \
        run<long>(source);                                    \
        run<long long>(source);                               \
        run<unsigned short>(source);                          \
        run<unsigned int>(source);                            \
        run<unsigned long>(source);                           \
        run<unsigned long long>(source);                      \
        run<std::basic_string<Char>>(source);                 \
        run<scn::basic_string_view<Char>>(source);            \
    } while (false)

#define SCN_FUZZ_RUN_FORMAT(source, Char)                    \
    do {                                                     \
        auto result = scn::make_result(source);              \
        std::basic_string<Char> str{};                       \
        while (!result.range().empty()) {                    \
            auto f = scn::basic_string_view<Char>{           \
                result.range().data(),                       \
                static_cast<size_t>(result.range().size())}; \
            result = scn::scan(result.range(), f, str);      \
            if (!result) {                                   \
                break;                                       \
            }                                                \
        }                                                    \
    } while (false)

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // a b c d
    scn::string_view source(reinterpret_cast<const char*>(data), size);

    // ab cd
    const auto wdata1_size = size < sizeof(wchar_t) ? 1 : (size / sizeof(wchar_t));
    std::wstring wdata1(wdata1_size, 0);
    std::memcpy(reinterpret_cast<char*>(&wdata1[0]), data, size);
    scn::wstring_view wsource1(wdata1.data(), wdata1.size());

    // a b c d
    std::wstring wdata2(size, 0);
    std::copy(data, data + size, &wdata2[0]);
    scn::wstring_view wsource2(wdata2.data(), size);

    {
        SCN_FUZZ_RUN_BASIC(source, char);
        SCN_FUZZ_RUN_ROUNDTRIP(source, char);
    }

    {
        SCN_FUZZ_RUN_BASIC(wsource1, wchar_t);
        SCN_FUZZ_RUN_ROUNDTRIP(wsource1, wchar_t);
    }

    {
        SCN_FUZZ_RUN_BASIC(wsource2, wchar_t);
        SCN_FUZZ_RUN_ROUNDTRIP(wsource2, wchar_t);
    }

    // format string
    if (size != 0) {
        // narrow
        {
            SCN_FUZZ_RUN_FORMAT(source, char);
        }
        // wide1
        {
            SCN_FUZZ_RUN_FORMAT(wsource1, wchar_t);
        }
        // wide2
        {
            SCN_FUZZ_RUN_FORMAT(wsource2, wchar_t);
        }
    }

    return 0;
}
