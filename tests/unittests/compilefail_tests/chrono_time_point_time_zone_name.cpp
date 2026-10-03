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

#include <scn/chrono.h>
#include <scn/scan.h>

int main()
{
    // build error: Timezone names (%Z) not supported with time_point
    using sys_seconds = std::chrono::time_point<std::chrono::system_clock,
                                                std::chrono::seconds>;
    auto result = scn::scan<sys_seconds>("2024 UTC", SCN_STRING("{:%Y %Z}"));
    return result.has_value();
}
