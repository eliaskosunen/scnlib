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
#include <vector>

template <typename T>
void run(const std::string& source, T& val)
{
    auto wrapped = scn::wrap(source);
    while (true) {
        auto ret = scn::scan(wrapped, scn::default_tag, val);
        wrapped = ret.range();
        if (!ret) {
            break;
        }
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string source(reinterpret_cast<const char*>(data), size);

    std::string str;
    run(source, str);

    char ch;
    run(source, ch);

    int i;
    run(source, i);

    double d;
    run(source, d);

    long long ll;
    run(source, ll);

    unsigned u;
    run(source, u);

    std::vector<char> vec(32, 0);
    auto s = scn::make_span(vec);
    run(source, s);

    // empty format string would loop forever
    if (size != 0) {
        auto wrapped = scn::wrap(source);
        std::string str{};
        while (wrapped.size() != 0) {
            auto f = scn::string_view(wrapped.data(), wrapped.size());
            auto ret = scn::scan(wrapped, f, str);
            wrapped = ret.range();
            if (!ret) {
                break;
            }
        }
    }

    return 0;
}
