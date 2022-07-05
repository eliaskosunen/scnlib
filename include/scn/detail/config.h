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

#pragma once

#include <scn/detail/pp_detect.h>

#define SCN_VERSION SCN_COMPILER(2, 0, 0)

// SCN_USE_EXCEPTIONS
#ifndef SCN_USE_EXCEPTIONS
#define SCN_USE_EXCEPTIONS 1
#endif

// SCN_USE_TRIVIAL_ABI
#ifndef SCN_USE_TRIVIAL_ABI
#define SCN_USE_TRIVIAL_ABI 1
#endif

// SCN_USE_STD_RANGES
#ifndef SCN_USE_STD_RANGES
#define SCN_USE_STD_RANGES 1
#endif

// SCN_USE_IOSTREAMS
#ifndef SCN_USE_IOSTREAMS
#define SCN_USE_IOSTREAMS 1
#endif
