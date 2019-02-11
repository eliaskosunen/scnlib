// Copyright 2017-2019 Elias Kosunen
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

#ifndef SCN_DETAIL_RESULT_H
#define SCN_DETAIL_RESULT_H

#include "config.h"
#include "util.h"

#include <type_traits>
#include <utility>

namespace scn {
    /**
     * Error class.
     * Used as a return value for functions without a success value.
     * \see result For a Either-like type
     */
    class error {
    public:
        /// Error code
        enum code : char {
            /// No error
            good,
            /// EOF
            end_of_stream,
            /// Format string was invalid
            invalid_format_string,
            /// Scanned value was invalid for given type.
            /// e.g. a period '.' when scanning for an int
            invalid_scanned_value,
            /// Stream does not support the performed operation
            invalid_operation,
            /// Scanned value was out of range for the desired type.
            /// (e.g. `>2^32` for an `uint32_t`)
            value_out_of_range,
            /// Invalid argument given to operation
            invalid_argument,
            /// The stream has encountered an error that cannot be recovered
            /// from. The stream is now unusable.
            unrecoverable_stream_error,
            /// The stream source emitted an error.
            stream_source_error,
            /// The stream source emitted an error that cannot be recovered
            /// from. The stream is now unusable.
            unrecoverable_stream_source_error
        };

        SCN_CONSTEXPR error() noexcept = default;
        SCN_CONSTEXPR error(code c) noexcept : m_code(c) {}

        /// Evaluated to true if there was no error
        SCN_CONSTEXPR explicit operator bool() const noexcept
        {
            return m_code == good;
        }
        SCN_CONSTEXPR bool operator!() const noexcept
        {
            return !(operator bool());
        }

        /// Get error code
        SCN_CONSTEXPR code get_code() const noexcept
        {
            return m_code;
        }

        /// Can the stream be used again
        SCN_CONSTEXPR bool is_recoverable() const noexcept
        {
            return !(m_code == unrecoverable_stream_error ||
                     m_code == unrecoverable_stream_source_error);
        }

    private:
        code m_code{good};
    };

    SCN_CONSTEXPR inline bool operator==(error a, error b) noexcept
    {
        return a.get_code() == b.get_code();
    }
    SCN_CONSTEXPR inline bool operator!=(error a, error b) noexcept
    {
        return !(a == b);
    }

    /**
     * Either-like type.
     * For situations where there can be a value in case of success or an error
     * code.
     */
    template <typename T, typename Enable = void>
    class result;

#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

    /**
     * Either-like type for default-constructible success values.
     * Not optimized for space-efficiency (both members are stored
     * simultaneously).
     * `error` is used as the error value and discriminant flag.
     */
    template <typename T>
    class result<T,
                 typename std::enable_if<
                     std::is_default_constructible<T>::value>::type> {
    public:
        using success_type = T;

        SCN_CONSTEXPR result(success_type s) : m_s(s) {}
        SCN_CONSTEXPR result(error e) : m_e(e) {}

        SCN_CONSTEXPR bool has_value() const noexcept
        {
            return m_e == error::good;
        }
        SCN_CONSTEXPR explicit operator bool() const noexcept
        {
            return has_value();
        }
        SCN_CONSTEXPR bool operator!() const noexcept
        {
            return !operator bool();
        }

        SCN_CONSTEXPR14 success_type& value() & noexcept
        {
            return m_s;
        }
        SCN_CONSTEXPR success_type value() const& noexcept
        {
            return m_s;
        }
        SCN_CONSTEXPR14 success_type value() && noexcept
        {
            return std::move(m_s);
        }

        SCN_CONSTEXPR14 error& get_error() noexcept
        {
            return m_e;
        }
        SCN_CONSTEXPR error get_error() const noexcept
        {
            return m_e;
        }

    private:
        success_type m_s{};
        error m_e{error::good};
    };

    /**
     * Either-like type for non-default-constructible success values.
     * Not optimized for space-efficiency.
     * `error` is used as the error value and discriminant flag.
     */
    template <typename T>
    class result<T,
                 typename std::enable_if<
                     !std::is_default_constructible<T>::value>::type> {
    public:
        using success_type = T;
        using success_storage = detail::erased_storage<T>;

        result(success_type s) : m_s(std::move(s)) {}
        result(error e) : m_e(e) {}

        bool has_value() const
        {
            return m_e == error::good;
        }
        explicit operator bool() const
        {
            return has_value();
        }
        bool operator!() const
        {
            return !operator bool();
        }

        success_type& value()
        {
            return *m_s;
        }
        const success_type& value() const
        {
            return *m_s;
        }

        error& get_error()
        {
            return m_e;
        }
        error get_error() const
        {
            return m_e;
        }

    private:
        success_storage m_s{};
        error m_e{error::good};
    };

#if SCN_CLANG
    // -Wpadded
#pragma clang diagnostic pop
#endif

    template <typename T,
              typename U = typename std::remove_cv<
                  typename std::remove_reference<T>::type>::type>
    result<U> make_result(T&& val)
    {
        return result<U>(std::forward<T>(val));
    }
    SCN_CONSTEXPR inline error make_error(error::code e) noexcept
    {
        return e;
    }

    namespace detail {
        struct error_handler {
            SCN_CONSTEXPR error_handler() = default;

            void on_error(error e);
            void on_error(const char* msg);
        };
    }  // namespace detail
}  // namespace scn

#endif  // SCN_DETAIL_RESULT_H
