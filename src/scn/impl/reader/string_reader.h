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

#include <scn/detail/format_string_parser.h>
#include <scn/impl/algorithms/take_width_view.h>
#include <scn/impl/reader/common.h>
#include <scn/impl/util/ascii_ctype.h>
#include <scn/impl/util/bits.h>

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
            transcode_valid_to_string(src, dst);
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
                              Iterator&& result,
                              std::basic_string<ValueCharT>& value)
            -> scan_expected<ranges::iterator_t<Range&>>
        {
            static_assert(
                ranges_std::forward_iterator<detail::remove_cvref_t<Iterator>>);

            auto src = make_contiguous_buffer(
                ranges::subrange{ranges::begin(range), result});
            if (!validate_unicode(src.view())) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Invalid encoding in scanned string");
            }
            if (auto e = transcode_if_necessary(SCN_MOVE(src), value);
                SCN_UNLIKELY(!e)) {
                return unexpected(e);
            }

            return SCN_MOVE(result);
        }

        template <typename Range, typename Iterator, typename ValueCharT>
        auto read_string_view_impl(Range& range,
                                   Iterator&& result,
                                   std::basic_string_view<ValueCharT>& value)
            -> scan_expected<ranges::iterator_t<Range&>>
        {
            static_assert(
                ranges_std::forward_iterator<detail::remove_cvref_t<Iterator>>);

            auto src = [&]() {
                if constexpr (detail::is_specialization_of_v<Range,
                                                             take_width_view>) {
                    return make_contiguous_buffer(ranges::subrange{
                        ranges::begin(range).base(), result.base()});
                }
                else {
                    return make_contiguous_buffer(
                        ranges::subrange{ranges::begin(range), result});
                }
            }();
            using src_type = decltype(src);

            if (src.stores_allocated_string()) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Cannot read a string_view from this source range (not "
                    "contiguous)");
            }
            if constexpr (!std::is_same_v<typename src_type::char_type,
                                          ValueCharT>) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Cannot read a string_view from "
                                             "this source range (would require "
                                             "transcoding)");
            }
            else {
                const auto view = src.view();
                value = std::basic_string_view<ValueCharT>(
                    ranges::data(view), ranges_polyfill::usize(view));

                if (!validate_unicode(value)) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "Invalid encoding in scanned string_view");
                }

                return SCN_MOVE(result);
            }
        }

        template <typename SourceCharT>
        class word_reader_impl {
        public:
            template <typename Range, typename ValueCharT>
            scan_expected<simple_borrowed_iterator_t<Range>> read_classic(
                Range&& range,
                std::basic_string<ValueCharT>& value)
            {
                return read_string_impl(range, read_until_classic_space(range),
                                        value);
            }

#if !SCN_DISABLE_LOCALE
            template <typename Range, typename ValueCharT>
            scan_expected<simple_borrowed_iterator_t<Range>> read_localized(
                Range&& range,
                detail::locale_ref loc,
                std::basic_string<ValueCharT>& value)
            {
                return read_string_impl(
                    range, read_until_localized_space(range, loc), value);
            }
#endif

            template <typename Range, typename ValueCharT>
            scan_expected<simple_borrowed_iterator_t<Range>> read_classic(
                Range&& range,
                std::basic_string_view<ValueCharT>& value)
            {
                return read_string_view_impl(
                    range, read_until_classic_space(range), value);
            }

#if !SCN_DISABLE_LOCALE
            template <typename Range, typename ValueCharT>
            scan_expected<simple_borrowed_iterator_t<Range>> read_localized(
                Range&& range,
                detail::locale_ref loc,
                std::basic_string_view<ValueCharT>& value)
            {
                return read_string_view_impl(
                    range, read_until_localized_space(range, loc), value);
            }
#endif
        };

        template <typename SourceCharT>
        class character_reader_impl {
        public:
            // Note: no localized version,
            // since it's equivalent in behavior

            template <typename Range, typename ValueCharT>
            auto read(Range&& range, std::basic_string<ValueCharT>& value)
                -> scan_expected<simple_borrowed_iterator_t<Range>>
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
                -> scan_expected<simple_borrowed_iterator_t<Range>>
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
                    simple_borrowed_iterator_t<take_width_view<View>&>>
            {
                return read_cb(range);
            }

            template <typename Range, typename ReadCb>
            static auto read_impl(Range&&, ReadCb&&, detail::priority_tag<0>)
                -> scan_expected<simple_borrowed_iterator_t<Range>>
            {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "character_reader requires take_width_view");
            }
        };

        detail::character_set_specifier get_charset_specifier_for_ascii(
            char ch);

        struct nonascii_specs_handler {
            constexpr void on_charset_specifier(detail::character_set_specifier)
            {
            }

            void on_charset_single(char32_t cp)
            {
                on_charset_range(cp, cp + 1);
            }

            void on_charset_range(char32_t begin, char32_t end)
            {
                if (end <= 127) {
                    return;
                }

                for (auto& elem : extra_ranges) {
                    // TODO: check for overlap
                    if (elem.first == end) {
                        elem.first = begin;
                        return;
                    }

                    if (elem.second == begin) {
                        elem.second = end;
                        return;
                    }
                }

                extra_ranges.push_back(std::make_pair(begin, end));
            }

            constexpr void on_error(const char* msg)
            {
                on_error(scan_error{scan_error::invalid_format_string, msg});
            }
            constexpr void on_error(scan_error e)
            {
                SCN_UNLIKELY_ATTR
                err = e;
            }

            constexpr explicit operator bool() const
            {
                return static_cast<bool>(err);
            }

            std::vector<std::pair<char32_t, char32_t>> extra_ranges;
            scan_error err;
        };

        template <typename SourceCharT>
        class character_set_reader_impl {
        public:
            template <typename Range, typename ValueCharT>
            scan_expected<simple_borrowed_iterator_t<Range>> read_classic(
                Range&& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                std::basic_string<ValueCharT>& value)
            {
                auto it = read_source_classic_impl(range, {specs});
                if (SCN_UNLIKELY(!it)) {
                    return unexpected(it.error());
                }

                return read_string_impl(range, *it, value);
            }

#if !SCN_DISABLE_LOCALE
            template <typename Range, typename ValueCharT>
            scan_expected<simple_borrowed_iterator_t<Range>> read_localized(
                Range&& range,
                detail::locale_ref loc,
                const detail::basic_format_specs<SourceCharT>& specs,
                std::basic_string<ValueCharT>& value)
            {
                auto it = read_source_localized_impl(range, {specs}, loc);
                if (SCN_UNLIKELY(!it)) {
                    return unexpected(it.error());
                }

                return read_string_impl(range, *it, value);
            }
#endif

            template <typename Range, typename ValueCharT>
            scan_expected<simple_borrowed_iterator_t<Range>> read_classic(
                Range&& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                std::basic_string_view<ValueCharT>& value)
            {
                auto it = read_source_classic_impl(range, {specs});
                if (SCN_UNLIKELY(!it)) {
                    return unexpected(it.error());
                }

                return read_string_view_impl(range, *it, value);
            }

#if !SCN_DISABLE_LOCALE
            template <typename Range, typename ValueCharT>
            scan_expected<simple_borrowed_iterator_t<Range>> read_localized(
                Range&& range,
                detail::locale_ref loc,
                const detail::basic_format_specs<SourceCharT>& specs,
                std::basic_string_view<ValueCharT>& value)
            {
                auto it = read_source_localized_impl(range, {specs}, loc);
                if (SCN_UNLIKELY(!it)) {
                    return unexpected(it.error());
                }

                return read_string_view_impl(range, *it, value);
            }
#endif

        private:
            struct specs_helper {
                constexpr specs_helper(
                    const detail::basic_format_specs<SourceCharT>& s)
                    : specs(s)
                {
                }

                constexpr bool is_char_set_in_literals(char ch) const
                {
                    SCN_EXPECT(is_ascii_char(ch));
                    const auto val =
                        static_cast<unsigned>(static_cast<unsigned char>(ch));
                    return (static_cast<unsigned>(
                                specs.charset_literals[val / 8]) >>
                            (val % 8)) &
                           1u;
                }

                constexpr bool is_char_set_in_specifiers(char ch) const
                {
                    SCN_EXPECT(is_ascii_char(ch));
                    return (get_charset_specifier_for_ascii(ch) &
                            specs.charset_specifiers) !=
                           detail::character_set_specifier::none;
                }

                bool is_char_set_in_extra_literals(char32_t cp) const
                {
                    // TODO: binary search?
                    if (nonascii.extra_ranges.empty()) {
                        return false;
                    }

                    const auto cp_val = static_cast<uint32_t>(cp);
                    return ranges::find_if(
                               nonascii.extra_ranges,
                               [cp_val](const auto& pair) SCN_NOEXCEPT {
                                   return static_cast<uint32_t>(pair.first) <=
                                              cp_val &&
                                          static_cast<uint32_t>(pair.second) >
                                              cp_val;
                               }) != nonascii.extra_ranges.end();
                }

                scan_error handle_nonascii()
                {
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::
                             has_nonascii_literals) ==
                        detail::character_set_specifier::none) {
                        return {};
                    }

                    auto it = detail::to_address(specs.charset_string.begin());
                    auto set = detail::parse_presentation_set(
                        it, detail::to_address(specs.charset_string.end()),
                        nonascii);
                    if (SCN_UNLIKELY(!nonascii)) {
                        return nonascii.err;
                    }
                    SCN_ENSURE(it ==
                               detail::to_address(specs.charset_string.end()));
                    SCN_ENSURE(set == specs.charset_string);

                    ranges::sort(nonascii.extra_ranges);
                    return {};
                }

