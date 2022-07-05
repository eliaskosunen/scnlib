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

#include <scn/detail/scan.h>

#include <iostream>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        scan_error handle_error(scan_error e)
        {
            return e;
        }

        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wexit-time-destructors")
        scn::istreambuf_view& internal_narrow_stdin()
        {
            static scn::istreambuf_view range{std::cin};
            return range;
        }
        scn::wistreambuf_view& internal_wide_stdin()
        {
            static scn::wistreambuf_view range{std::wcin};
            return range;
        }
        SCN_CLANG_POP

        erased_range_impl_base::~erased_range_impl_base() = default;
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
