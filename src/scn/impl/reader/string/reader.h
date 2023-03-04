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

#include <scn/impl/reader/string/character_reader.h>
#include <scn/impl/reader/string/character_set_reader.h>
#include <scn/impl/reader/string/word_reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename Iterator, typename SourceCharT, typename ValueCharT>
        scan_expected<Iterator> transcode_if_necessary(
            iterator_value_result<Iterator, std::basic_string_view<SourceCharT>>
                result,
            std::basic_string<SourceCharT>& buffer,
            std::basic_string<ValueCharT>& value)
        {
            if constexpr (std::is_same_v<SourceCharT, ValueCharT>) {
                if (!buffer.empty() && result.value.data() == buffer.data()) {
                    // result.value points to a std::string,
                    // which is the source_reader buffer
                    value = SCN_MOVE(buffer);
                    buffer = std::basic_string<SourceCharT>{};
                }
                else {
                    (void)impl::copy(result.value, back_insert(value));
                }
            }
            else {
                SCN_UNUSED(buffer);
                if (auto e = transcode_to_string(result.value, value); !e) {
                    return unexpected_scan_error(scan_error::invalid_encoding,
                                                 "Transcode failed");
                }
            }
            return result.iterator;
        }

        template <typename CharT>
        class string_reader_factory {
        public:
            string_reader_factory(
                const detail::basic_format_specs<CharT>& specs)
                : m_specs(specs)
            {
            }

        private:
            const detail::basic_format_specs<CharT>& m_specs;
        };

        template <typename CharT>
        class string_reader_base {
        public:
            string_reader_base() = default;

            bool skip_ws_before_read() const
            {
                return m_type == reader_type::word;
            }

            scan_error check_specs(
                const detail::basic_format_specs<CharT>& specs)
            {
                reader_error_handler eh{};
                detail::check_string_type_specs(specs, eh);
                if (!eh) {
                    return {scan_error::invalid_format_string, eh.m_msg};
                }
                set_type_from_specs(specs);
                return {};
            }

        protected:
            enum class reader_type {
                word,
                characters,
                unicode_characters,
                character_set,
            };

            void set_type_from_specs(
                const detail::basic_format_specs<CharT>& specs)
            {
                SCN_GCC_COMPAT_PUSH
                SCN_GCC_COMPAT_IGNORE("-Wswitch-enum")
                switch (specs.type) {
                    case detail::presentation_type::none:
                    case detail::presentation_type::string:
                        m_type = reader_type::word;
                        break;

                    case detail::presentation_type::character:
                        m_type = reader_type::characters;
                        break;

                    case detail::presentation_type::unicode_character:
                        m_type = reader_type::unicode_characters;
                        break;

                    case detail::presentation_type::string_set:
                        m_type = reader_type::character_set;
                        break;

                    default:
                        SCN_EXPECT(false);
                        SCN_UNREACHABLE;
                }
                SCN_GCC_COMPAT_POP
            }

            template <typename FormatParser>
            scan_error parse_set_format(
                FormatParser& format_parser,
                const detail::basic_format_specs<CharT>& specs,
                detail::locale_ref loc)
            {
                if (specs.set_string.empty()) {
                    return scan_error{
                        scan_error::invalid_format_string,
                        "Empty [character set] specifier is not valid"};
                }
                if (auto ret = format_parser.parse(specs.set_string); !ret) {
                    return ret.error();
                }
                else if (detail::to_address(*ret) !=
                         detail::to_address(specs.set_string.end())) {
                    return scan_error{
                        scan_error::invalid_format_string,
                        "[character set] specifier not exhausted"};
                }

                if (auto e = format_parser.sanitize(loc); !e) {
                    return e;
                }
                return scan_error{};
            }

            reader_type m_type{reader_type::word};
        };

        template <typename SourceCharT, typename ValueCharT>
        class string_reader : public string_reader_base<SourceCharT> {
        public:
            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_default(
                Range& range,
                std::basic_string<ValueCharT>& value,
                detail::locale_ref loc)
            {
                return read_value_word_impl(range, value, loc,
                                            static_cast<bool>(loc));
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_specs(
                Range& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                std::basic_string<ValueCharT>& value,
                detail::locale_ref loc)
            {
                using base = string_reader_base<SourceCharT>;

                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wswitch-default")

                switch (this->m_type) {
                    case base::reader_type::word:
                        return read_value_word_impl(range, value, loc,
                                                    specs.localized || loc);

                    case base::reader_type::characters: {
                        SCN_EXPECT(specs.width != 0);
                        auto reader = character_reader<SourceCharT>{};
                        return reader.read(range, specs.width)
                            .and_then([&](auto result) {
                                return transcode_if_necessary(
                                    result, source_reader_buffer<SourceCharT>(),
                                    value);
                            });
                    }

                    case base::reader_type::unicode_characters: {
                        SCN_EXPECT(specs.width != 0);
                        auto reader = unicode_character_reader<SourceCharT>{};
                        return reader.read(range, specs.width)
                            .and_then([&](auto result) {
                                return transcode_if_necessary(
                                    result, source_reader_buffer<SourceCharT>(),
                                    value);
                            });
                    }

                    case base::reader_type::character_set:
                        return read_value_set_impl(range, specs, value, loc);
                }

                SCN_GCC_POP

                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

        private:
            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_word_impl(
                Range& range,
                std::basic_string<ValueCharT>& value,
                detail::locale_ref loc,
                bool do_localized)
            {
                if (do_localized) {
                    auto source_reader =
                        until_space_localized_source_reader<SourceCharT>{loc};
                    return word_reader<SourceCharT>{}
                        .read(range, source_reader)
                        .and_then([&](auto result) {
                            return transcode_if_necessary(
                                result, source_reader_buffer<SourceCharT>(),
                                value);
                        });
                }

                auto source_reader =
                    until_space_classic_source_reader<SourceCharT>{};
                return word_reader<SourceCharT>{}
                    .read(range, source_reader)
                    .and_then([&](auto result) {
                        return transcode_if_necessary(
                            result, source_reader_buffer<SourceCharT>(), value);
                    });
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_set_impl(
                Range& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                std::basic_string<ValueCharT>& value,
                detail::locale_ref loc)
            {
                if (specs.localized) {
                    auto format_parser =
                        character_set_localized_format_parser<SourceCharT>{};
                    if (auto e =
                            this->parse_set_format(format_parser, specs, loc);
                        !e) {
                        return unexpected(e);
                    }

                    auto reader = make_character_set_reader(format_parser);
                    return reader.read(range, loc).and_then([&](auto result) {
                        return transcode_if_necessary(result, reader.buffer,
                                                      value);
                    });
                }

                auto format_parser =
                    character_set_classic_format_parser<SourceCharT>{};
                if (auto e = this->parse_set_format(format_parser, specs, loc);
                    !e) {
                    return unexpected(e);
                }

                auto reader = make_character_set_reader(format_parser);
                return reader.read(range).and_then([&](auto result) {
                    return transcode_if_necessary(result, reader.buffer, value);
                });
            }
        };

        template <typename SourceCharT, typename ValueCharT>
        class string_view_reader : public string_reader_base<SourceCharT> {
        public:
            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_default(
                Range& range,
                std::basic_string_view<ValueCharT>& value,
                detail::locale_ref loc)
            {
                if constexpr (range_supports_nocopy<Range>() &&
                              std::is_same_v<SourceCharT, ValueCharT>) {
                    return read_value_default_impl(range, loc)
                        .transform([&](auto result) SCN_NOEXCEPT {
                            value = result.value;
                            return result.iterator;
                        });
                }
                else {
                    SCN_UNUSED(range);
                    SCN_UNUSED(value);
                    SCN_UNUSED(loc);
                    SCN_EXPECT(false);
                    SCN_UNREACHABLE;
                }
            }

            template <typename Range>
            scan_expected<ranges::iterator_t<Range>> read_value_specs(
                Range& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                std::basic_string_view<ValueCharT>& value,
                detail::locale_ref loc)
            {
                if constexpr (range_supports_nocopy<Range>() &&
                              std::is_same_v<SourceCharT, ValueCharT>) {
                    return read_value_specs_impl(range, specs, loc)
                        .transform([&](auto result) SCN_NOEXCEPT {
                            value = result.value;
                            return result.iterator;
                        });
                }
                else {
                    SCN_UNUSED(range);
                    SCN_UNUSED(specs);
                    SCN_UNUSED(value);
                    SCN_UNUSED(loc);

                    SCN_EXPECT(false);
                    SCN_UNREACHABLE;
                }
            }

        private:
            template <typename Range>
            auto read_value_default_impl(Range& range, detail::locale_ref loc)
            {
                return read_value_word_impl(range, loc, static_cast<bool>(loc));
            }

            template <typename Range>
            auto read_value_specs_impl(
                Range& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                detail::locale_ref loc)
            {
                using base = string_reader_base<SourceCharT>;

                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wswitch-default")

                switch (this->m_type) {
                    case base::reader_type::word:
                        return read_value_word_impl(range, loc,
                                                    specs.localized || loc);

                    case base::reader_type::characters: {
                        SCN_EXPECT(specs.width != 0);
                        auto reader = character_reader<SourceCharT>{};
                        return reader.read(range, specs.width);
                    }

                    case base::reader_type::unicode_characters: {
                        SCN_EXPECT(specs.width != 0);
                        auto reader = unicode_character_reader<SourceCharT>{};
                        return reader.read(range, specs.width);
                    }

                    case base::reader_type::character_set:
                        return read_value_set_impl(range, specs, loc);
                }

                SCN_GCC_POP

                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

            template <typename Range>
            auto read_value_word_impl(Range& range,
                                      detail::locale_ref loc,
                                      bool do_localized)
            {
                if (do_localized) {
                    auto source_reader =
                        until_space_localized_source_reader<SourceCharT>{loc};
                    return word_reader<SourceCharT>{}.read(range,
                                                           source_reader);
                }

                auto source_reader =
                    until_space_classic_source_reader<SourceCharT>{};
                return word_reader<SourceCharT>{}.read(range, source_reader);
            }

            template <typename Range>
            scan_expected<
                iterator_value_result<ranges::iterator_t<Range>,
                                      std::basic_string_view<SourceCharT>>>
            read_value_set_impl(
                Range& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                detail::locale_ref loc)
            {
                if (specs.localized) {
                    auto format_parser =
                        character_set_localized_format_parser<SourceCharT>{};
                    if (auto e =
                            this->parse_set_format(format_parser, specs, loc);
                        !e) {
                        return unexpected(e);
                    }

                    auto reader = make_character_set_reader(format_parser);
                    return reader.read(range, loc);
                }

                auto format_parser =
                    character_set_classic_format_parser<SourceCharT>{};
                if (auto e = this->parse_set_format(format_parser, specs, loc);
                    !e) {
                    return unexpected(e);
                }

                auto reader = make_character_set_reader(format_parser);
                return reader.read(range);
            }
        };

        template <typename SourceCharT, typename ValueCharT>
        class reader<std::basic_string<ValueCharT>, SourceCharT>
            : public string_reader<SourceCharT, ValueCharT> {};

        template <typename SourceCharT, typename ValueCharT>
        class reader<std::basic_string_view<ValueCharT>, SourceCharT>
            : public string_view_reader<SourceCharT, ValueCharT> {};
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