#if !SCN_DISABLE_LOCALE
                scan_error handle_localized_nonmask(detail::locale_ref loc)
                {
                    SCN_UNUSED(loc);

                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::space_literal) !=
                        detail::character_set_specifier::none) {
                        nonascii.extra_ranges.push_back(
                            std::make_pair(char32_t{' '}, char32_t{' ' + 1}));
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::underscore_literal) !=
                        detail::character_set_specifier::none) {
                        nonascii.extra_ranges.push_back(
                            std::make_pair(char32_t{'_'}, char32_t{'_' + 1}));
                    }

                    return {};
                }

                std::pair<std::pair<std::ctype_base::mask, bool>,
                          std::pair<std::ctype_base::mask, bool>>
                map_localized_mask() const
                {
                    std::ctype_base::mask mask{}, inverted_mask{};
                    bool is_exhaustive{true}, is_inverted_exhaustive{true};

                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::space_literal) !=
                        detail::character_set_specifier::none) {
                        is_exhaustive = false;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::underscore_literal) !=
                        detail::character_set_specifier::none) {
                        is_exhaustive = false;
                    }

                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::space) !=
                        detail::character_set_specifier::none) {
                        mask |= std::ctype_base::space;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::blank) !=
                        detail::character_set_specifier::none) {
                        mask |= std::ctype_base::blank;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::punct) !=
                        detail::character_set_specifier::none) {
                        mask |= std::ctype_base::punct;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::upper) !=
                        detail::character_set_specifier::none) {
                        mask |= std::ctype_base::upper;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::lower) !=
                        detail::character_set_specifier::none) {
                        mask |= std::ctype_base::lower;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::alpha) !=
                        detail::character_set_specifier::none) {
                        mask |= std::ctype_base::alpha;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::digit) !=
                        detail::character_set_specifier::none) {
                        mask |= std::ctype_base::digit;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::xdigit) !=
                        detail::character_set_specifier::none) {
                        mask |= std::ctype_base::xdigit;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::cntrl) !=
                        detail::character_set_specifier::none) {
                        mask |= std::ctype_base::cntrl;
                    }

                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::inverted_letters) !=
                        detail::character_set_specifier::none) {
                        inverted_mask |= std::ctype_base::alpha;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::
                             inverted_alnum_underscore) !=
                        detail::character_set_specifier::none) {
                        inverted_mask |= std::ctype_base::alnum;
                        is_inverted_exhaustive = false;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::
                             inverted_whitespace) !=
                        detail::character_set_specifier::none) {
                        inverted_mask |= std::ctype_base::space;
                    }
                    if ((specs.charset_specifiers &
                         detail::character_set_specifier::inverted_numbers) !=
                        detail::character_set_specifier::none) {
                        inverted_mask |= std::ctype_base::digit;
                    }

                    return {{mask, is_exhaustive},
                            {inverted_mask, is_inverted_exhaustive}};
                }
