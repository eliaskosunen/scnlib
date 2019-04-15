
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

#ifndef SCN_DETAIL_VSCAN_H
#define SCN_DETAIL_VSCAN_H

#include "stream.h"
#include "visitor.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename CharT>
    using basic_erased_stream_context = basic_context<erased_stream<CharT>>;
    template <typename CharT>
    using basic_erased_sized_stream_context =
        basic_context<erased_sized_stream<CharT>>;

    using erased_stream_context = basic_erased_stream_context<char>;
    using werased_stream_context = basic_erased_stream_context<wchar_t>;
    using erased_sized_stream_context = basic_erased_sized_stream_context<char>;
    using werased_sized_stream_context =
        basic_erased_sized_stream_context<wchar_t>;

    template <typename Stream>
    struct erased_stream_context_type {
        using char_type = typename Stream::char_type;
        using type = typename std::conditional<
            is_sized_stream<Stream>::value,
            basic_erased_sized_stream_context<char_type>,
            basic_erased_stream_context<char_type>>::type;
    };

    result<int> vscan(erased_stream_context&);
    result<int> vscan(werased_stream_context&);
    result<int> vscan(erased_sized_stream_context&);
    result<int> vscan(werased_sized_stream_context&);

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_VSCAN_H

