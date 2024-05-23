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

#include "fuzz.h"

#include <scn/impl.h>

namespace {
template <typename Cb>
void do_find(std::string_view sv, Cb cb)
{
    auto it = sv.begin();
    while (it != sv.end()) {
        SCN_EXPECT(it < sv.end());
        auto in = std::string_view{&*it, static_cast<size_t>(sv.end() - it)};
        SCN_EXPECT(!in.empty());
        it = cb(in);
        SCN_ENSURE(it <= sv.end());
        if (it != sv.end())
            ++it;
        SCN_ENSURE(it <= sv.end());
    }
}
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    auto sv = std::string_view{reinterpret_cast<const char*>(data), size};
    do_find(sv, scn::impl::find_classic_space_narrow_fast);
    do_find(sv, scn::impl::find_classic_nonspace_narrow_fast);
    do_find(sv, scn::impl::find_nondecimal_digit_narrow_fast);

    std::wstring widened{};
    scn::impl::transcode_to_string(sv, widened);

    std::string narrowed{};
    scn::impl::transcode_to_string(std::wstring_view{widened}, narrowed);

    return 0;
}
