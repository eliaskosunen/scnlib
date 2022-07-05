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
#include <scn/impl/algorithms/find_whitespace.h>
#include <scn/impl/algorithms/read_copying.h>
#include <scn/impl/algorithms/read_localized.h>
#include <scn/impl/algorithms/read_nocopy.h>

#include <variant>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        struct reader_error_handler {
            constexpr void on_error(const char* msg)
            {
                m_msg = msg;
            }
            explicit constexpr operator bool() const
            {
                return m_msg == nullptr;
            }

            const char* m_msg{nullptr};
        };

        template <typename CharT>
        std::basic_string<CharT>& source_reader_buffer();

        extern template std::string& source_reader_buffer();
        extern template std::wstring& source_reader_buffer();

        /**
         * SourceReader:
         * auto read(SourceRange&&) -> iterator_value_result<SourceRange, sv>
         */

        template <typename CharT>
        class until_space_classic_source_reader {
        public:
            using char_type = CharT;
            using string_view_type = std::basic_string_view<CharT>;

            until_space_classic_source_reader() = default;

            template <typename SourceRange>
            using source_read_result =
                iterator_value_result<ranges::borrowed_iterator_t<SourceRange>,
                                      string_view_type>;

            template <typename SourceRange>
            source_read_result<SourceRange> read(SourceRange&& source)
            {
                if constexpr (range_supports_nocopy<SourceRange>()) {
                    return read_until_classic_space_nocopy(SCN_FWD(source));
                }
                else {
                    source_reader_buffer<CharT>().clear();
                    auto r = read_until_classic_space_copying(
                        SCN_FWD(source),
                        back_insert(source_reader_buffer<CharT>()));
                    return {r.in, {source_reader_buffer<CharT>()}};
                }
            }
        };

        template <typename CharT>
        class until_space_localized_source_reader {
        public:
            using char_type = CharT;
            using string_view_type = std::basic_string_view<CharT>;

            until_space_localized_source_reader(detail::locale_ref loc)
                : m_locale(loc)
            {
            }

            template <typename SourceRange>
            using source_read_result =
                iterator_value_result<ranges::borrowed_iterator_t<SourceRange>,
                                      string_view_type>;

            template <typename SourceRange>
            scan_expected<source_read_result<SourceRange>> read(
                SourceRange&& source)
            {
                if constexpr (range_supports_nocopy<SourceRange>()) {
                    return read_until_localized_nocopy(
                        SCN_FWD(source), m_locale, std::ctype_base::space,
                        true);
                }
                else {
                    source_reader_buffer<CharT>().clear();
                    return read_until_localized_copy(
                               SCN_FWD(source),
                               back_insert(source_reader_buffer<CharT>()),
                               null_output_range<wchar_t>{}, m_locale,
                               std::ctype_base::space, true)
                        .transform([&](auto result) SCN_NOEXCEPT {
                            return source_read_result<SourceRange>{
                                result.in, string_view_type{
                                               source_reader_buffer<CharT>()}};
                        });
                }
            }

        private:
            detail::locale_ref m_locale{};
        };

        template <typename CharT>
        class whitespace_skipper {
        public:
            using char_type = CharT;

            whitespace_skipper() = default;

            template <typename SourceRange>
            ranges::borrowed_iterator_t<SourceRange> skip_classic(
                SourceRange&& source)
            {
                if constexpr (range_supports_nocopy<SourceRange>() &&
                              std::is_same_v<ranges::range_value_t<SourceRange>,
                                             char>) {
                    return find_classic_nonspace_narrow_fast(
                        {ranges::data(source),
                         static_cast<size_t>(ranges::size(source))});
                }
                else if constexpr (range_supports_nocopy<SourceRange>()) {
                    const auto result = read_until_classic_nocopy(
                        SCN_FWD(source), is_not_ascii_space);
                    return result.iterator;
                }
                else {
                    const auto result = read_until_classic_copying(
                        SCN_FWD(source), null_output_range<CharT>{},
                        is_not_ascii_space);
                    return result.in;
                }
            }

            template <typename SourceRange>
            scan_expected<ranges::borrowed_iterator_t<SourceRange>>
            skip_localized(SourceRange&& source, detail::locale_ref loc)
            {
                return read_until_localized_skip(SCN_FWD(source), loc,
                                                 std::ctype_base::space, false);
            }

        private:
            static bool is_not_ascii_space(CharT ch) SCN_NOEXCEPT
            {
                return !is_ascii_space(ch);
            }
        };

        template <typename SourceRange>
        scan_expected<detail::borrowed_itsen_subrange_t<SourceRange>>
        skip_classic_whitespace(SourceRange&& range,
                                bool allow_exhaustion = false)
        {
            auto end = ranges::end(range);
            auto result =
                whitespace_skipper<ranges::range_value_t<SourceRange>>{}
                    .skip_classic(range);
            if (!allow_exhaustion && result == end) {
                return unexpected_scan_error(scan_error::end_of_range, "EOF");
            }
            return {{result, end}};
        }

        template <typename SourceRange>
        scan_expected<detail::borrowed_itsen_subrange_t<SourceRange>>
        skip_localized_whitespace(SourceRange&& range,
                                  detail::locale_ref loc,
                                  bool allow_exhaustion = false)
        {
            return whitespace_skipper<ranges::range_value_t<SourceRange>>{}
                .skip_localized(range, loc)
                .and_then([&](auto it) SCN_NOEXCEPT
                          -> scan_expected<
                              detail::borrowed_itsen_subrange_t<SourceRange>> {
                    const auto end = ranges::end(range);
                    if (!allow_exhaustion && it == end) {
                        return unexpected_scan_error(scan_error::end_of_range,
                                                     "EOF");
                    }
                    return {{it, end}};
                });
        }

        template <typename SourceReader, typename SourceRange>
        auto read_with_source_reader(SourceReader& reader, SourceRange&& range)
        {
            if constexpr (detail::is_expected<decltype(reader.read(
                              SCN_FWD(range)))>::value) {
                return reader.read(SCN_FWD(range));
            }
            else {
                return detail::always_success_expected{
                    reader.read(SCN_FWD(range))};
            }
        }

        template <typename CharT>
        class classic_numeric_source_reader
            : public until_space_classic_source_reader<CharT> {
            using base = until_space_classic_source_reader<CharT>;

        public:
            classic_numeric_source_reader() = default;

            template <typename SourceRange>
            auto read(SourceRange&& source) ->
                typename base::template source_read_result<SourceRange>
            {
                if constexpr (range_supports_nocopy<SourceRange>()) {
                    return read_all_nocopy(SCN_FWD(source));
                }
                else {
                    return base::template read(SCN_FWD(source));
                }
            }
        };

        template <typename T, typename CharT, typename Enable = void>
        class reader;

        template <typename Derived, typename T, typename CharT>
        class reader_facade {
        public:
            constexpr reader_facade() = default;

            bool skip_ws_before_read() const
            {
                return true;
            }

            scan_error check_specs(
                const detail::basic_format_specs<CharT>& specs)
            {
                reader_error_handler eh{};
                get_derived().check_specs_impl(specs, eh);
                if (!eh) {
                    return {scan_error::invalid_format_string, eh.m_msg};
                }
                return {};
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>>
            read_value_default(Range& range, T& value, detail::locale_ref loc)
            {
                if (loc) {
                    auto [source_reader, value_reader] =
                        get_derived().make_default_userlocale_readers(loc);
                    return read_impl(range, source_reader, value_reader, value);
                }

                auto [source_reader, value_reader] =
                    get_derived().make_default_classic_readers();
                return read_impl(range, source_reader, value_reader, value);
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_specs(
                Range& range,
                const detail::basic_format_specs<CharT>& specs,
                T& value,
                detail::locale_ref loc)
            {
                if (specs.localized) {
                    auto [source_reader, value_reader] =
                        get_derived().make_specs_localized_readers(specs, loc);
                    return read_impl(range, source_reader, value_reader, value);
                }

                if (loc) {
                    auto [source_reader, value_reader] =
                        get_derived().make_specs_userlocale_readers(specs, loc);
                    return read_impl(range, source_reader, value_reader, value);
                }

                auto [source_reader, value_reader] =
                    get_derived().make_specs_classic_readers(specs);
                return read_impl(range, source_reader, value_reader, value);
            }

        private:
            Derived& get_derived()
            {
                return static_cast<Derived&>(*this);
            }
            const Derived& get_derived() const
            {
                return static_cast<const Derived&>(*this);
            }

            template <typename U>
            struct debug;

            template <typename SourceRange,
                      typename SourceReader,
                      typename ValueReader>
            static scan_expected<ranges::iterator_t<SourceRange>> read_impl(
                SourceRange& src,
                SourceReader& source_reader,
                ValueReader& value_reader,
                T& value)
            {
                const auto src_read =
                    read_with_source_reader(source_reader, src);
                if (!src_read) {
                    return unexpected(src_read.error());
                }

                if (src_read->value.empty()) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "Failed to scan value: no valid characters found");
                }

                return value_reader.read(src_read->value, value)
                    .transform([&](auto it) {
                        return ranges::next(
                            ranges::begin(src),
                            ranges::distance(src_read->value.begin(), it));
                    });
            }
        };

        template <typename CharT>
        class monostate_reader {
        public:
            constexpr monostate_reader() = default;

            bool skip_ws_before_read() const
            {
                return true;
            }

            static scan_error check_specs(
                const detail::basic_format_specs<CharT>&)
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>>
            read_value_default(Range&, detail::monostate&, detail::locale_ref)
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_specs(
                Range&,
                const detail::basic_format_specs<CharT>&,
                detail::monostate&,
                detail::locale_ref)
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }
        };

        template <typename CharT>
        class reader<detail::monostate, CharT>
            : public monostate_reader<CharT> {};
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
