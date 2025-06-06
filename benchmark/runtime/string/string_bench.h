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

#include "bench_helpers.h"

#include <scn/impl.h>

#include <fstream>
#include <random>
#include <sstream>
#include <vector>

template <typename CharT>
std::basic_string<CharT> make_benchmark_string(const std::string& sourcefile)
{
    std::ifstream fstr{sourcefile};
    std::stringstream ss;
    ss << fstr.rdbuf();

    if constexpr (sizeof(CharT) == 1) {
        return ss.str();
    }
    else {
        std::basic_string<CharT> str;
        scn::impl::transcode_to_string(
            std::string_view{ss.str().data(), ss.str().size()}, str);
        return str;
    }
}

struct lipsum_tag {};
struct unicode_tag {};

template <typename CharT, typename Tag>
std::basic_string<CharT> get_benchmark_input()
{
    if (std::is_same_v<Tag, lipsum_tag>)
        return make_benchmark_string<CharT>("lipsum.txt");
    return make_benchmark_string<CharT>("unicode.txt");
}
