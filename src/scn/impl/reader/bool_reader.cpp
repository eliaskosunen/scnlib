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

#include <scn/impl/reader/bool_reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        namespace {
            template <typename CharT>
            constexpr bool do_allow_text(uint8_t f)
            {
                return (f & static_cast<uint8_t>(
                                bool_value_reader<CharT>::flags::allow_text)) !=
                       0;
            }

            template <typename CharT>
            constexpr bool do_allow_numeric(uint8_t f)
            {
                return (f &
                        static_cast<uint8_t>(
                            bool_value_reader<CharT>::flags::allow_numeric)) !=
                       0;
            }

            template <typename CharT>
            constexpr bool do_use_localized_numpunct(uint8_t f)
            {
                return (f & static_cast<uint8_t>(
                                bool_value_reader<
                                    CharT>::flags::use_localized_numpunct)) !=
                       0;
            }
        }  // namespace

        template <typename CharT>
        auto bool_value_reader<CharT>::read(string_view_type range,
                                            bool& val) const
            -> scan_expected<iterator>
        {
            auto do_read_text = [&]() -> scan_expected<iterator> {
                if (!do_allow_text<CharT>(m_flags)) {
                    return unexpected(scan_error{});
                }
                if (do_use_localized_numpunct<CharT>(m_flags)) {
                    auto locale = m_locale.get<std::locale>();
                    const auto& numpunct =
                        get_or_add_facet<std::numpunct<CharT>>(locale);
                    const auto truename = numpunct.truename();
                    const auto falsename = numpunct.falsename();
                    return read_text(range, truename, falsename, val);
                }

                return read_text(range, classic_truename(), classic_falsename(),
                                 val);
            };

            return do_read_text()
                .or_else([&](auto) -> scan_expected<iterator> {
                    if (!do_allow_numeric<CharT>(m_flags)) {
                        return unexpected(scan_error{});
                    }

                    return read_numeric(range, val);
                })
                .transform_error([&](scan_error err) SCN_NOEXCEPT {
                    if (err.code() == scan_error::good) {
                        return scan_error{scan_error::invalid_scanned_value,
                                          "Failed to scan boolean"};
                    }
                    return err;
                });
        }

        template <typename CharT>
        auto bool_value_reader<CharT>::read_text(string_view_type range,
                                                 string_view_type truename,
                                                 string_view_type falsename,
                                                 bool& val) const
            -> scan_expected<iterator>
        {
            if (range.size() >= truename.size() &&
                std::equal(truename.begin(), truename.end(), range.begin())) {
                val = true;
                return range.begin() + truename.size();
            }

            if (range.size() >= falsename.size() &&
                std::equal(falsename.begin(), falsename.end(), range.begin())) {
                val = false;
                return range.begin() + falsename.size();
            }

            return unexpected(scan_error{});
        }

        template <typename CharT>
        auto bool_value_reader<CharT>::read_numeric(string_view_type range,
                                                    bool& val) const
            -> scan_expected<iterator>
        {
            if (range.size() == 0) {
                return unexpected(scan_error{});
            }

            if (range.front() == '0') {
                val = false;
                return ranges::next(range.begin());
            }
            if (range.front() == '1') {
                val = true;
                return ranges::next(range.begin());
            }

            return unexpected(scan_error{});
        }

        template auto bool_value_reader<char>::read(std::string_view,
                                                    bool&) const
            -> scan_expected<std::string_view::iterator>;
        template auto bool_value_reader<wchar_t>::read(std::wstring_view,
                                                       bool&) const
            -> scan_expected<std::wstring_view::iterator>;

        template auto bool_value_reader<char>::read_text(std::string_view,
                                                         std::string_view,
                                                         std::string_view,
                                                         bool&) const
            -> scan_expected<std::string_view::iterator>;
        template auto bool_value_reader<wchar_t>::read_text(std::wstring_view,
                                                            std::wstring_view,
                                                            std::wstring_view,
                                                            bool&) const
            -> scan_expected<std::wstring_view::iterator>;

        template auto bool_value_reader<char>::read_numeric(std::string_view,
                                                            bool&) const
            -> scan_expected<std::string_view::iterator>;
        template auto bool_value_reader<wchar_t>::read_numeric(
            std::wstring_view,
            bool&) const -> scan_expected<std::wstring_view::iterator>;
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
