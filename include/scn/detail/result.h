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

#include "util.h"

namespace scn {
    SCN_BEGIN_NAMESPACE
    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")

    /**
     * Error class.
     * Used as a return value for functions without a success value.
     */
    class SCN_TRIVIAL_ABI error {
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
            /// This operation is only possible with exceptions enabled
            exceptions_required,
            /// The stream has encountered an error that cannot be recovered
            /// from. The stream is now unusable.
            unrecoverable_stream_error,
            /// The stream source emitted an error.
            stream_source_error,
            /// The stream source emitted an error that cannot be recovered
            /// from. The stream is now unusable.
            unrecoverable_stream_source_error,

            unrecoverable_internal_error,

            max_error
        };

        struct success_tag_t {
        };
        static SCN_CONSTEXPR success_tag_t success_tag()
        {
            return {};
        }

        SCN_CONSTEXPR error() noexcept = default;
        SCN_CONSTEXPR error(success_tag_t) noexcept : error() {}
        SCN_CONSTEXPR error(enum code c, const char* m) noexcept
            : m_msg(m), m_code(c)
        {
        }

        /// Evaluated to true if there was no error
        SCN_CONSTEXPR explicit operator bool() const noexcept
        {
            return m_code == good;
        }
        SCN_CONSTEXPR bool operator!() const noexcept
        {
            return !(operator bool());
        }

        SCN_CONSTEXPR operator enum code() const noexcept { return m_code; }

        /// Get error code
        SCN_CONSTEXPR enum code code() const noexcept
        {
            return m_code;
        }
        SCN_CONSTEXPR const char* msg() const noexcept
        {
            return m_msg;
        }

        /// Can the stream be used again
        SCN_CONSTEXPR bool is_recoverable() const noexcept
        {
            return !(m_code == unrecoverable_stream_error ||
                     m_code == unrecoverable_stream_source_error ||
                     m_code == unrecoverable_internal_error);
        }

    private:
        const char* m_msg{nullptr};
        enum code m_code { good };
    };

    SCN_CONSTEXPR inline bool operator==(error a, error b) noexcept
    {
        return a.code() == b.code();
    }
    SCN_CONSTEXPR inline bool operator!=(error a, error b) noexcept
    {
        return !(a == b);
    }

    template <typename T, typename Error = ::scn::error>
    class result {
    public:
        using success_type = T;
        using error_type = Error;

        SCN_CONSTEXPR result(success_type val) : m_value(std::move(val)) {}
        SCN_CONSTEXPR result(success_type val, error_type err)
            : m_value(std::move(val)), m_error(std::move(err))
        {
        }

        SCN_CONSTEXPR14 success_type& value() & noexcept
        {
            return m_value;
        }
        SCN_CONSTEXPR const success_type& value() const& noexcept
        {
            return m_value;
        }
        SCN_CONSTEXPR14 success_type value() && noexcept
        {
            return m_value;
        }

        SCN_CONSTEXPR14 error_type& error() & noexcept
        {
            return m_error;
        }
        SCN_CONSTEXPR const error_type& error() const& noexcept
        {
            return m_error;
        }
        SCN_CONSTEXPR14 error_type error() && noexcept
        {
            return m_error;
        }

        SCN_CONSTEXPR explicit operator bool() const noexcept
        {
            return m_error.operator bool();
        }
        SCN_CONSTEXPR bool has_error() const noexcept
        {
            return !(operator bool());
        }

    private:
        success_type m_value;
        Error m_error{Error::success_tag()};
    };

    using scan_result = result<int>;

    /**
     * expected-like type.
     * For situations where there can be a value in case of success or an error
     * code.
     */
    template <typename T, typename Error = ::scn::error, typename Enable = void>
    class expected;

    /**
     * expected-like type for default-constructible success values.
     * Not optimized for space-efficiency (both members are stored
     * simultaneously).
     * `error` is used as the error value and discriminant flag.
     */
    template <typename T, typename Error>
    class expected<T,
                   Error,
                   typename std::enable_if<
                       std::is_default_constructible<T>::value>::type> {
    public:
        using success_type = T;
        using error_type = Error;

        SCN_CONSTEXPR expected(success_type s) : m_s(s) {}
        SCN_CONSTEXPR expected(error_type e) : m_e(e) {}

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

        SCN_CONSTEXPR14 error_type& error() noexcept
        {
            return m_e;
        }
        SCN_CONSTEXPR error_type error() const noexcept
        {
            return m_e;
        }

    private:
        success_type m_s{};
        error_type m_e{error_type::success_tag()};
    };

    /**
     * expected-like type for non-default-constructible success values.
     * Not optimized for space-efficiency.
     * `error` is used as the error value and discriminant flag.
     */
    template <typename T, typename Error>
    class expected<T,
                   Error,
                   typename std::enable_if<
                       !std::is_default_constructible<T>::value>::type> {
    public:
        using success_type = T;
        using success_storage = detail::erased_storage<T>;
        using error_type = Error;

        expected(success_type s) : m_s(std::move(s)) {}
        SCN_CONSTEXPR expected(error_type e) : m_e(e) {}

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

        SCN_CONSTEXPR14 success_type& value() noexcept
        {
            return *m_s;
        }
        SCN_CONSTEXPR const success_type& value() const noexcept
        {
            return *m_s;
        }

        SCN_CONSTEXPR14 error_type& error() noexcept
        {
            return m_e;
        }
        SCN_CONSTEXPR error_type error() const noexcept
        {
            return m_e;
        }

    private:
        success_storage m_s{};
        error_type m_e{error_type::success_tag()};
    };

    // -Wpadded
    SCN_CLANG_POP

    template <typename T,
              typename U = typename std::remove_cv<
                  typename std::remove_reference<T>::type>::type>
    expected<U> make_expected(T&& val)
    {
        return expected<U>(std::forward<T>(val));
    }

    namespace detail {
        struct error_handler {
            SCN_CONSTEXPR error_handler() = default;

            void on_error(error e);
            void on_error(const char* msg);
        };
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_RESULT_H