#endif  // !SCN_DISABLE_LOCALE

                const detail::basic_format_specs<SourceCharT>& specs;
                nonascii_specs_handler nonascii;
            };

            struct read_source_callback {
                bool on_ascii_only(SourceCharT ch) const
                {
                    if (!is_ascii_char(ch)) {
                        return false;
                    }

                    return helper.is_char_set_in_literals(
                               static_cast<char>(ch)) ||
                           helper.is_char_set_in_specifiers(
                               static_cast<char>(ch));
                }

                bool on_classic_with_extra_ranges(char32_t cp) const
                {
                    if (!is_ascii_char(cp)) {
                        return helper.is_char_set_in_extra_literals(cp);
                    }

                    return helper.is_char_set_in_literals(
                               static_cast<char>(cp)) ||
                           helper.is_char_set_in_specifiers(
                               static_cast<char>(cp));
                }

                bool on_localized(char32_t cp) const
                {
                    if (!is_ascii_char(cp)) {
                        return helper.is_char_set_in_extra_literals(cp);
                    }

                    return helper.is_char_set_in_literals(
                               static_cast<char>(cp)) ||
                           helper.is_char_set_in_extra_literals(cp);
                }

                const specs_helper& helper;
                detail::locale_ref loc{};
            };

            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>>
            read_source_classic_impl(Range&& range, specs_helper helper) const
            {
                const bool is_inverted =
                    (helper.specs.charset_specifiers &
                     detail::character_set_specifier::has_inverted_flag) !=
                    detail::character_set_specifier::none;
                const bool accepts_nonascii =
                    (helper.specs.charset_specifiers &
                     detail::character_set_specifier::has_nonascii_literals) !=
                    detail::character_set_specifier::none;

                if (auto e = helper.handle_nonascii(); SCN_UNLIKELY(!e)) {
                    return unexpected(e);
                }

                read_source_callback cb_wrapper{helper};

                if (accepts_nonascii) {
                    const auto cb = [&](char32_t cp) {
                        return cb_wrapper.on_classic_with_extra_ranges(cp);
                    };

                    if (is_inverted) {
                        auto it = read_until_code_point(range, cb);
                        return check_nonempty(it, range);
                    }
                    auto it = read_while_code_point(range, cb);
                    return check_nonempty(it, range);
                }

                const auto cb = [&](SourceCharT ch) {
                    return cb_wrapper.on_ascii_only(ch);
                };

                if (is_inverted) {
                    auto it = read_until_code_unit(range, cb);
                    return check_nonempty(it, range);
                }
                auto it = read_while_code_unit(range, cb);
                return check_nonempty(it, range);
            }

