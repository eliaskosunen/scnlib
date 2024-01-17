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

#include <scn/detail/scanner.h>
#include <scn/impl/reader/integer_reader.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
template <typename CharT>
class reader_impl_for_voidptr {
public:
    constexpr reader_impl_for_voidptr() = default;

    bool skip_ws_before_read() const
    {
        return true;
    }

    static scan_error check_specs(const detail::format_specs& specs)
    {
        reader_error_handler eh{};
        detail::check_pointer_type_specs(specs, eh);
        if (SCN_UNLIKELY(!eh)) {
            return {scan_error::invalid_format_string, eh.m_msg};
        }
        return {};
    }

    template <typename Range>
    scan_expected<ranges::iterator_t<Range>>
    read_default(Range range, void*& value, detail::locale_ref loc)
    {
        detail::format_specs specs{};
        specs.type = detail::presentation_type::int_hex;

        std::uintptr_t intvalue{};
        SCN_TRY(result, reader_impl_for_int<CharT>{}.read_specs(range, specs,
                                                                intvalue, loc));
        value = reinterpret_cast<void*>(intvalue);
        return result;
    }

    template <typename Range>
    scan_expected<ranges::iterator_t<Range>> read_specs(
        Range range,
        const detail::format_specs& specs,
        void*& value,
        detail::locale_ref loc)
    {
        SCN_UNUSED(specs);
        return read_default(range, value, loc);
    }
};
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
