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

module;

#define SCNLIB_MODULES
#include <scn/chrono.h>

export module scn.chrono;

export namespace scn {
    using scn::weekday;
    using scn::day;
    using scn::month;
    using scn::year;

    using scn::month_day;
    using scn::year_month;
    using scn::year_month_day;

    using scn::Sunday;
    using scn::Monday;
    using scn::Tuesday;
    using scn::Wednesday;
    using scn::Thursday;
    using scn::Friday;
    using scn::Saturday;

    using scn::January;
    using scn::February;
    using scn::March;
    using scn::April;
    using scn::May;
    using scn::June;
    using scn::July;
    using scn::August;
    using scn::September;
    using scn::October;
    using scn::November;
    using scn::December;

    using scn::datetime_components;
    using scn::tm_with_tz;
}