#if !SCN_DISABLE_LOCALE
            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>>
            read_source_localized_impl(Range&& range,
                                       specs_helper helper,
                                       detail::locale_ref loc) const
            {
                const bool is_inverted =
                    (helper.specs.charset_specifiers &
                     detail::character_set_specifier::has_inverted_flag) !=
                    detail::character_set_specifier::none;

                if (auto e = helper.handle_nonascii(); SCN_UNLIKELY(!e)) {
                    return unexpected(e);
                }
                if (auto e = helper.handle_localized_nonmask(loc);
                    SCN_UNLIKELY(!e)) {
                    return unexpected(e);
                }

                read_source_callback cb_wrapper{helper, loc};

                const auto [m1, m2] = helper.map_localized_mask();
                const auto [mask, is_mask_exhaustive] = m1;
                const auto [inverted_mask, is_inverted_mask_exhaustive] = m2;
                const bool has_any_ascii_literals =
                    ranges::any_of(helper.specs.charset_literals,
                                   [](auto b) SCN_NOEXCEPT { return b != 0; });
                const bool has_any_nonascii_literals =
                    !helper.nonascii.extra_ranges.empty();

                if (is_mask_exhaustive && is_inverted_mask_exhaustive &&
                    inverted_mask == std::ctype_base::mask{} &&
                    !has_any_ascii_literals && !has_any_nonascii_literals) {
                    if (is_inverted) {
                        auto it = read_until_localized_mask(range, loc, mask);
                        return check_nonempty(it, range);
                    }
                    auto it = read_while_localized_mask(range, loc, mask);
                    return check_nonempty(it, range);
                }

                const auto cb = [&](char32_t cp) {
                    return cb_wrapper.on_localized(cp);
                };

                if (is_mask_exhaustive && is_inverted_mask_exhaustive &&
                    mask == std::ctype_base::mask{} &&
                    inverted_mask == std::ctype_base::mask{}) {
                    if (is_inverted) {
                        auto it = read_until_code_point(range, cb);
                        return check_nonempty(it, range);
                    }
                    auto it = read_while_code_point(range, cb);
                    return check_nonempty(it, range);
                }

                if (is_inverted) {
                    auto it =
                        read_until_localized_mask_or_inverted_mask_or_code_point(
                            range, loc, mask, inverted_mask, cb);
                    return check_nonempty(it, range);
                }
                auto it =
                    read_while_localized_mask_or_inverted_mask_or_code_point(
                        range, loc, mask, inverted_mask, cb);
                return check_nonempty(it, range);
            }
