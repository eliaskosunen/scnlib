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

#ifndef SCN_DETAIL_RANGES_CONFIG_H
#define SCN_DETAIL_RANGES_CONFIG_H

#ifdef SCN_RANGES_USE_RANGEV3
#define SCN_RANGES_RANGEV3 1
#define SCN_RANGES_CMCSTL2 0
#endif

#ifdef SCN_RANGES_USE_CMCSTL2
#define SCN_RANGES_RANGEV3 0
#define SCN_RANGES_CMCSTL2 1
#endif

#ifndef SCN_RANGES_RANGEV3
#define SCN_RANGES_RANGEV3 1
#define SCN_RANGES_CMCSTL2 0
#endif

#if SCN_RANGES_RANGEV3
#include <range/v3/core.hpp>
#define SCN_RANGES_NS ::ranges
#endif

#if SCN_RANGES_CMCSTL2
#include <stl2/algorithm.hpp>
#include <stl2/concepts.hpp>
#include <stl2/iterator.hpp>
#include <stl2/ranges.hpp>
#include <stl2/type_traits.hpp>
#include <stl2/utility.hpp>
#define SCN_RANGES_NS ::__stl2
#endif

#endif
