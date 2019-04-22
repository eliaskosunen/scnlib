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

#include <scn/ranges.h>
#include <scn/scn.h>

template <typename T>
void run(const std::string& source, T& val)
{
    auto stream = scn::make_stream(source);
    while (true) {
        auto ret = scn::scan_default(stream, val);
        if (!ret) {
            if (ret.error() == scn::error::end_of_stream) {
                break;
            }
        }
    }
}

template <typename T>
void run_range(const std::string& source, T& val)
{
    while (true) {
        auto ret = scn::ranges::scan(source, "{}", val);
        if (!ret) {
            if (ret.error() == scn::error::end_of_stream) {
                break;
            }
        }
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string source(reinterpret_cast<const char*>(data), size);

    std::string str;
    run(source, str);
    run_range(source, str);

    char ch;
    run(source, ch);
    run_range(source, ch);

    int i;
    run(source, i);
    run_range(source, i);

    double d;
    run(source, d);
    run_range(source, d);

    long long ll;
    run(source, ll);
    run_range(source, ll);

    unsigned u;
    run(source, u);
    run_range(source, u);

    std::vector<char> vec(32, 0);
    auto s = scn::make_span(vec);
    run(source, s);
    run_range(source, s);

    {
        std::string source(reinterpret_cast<const char*>(data), size);
        auto stream = scn::make_stream(source);
        std::string str{};
        while (true) {
            auto f = scn::string_view(source.data(), source.size());
            auto ret = scn::scan(stream, f, str);
            if (!ret) {
                if (ret.error() == scn::error::end_of_stream) {
                    break;
                }
            }
        }
    }

    return 0;
}
