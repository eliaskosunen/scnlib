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

namespace scn::fuzz {
    namespace {
        template <typename T, typename Source>
        void run_for_type(Source& source)
        {
            {
                auto _ = scn::scan<T>(source, source);
            }
            {
                auto _ = scn::scan<T>(global_locale, source, source);
            }
        }

        template <typename Source>
        void run_for_source(Source& source)
        {
            using char_type = ranges::range_value_t<Source>;

            run_for_type<char_type>(source);
            run_for_type<int>(source);
            run_for_type<unsigned>(source);
            run_for_type<double>(source);
            run_for_type<bool>(source);
            run_for_type<void*>(source);
            run_for_type<std::string>(source);
            run_for_type<std::wstring>(source);
            run_for_type<std::basic_string_view<char_type>>(source);
        }

        void run(span<const uint8_t> data)
        {
            if (data.size() > max_input_bytes || data.size() == 0) {
                return;
            }

            auto [sv, wsv_direct, wsv_reinterpret, wsv_transcode] =
                make_input_views(data);

            run_for_source(sv);
            run_for_source(wsv_direct);
            run_for_source(wsv_reinterpret);
            run_for_source(wsv_transcode);
        }
    }  // namespace
}  // namespace scn::fuzz

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    scn::fuzz::run({data, size});
    return 0;
}
