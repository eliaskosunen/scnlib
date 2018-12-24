// Copyright 2017-2018 Elias Kosunen
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

#ifndef SCN_RESULT_H
#define SCN_RESULT_H

#include <memory>
#include <type_traits>
#include <utility>

namespace scn {
    class error {
    public:
        enum code {
            good,
            end_of_stream,
            invalid_format_string,
            invalid_scanned_value,
            invalid_operation,
            value_out_of_range,
            unrecoverable_stream_error,
            stream_source_error,
            unrecoverable_stream_source_error
        };

        error() = default;
        error(code c) : m_code(c) {}

        explicit operator bool() const
        {
            return m_code == good;
        }
        bool operator!() const
        {
            return !(operator bool());
        }

        code get_code() const
        {
            return m_code;
        }

        bool is_recoverable() const
        {
            return !(m_code == unrecoverable_stream_error ||
                     m_code == unrecoverable_stream_source_error);
        }

    private:
        code m_code{good};
    };

    inline bool operator==(error a, error b)
    {
        return a.get_code() == b.get_code();
    }
    inline bool operator!=(error a, error b)
    {
        return !(a == b);
    }

    template <typename T, typename Enable = void>
    class result;

#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

    template <typename T>
    class result<T,
                 typename std::enable_if<
                     std::is_default_constructible<T>::value>::type> {
    public:
        using success_type = T;

        result(success_type s) : m_s(s) {}
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

        success_type& value() &
        {
            return m_s;
        }
        success_type value() const&
        {
            return m_s;
        }
        success_type&& value() &&
        {
            return std::move(m_s);
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
        success_type m_s{};
        error m_e{error::good};
    };

    template <typename T>
    class result<T,
                 typename std::enable_if<
                     !std::is_default_constructible<T>::value>::type> {
        struct deleter {
            void operator()(T* ptr)
            {
                ptr->~T();
            }
        };

    public:
        using deleter_type = deleter;

        using success_type = T;
        using success_storage =
            typename std::aligned_storage<sizeof(T), alignof(T)>::type;
        using success_ptr = std::unique_ptr<success_type, deleter_type>;

        result(success_type s) : m_ptr(::new (&m_s) T(std::move(s))) {}
        result(error e) : m_e(e) {}

        result(const result& o)
        {
            if (o.has_value()) {
                m_ptr.reset(::new (&m_s) T(o.value()));
            }
            else {
                m_e = o.get_error();
            }
        }
        result& operator=(const result& o)
        {
            if (o.has_value()) {
                m_ptr.reset(::new (&m_s) T(o.value()));
            }
            else {
                m_e = o.get_error();
            }
        }

        result(result&& o) noexcept
        {
            if (o.has_value()) {
                m_ptr.reset(::new (&m_s) T(std::move(o.value())));
            }
            else {
                m_e = std::move(o.get_error());
            }
        }
        result& operator=(result&& o) noexcept
        {
            if (o.has_value()) {
                m_ptr.reset(::new (&m_s) T(std::move(o.value())));
            }
            else {
                m_e = std::move(o.get_error());
            }
        }

        ~result() = default;

        bool has_value() const
        {
            return m_ptr != nullptr;
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
            return *m_ptr;
        }
        const success_type& value() const
        {
            return *m_ptr;
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
        success_ptr m_ptr{nullptr};
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
    inline error make_error(error e)
    {
        return e;
    }
}  // namespace scn

#endif  // SCN_RESULT_H