#endif  // !SCN_DISABLE_LOCALE

            template <typename Iterator, typename Range>
            static scan_expected<Iterator> check_nonempty(const Iterator& it,
                                                          const Range& range)
            {
                if (it == ranges::begin(range)) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "No characters matched in [character set]");
                }

                return it;
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
            scan_expected<simple_borrowed_iterator_t<Range>>
            read_default(Range&& range, Value& value, detail::locale_ref loc)
            {
                SCN_UNUSED(loc);
                return word_reader_impl<SourceCharT>{}.read_classic(
                    SCN_FWD(range), value);
            }

            template <typename Range, typename Value>
            scan_expected<simple_borrowed_iterator_t<Range>> read_specs(
                Range&& range,
                const detail::basic_format_specs<SourceCharT>& specs,
                Value& value,
                detail::locale_ref loc)
            {
#if !SCN_DISABLE_LOCALE
                if (specs.localized) {
                    return read_localized(SCN_FWD(range), loc, specs, value);
                }
#endif

                return read_classic(SCN_FWD(range), specs, value);
            }

        protected:
            enum class reader_type { word, character, character_set };

            template <typename Range, typename Value>
            scan_expected<simple_borrowed_iterator_t<Range>> read_classic(
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

#if !SCN_DISABLE_LOCALE
            template <typename Range, typename Value>
            scan_expected<simple_borrowed_iterator_t<Range>> read_localized(
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
#endif

            reader_type m_type{reader_type::word};
        };

        template <typename SourceCharT>
        class reader_impl_for_string : public string_reader<SourceCharT> {};
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
