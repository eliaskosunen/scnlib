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

#include <bits/ranges_base.h>
#include <scn/impl/algorithms/read_code_points.h>
#include <scn/impl/reader/common.h>
#include "scn/fwd.h"
#include "scn/util/expected.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename SourceCharT>
        class escaped_character_reader_impl {
        public:
            escaped_character_reader_impl(
                std::basic_string<SourceCharT>& buffer)
                : m_buffer(buffer)
            {
            }

            template <typename SourceRange>
            scan_expected<iterator_value_result<ranges::iterator_t<SourceRange>,
                                                code_point>>
            read_single(SourceRange& source, SourceCharT delimeter)
            {
                if constexpr (ranges::contiguous_range<SourceRange>) {
                    return read_char_nocopy(source, delimeter);
                }
                else {
                    return read_char_copying(source, delimeter);
                }
            }

        private:
            template <typename Iterator>
            scan_error check_delimeter(Iterator& it, SourceCharT delimeter)
            {
                if (SCN_UNLIKELY(*it != delimeter)) {
                    return scan_error{scan_error::invalid_scanned_value,
                                      "Expected delimeter"};
                }
                ++it;
                return {};
            }

            template <typename Iterator, typename Sentinel>
            scan_expected<code_point> check_escaped(Iterator& it,
                                                    Sentinel end,
                                                    SourceCharT delimeter)
            {
                SCN_EXPECT(*it == SourceCharT{'\\'});
                ++it;
                if (SCN_UNLIKELY(it == end)) {
                    return unexpected(
                        scan_error{scan_error::end_of_range, "EOF"});
                }

                switch (*it) {
                    case SourceCharT{'t'}:
                        ++it;
                        return code_point{'\t'};
                    case SourceCharT{'n'}:
                        ++it;
                        return code_point{'\n'};
                    case SourceCharT{'r'}:
                        ++it;
                        return code_point{'\r'};
                    case delimeter:
                        ++it;
                        return code_point{delimeter};
                    case SourceCharT{'\\'}:
                        ++it;
                        return code_point{'\\'};
                    case SourceCharT{'x'}:
                    case SourceCharT{'u'}:
                        // TODO
                    case SourceCharT{'"'}:
                    case SourceCharT{'\''}:
                    case SourceCharT{'b'}:
                    case SourceCharT{'v'}:
                        // TODO?
                        ;
                    default:
                        // TODO: error
                        ;
                }
            }

            template <typename SourceRange>
            scan_expected<iterator_value_result<ranges::iterator_t<SourceRange>,
                                                code_point>>
            read_char_nocopy(SourceRange& source, SourceCharT delimeter)
            {
                auto it = ranges::begin(source);
                if (auto e = check_delimeter(it, delimeter); !e) {
                    return unexpected(e);
                }
                if (SCN_UNLIKELY(it == ranges::end(source))) {
                    return scan_error{scan_error::end_of_range, "EOF"};
                }

                if (*it == SourceCharT{'\\'}) {}
            }

            template <typename SourceRange>
            scan_expected<iterator_value_result<ranges::iterator_t<SourceRange>,
                                                code_point>>
            read_char_copying(SourceRange& source, SourceCharT delimeter)
            {
            }

            std::basic_string<SourceCharT>& m_buffer;
        };
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
