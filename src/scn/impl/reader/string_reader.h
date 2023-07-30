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

#include <scn/impl/algorithms/take_width_view.h>
#include <scn/impl/reader/character_set_format_parser.h>

#include <string>
#include <string_view>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename SourceCharT, typename DestCharT>
        scan_error transcode_impl(std::basic_string_view<SourceCharT> src,
                                  std::basic_string<DestCharT>& dst)
        {
            dst.clear();
            if (!transcode_to_string(src, dst)) {
                SCN_UNLIKELY_ATTR
                return scan_error(scan_error::invalid_encoding,
                                  "Failed to transcode string value");
            }
            return {};
        }

        template <typename SourceCharT, typename DestCharT>
        scan_error transcode_if_necessary(
            const contiguous_range_factory<SourceCharT>& source,
            std::basic_string<DestCharT>& dest)
        {
            if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
                dest.assign(source.view());
            }
            else {
                return transcode_impl(source.view(), dest);
            }

            return {};
        }

        template <typename SourceCharT, typename DestCharT>
        scan_error transcode_if_necessary(
            contiguous_range_factory<SourceCharT>&& source,
            std::basic_string<DestCharT>& dest)
        {
            if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
                if (source.stores_allocated_string()) {
                    dest.assign(SCN_MOVE(source.get_allocated_string()));
                }
                else {
                    dest.assign(source.view());
                }
            }
            else {
                return transcode_impl(source.view(), dest);
            }

            return {};
        }

        template <typename SourceCharT, typename DestCharT>
        scan_error transcode_if_necessary(
            string_view_wrapper<SourceCharT> source,
            std::basic_string<DestCharT>& dest)
        {
            if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
                dest.assign(source.view());
            }
            else {
                return transcode_impl(source.view(), dest);
            }

            return {};
        }

        template <typename Range, typename Iterator, typename ValueCharT>
        auto read_string_impl(Range& range,
                              scan_expected<Iterator>&& result,
                              std::basic_string<ValueCharT>& value)
            -> scan_expected<ranges::iterator_t<Range&>>
        {
            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }

            auto src = make_contiguous_buffer(
                ranges::subrange{ranges::begin(range), *result});
            if (auto e = transcode_if_necessary(SCN_MOVE(src), value);
                SCN_UNLIKELY(!e)) {
                return unexpected(e);
            }

            return *result;
        }

        template <typename Range, typename Iterator, typename ValueCharT>
        auto read_string_view_impl(Range& range,
                                   scan_expected<Iterator>&& result,
                                   std::basic_string_view<ValueCharT>& value)
            -> scan_expected<ranges::iterator_t<Range&>>
        {
            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }

            auto src = [&]() {
                if constexpr (detail::is_specialization_of_v<Range,
                                                             take_width_view>) {
                    return make_contiguous_buffer(ranges::subrange{
                        ranges::begin(range).base(), result->base()});
                }
                else {
                    return make_contiguous_buffer(
                        ranges::subrange{ranges::begin(range), *result});
                }
            }();
            if (src.stores_allocated_string()) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Cannot read a string_view from this source range (not "
                    "contiguous)");
            }
            if constexpr (!std::is_same_v<typename decltype(src)::char_type,
                                          ValueCharT>) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Cannot read a string_view from "
                                             "this source range (would require "
                                             "transcoding)");
            }
            else {
                value = std::basic_string_view<ValueCharT>(
                    ranges::data(src.view()),
                    ranges_polyfill::usize(src.view()));

                return *result;
            }
        }

        template <typename SourceCharT>
        class word_reader_impl {
        public:
            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_classic(
                Range&& range,
                std::basic_string<ValueCharT>& value)
            {
                return read_string_impl(range, read_until_classic_space(range),
                                        value);
            }

            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_localized(
                Range&& range,
                detail::locale_ref loc,
                std::basic_string<ValueCharT>& value)
            {
                return read_string_impl(
                    range, read_until_localized_space(range, loc), value);
            }

            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_classic(
                Range&& range,
                std::basic_string_view<ValueCharT>& value)
            {
                return read_string_view_impl(
                    range, read_until_classic_space(range), value);
            }

            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_localized(
                Range&& range,
                detail::locale_ref loc,
                std::basic_string_view<ValueCharT>& value)
            {
                return read_string_view_impl(
                    range, read_until_localized_space(range, loc), value);
            }
        };

        template <typename SourceCharT>
        class character_reader_impl {
        public:
            // Note: no localized version,
            // since it's equivalent in behavior

            template <typename Range, typename ValueCharT>
            auto read(Range&& range, std::basic_string<ValueCharT>& value)
                -> scan_expected<ranges::borrowed_iterator_t<Range>>
            {
                return read_impl(
                    range,
                    [&](auto&& rng) {
                        return read_string_impl(rng, read_all(rng), value);
                    },
                    detail::priority_tag<1>{});
            }

            template <typename Range, typename ValueCharT>
            auto read(Range&& range, std::basic_string_view<ValueCharT>& value)
                -> scan_expected<ranges::borrowed_iterator_t<Range>>
            {
                return read_impl(
                    range,
                    [&](auto&& rng) {
                        return read_string_view_impl(rng, read_all(rng), value);
                    },
                    detail::priority_tag<1>{});
            }

        private:
            template <typename View, typename ReadCb>
            static auto read_impl(take_width_view<View>& range,
                                  ReadCb&& read_cb,
                                  detail::priority_tag<1>)
                -> scan_expected<
                    ranges::borrowed_iterator_t<take_width_view<View>&>>
            {
                return read_cb(range);
            }

            template <typename Range, typename ReadCb>
            static auto read_impl(Range&&, ReadCb&&, detail::priority_tag<0>)
                -> scan_expected<ranges::borrowed_iterator_t<Range>>
            {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "character_reader requires take_width_view");
            }
        };

        detail::character_set_specifier get_charset_specifier_for_ascii(
            char ch);

        template <typename SourceCharT>
        class character_set_reader_impl {
        public:
            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_classic(
                Range&& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                std::basic_string<ValueCharT>& value)
            {
                // TODO: reparse format string
                SCN_EXPECT(
                    (specs.charset_specifiers &
                     detail::character_set_specifier::has_nonascii_literals) ==
                    detail::character_set_specifier::none);

                return read_string_impl(
                    range, read_source_classic_impl(range, specs), value);
            }

            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_localized(
                Range&& range,
                detail::locale_ref loc,
                const detail::basic_format_specs<SourceCharT>& specs,
                std::basic_string<ValueCharT>& value)
            {
                SCN_EXPECT(
                    (specs.charset_specifiers &
                     detail::character_set_specifier::has_nonascii_literals) ==
                    detail::character_set_specifier::none);

                SCN_UNUSED(loc);
                return read_string_impl(
                    range, read_source_classic_impl(range, specs), value);
            }

            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_classic(
                Range&& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                std::basic_string_view<ValueCharT>& value)
            {
                // TODO: reparse format string
                SCN_EXPECT(
                    (specs.charset_specifiers &
                     detail::character_set_specifier::has_nonascii_literals) ==
                    detail::character_set_specifier::none);

                return read_string_view_impl(
                    range, read_source_classic_impl(range, specs), value);
            }

            template <typename Range, typename ValueCharT>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_localized(
                Range&& range,
                detail::locale_ref loc,
                const detail::basic_format_specs<SourceCharT>& specs,
                std::basic_string_view<ValueCharT>& value)
            {
                SCN_EXPECT(
                    (specs.charset_specifiers &
                     detail::character_set_specifier::has_nonascii_literals) ==
                    detail::character_set_specifier::none);

                SCN_UNUSED(loc);
                return read_string_view_impl(
                    range, read_source_classic_impl(range, specs), value);
            }

        private:
            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>>
            read_source_classic_impl(
                Range&& range,
                const detail::basic_format_specs<SourceCharT>& specs) const
            {
                auto cb = [&specs](SourceCharT ch) {
                    if (!is_ascii_char(ch)) {
                        return false;
                    }

                    const auto ch_value = static_cast<unsigned>(ch);

                    return ((specs.charset_literals[ch_value / 8] >>
                             (ch_value % 8)) &
                            1u) ||
                           ((get_charset_specifier_for_ascii(
                                 static_cast<char>(ch)) &
                             specs.charset_specifiers) !=
                            detail::character_set_specifier::none);
                };

                if ((specs.charset_specifiers &
                     detail::character_set_specifier::has_inverted_flag) !=
                    detail::character_set_specifier::none) {
                    return read_until_code_unit(SCN_FWD(range), cb);
                }
                return read_while_code_unit(SCN_FWD(range), cb);
            }
        };

        template <typename SourceCharT>
        class string_reader
            : public reader_base<string_reader<SourceCharT>, SourceCharT> {
        public:
            constexpr string_reader() = default;

            void check_specs_impl(
                const detail::basic_format_specs<SourceCharT>& specs,
                reader_error_handler& eh)
            {
                detail::check_string_type_specs(specs, eh);

                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wswitch")
                SCN_GCC_IGNORE("-Wswitch-default")

                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wswitch")
                SCN_CLANG_IGNORE("-Wcovered-switch-default")

                switch (specs.type) {
                    case detail::presentation_type::none:
                    case detail::presentation_type::string:
                        m_type = reader_type::word;
                        break;

                    case detail::presentation_type::character:
                        m_type = reader_type::character;
                        break;

                    case detail::presentation_type::string_set:
                        m_type = reader_type::character_set;
                        break;
                }

                SCN_CLANG_POP    // -Wswitch-enum, -Wcovered-switch-default
                    SCN_GCC_POP  // -Wswitch-enum, -Wswitch-default
            }

            bool skip_ws_before_read() const
            {
                return m_type == reader_type::word;
            }

            template <typename Range, typename Value>
            scan_expected<ranges::borrowed_iterator_t<Range>>
            read_default(Range&& range, Value& value, detail::locale_ref loc)
            {
                SCN_UNUSED(loc);
                return word_reader_impl<SourceCharT>{}.read_classic(
                    SCN_FWD(range), value);
            }

            template <typename Range, typename Value>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_specs(
                Range&& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                Value& value,
                detail::locale_ref loc)
            {
                if (specs.localized) {
                    return read_localized(SCN_FWD(range), loc, specs, value);
                }

                return read_classic(SCN_FWD(range), specs, value);
            }

        protected:
            enum class reader_type { word, character, character_set };

            template <typename Range, typename Value>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_classic(
                Range&& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                Value& value)
            {
                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wcovered-switch-default")

                switch (m_type) {
                    case reader_type::word:
                        return word_reader_impl<SourceCharT>{}.read_classic(
                            SCN_FWD(range), value);

                    case reader_type::character:
                        return character_reader_impl<SourceCharT>{}.read(
                            SCN_FWD(range), value);

                    case reader_type::character_set:
                        return character_set_reader_impl<SourceCharT>{}
                            .read_classic(SCN_FWD(range), specs, value);

                    default:
                        SCN_EXPECT(false);
                        SCN_UNREACHABLE;
                }

                SCN_CLANG_POP
            }

            template <typename Range, typename Value>
            scan_expected<ranges::borrowed_iterator_t<Range>> read_localized(
                Range&& range,
                detail::locale_ref loc,
                const detail::basic_format_specs<SourceCharT>& specs,
                Value& value)
            {
                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wcovered-switch-default")

                switch (m_type) {
                    case reader_type::word:
                        return word_reader_impl<SourceCharT>{}.read_localized(
                            SCN_FWD(range), loc, value);

                    case reader_type::character:
                        return character_reader_impl<SourceCharT>{}.read(
                            SCN_FWD(range), value);

                    case reader_type::character_set:
                        return character_set_reader_impl<SourceCharT>{}
                            .read_localized(SCN_FWD(range), loc, specs, value);

                    default:
                        SCN_EXPECT(false);
                        SCN_UNREACHABLE;
                }

                SCN_CLANG_POP
            }

            reader_type m_type{reader_type::word};
        };

        template <typename SourceCharT, typename ValueCharT>
        class reader<std::basic_string<ValueCharT>, SourceCharT, void>
            : public string_reader<SourceCharT> {};

        template <typename SourceCharT, typename ValueCharT>
        class reader<std::basic_string_view<ValueCharT>, SourceCharT, void>
            : public string_reader<SourceCharT> {};
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
