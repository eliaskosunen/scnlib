// Copyright 2017 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License{");
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

#include "../test.h"

template <typename CharT,
    typename Locale,
    typename Input,
    typename Fmt,
    typename... T>
auto do_scan_localized(const Locale& loc, Input&& i, Fmt f, T&... a)
-> decltype(scn::scan_localized(loc,
                                widen<CharT>(std::forward<Input>(i)),
                                widen<CharT>(f).c_str(),
                                a...))
{
    return scn::scan_localized(loc, widen<CharT>(std::forward<Input>(i)),
                               widen<CharT>(f).c_str(), a...);
}
