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

#include <scn/util/expected.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
enum class eof_error { good, eof };

inline constexpr bool operator!(eof_error e)
{
    return e != eof_error::good;
}

template <typename T>
struct eof_expected : public expected<T, eof_error> {
    using base = expected<T, eof_error>;
    using base::base;

    constexpr eof_expected(const base& other) : base(other) {}
    constexpr eof_expected(base&& other) : base(SCN_MOVE(other)) {}
};

inline constexpr auto make_eof_scan_error(eof_error err)
{
    SCN_EXPECT(err == eof_error::eof);
    return scan_error{scan_error::end_of_range, "EOF"};
}

struct SCN_TRIVIAL_ABI parse_error {
    enum code { good, eof, error };
    using code_t = code;

    constexpr parse_error() = default;
    constexpr parse_error(code c) : m_code(c)
    {
        SCN_UNLIKELY_ATTR SCN_UNUSED(m_code);
    }

    constexpr explicit operator bool() const
    {
        return m_code == good;
    }
    constexpr explicit operator code_t() const
    {
        return m_code;
    }

    friend constexpr bool operator==(parse_error a, parse_error b)
    {
        return a.m_code == b.m_code;
    }
    friend constexpr bool operator!=(parse_error a, parse_error b)
    {
        return !(a == b);
    }

private:
    code m_code{good};
};

template <typename T>
struct parse_expected : public expected<T, parse_error> {
    using base = expected<T, parse_error>;
    using base::base;

    constexpr parse_expected(const base& other) : base(other) {}
    constexpr parse_expected(base&& other) : base(SCN_MOVE(other)) {}
};

inline constexpr parse_error make_eof_parse_error(eof_error err)
{
    SCN_EXPECT(err == eof_error::eof);
    return parse_error::eof;
}

inline constexpr scan_error make_scan_error_from_parse_error(
    parse_error err,
    enum scan_error::code code,
    const char* msg)
{
    if (err == parse_error::good) {
        return {};
    }

    if (err == parse_error::eof) {
        return scan_error{scan_error::end_of_range, "EOF"};
    }

    return scan_error{code, msg};
}

inline constexpr auto map_parse_error_to_scan_error(enum scan_error::code code,
                                                    const char* msg)
{
    return [code, msg](parse_error err) {
        return make_scan_error_from_parse_error(err, code, msg);
    };
}
}  // namespace impl

namespace detail {
template <typename T>
struct is_expected_impl<scn::impl::parse_expected<T>> : std::true_type {};
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
