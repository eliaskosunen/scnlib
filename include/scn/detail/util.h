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

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
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

        template <typename T>
        struct static_const {
            static SCN_CONSTEXPR T value{};
        };
        template <typename T>
        SCN_CONSTEXPR T static_const<T>::value;

        template <size_t I>
        struct priority_tag : priority_tag<I - 1> {
        };
        template <>
        struct priority_tag<0> {
        };

        template <typename Integral>
        SCN_CONSTEXPR14 int _max_digits(int base) noexcept
        {
            using lim = std::numeric_limits<Integral>;

            char base8_digits[8] = {3, 5, 0, 11, 0, 0, 0, 21};

            if (base == 10) {
                return lim::digits10;
            }
            if (base == 8) {
                return static_cast<int>(base8_digits[sizeof(Integral) - 1]);
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
        SCN_CONSTEXPR14 int max_digits(int base) noexcept
        {
            auto b = base == 0 ? 8 : base;
            auto d = _max_digits<Integral>(b) +
                     (std::is_signed<Integral>::value ? 1 : 0);
            if (base == 0) {
                return d + 2;  // accommondate for 0x/0o
            }
            return d;
        }

        template <typename T>
        SCN_CONSTEXPR T* launder(T* p) noexcept
        {
#if SCN_HAS_LAUNDER
            return std::launder(p);
#else
            return p;
#endif
        }

        template <typename CharT>
        CharT ascii_widen(char ch);
        template <>
        SCN_CONSTEXPR char ascii_widen(char ch)
        {
            return ch;
        }
        template <>
        SCN_CONSTEXPR wchar_t ascii_widen(char ch)
        {
            return static_cast<wchar_t>(ch);
        }

        template <typename T>
        SCN_CONSTEXPR T max(T a, T b) noexcept
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
        SCN_CONSTEXPR T min(T a, T b) noexcept
        {
            return (b < a) ? b : a;
        }
        template <typename T>
        SCN_CONSTEXPR14 T min(std::initializer_list<T> list) noexcept
        {
            return *min_element(list.begin(), list.end());
        }

        template <typename ForwardIt, typename T>
        ForwardIt remove(ForwardIt first, ForwardIt last, const T& value)
        {
            for (; first != last; ++first) {
                if (*first == value) {
                    break;
                }
            }
            if (first != last) {
                for (ForwardIt i = first; ++i != last;) {
                    if (!(*i == value)) {
                        *first++ = std::move(*i);
                    }
                }
            }
            return first;
        }

        template <class InputIt, class T>
        SCN_CONSTEXPR14 InputIt find(InputIt first,
                                     InputIt last,
                                     const T& value)
        {
            for (; first != last; ++first) {
                if (*first == value) {
                    return first;
                }
            }
            return last;
        }

        template <typename T>
        class SCN_TRIVIAL_ABI unique_ptr {
        public:
            using element_type = T;
            using pointer = T*;

            SCN_CONSTEXPR unique_ptr() noexcept = default;
            SCN_CONSTEXPR unique_ptr(std::nullptr_t) noexcept {}

            SCN_CONSTEXPR explicit unique_ptr(pointer p) noexcept : m_ptr(p) {}

            template <
                typename U,
                typename std::enable_if<
                    std::is_convertible<U*, pointer>::value>::type* = nullptr>
            SCN_CONSTEXPR14 unique_ptr(unique_ptr<U>&& u) noexcept
                : m_ptr(std::move(u.get()))
            {
                u.reset();
            }

            unique_ptr(const unique_ptr&) = delete;
            unique_ptr& operator=(const unique_ptr&) = delete;

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
                p.m_ptr = nullptr;
                return *this;
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
            return unique_ptr<T>(new T(std::forward<Args>(a)...));
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
            SCN_CONSTEXPR const_reference operator[](size_type i) const
            {
                SCN_EXPECT(i < size());
                return m_data[i];
            }

            SCN_CONSTEXPR14 iterator begin() noexcept
            {
                return m_data;
            }
            SCN_CONSTEXPR const_iterator begin() const noexcept
            {
                return m_data;
            }
            SCN_CONSTEXPR const_iterator cbegin() const noexcept
            {
                return m_data;
            }

            SCN_CONSTEXPR14 iterator end() noexcept
            {
                return m_data + N;
            }
            SCN_CONSTEXPR const_iterator end() const noexcept
            {
                return m_data + N;
            }
            SCN_CONSTEXPR const_iterator cend() const noexcept
            {
                return m_data + N;
            }

            SCN_CONSTEXPR14 pointer data() noexcept
            {
                return m_data;
            }
            SCN_CONSTEXPR const_pointer data() const noexcept
            {
                return m_data;
            }

            SCN_CONSTEXPR size_type size() const noexcept
            {
                return N;
            }

            T m_data[N];
        };

        template <typename T>
        class SCN_TRIVIAL_ABI erased_storage {
        public:
            using value_type = T;
            using pointer = T*;
            using storage_type = unsigned char[sizeof(T)];

            SCN_CONSTEXPR erased_storage() noexcept = default;

            erased_storage(T val) noexcept(
                std::is_nothrow_move_constructible<T>::value)
                : m_ptr(::new (static_cast<void*>(&m_data)) T(std::move(val)))
            {
            }

            erased_storage(const erased_storage& other)
                : m_ptr(other ? ::new (static_cast<void*>(&m_data))
                                    T(other.get())
                              : nullptr)
            {
            }
            erased_storage& operator=(const erased_storage& other)
            {
                _destruct();
                if (other) {
                    m_ptr = ::new (static_cast<void*>(&m_data)) T(other.get());
                }
                return *this;
            }

            erased_storage(erased_storage&& other) noexcept
                : m_ptr(other ? ::new (static_cast<void*>(&m_data))
                                    T(std::move(other.get()))
                              : nullptr)
            {
                other.m_ptr = nullptr;
            }
            erased_storage& operator=(erased_storage&& other) noexcept
            {
                _destruct();
                if (other) {
                    m_ptr = ::new (static_cast<void*>(&m_data))
                        T(std::move(other.get()));
                    other.m_ptr = nullptr;
                }
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
                SCN_EXPECT(has_value());
                return _get();
            }
            SCN_CONSTEXPR14 const T& get() const noexcept
            {
                SCN_EXPECT(has_value());
                return _get();
            }

            SCN_CONSTEXPR14 T& operator*() noexcept
            {
                SCN_EXPECT(has_value());
                return _get();
            }
            SCN_CONSTEXPR14 const T& operator*() const noexcept
            {
                SCN_EXPECT(has_value());
                return _get();
            }

            SCN_CONSTEXPR14 T* operator->() noexcept
            {
                return m_ptr;
            }
            SCN_CONSTEXPR14 const T* operator->() const noexcept
            {
                return m_ptr;
            }

        private:
            void _destruct()
            {
                if (m_ptr) {
                    _get().~T();
                }
                m_ptr = nullptr;
            }
            static pointer _toptr(storage_type& data)
            {
                return ::scn::detail::launder(
                    reinterpret_cast<T*>(std::addressof(data)));
            }
            SCN_CONSTEXPR14 T& _get() noexcept
            {
                return *m_ptr;
            }
            SCN_CONSTEXPR14 const T& _get() const noexcept
            {
                return *m_ptr;
            }

            alignas(T) storage_type m_data{};
            pointer m_ptr{nullptr};
        };

        SCN_CLANG_POP
    }  // namespace detail

    template <typename T>
    class wrap_default {
    public:
        using value_type = T;
        using storage_type = detail::erased_storage<T>;

        wrap_default() = default;

        wrap_default(value_type&& val) : m_storage(std::move(val)) {}
        wrap_default& operator=(value_type&& val)
        {
            m_storage = storage_type(std::move(val));
            return *this;
        }

        SCN_CONSTEXPR bool has_value() const noexcept
        {
            return m_storage.operator bool();
        }
        SCN_CONSTEXPR explicit operator bool() const noexcept
        {
            return has_value();
        }

        SCN_CONSTEXPR14 T& get() noexcept
        {
            return m_storage.get();
        }
        SCN_CONSTEXPR14 const T& get() const noexcept
        {
            return m_storage.get();
        }

        SCN_CONSTEXPR14 T& operator*() noexcept
        {
            return get();
        }
        SCN_CONSTEXPR14 const T& operator*() const noexcept
        {
            return get();
        }

        SCN_CONSTEXPR14 T* operator->() noexcept
        {
            return m_storage.operator->();
        }
        SCN_CONSTEXPR14 const T* operator->() const noexcept
        {
            return m_storage.operator->();
        }

    private:
        storage_type m_storage;
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_UTIL_H
