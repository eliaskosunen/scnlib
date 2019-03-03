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

#ifndef SCN_DETAIL_UTIL_H
#define SCN_DETAIL_UTIL_H

#include "config.h"
#include "small_vector.h"

#include <limits>
#include <new>
#include <type_traits>

namespace scn {
#if SCN_HAS_VOID_T
    template <typename... Ts>
    using void_t = std::void_t<Ts...>;
#else
    template <typename... Ts>
    struct make_void {
        using type = void;
    };
    template <typename... Ts>
    using void_t = typename make_void<Ts...>::type;
#endif

    template <size_t I>
    struct priority_tag : priority_tag<I - 1> {
    };
    template <>
    struct priority_tag<0> {
    };

    namespace detail {
        template <typename Integral>
        int _max_digits(int base) noexcept
        {
            using lim = std::numeric_limits<Integral>;

            static int base8_digits[4] = {3, 5, 11, 21};

            if (base == 10) {
                return lim::digits10;
            }
            if (base == 8) {
                return base8_digits[sizeof(Integral) - 1];
            }
            if (base == lim::radix) {
                return lim::digits;
            }

            auto i = lim::max();

            int digits = 0;
            while (i) {
                i /= static_cast<Integral>(base);
                digits++;
            }
            return digits;
        }
        template <typename Integral>
        int max_digits(int base) noexcept
        {
            auto b = base == 0 ? 8 : base;
            auto d = _max_digits<Integral>(b) +
                     (std::is_signed<Integral>::value ? 1 : 0);
            if (base == 0) {
                return d + 2;  // accommondate for 0x/0o
            }
            return d;
        }

        struct disable_copy {
            SCN_CONSTEXPR disable_copy() = default;
            ~disable_copy() = default;

            disable_copy(const disable_copy&) = delete;
            disable_copy& operator=(const disable_copy&) = delete;

            disable_copy(disable_copy&&) noexcept = default;
            disable_copy& operator=(disable_copy&&) noexcept = default;
        };

        template <typename T>
        class unique_ptr : disable_copy {
        public:
            using element_type = T;
            using pointer = T*;

            SCN_CONSTEXPR unique_ptr() noexcept = default;
            SCN_CONSTEXPR unique_ptr(std::nullptr_t) noexcept {}

            SCN_CONSTEXPR explicit unique_ptr(pointer p) : m_ptr(p) {}

            SCN_CONSTEXPR14 unique_ptr(unique_ptr&& p) noexcept
                : m_ptr(std::move(p.m_ptr))
            {
                p.m_ptr = nullptr;
            }
            unique_ptr& operator=(unique_ptr&& p) noexcept
            {
                if (m_ptr) {
                    delete m_ptr;
                }
                m_ptr = p.m_ptr;
                p.m_ptr = 0;
            }

            ~unique_ptr() noexcept
            {
                if (m_ptr) {
                    delete m_ptr;
                }
            }

            SCN_CONSTEXPR explicit operator bool() const noexcept
            {
                return get() != nullptr;
            }

            SCN_CONSTEXPR pointer get() const noexcept
            {
                return m_ptr;
            }

            SCN_CONSTEXPR pointer operator->() const noexcept
            {
                return m_ptr;
            }
            SCN_CONSTEXPR typename std::add_lvalue_reference<T>::type
            operator*() const
            {
                return *m_ptr;
            }

        private:
            pointer m_ptr{nullptr};
        };

        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wpadded")

        template <typename T>
        class erased_storage {
        public:
            using value_type = T;
            using pointer = T*;
            using storage_type =
                typename std::aligned_storage<sizeof(T), alignof(T)>::type;

            erased_storage() noexcept = default;

            erased_storage(T val) noexcept(
                std::is_nothrow_move_constructible<T>::value)
                : m_ptr(::new (static_cast<void*>(&m_data)) T(std::move(val)))
            {
            }

            erased_storage(const erased_storage& other)
                : m_ptr(::new (static_cast<void*>(&m_data)) T(other.get()))
            {
            }
            erased_storage& operator=(const erased_storage& other)
            {
                _destruct();
                m_ptr = ::new (static_cast<void*>(&m_data)) T(other.get());
                return *this;
            }

            erased_storage(erased_storage&& other) noexcept
                : m_ptr(::new (static_cast<void*>(&m_data))
                            T(std::move(other.get())))
            {
            }
            erased_storage& operator=(erased_storage&& other) noexcept
            {
                _destruct();
                m_ptr = ::new (static_cast<void*>(&m_data))
                    T(std::move(other.get()));
                return *this;
            }

            ~erased_storage() noexcept
            {
                _destruct();
            }

            SCN_CONSTEXPR bool has_value() const noexcept
            {
                return m_ptr != nullptr;
            }
            SCN_CONSTEXPR explicit operator bool() const noexcept
            {
                return has_value();
            }

            SCN_CONSTEXPR14 T& get() noexcept
            {
                return _get();
            }
            SCN_CONSTEXPR14 const T& get() const noexcept
            {
                return _get();
            }

            SCN_CONSTEXPR14 T& operator*() noexcept
            {
                return _get();
            }
            SCN_CONSTEXPR14 const T& operator*() const noexcept
            {
                return _get();
            }

            SCN_CONSTEXPR14 T* operator->() noexcept
            {
                return std::addressof(_get());
            }
            SCN_CONSTEXPR14 const T* operator->() const noexcept
            {
                return std::addressof(_get());
            }

        private:
            void _destruct()
            {
                if (m_ptr) {
                    _get().~T();
                }
            }
            static pointer _toptr(storage_type& data)
            {
#if SCN_HAS_LAUNDER
                return std::launder<T>(
                    reinterpret_cast<T*>(std::addressof(data)));
#else
                return reinterpret_cast<T*>(std::addressof(data));
#endif
            }
            SCN_CONSTEXPR14 T& _get() noexcept
            {
                return *m_ptr;
            }
            SCN_CONSTEXPR14 const T& _get() const noexcept
            {
                return *m_ptr;
            }

            storage_type m_data{};
            pointer m_ptr{nullptr};
        };

        SCN_CLANG_POP
    }  // namespace detail
}  // namespace scn

#endif  // SCN_DETAIL_UTIL_H
