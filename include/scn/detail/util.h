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

#ifndef SCN_DETAIL_UTIL_H
#define SCN_DETAIL_UTIL_H

#include "config.h"

#include <iterator>
#include <limits>
#include <new>
#include <type_traits>

namespace scn {
    namespace detail {
        /**
         * Maximum digits potentially required to represent an integer of type
         * Integral. Includes possible sign.
         * \param base Base of the integer
         */
        template <typename Integral>
        int max_digits(int base) noexcept
        {
            auto i = std::numeric_limits<Integral>::max();

            int digits = 0;
            while (i) {
                i /= static_cast<Integral>(base);
                digits++;
            }

            return digits + (std::is_signed<Integral>::value ? 1 : 0);
        }

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
    }  // namespace detail
}  // namespace scn

#endif  // SCN_DETAIL_UTIL_H
