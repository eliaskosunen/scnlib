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

#include <scn/impl/algorithms/read_copying.h>
#include <scn/impl/reader/common.h>
#include <scn/util/string_view.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        struct character_set_format_parser_base {
        public:
            constexpr character_set_format_parser_base() = default;

            enum class specifier : unsigned {
                alnum,
                alpha,
                blank,
                cntrl,
                digit,
                graph,
                lower,
                print,
                punct,
                space,
                upper,
                xdigit,
                last_colon,
                letters,                    // \l
                inverted_letters,           // \L
                alnum_underscore,           // \w
                inverted_alnum_underscore,  // \W
                whitespace,                 // \s
                inverted_whitespace,        // \S
                numbers,                    // \d
                inverted_numbers,           // \D
                last
            };
            static_assert(static_cast<size_t>(specifier::last) <=
                          sizeof(uint64_t) * 8);

        protected:
            SCN_NODISCARD bool _get_char_allowed(char ch) const
            {
                const auto ch_bits = static_cast<uint64_t>(ch);
                const auto val = (ch_bits > 63 ? m_char_allowed_upper
                                               : m_char_allowed_lower);
                return ((val >> (ch_bits % 64)) & 1ULL) != 0;
            }
            void _enable_char_allowed(char ch)
            {
                const auto ch_bits = static_cast<uint64_t>(ch);
                auto& val = (ch_bits > 63 ? m_char_allowed_upper
                                          : m_char_allowed_lower);
                val |= 1ULL << (ch_bits % 64);
            }
            void _set_char_allowed(char ch, bool val)
            {
                const auto ch_bits = static_cast<uint64_t>(ch);
                auto& v = (ch_bits > 63 ? m_char_allowed_upper
                                        : m_char_allowed_lower);
                SCN_MSVC_PUSH
                SCN_MSVC_IGNORE(4146)  // unary minus applied to unsigned
                v ^= (-static_cast<uint64_t>(val) ^ v) &
                     (1ULL << (ch_bits % 64));
                SCN_MSVC_POP
            }
            SCN_NODISCARD bool _has_no_allowed_chars() const
            {
                return m_char_allowed_lower == 0 && m_char_allowed_upper == 0;
            }

            SCN_NODISCARD bool _get_specifier(specifier s) const
            {
                const auto s_bits = static_cast<uint64_t>(s);
                return ((m_set_specifiers >> s_bits) & 1ULL) != 0;
            }
            void _enable_specifier(specifier s)
            {
                const auto s_bits = static_cast<uint64_t>(s);
                m_set_specifiers |= 1ULL << s_bits;
            }
            void _set_specifier(specifier s, bool val)
            {
                SCN_MSVC_PUSH
                SCN_MSVC_IGNORE(4146)  // unary minus applied to unsigned
                const auto s_bits = static_cast<uint64_t>(s);
                m_set_specifiers ^=
                    (-static_cast<uint64_t>(val) ^ m_set_specifiers) &
                    (1ULL << s_bits);
                SCN_MSVC_POP
            }
            SCN_NODISCARD bool _has_no_set_specifiers() const
            {
                return m_set_specifiers == 0;
            }

            bool _is_allowed_by_extra_ranges(code_point cp) const
            {
                return ranges::any_of(
                    m_extra_ranges, [cp](const auto& r) SCN_NOEXCEPT {
                        return r.first >= static_cast<uint32_t>(cp) &&
                               r.second <= static_cast<uint32_t>(cp);
                    });
            }

            uint64_t m_char_allowed_lower{0};
            uint64_t m_char_allowed_upper{0};
            uint64_t m_set_specifiers{0};

            std::vector<std::pair<uint32_t, uint32_t>> m_extra_ranges{};

            bool m_inverted_flag_set{false};
        };

        constexpr std::pair<const char*,
                            character_set_format_parser_base::specifier>
            colon_specifier_narrow_map[] = {
                {"alnum:", character_set_format_parser_base::specifier::alnum},
                {"alpha:", character_set_format_parser_base::specifier::alpha},
                {"blank:", character_set_format_parser_base::specifier::blank},
                {"cntrl:", character_set_format_parser_base::specifier::cntrl},
                {"digit:", character_set_format_parser_base::specifier::digit},
                {"graph:", character_set_format_parser_base::specifier::graph},
                {"lower:", character_set_format_parser_base::specifier::lower},
                {"print:", character_set_format_parser_base::specifier::print},
                {"punct:", character_set_format_parser_base::specifier::punct},
                {"space:", character_set_format_parser_base::specifier::space},
                {"upper:", character_set_format_parser_base::specifier::upper},
                {"xdigit:",
                 character_set_format_parser_base::specifier::xdigit},
        };
        constexpr std::pair<const wchar_t*,
                            character_set_format_parser_base::specifier>
            colon_specifier_wide_map[] = {
                {L"alnum:", character_set_format_parser_base::specifier::alnum},
                {L"alpha:", character_set_format_parser_base::specifier::alpha},
                {L"blank:", character_set_format_parser_base::specifier::blank},
                {L"cntrl:", character_set_format_parser_base::specifier::cntrl},
                {L"digit:", character_set_format_parser_base::specifier::digit},
                {L"graph:", character_set_format_parser_base::specifier::graph},
                {L"lower:", character_set_format_parser_base::specifier::lower},
                {L"print:", character_set_format_parser_base::specifier::print},
                {L"punct:", character_set_format_parser_base::specifier::punct},
                {L"space:", character_set_format_parser_base::specifier::space},
                {L"upper:", character_set_format_parser_base::specifier::upper},
                {L"xdigit:",
                 character_set_format_parser_base::specifier::xdigit},
        };

        template <typename CharT>
        constexpr const auto& get_colon_specifier_map()
        {
            if constexpr (std::is_same_v<CharT, char>) {
                return colon_specifier_narrow_map;
            }
            else {
                return colon_specifier_wide_map;
            }
        }

        constexpr std::pair<char, character_set_format_parser_base::specifier>
            backslash_specifier_map[] = {
                {'l', character_set_format_parser_base::specifier::letters},
                {'L',
                 character_set_format_parser_base::specifier::inverted_letters},
                {'w',
                 character_set_format_parser_base::specifier::alnum_underscore},
                {'W', character_set_format_parser_base::specifier::
                          inverted_alnum_underscore},
                {'s', character_set_format_parser_base::specifier::whitespace},
                {'S', character_set_format_parser_base::specifier::
                          inverted_whitespace},
                {'d', character_set_format_parser_base::specifier::numbers},
                {'D',
                 character_set_format_parser_base::specifier::inverted_numbers},
        };

        template <typename CharT>
        struct basic_character_set_format_parser_base
            : public character_set_format_parser_base {
            using iterator = typename std::basic_string_view<CharT>::iterator;

        public:
            SCN_NODISCARD scan_expected<iterator> parse(
                std::basic_string_view<CharT> fmt)
            {
                SCN_EXPECT(fmt.front() == '[');
                SCN_EXPECT(!fmt.empty());

                auto it = ranges::next(fmt.begin());
                if (it == fmt.end()) {
                    return unexpected_scan_error(
                        scan_error::invalid_format_string,
                        "Unexpected end of [character set] format string "
                        "argument");
                }

                if (*it == '^') {
                    this->m_inverted_flag_set = true;
                    ++it;
                }
                if (it == fmt.end()) {
                    return unexpected_scan_error(
                        scan_error::invalid_format_string,
                        "Unexpected end of [character set] format string "
                        "argument");
                }

                if (*it == ']') {
                    this->_enable_char_allowed(']');
                    ++it;
                }

                while (detail::to_address(it) !=
                       detail::to_address(fmt.end())) {
                    if (*it == ']') {
                        return {++it};
                    }

                    if (*it == ':') {
                        if (auto err = on_colon(it, fmt.end()); !err) {
                            return unexpected(err);
                        }
                    }
                    else if (*it == '\\') {
                        if (auto err = on_backslash(it, fmt.end()); !err) {
                            return unexpected(err);
                        }
                    }
                    else {
                        if (auto err = on_literal(it, fmt.end()); !err) {
                            return unexpected(err);
                        }
                    }
                }
                return unexpected_scan_error(
                    scan_error::invalid_format_string,
                    "Unexpected end of [character set] format string argument");
            }

        private:
            scan_error on_colon(iterator& it, iterator end)
            {
                SCN_EXPECT(it != end);
                SCN_EXPECT(*it == ':');
                ++it;

                const auto chars_left = end - it;

                for (const auto& elem : get_colon_specifier_map<CharT>()) {
                    const auto len = ::scn::detail::strlen(elem.first);
                    if (static_cast<size_t>(chars_left) < len) {
                        continue;
                    }
                    if (std::basic_string_view<CharT>{&*it, len} ==
                        std::basic_string_view<CharT>{elem.first, len}) {
                        this->_enable_specifier(elem.second);
                        it += len;
                        SCN_ENSURE(*(it - 1) == ':');
                        return {};
                    }
                }

                return {scan_error::invalid_format_string,
                        "Invalid :colon: specifier in a [character set] format "
                        "string argument"};
            }

            scan_error on_backslash(iterator& it, iterator end)
            {
                SCN_EXPECT(it != end);
                SCN_EXPECT(*it == '\\');
                ++it;

                for (const auto& elem : backslash_specifier_map) {
                    if (*it == static_cast<CharT>(elem.first)) {
                        this->_enable_specifier(elem.second);
                        ++it;
                        return {};
                    }
                }
                if (*it == '\\') {
                    this->_enable_char_allowed('\\');
                    ++it;
                    return {};
                }
                if (*it == ':') {
                    this->_enable_char_allowed(':');
                    ++it;
                    return {};
                }

                return {scan_error::invalid_format_string,
                        "Invalid \\backslash specifier in a [character set] "
                        "format string argument"};
            }

            scan_error on_literal(iterator& it, iterator end)
            {
                SCN_EXPECT(detail::to_address(it) != detail::to_address(end));
                SCN_EXPECT(*it != ':' && *it != '\\');

                auto cp_result = parse_cp(it, end);
                if (!cp_result) {
                    return cp_result.error();
                }

                const auto it_addr = detail::to_address(it);
                const auto end_addr = detail::to_address(end);
                if (it_addr != end_addr && *it_addr == '-' &&
                    (it_addr + 1) != end_addr && *(it_addr + 1) != ']') {
                    ++it;
                    auto cp2_result = parse_cp(it, end);
                    if (!cp2_result) {
                        return cp2_result.error();
                    }

                    auto cp1 = static_cast<uint32_t>(*cp_result);
                    auto cp2 = static_cast<uint32_t>(*cp2_result);

                    if (cp2 < cp1) {
                        return {scan_error::invalid_format_string,
                                "Invalid range in [character set] format "
                                "string argument: end before beginning"};
                    }

                    auto set_cp_chars = [&](uint32_t cp, uint32_t until) {
                        for (; cp <= until; ++cp) {
                            this->_enable_char_allowed(static_cast<char>(cp));
                        }
                    };

                    if (cp1 <= 0x7f) {
                        set_cp_chars(cp1, (std::min)(cp2, 0x7fu));
                        cp1 = 0x80;
                    }
                    if (cp2 >= 0x80) {
                        this->m_extra_ranges.emplace_back(cp1, cp2);
                    }

                    return {};
                }

                if (*cp_result <= 0x7f) {
                    this->_enable_char_allowed(static_cast<char>(*cp_result));
                }
                else {
                    this->m_extra_ranges.emplace_back(
                        static_cast<uint32_t>(*cp_result),
                        static_cast<uint32_t>(*cp_result));
                }

                return {};
            }

            scan_expected<code_point> parse_cp(iterator& it, iterator end)
            {
                SCN_EXPECT(detail::to_address(it) != detail::to_address(end));

                return get_next_code_point(
                           detail::make_string_view_from_iterators<CharT>(it,
                                                                          end))
                    .transform([&](auto result) SCN_NOEXCEPT {
                        it = result.iterator;
                        return result.value;
                    });
            }
        };

        template <typename CharT>
        class character_set_classic_format_parser
            : public basic_character_set_format_parser_base<CharT> {
        public:
            constexpr character_set_classic_format_parser() = default;

            SCN_NODISCARD bool check_code_point(code_point cp,
                                                detail::locale_ref = {}) const
            {
                // Ensured by sanitize()
                SCN_EXPECT(this->_has_no_set_specifiers());

                if (cp >= 0 && cp <= 0x7f) {
                    return this->_get_char_allowed(static_cast<char>(cp))
                               ? !this->m_inverted_flag_set
                               : this->m_inverted_flag_set;
                }
                if (this->_is_allowed_by_extra_ranges(cp)) {
                    return !this->m_inverted_flag_set;
                }
                return this->m_inverted_flag_set;
            }

            SCN_NODISCARD bool accepts_non_ascii_codepoints() const
            {
                SCN_EXPECT(this->_has_no_set_specifiers());
                return !this->m_extra_ranges.empty();
            }

            scan_error sanitize(detail::locale_ref = {})
            {
                if (!this->_has_no_set_specifiers()) {
                    // specifiers -> chars
                    SCN_GCC_PUSH
                    SCN_GCC_IGNORE("-Wshadow")
                    using specifier =
                        character_set_format_parser_base::specifier;
                    SCN_GCC_POP

                    auto set_range = [&](char a, char b) {
                        for (; a < b; ++a) {
                            this->_enable_char_allowed(a);
                        }
                        this->_enable_char_allowed(b);
                    };
                    auto set_lower = [&]() { set_range('a', 'z'); };
                    auto set_upper = [&]() { set_range('A', 'Z'); };
                    auto set_digit = [&]() { set_range('0', '9'); };

                    if (this->_get_specifier(specifier::letters)) {
                        this->_enable_specifier(specifier::alpha);
                    }
                    if (this->_get_specifier(specifier::alnum_underscore)) {
                        this->_enable_specifier(specifier::alnum);
                        this->_enable_char_allowed(' ');
                    }
                    if (this->_get_specifier(specifier::whitespace)) {
                        this->_enable_specifier(specifier::space);
                    }
                    if (this->_get_specifier(specifier::numbers)) {
                        this->_enable_specifier(specifier::digit);
                    }

                    if (this->_get_specifier(specifier::alnum)) {
                        set_lower();
                        set_upper();
                        set_digit();
                    }
                    if (this->_get_specifier(specifier::alpha)) {
                        set_lower();
                        set_upper();
                    }
                    if (this->_get_specifier(specifier::blank)) {
                        this->_enable_char_allowed(' ');
                        this->_enable_char_allowed('\t');
                    }
                    if (this->_get_specifier(specifier::cntrl)) {
                        set_range(0, 0x1f);
                        this->_enable_char_allowed(0x7f);
                    }
                    if (this->_get_specifier(specifier::digit)) {
                        set_digit();
                    }
                    if (this->_get_specifier(specifier::graph)) {
                        set_range(0x21, 0x7e);
                    }
                    if (this->_get_specifier(specifier::lower)) {
                        set_lower();
                    }
                    if (this->_get_specifier(specifier::print)) {
                        set_range(0x20, 0x7e);
                    }
                    if (this->_get_specifier(specifier::punct)) {
                        set_range(0x21, 0x2f);
                        set_range(0x3a, 0x40);
                        set_range(0x5b, 0x60);
                        set_range(0x7b, 0x7e);
                    }
                    if (this->_get_specifier(specifier::space)) {
                        set_range(0x9, 0xd);
                        this->_enable_char_allowed(' ');
                    }
                    if (this->_get_specifier(specifier::upper)) {
                        set_upper();
                        this->_enable_char_allowed(' ');
                    }
                    if (this->_get_specifier(specifier::xdigit)) {
                        set_digit();
                        set_range('a', 'f');
                        set_range('A', 'F');
                    }

                    if (this->_get_specifier(specifier::inverted_letters)) {
                        set_range(0, 0x40);
                        set_range(0x5b, 0x60);
                        set_range(0x7b, 0x7f);
                    }
                    if (this->_get_specifier(
                            specifier::inverted_alnum_underscore)) {
                        bool underscore_state = this->_get_char_allowed('_');
                        set_range(0, 0x2f);
                        set_range(0x3a, 0x40);
                        set_range(0x5b, 0x60);
                        set_range(0x7b, 0x7f);
                        this->_set_char_allowed('_', underscore_state);
                    }
                    if (this->_get_specifier(specifier::inverted_whitespace)) {
                        bool space_state = this->_get_char_allowed(' ');
                        set_range(0, 0x8);
                        set_range(0xe, 0x7f);
                        this->_set_char_allowed(' ', space_state);
                    }
                    if (this->_get_specifier(specifier::inverted_numbers)) {
                        set_range(0, 0x2f);
                        set_range(0x3a, 0x7f);
                    }

                    this->m_set_specifiers = 0;
                }

                return {};
            }
        };

        constexpr std::pair<character_set_format_parser_base::specifier,
                            std::ctype_base::mask>
            ctype_specifier_map[] = {
                {character_set_format_parser_base::specifier::alnum,
                 std::ctype_base::alnum},
                {character_set_format_parser_base::specifier::alpha,
                 std::ctype_base::alpha},
                {character_set_format_parser_base::specifier::blank,
                 std::ctype_base::blank},
                {character_set_format_parser_base::specifier::cntrl,
                 std::ctype_base::cntrl},
                {character_set_format_parser_base::specifier::digit,
                 std::ctype_base::digit},
                {character_set_format_parser_base::specifier::graph,
                 std::ctype_base::graph},
                {character_set_format_parser_base::specifier::lower,
                 std::ctype_base::lower},
                {character_set_format_parser_base::specifier::print,
                 std::ctype_base::print},
                {character_set_format_parser_base::specifier::punct,
                 std::ctype_base::punct},
                {character_set_format_parser_base::specifier::space,
                 std::ctype_base::space},
                {character_set_format_parser_base::specifier::upper,
                 std::ctype_base::upper},
                {character_set_format_parser_base::specifier::xdigit,
                 std::ctype_base::xdigit},
        };

        template <typename CharT>
        class character_set_localized_format_parser
            : public basic_character_set_format_parser_base<CharT> {
        public:
            constexpr character_set_localized_format_parser() = default;

            SCN_NODISCARD bool check_code_point(code_point cp,
                                                detail::locale_ref loc) const
            {
                if (cp >= 0 && cp <= 0x7f &&
                    this->_get_char_allowed(static_cast<char>(cp))) {
                    return !this->m_inverted_flag_set;
                }
                if ((m_specifier_mask != 0 || m_inverted_specifier_mask != 0) &&
                    check_locale_ctype(cp, loc)) {
                    return !this->m_inverted_flag_set;
                }
                if (this->_is_allowed_by_extra_ranges(cp)) {
                    return !this->m_inverted_flag_set;
                }
                return this->m_inverted_flag_set;
            }

            SCN_NODISCARD bool accepts_non_ascii_codepoints() const
            {
                return !this->m_extra_ranges.empty() ||
                       !this->_has_no_set_specifiers();
            }

            scan_error sanitize(detail::locale_ref loc)
            {
                if (!this->_has_no_set_specifiers()) {
                    // specifiers -> chars
                    SCN_GCC_PUSH
                    SCN_GCC_IGNORE("-Wshadow")
                    using specifier =
                        character_set_format_parser_base::specifier;
                    SCN_GCC_POP

                    if (this->_get_specifier(specifier::letters)) {
                        this->_enable_specifier(specifier::alpha);
                        this->_set_specifier(specifier::letters, false);
                    }
                    if (this->_get_specifier(specifier::alnum_underscore)) {
                        this->_enable_specifier(specifier::alnum);
                        this->_enable_char_allowed(' ');
                        this->_set_specifier(specifier::alnum_underscore,
                                             false);
                    }
                    if (this->_get_specifier(specifier::whitespace)) {
                        this->_enable_specifier(specifier::space);
                        this->_set_specifier(specifier::whitespace, false);
                    }
                    if (this->_get_specifier(specifier::numbers)) {
                        this->_enable_specifier(specifier::digit);
                        this->_set_specifier(specifier::numbers, false);
                    }

                    make_ctype_masks(loc);
                }

                return {};
            }

        private:
            SCN_NODISCARD bool check_locale_ctype(code_point cp,
                                                  detail::locale_ref loc) const
            {
                auto stdloc = loc.get<std::locale>();
                const auto& ctype_facet =
                    get_or_add_facet<std::ctype<wchar_t>>(stdloc);

                if (ctype_facet.is(m_specifier_mask,
                                   static_cast<wchar_t>(cp))) {
                    return true;
                }
                if (!ctype_facet.is(m_inverted_specifier_mask,
                                    static_cast<wchar_t>(cp))) {
                    return true;
                }
                if (this->m_inverted_specifier_underscore &&
                    static_cast<wchar_t>(cp) != L'_') {
                    return true;
                }
                return false;
            }

            void make_ctype_masks(detail::locale_ref)
            {
                m_specifier_mask = [&]() {
                    std::ctype_base::mask m{};
                    for (const auto& elem : ctype_specifier_map) {
                        if (this->_get_specifier(elem.first)) {
                            m |= elem.second;
                        }
                    }
                    return m;
                }();

                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wshadow")
                using specifier = character_set_format_parser_base::specifier;
                SCN_GCC_POP

                if (this->_get_specifier(specifier::inverted_letters)) {
                    m_inverted_specifier_mask |=
                        static_cast<unsigned short>(~std::ctype_base::alpha);
                }
                if (this->_get_specifier(
                        specifier::inverted_alnum_underscore)) {
                    m_inverted_specifier_mask |=
                        static_cast<unsigned short>(~std::ctype_base::alnum);
                    m_inverted_specifier_underscore = true;
                }
                if (this->_get_specifier(specifier::inverted_whitespace)) {
                    m_inverted_specifier_mask |=
                        static_cast<unsigned short>(~std::ctype_base::space);
                }
                if (this->_get_specifier(specifier::inverted_numbers)) {
                    m_inverted_specifier_mask |=
                        static_cast<unsigned short>(~std::ctype_base::digit);
                }

                this->m_set_specifiers = 0;
            }

            std::ctype_base::mask m_specifier_mask{},
                m_inverted_specifier_mask{};
            bool m_inverted_specifier_underscore{false};
        };

        template <typename CharT, typename FormatParser>
        class character_set_reader {
        public:
            explicit character_set_reader(FormatParser& parser)
                : m_parser(parser)
            {
            }

            template <typename SourceRange>
            scan_expected<iterator_value_result<ranges::iterator_t<SourceRange>,
                                                std::basic_string_view<CharT>>>
            read(SourceRange& source, detail::locale_ref loc = {})
            {
                auto begin = source.begin();
                return read_impl(source, loc)
                    .and_then(
                        [&](auto result) -> scan_expected<iterator_value_result<
                                             ranges::iterator_t<SourceRange>,
                                             std::basic_string_view<CharT>>> {
                            if (begin == result.iterator) {
                                return unexpected_scan_error(
                                    scan_error::invalid_scanned_value,
                                    "[character set] matched no characters");
                            }
                            return result;
                        });
            }

            std::basic_string<CharT> buffer;

        private:
            template <typename SourceRange>
            scan_expected<iterator_value_result<ranges::iterator_t<SourceRange>,
                                                std::basic_string_view<CharT>>>
            read_impl(SourceRange& source, detail::locale_ref loc)
            {
                const auto predicate = [&](auto ch) {
                    return !m_parser.check_code_point(make_code_point(ch), loc);
                };

                if constexpr (range_supports_nocopy<SourceRange>()) {
                    if (m_parser.accepts_non_ascii_codepoints()) {
                        return read_until_code_point_nocopy(source, predicate);
                    }
                    return read_until_classic_nocopy(source, predicate);
                }
                else {
                    if (m_parser.accepts_non_ascii_codepoints()) {
                        return read_until_code_point_copying(
                                   source, back_insert(buffer), predicate)
                            .transform([&](auto result) SCN_NOEXCEPT
                                       -> iterator_value_result<
                                           ranges::iterator_t<SourceRange>,
                                           std::basic_string_view<CharT>> {
                                SCN_GCC_PUSH
                                SCN_GCC_IGNORE("-Wconversion")
                                return {result.in, {buffer}};
                                SCN_GCC_POP
                            });
                    }

                    auto [it, _] = read_until_classic_copying(
                        source, back_insert(buffer), predicate);
                    SCN_UNUSED(_);

                    SCN_GCC_PUSH
                    SCN_GCC_IGNORE("-Wconversion")
                    return iterator_value_result<
                        ranges::iterator_t<SourceRange>,
                        std::basic_string_view<CharT>>{it, {buffer}};
                    SCN_GCC_POP
                }
            }

            FormatParser& m_parser;
        };

        template <typename CharT, template <class> class FormatParser>
        auto make_character_set_reader(FormatParser<CharT>& format_parser)
        {
            return character_set_reader<CharT, FormatParser<CharT>>{
                format_parser};
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
