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
#include <scn/impl/reader/integer/reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        class pointer_reader {
        public:
            constexpr pointer_reader() = default;

            bool skip_ws_before_read() const
            {
                return true;
            }

            static scan_error check_specs(
                const detail::basic_format_specs<CharT>& specs)
            {
                reader_error_handler eh{};
                detail::check_pointer_type_specs(specs, eh);
                if (!eh) {
                    return {scan_error::invalid_format_string, eh.m_msg};
                }
                return {};
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_default(
                Range& range,
                void*& value,
                detail::locale_ref loc)
            {
                detail::basic_format_specs<CharT> specs{};
                specs.type = detail::presentation_type::int_hex;

                std::uintptr_t intvalue{};
                return int_reader<std::uintptr_t, CharT>{}
                    .read_value_specs(range, specs, intvalue, loc)
                    .transform([&](auto result) SCN_NOEXCEPT {
                        value = reinterpret_cast<void*>(intvalue);
                        return result;
                    });
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_specs(
                Range& range,
                const detail::basic_format_specs<CharT>& specs,
                void*& value,
                detail::locale_ref loc)
            {
                SCN_UNUSED(specs);
                return read_value_default(range, value, loc);
            }
        };

        template <typename CharT>
        class reader<void*, CharT> : public pointer_reader<CharT> {};
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
