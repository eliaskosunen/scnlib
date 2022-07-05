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
#include <scn/impl/algorithms/read_code_point.h>
#include <scn/impl/reader/common.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        class code_unit_reader {
        public:
            template <typename SourceRange>
            scan_expected<ranges::iterator_t<SourceRange>> read(
                SourceRange& range,
                CharT& ch)
            {
                auto it = ranges::begin(range);
                ch = *it;
                return {++it};
            }
        };

        template <typename CharT>
        class code_point_reader;

        template <>
        class code_point_reader<code_point> {
        public:
            template <typename SourceRange>
            scan_expected<ranges::iterator_t<SourceRange>> read(
                SourceRange& range,
                code_point& cp)
            {
                using char_type =
                    char_type_for_encoding<ranges::range_value_t<SourceRange>>;
                alignas(char32_t) char_type buffer[4 / sizeof(char_type)]{};
                const auto read_result = read_code_point(
                    range, span<char_type>{buffer, 4 / sizeof(char_type)});
                if (!read_result) {
                    return unexpected(read_result.error());
                }

                const auto decode_sv = std::basic_string_view<char_type>{
                    read_result->value.data(), read_result->value.size()};
                const auto decode_result = decode_code_point(decode_sv, cp);
                if (!decode_result) {
                    return unexpected(decode_result.error());
                }
                return read_result->iterator;
            }
        };

        template <>
        class code_point_reader<wchar_t> {
        public:
            template <typename SourceRange>
            scan_expected<ranges::iterator_t<SourceRange>> read(
                SourceRange& range,
                wchar_t& ch)
            {
                code_point_reader<code_point> reader{};
                code_point cp{};
                auto ret = reader.read(range, cp);
                if (!ret) {
                    return unexpected(ret.error());
                }

                if constexpr (sizeof(wchar_t) < sizeof(code_point)) {
                    if (static_cast<uint32_t>(cp) >=
                        static_cast<uint32_t>(
                            std::numeric_limits<wchar_t>::max())) {
                        return unexpected_scan_error(
                            scan_error::value_out_of_range,
                            "Can't fit scanned code point into wchar_t");
                    }
                }
                ch = static_cast<wchar_t>(cp);
                return ret;
            }
        };

        template <typename SourceCharT, typename ValueCharT>
        class char_reader_base {
        public:
            constexpr char_reader_base() = default;

            bool skip_ws_before_read() const
            {
                return false;
            }

            static scan_error check_specs(
                const detail::basic_format_specs<SourceCharT>& specs)
            {
                reader_error_handler eh{};
                detail::check_char_type_specs(specs, eh);
                if (!eh) {
                    return {scan_error::invalid_format_string, eh.m_msg};
                }
                return {};
            }
        };

        template <typename CharT>
        class reader<char, CharT> : public char_reader_base<CharT, char> {
        public:
            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_default(
                Range& range,
                char& value,
                detail::locale_ref loc)
            {
                SCN_UNUSED(loc);
                if constexpr (std::is_same_v<CharT, char>) {
                    return code_unit_reader<char>{}.read(range, value);
                }
                else {
                    SCN_EXPECT(false);
                    SCN_UNREACHABLE;
                }
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_specs(
                Range& range,
                const detail::basic_format_specs<CharT>& specs,
                char& value,
                detail::locale_ref loc)
            {
                // TODO: do something with specs
                SCN_UNUSED(specs);
                return read_value_default(range, value, loc);
            }
        };

        template <typename CharT>
        class reader<wchar_t, CharT> : public char_reader_base<CharT, wchar_t> {
        public:
            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_default(
                Range& range,
                wchar_t& value,
                detail::locale_ref loc)
            {
                SCN_UNUSED(loc);
                if constexpr (std::is_same_v<CharT, char>) {
                    return code_point_reader<wchar_t>{}.read(range, value);
                }
                else {
                    return code_unit_reader<wchar_t>{}.read(range, value);
                }
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_specs(
                Range& range,
                const detail::basic_format_specs<CharT>& specs,
                wchar_t& value,
                detail::locale_ref loc)
            {
                // TODO: do something with specs
                SCN_UNUSED(specs);
                return read_value_default(range, value, loc);
            }
        };

        template <typename CharT>
        class reader<code_point, CharT>
            : public char_reader_base<CharT, code_point> {
        public:
            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_default(
                Range& range,
                code_point& value,
                detail::locale_ref loc)
            {
                SCN_UNUSED(loc);
                return code_point_reader<code_point>{}.read(range, value);
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_specs(
                Range& range,
                const detail::basic_format_specs<CharT>& specs,
                code_point& value,
                detail::locale_ref loc)
            {
                // TODO: do something with specs
                SCN_UNUSED(specs);
                return read_value_default(range, value, loc);
            }
        };
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
