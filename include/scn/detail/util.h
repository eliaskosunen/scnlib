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

#include "util_min.h"

#include <cmath>
#include <limits>
#include <new>
#include <utility>

#if SCN_MSVC
#include "string_view.h"
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
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
        bool float_eq(T a, T b, T tolerance = std::numeric_limits<T>::epsilon())
        {
            T diff = std::abs(a - b);
            if (diff <= tolerance) {
                return true;
            }
            return diff < std::fmax(std::abs(a), std::abs(b)) * tolerance;
        }
        template <typename T>
        bool float_eq_zero(T a, T tolerance = std::numeric_limits<T>::epsilon())
        {
            return std::abs(a) < tolerance;
        }
        template <typename T>
        bool float_eq_within(T a, T b, std::size_t interval = 1)
        {
            T min_a =
                a - (a - std::nextafter(a, std::numeric_limits<T>::lowest())) *
                        interval;
            T max_a =
                a + (std::nextafter(a, std::numeric_limits<T>::max()) - a) *
                        interval;

            return min_a <= b && max_a >= b;
        }

        template <typename T>
        struct zero_value;
        template <>
        struct zero_value<float> {
            static constexpr float value = 0.0f;
        };
        template <>
        struct zero_value<double> {
            static constexpr double value = 0.0;
        };
        template <>
        struct zero_value<long double> {
            static constexpr long double value = 0.0l;
        };

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
            -> decltype(::scn::detail::to_address_impl(SCN_FWD(p),
                                                       priority_tag<2>{}))
        {
            return ::scn::detail::to_address_impl(SCN_FWD(p),
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
        class SCN_TRIVIAL_ABI erased_storage {
        public:
            using value_type = T;
            using pointer = T*;
            using storage_type = unsigned char[sizeof(T)];

            constexpr erased_storage() noexcept = default;

            erased_storage(T val) noexcept(
                std::is_nothrow_move_constructible<T>::value)
                : m_ptr(::new (static_cast<void*>(&m_data)) T(SCN_MOVE(val)))
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
                                    T(SCN_MOVE(other.get()))
                              : nullptr)
            {
                other.m_ptr = nullptr;
            }
            erased_storage& operator=(erased_storage&& other) noexcept
            {
                _destruct();
                if (other) {
                    m_ptr = ::new (static_cast<void*>(&m_data))
                        T(SCN_MOVE(other.get()));
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
                    reinterpret_cast<T*>(reinterpret_cast<void*>(data.data())));
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
     * Useful when scanning non-default-constructible types, especially with
     * <tuple_return.h>:
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

        optional(value_type&& val) : m_storage(SCN_MOVE(val)) {}
        optional& operator=(value_type&& val)
        {
            m_storage = storage_type(SCN_MOVE(val));
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
