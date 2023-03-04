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

#include "scn/detail/format_string_parser.h"

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")

#include <gmock/gmock.h>

SCN_GCC_POP

template <typename CharT>
class value_reader_interface {
public:
    virtual ~value_reader_interface() = default;

    virtual void make_value_reader() = 0;
    virtual void make_value_reader(uint8_t flags1, uint8_t flags2 = 0) = 0;
    virtual void make_value_reader_from_specs(
        const scn::detail::basic_format_specs<CharT>&) = 0;

    [[nodiscard]] virtual bool is_localized() const = 0;
};
