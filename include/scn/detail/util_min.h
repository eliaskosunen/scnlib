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

#ifndef SCN_DETAIL_UTIL_MIN_H
#define SCN_DETAIL_UTIL_MIN_H

#include "fwd.h"

#include <type_traits>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename... Ts>
        struct make_void {
            using type = void;
        };
        template <typename... Ts>
        using void_t = typename make_void<Ts...>::type;

        template <typename... T>
        void valid_expr(T&&...);

        template <typename T>
        struct remove_cvref {
            using type = typename std::remove_cv<
                typename std::remove_reference<T>::type>::type;
        };
        template <typename T>
        using remove_cvref_t = typename remove_cvref<T>::type;

        // Stolen from range-v3
        template <typename T>
        struct static_const {
            static constexpr T value{};
        };
        template <typename T>
        constexpr T static_const<T>::value;

        template <std::size_t I>
        struct priority_tag : priority_tag<I - 1> {
        };
        template <>
        struct priority_tag<0> {
        };

        template <typename T>
        struct dependent_false : std::false_type {
        };

        template <typename T, typename U = T>
        T exchange(T& obj, U&& new_value)
        {
            T old_value = SCN_MOVE(obj);
            obj = SCN_FWD(new_value);
            return old_value;
        }

        template <typename T>
        constexpr T max(T a, T b) noexcept
        {
            return (a < b) ? b : a;
        }

        template <typename It>
        SCN_CONSTEXPR14 It min_element(It first, It last)
        {
            if (first == last) {
                return last;
            }

            It smallest = first;
            ++first;
            for (; first != last; ++first) {
                if (*first < *smallest) {
                    smallest = first;
                }
            }
            return smallest;
        }

        template <typename T>
        constexpr T min(T a, T b) noexcept
        {
            return (b < a) ? b : a;
        }

        template <typename T>
        class SCN_TRIVIAL_ABI unique_ptr {
        public:
            using element_type = T;
            using pointer = T*;

            constexpr unique_ptr() noexcept = default;
            constexpr unique_ptr(std::nullptr_t) noexcept {}

            constexpr explicit unique_ptr(pointer p) noexcept : m_ptr(p) {}

            template <
                typename U,
                typename std::enable_if<
                    std::is_convertible<U*, pointer>::value>::type* = nullptr>
            SCN_CONSTEXPR14 unique_ptr(unique_ptr<U>&& u) noexcept
                : m_ptr(SCN_MOVE(u.get()))
            {
                u.reset();
            }

            unique_ptr(const unique_ptr&) = delete;
            unique_ptr& operator=(const unique_ptr&) = delete;

            SCN_CONSTEXPR14 unique_ptr(unique_ptr&& p) noexcept
                : m_ptr(SCN_MOVE(p.m_ptr))
            {
                p.m_ptr = nullptr;
            }
            unique_ptr& operator=(unique_ptr&& p) noexcept
            {
                if (m_ptr) {
                    delete m_ptr;
                }
                m_ptr = p.m_ptr;
                p.m_ptr = nullptr;
                return *this;
            }

            ~unique_ptr() noexcept
            {
                if (m_ptr) {
                    delete m_ptr;
                }
            }

            constexpr explicit operator bool() const noexcept
            {
                return get() != nullptr;
            }

            constexpr pointer get() const noexcept
            {
                return m_ptr;
            }

            constexpr pointer operator->() const noexcept
            {
                return m_ptr;
            }
            constexpr typename std::add_lvalue_reference<T>::type operator*()
                const
            {
                return *m_ptr;
            }

            SCN_CONSTEXPR14 void reset()
            {
                m_ptr = nullptr;
            }

        private:
            pointer m_ptr{nullptr};
        };

        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wpadded")

        template <typename T, typename... Args>
        unique_ptr<T> make_unique(Args&&... a)
        {
            return unique_ptr<T>(new T(SCN_FWD(a)...));
        }

        template <typename T, std::size_t N>
        struct array {
            static_assert(N > 0, "zero-sized array not supported");

            using value_type = T;
            using size_type = std::size_t;
            using difference_type = std::ptrdiff_t;
            using reference = T&;
            using const_reference = const T&;
            using pointer = T*;
            using const_pointer = const T*;
            using iterator = pointer;
            using const_iterator = const_pointer;

            SCN_CONSTEXPR14 reference operator[](size_type i)
            {
                SCN_EXPECT(i < size());
                return m_data[i];
            }
            constexpr const_reference operator[](size_type i) const
            {
                SCN_EXPECT(i < size());
                return m_data[i];
            }

            SCN_CONSTEXPR14 iterator begin() noexcept
            {
                return m_data;
            }
            constexpr const_iterator begin() const noexcept
            {
                return m_data;
            }
            constexpr const_iterator cbegin() const noexcept
            {
                return m_data;
            }

            SCN_CONSTEXPR14 iterator end() noexcept
            {
                return m_data + N;
            }
            constexpr const_iterator end() const noexcept
            {
                return m_data + N;
            }
            constexpr const_iterator cend() const noexcept
            {
                return m_data + N;
            }

            SCN_CONSTEXPR14 pointer data() noexcept
            {
                return m_data;
            }
            constexpr const_pointer data() const noexcept
            {
                return m_data;
            }

            constexpr size_type size() const noexcept
            {
                return N;
            }

            T m_data[N];
        };

        template <typename CharT>
        CharT ascii_widen(char ch);
        template <>
        constexpr char ascii_widen(char ch)
        {
            return ch;
        }
        template <>
        constexpr wchar_t ascii_widen(char ch)
        {
            return static_cast<wchar_t>(ch);
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_UTIL_MIN_H
