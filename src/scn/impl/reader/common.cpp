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

#include <scn/impl/reader/common.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wexit-time-destructors")

        template <typename CharT>
        std::basic_string<CharT>& source_reader_buffer()
        {
            static std::basic_string<CharT> buffer;
            return buffer;
        }

        SCN_CLANG_POP

        template std::string& source_reader_buffer();
        template std::wstring& source_reader_buffer();
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
