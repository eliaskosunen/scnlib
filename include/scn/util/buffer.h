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

#include <scn/util/memory.h>

#include <array>
#include <memory>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename T, size_t N>
        class basic_buffer {
        public:
            using value_type = T;
            using reference = T&;
            using pointer = T*;
            using const_pointer = const T*;
            using iterator = pointer;
            using const_iterator = const_pointer;
            using size_type = std::size_t;
            using difference_type = std::ptrdiff_t;

            constexpr basic_buffer() SCN_NOEXCEPT = default;

            constexpr basic_buffer(const_pointer first, const_pointer last)
            {
                std::uninitialized_copy(first, last,
                                        reinterpret_cast<T*>(m_buffer.data()));
                m_size = static_cast<size_type>(std::distance(first, last));
            }

            constexpr basic_buffer(const basic_buffer&) = delete;
            constexpr basic_buffer(basic_buffer&&) = delete;

            constexpr basic_buffer& operator=(const basic_buffer&) = delete;
            constexpr basic_buffer& operator=(basic_buffer&&) = delete;

            ~basic_buffer()
            {
                std::destroy_n(data(), size());
            }

            constexpr iterator begin() SCN_NOEXCEPT
            {
                return reinterpret_cast<T*>(m_buffer.data());
            }
            constexpr const_iterator begin() const SCN_NOEXCEPT
            {
                return reinterpret_cast<const T*>(m_buffer.data());
            }

            constexpr iterator end() SCN_NOEXCEPT
            {
                return begin() + size();
            }
            constexpr const_iterator end() const SCN_NOEXCEPT
            {
                return begin() + size();
            }

            constexpr pointer data() SCN_NOEXCEPT
            {
                return begin();
            }
            constexpr const_pointer data() const SCN_NOEXCEPT
            {
                return begin();
            }

            constexpr size_type size() const SCN_NOEXCEPT
            {
                return m_size;
            }

            constexpr size_type max_size() const SCN_NOEXCEPT
            {
                return N;
            }
            constexpr size_type capacity() const SCN_NOEXCEPT
            {
                return N;
            }

            reference push_back(const value_type& val)
            {
                SCN_EXPECT(size() < capacity());
                ::new (static_cast<void*>(end())) value_type(val);
                ++m_size;
            }
            reference push_back(value_type&& val)
            {
                SCN_EXPECT(size() < capacity());
                ::new (static_cast<void*>(end())) value_type(std::move(val));
                ++m_size;
            }

        private:
            alignas(T) std::array<unsigned char, N * sizeof(T)> m_buffer{};
            size_type m_size{0};
        };
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
