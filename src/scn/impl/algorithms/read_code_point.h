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
#include <scn/impl/unicode/unicode.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename Range>
        scan_expected<ranges::borrowed_iterator_t<Range>>
        read_code_point_impl_nocopy(Range&& range, span<unsigned char> buf)
        {
            static_assert(range_supports_nocopy<Range>());

            static constexpr auto char_size =
                sizeof(ranges::range_value_t<Range>);

            if (SCN_UNLIKELY(range_nocopy_size(range) * char_size <
                             buf.size())) {
                return unexpected_scan_error(scan_error::invalid_encoding,
                                             "EOF in the middle of code point");
            }

            std::memcpy(buf.data(), range_nocopy_data(range), buf.size());
            return ranges::next(
                ranges::begin(range),
                static_cast<std::ptrdiff_t>(buf.size() / char_size));
        }

        template <typename Range, typename U8>
        scan_expected<
            iterator_value_result<ranges::borrowed_iterator_t<Range>, span<U8>>>
        read_utf8_code_point(Range&& range, span<U8> buf)
        {
            static_assert(sizeof(U8) == 1);
            SCN_EXPECT(!ranges::empty(range));
            SCN_EXPECT(buf.size() >= 4);

            auto it = ranges::begin(range);
            const auto first = *it;
            buf[0] = static_cast<U8>(first);
            ++it;

            const auto len = code_point_length_by_starting_code_unit(first);
            if (SCN_UNLIKELY(!len)) {
                return unexpected(len.error());
            }

            if (*len == 1) {
                return {{it, buf.first(1)}};
            }

            if constexpr (range_supports_nocopy<Range>()) {
                auto s = span<unsigned char>{
                    reinterpret_cast<unsigned char*>(buf.data()), *len};
                return read_code_point_impl_nocopy(SCN_FWD(range), s)
                    .transform([s](auto iter) {
                        return iterator_value_result<
                            ranges::borrowed_iterator_t<Range>, span<U8>>{
                            iter, {reinterpret_cast<U8*>(s.data()), s.size()}};
                    });
            }
            else {
                SCN_EXPECT(*len <= 4);
                for (std::size_t i = 1; i < *len; ++i) {
                    if (SCN_UNLIKELY(it == ranges::end(range))) {
                        return unexpected_scan_error(
                            scan_error::invalid_encoding,
                            "EOF in the middle of UTF-8 code point");
                    }

                    const auto ch = *it;

                    SCN_GCC_PUSH
                    SCN_GCC_IGNORE("-Wstringop-overflow")
                    buf[i] = static_cast<U8>(ch);
                    SCN_GCC_POP

                    ++it;
                }
                return {{it, buf.first(*len)}};
            }
        }

        template <typename Range, typename U16>
        scan_expected<iterator_value_result<ranges::borrowed_iterator_t<Range>,
                                            span<U16>>>
        read_utf16_code_point(Range&& range, span<U16> buf)
        {
            static_assert(sizeof(U16) == 2);
            SCN_EXPECT(!ranges::empty(range));
            SCN_EXPECT(buf.size() >= 2);

            auto it = ranges::begin(range);
            const auto first = *it;
            buf[0] = static_cast<U16>(first);
            ++it;

            const auto len = code_point_length_by_starting_code_unit(first);
            if (SCN_UNLIKELY(!len)) {
                return unexpected(len.error());
            }
            if (*len == 1) {
                return {{it, buf.first(1)}};
            }

            if (SCN_UNLIKELY(it == ranges::end(range))) {
                return unexpected_scan_error(
                    scan_error::invalid_encoding,
                    "EOF in the middle of UTF-16 code point");
            }

            buf[1] = static_cast<U16>(*it);
            ++it;
            return {{it, buf.first(2)}};
        }

        template <typename Range, typename U32>
        scan_expected<iterator_value_result<ranges::borrowed_iterator_t<Range>,
                                            span<U32>>>
        read_utf32_code_point(Range&& range, span<U32> buf)
        {
            static_assert(sizeof(U32) == 4);
            SCN_EXPECT(!ranges::empty(range));
            SCN_EXPECT(buf.size() != 0);

            buf[0] = static_cast<U32>(*ranges::begin(range));
            return {{ranges::next(ranges::begin(range)), buf.first(1)}};
        }

        template <typename Range, typename CharT>
        scan_expected<iterator_value_result<ranges::borrowed_iterator_t<Range>,
                                            span<CharT>>>
        read_code_point(Range&& range, span<CharT> buf)
        {
            static_assert(!std::is_const_v<CharT>);

            if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
                return unexpected(e);
            }

            if constexpr (sizeof(CharT) == 1) {
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
