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

#include <scn/impl/algorithms/common.h>
#include <scn/impl/unicode/utf16.h>
#include <scn/impl/unicode/utf8.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {

        template <typename Range>
        scan_expected<iterator_value_result<ranges::borrowed_iterator_t<Range>,
                                            span<char>>>
        read_utf8_code_point(Range&& range, span<char> buf)
        {
            SCN_EXPECT(!ranges::empty(range));
            SCN_EXPECT(buf.size() >= 4);

            auto it = ranges::begin(range);
            const auto first = *it;
            buf[0] = first;
            ++it;

            const auto len = code_point_length(first);
            if (!len) {
                return unexpected(len.error());
            }

            if (*len == 1) {
                return {{it, buf.first(1)}};
            }

            for (std::size_t i = 1; i < *len; ++i) {
                if (it == ranges::end(range)) {
                    return unexpected_scan_error(
                        scan_error::invalid_encoding,
                        "EOF in the middle of UTF-8 code point");
                }

                const auto ch = *it;
                buf[i] = ch;
                ++it;
            }
            return {{it, buf.first(*len)}};
        }

        template <typename Range>
        scan_expected<iterator_value_result<ranges::borrowed_iterator_t<Range>,
                                            span<char16_t>>>
        read_utf16_code_point(Range&& range, span<char16_t> buf)
        {
            SCN_EXPECT(!ranges::empty(range));
            SCN_EXPECT(buf.size() >= 2);

            auto it = ranges::begin(range);
            const auto first = *it;
            buf[0] = static_cast<char16_t>(first);
            ++it;

            if (utf16::code_point_length(first) == 1) {
                return {{it, buf.first(1)}};
            }

            if (it == ranges::end(range)) {
                return unexpected_scan_error(
                    scan_error::invalid_encoding,
                    "EOF in the middle of UTF-16 code point");
            }

            buf[1] = static_cast<char16_t>(*it);
            ++it;
            return {{it, buf.first(2)}};
        }

        template <typename Range>
        scan_expected<iterator_value_result<ranges::borrowed_iterator_t<Range>,
                                            span<char32_t>>>
        read_utf32_code_point(Range&& range, span<char32_t> buf)
        {
            SCN_EXPECT(!ranges::empty(range));
            SCN_EXPECT(buf.size() != 0);

            buf[0] = static_cast<char32_t>(*ranges::begin(range));
            return {{ranges::next(ranges::begin(range)), buf.first(1)}};
        }

        template <typename Range, typename CharT>
        scan_expected<iterator_value_result<ranges::borrowed_iterator_t<Range>,
                                            span<CharT>>>
        read_code_point(Range&& range, span<CharT> buf)
        {
            static_assert(!std::is_const_v<CharT>);

            if (auto e = eof_check(range); !e) {
                return unexpected(e);
            }

            if constexpr (std::is_same_v<CharT, char>) {
                return read_utf8_code_point(SCN_FWD(range), buf);
            }
            else if constexpr (sizeof(CharT) == 2) {
                return read_utf16_code_point(SCN_FWD(range), buf);
            }
            else if constexpr (sizeof(CharT) == 4) {
                return read_utf32_code_point(SCN_FWD(range), buf);
            }
            else {
                static_assert(scn::detail::dependent_false<CharT>::value,
                              "Nonsensical CharT in read_code_point");
            }
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
