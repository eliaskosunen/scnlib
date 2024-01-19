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

#include <scn/fwd.h>

namespace scn {
SCN_BEGIN_NAMESPACE

/**
 * Error class.
 * Used as a return value for functions without a success value.
 *
 * \ingroup result
 */
class SCN_TRIVIAL_ABI scan_error {
public:
    /// Error code
    enum code {
        /// No error
        good = 0,
        /// EOF
        end_of_range,
        /// Format string was invalid
        invalid_format_string,
        /// Scanned value was invalid for given type.
        /// e.g. a period '.' when scanning for an int
        invalid_scanned_value,
        /// Scanned value was out of range for the desired type.
        /// (e.g. `>2^32` for an `uint32_t`)
        value_out_of_range,

        max_error
    };

private:
    using code_t = code;

public:
    struct success_tag_t {};
    static constexpr success_tag_t success_tag() SCN_NOEXCEPT
    {
        return {};
    }

    /// Constructs an error with `code::good` and no message.
    constexpr scan_error() SCN_NOEXCEPT = default;
    constexpr scan_error(success_tag_t) SCN_NOEXCEPT : scan_error() {}

    /// Constructs an error with `c` and `m`
    constexpr scan_error(code_t c, const char* m) SCN_NOEXCEPT : m_msg(m),
                                                                 m_code(c)
    {
        SCN_UNLIKELY_ATTR SCN_UNUSED(m_code);
    }

    /// Evaluated to true if there was no error
    constexpr explicit operator bool() const SCN_NOEXCEPT
    {
        return m_code == good;
    }

    constexpr explicit operator code_t() const SCN_NOEXCEPT
    {
        return m_code;
    }

    /// Get error code
    SCN_NODISCARD constexpr code_t code() const SCN_NOEXCEPT
    {
        return m_code;
    }
    /// Get error message
    SCN_NODISCARD constexpr auto msg() const SCN_NOEXCEPT->const char*
    {
        return m_msg;
    }

private:
    const char* m_msg{nullptr};
    code_t m_code{good};
};

constexpr inline bool operator==(scan_error a, scan_error b) SCN_NOEXCEPT
{
    return a.code() == b.code();
}
constexpr inline bool operator!=(scan_error a, scan_error b) SCN_NOEXCEPT
{
    return !(a == b);
}

constexpr inline bool operator==(scan_error a,
                                 enum scan_error::code b) SCN_NOEXCEPT
{
    return a.code() == b;
}
constexpr inline bool operator!=(scan_error a,
                                 enum scan_error::code b) SCN_NOEXCEPT
{
    return !(a == b);
}

constexpr inline bool operator==(enum scan_error::code a,
                                 scan_error b) SCN_NOEXCEPT
{
    return a == b.code();
}
constexpr inline bool operator!=(enum scan_error::code a,
                                 scan_error b) SCN_NOEXCEPT
{
    return !(a == b);
}

namespace detail {
// Intentionally not constexpr, to give out a compile-time error
scan_error handle_error(scan_error e);
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
