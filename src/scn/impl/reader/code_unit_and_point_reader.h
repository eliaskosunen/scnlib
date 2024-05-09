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
#include <scn/impl/algorithms/read.h>
#include <scn/impl/reader/common.h>
#include <scn/impl/reader/integer_reader.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
template <typename CharT>
class code_unit_reader {
public:
    template <typename SourceRange>
    auto read(const SourceRange& range, CharT& ch)
        -> scan_expected<ranges::const_iterator_t<SourceRange>>
    {
        SCN_TRY(it, read_code_unit(range).transform_error(make_eof_scan_error));
        ch = *range.begin();
        return it;
    }
};

template <typename CharT>
class code_point_reader;

template <>
class code_point_reader<char32_t> {
public:
    template <typename SourceRange>
    auto read(const SourceRange& range, char32_t& cp)
        -> scan_expected<ranges::const_iterator_t<SourceRange>>
    {
        auto result = read_code_point_into(range);
        cp = detail::decode_code_point_exhaustive_valid(
            std::basic_string_view<detail::char_t<SourceRange>>{result.value});
        return result.iterator;
    }
};

template <>
class code_point_reader<wchar_t> {
public:
    template <typename SourceRange>
    auto read(const SourceRange& range, wchar_t& ch)
        -> scan_expected<ranges::const_iterator_t<SourceRange>>
    {
        code_point_reader<char32_t> reader{};
        char32_t cp{};
        auto ret = reader.read(range, cp);
        if (SCN_UNLIKELY(!ret)) {
            return unexpected(ret.error());
        }

        SCN_TRY(encoded_ch, encode_code_point_as_wide_character(cp, true));
        ch = encoded_ch;
        return *ret;
    }
};

template <typename ValueCharT>
class char_reader_base {
public:
    constexpr char_reader_base() = default;

    bool skip_ws_before_read() const
    {
        return false;
    }

    static scan_error check_specs(const detail::format_specs& specs)
    {
        reader_error_handler eh{};
        if constexpr (std::is_same_v<ValueCharT, char32_t>) {
            detail::check_code_point_type_specs(specs, eh);
        }
        else {
            detail::check_char_type_specs(specs, eh);
        }
        if (SCN_UNLIKELY(!eh)) {
            return {scan_error::invalid_format_string, eh.m_msg};
        }
        return {};
    }
};

template <typename CharT>
class reader_impl_for_char : public char_reader_base<char> {
public:
    template <typename Range>
    auto read_default(const Range& range, char& value, detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(loc);
        if constexpr (std::is_same_v<CharT, char>) {
            return code_unit_reader<char>{}.read(range, value);
        }
        else {
            SCN_UNUSED(range);
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }
    }

    template <typename Range>
    auto read_specs(const Range& range,
                    const detail::format_specs& specs,
                    char& value,
                    detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        if (specs.type == detail::presentation_type::none ||
            specs.type == detail::presentation_type::character) {
            return read_default(range, value, loc);
        }

        reader_impl_for_int<CharT> reader{};
        signed char tmp_value{};
        auto ret = reader.read_specs(range, specs, tmp_value, loc);
        value = static_cast<signed char>(value);
        return ret;
    }
};

template <typename CharT>
class reader_impl_for_wchar : public char_reader_base<wchar_t> {
public:
    template <typename Range>
    auto read_default(const Range& range,
                      wchar_t& value,
                      detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
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
    auto read_specs(const Range& range,
                    const detail::format_specs& specs,
                    wchar_t& value,
                    detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        if (specs.type == detail::presentation_type::none ||
            specs.type == detail::presentation_type::character) {
            return read_default(range, value, loc);
        }

        reader_impl_for_int<CharT> reader{};
        using integer_type =
            std::conditional_t<sizeof(wchar_t) == 2, int16_t, int32_t>;
        integer_type tmp_value{};
        auto ret = reader.read_specs(range, specs, tmp_value, loc);
        value = static_cast<integer_type>(value);
        return ret;
    }
};

template <typename CharT>
class reader_impl_for_code_point : public char_reader_base<char32_t> {
public:
    template <typename Range>
    auto read_default(const Range& range,
                      char32_t& value,
                      detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(loc);
        return code_point_reader<char32_t>{}.read(range, value);
    }

    template <typename Range>
    auto read_specs(const Range& range,
                    const detail::format_specs& specs,
                    char32_t& value,
                    detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(specs);
        return read_default(range, value, loc);
    }
};
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
