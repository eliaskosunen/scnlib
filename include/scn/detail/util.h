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

#ifndef SCN_DETAIL_UTIL_H
#define SCN_DETAIL_UTIL_H

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

#include "config.h"

#if SCN_HAS_INCLUDE(<string_view>) && SCN_STD >= SCN_STD_17
#include <string_view>
#endif

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

        template <size_t I>
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
            T old_value = std::move(obj);
            obj = std::forward<U>(new_value);
            return old_value;
        }

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

            Integral digits = 0;
            while (i) {
                i /= static_cast<Integral>(base);
                digits++;
            }
            return static_cast<int>(digits);
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
        constexpr std::pair<T, T> div(T l, T r) noexcept
        {
            return {l / r, l % r};
        }

        template <typename T>
        constexpr T* launder(T* p) noexcept
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
        constexpr char ascii_widen(char ch)
        {
            return ch;
        }
        template <>
        constexpr wchar_t ascii_widen(char ch)
        {
            return static_cast<wchar_t>(ch);
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
        SCN_CONSTEXPR14 T min(std::initializer_list<T> list) noexcept
        {
            return *min_element(list.begin(), list.end());
        }

        template <typename CharT>
        bool is_base_digit(CharT ch, int base)
        {
            if (base <= 10) {
                return ch >= ascii_widen<CharT>('0') &&
                       ch <= ascii_widen<CharT>('0') + base - 1;
            }
            return is_base_digit(ch, 10) ||
                   (ch >= ascii_widen<CharT>('a') &&
                    ch <= ascii_widen<CharT>('a') + base - 1) ||
                   (ch >= ascii_widen<CharT>('A') &&
                    ch <= ascii_widen<CharT>('A') + base - 1);
        }

        template <typename T>
        struct pointer_traits;

        template <typename T>
        struct pointer_traits<T*> {
            using pointer = T*;
            using element_type = T;
            using difference_type = std::ptrdiff_t;

            template <typename U>
            using rebind = U*;

            template <typename U = T,
                      typename std::enable_if<!std::is_void<U>::value>::type* =
                          nullptr>
            static SCN_CONSTEXPR14 pointer pointer_to(U& r) noexcept
            {
                return std::addressof(r);
            }
        };

        template <typename T>
        T* to_address_impl(T* p, priority_tag<2>) noexcept
        {
            return p;
        }
        template <typename Ptr>
        auto to_address_impl(const Ptr& p, priority_tag<1>) noexcept
            -> decltype(::scn::detail::pointer_traits<Ptr>::to_address(p))
        {
            return ::scn::detail::pointer_traits<Ptr>::to_address(p);
        }
        template <typename Ptr>
        auto to_address_impl(const Ptr& p, priority_tag<0>) noexcept
            -> decltype(::scn::detail::to_address_impl(p.operator->(),
                                                       priority_tag<2>{}))
        {
            return ::scn::detail::to_address_impl(p.operator->(),
                                                  priority_tag<2>{});
        }

        template <typename Ptr>
        auto to_address(Ptr&& p) noexcept
            -> decltype(::scn::detail::to_address_impl(std::forward<Ptr>(p),
                                                       priority_tag<2>{}))
        {
            return ::scn::detail::to_address_impl(std::forward<Ptr>(p),
                                                  priority_tag<2>{});
        }

        // Workaround for MSVC _String_view_iterator
#if SCN_MSVC && SCN_HAS_STRING_VIEW
        template <typename Traits>
        struct pointer_traits<std::_String_view_iterator<Traits>> {
            using iterator = std::_String_view_iterator<Traits>;
            using pointer = typename iterator::pointer;
            using element_type = typename iterator::value_type;
            using difference_type = typename iterator::difference_type;

            static constexpr pointer to_address(const iterator& it) noexcept
            {
                // operator-> of _String_view_iterator
                // is checked for past-the-end dereference,
                // even though operator-> isn't dereferencing anything :)))
                return it._Unwrapped();
            }
        };
#endif

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

        template <typename T>
        class SCN_TRIVIAL_ABI erased_storage {
        public:
            using value_type = T;
            using pointer = T*;
            using storage_type = unsigned char[sizeof(T)];

            constexpr erased_storage() noexcept = default;

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

            constexpr bool has_value() const noexcept
            {
                return m_ptr != nullptr;
            }
            constexpr explicit operator bool() const noexcept
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

    /**
     * A very lackluster optional implementation.
     * Useful when scanning non-default-constructible types, especially with <tuple_return.h>:
     *
     * \code{.cpp}
     * // implement scn::scanner for optional<mytype>
     * optional<mytype> val;
     * scn::scan(source, "{}", val);
     *
     * // with tuple_return:
     * auto [result, val] = scn::scan_tuple<optional<mytype>>(source, "{}");
     * \endcode
     */
    template <typename T>
    class optional {
    public:
        using value_type = T;
        using storage_type = detail::erased_storage<T>;

        optional() = default;

        optional(value_type&& val) : m_storage(std::move(val)) {}
        optional& operator=(value_type&& val)
        {
            m_storage = storage_type(std::move(val));
            return *this;
        }

        constexpr bool has_value() const noexcept
        {
            return m_storage.operator bool();
        }
        constexpr explicit operator bool() const noexcept
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
