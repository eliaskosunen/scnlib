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

#ifndef SCN_DETAIL_SMALL_VECTOR_H
#define SCN_DETAIL_SMALL_VECTOR_H

#include "config.h"

#include <iterator>
#include <limits>
#include <new>

namespace scn {
    namespace detail {
        template <typename Iter>
        std::reverse_iterator<Iter> make_reverse_iterator(Iter i)
        {
            return std::reverse_iterator<Iter>(i);
        }

        struct small_vector_base {
            uint64_t next_pow2(uint64_t x)
            {
                --x;
                x |= (x >> 1);
                x |= (x >> 2);
                x |= (x >> 4);
                x |= (x >> 8);
                x |= (x >> 16);
                x |= (x >> 32);
                return x + 1;
            }
            uint32_t next_pow2(uint32_t x)
            {
                --x;
                x |= (x >> 1);
                x |= (x >> 2);
                x |= (x >> 4);
                x |= (x >> 8);
                x |= (x >> 16);
                return x + 1;
            }

            template <typename ForwardIt, typename T>
            static void uninitialized_fill(ForwardIt first,
                                           ForwardIt last,
                                           const T& value) noexcept
            {
                using value_type =
                    typename std::iterator_traits<ForwardIt>::value_type;
                ForwardIt current = first;
                for (; current != last; ++current) {
                    ::new (static_cast<void*>(std::addressof(*current)))
                        value_type(value);
                }
            }
            template <typename T, typename ForwardIt>
            static void uninitialized_fill_default_construct(
                ForwardIt first,
                ForwardIt last) noexcept
            {
                using value_type =
                    typename std::iterator_traits<ForwardIt>::value_type;
                ForwardIt current = first;
                for (; current != last; ++current) {
                    ::new (static_cast<void*>(std::addressof(*current)))
                        value_type();
                }
            }
            template <typename InputIt, typename ForwardIt>
            static ForwardIt uninitialized_copy(InputIt first,
                                                InputIt last,
                                                ForwardIt d_first) noexcept
            {
                using value_type =
                    typename std::iterator_traits<ForwardIt>::value_type;
                ForwardIt current = d_first;
                for (; first != last; ++first, (void)++current) {
                    ::new (static_cast<void*>(std::addressof(*current)))
                        value_type(*first);
                }
                return current;
            }
            template <typename InputIt, typename ForwardIt>
            static ForwardIt uninitialized_move(InputIt first,
                                                InputIt last,
                                                ForwardIt d_first) noexcept
            {
                using value_type =
                    typename std::iterator_traits<ForwardIt>::value_type;
                ForwardIt current = d_first;
                for (; first != last; ++first, (void)++current) {
                    ::new (static_cast<void*>(std::addressof(*current)))
                        value_type(std::move(*first));
                }
                return current;
            }
        };

        template <typename T>
        using basic_stack_storage_type =
            typename std::aligned_storage<sizeof(T), alignof(T)>::type;

        template <typename T, size_t N>
        struct basic_stack_storage {
            basic_stack_storage_type<T> data[N];
            size_t size{0};

            T* get_data()
            {
                return reinterpret_cast<T*>(data);
            }
            const T* get_data() const
            {
                return reinterpret_cast<const T*>(data);
            }
        };
        template <typename T>
        struct basic_stack_storage<T, 0> {
            basic_stack_storage_type<T>* data{nullptr};
            size_t size{0};

            T* get_data()
            {
                return nullptr;
            }
            const T* get_data() const
            {
                return nullptr;
            }
        };

        template <typename T>
        SCN_CONSTEXPR T constexpr_max(T val)
        {
            return val;
        }
        template <typename T, typename... Ts>
        SCN_CONSTEXPR T constexpr_max(T val, Ts... a)
        {
            return val > constexpr_max(a...) ? val : constexpr_max(a...);
        }

        template <typename... Types>
        struct aligned_union {
            static SCN_CONSTEXPR const size_t alignment_value =
                constexpr_max(alignof(Types)...);
            static SCN_CONSTEXPR const size_t size_value =
                constexpr_max(sizeof(Types)...);
            struct type {
                alignas(alignment_value) unsigned char _s[size_value];
            };
        };

        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wpadded")

        template <typename T, size_t StackN>
        class small_vector : protected small_vector_base {
        public:
            using value_type = T;
            using size_type = size_t;
            using difference_type = std::ptrdiff_t;
            using reference = T&;
            using const_reference = const T&;
            using pointer = T*;
            using const_pointer = const T*;
            using iterator = pointer;
            using const_iterator = const_pointer;
            using reverse_iterator = std::reverse_iterator<pointer>;
            using const_reverse_iterator = std::reverse_iterator<const_pointer>;

            using stack_storage_type = basic_stack_storage_type<T>;

            struct stack_storage : basic_stack_storage<T, StackN> {
            };
            struct heap_storage {
                pointer ptr{nullptr};
                size_type size{0};
                size_type cap{0};
            };

            using storage_type =
                typename aligned_union<stack_storage, heap_storage>::type;

            small_vector() noexcept
            {
                if (StackN != 0) {
                    _construct_stack_storage();
                }
                else {
                    _construct_heap_storage();
                }
            }

            explicit small_vector(size_type count, const T& value)
            {
                if (!can_be_small(count)) {
                    _construct_heap_storage();
                    auto cap = next_pow2(count);
                    auto storage_ptr = new stack_storage_type[count];
                    auto ptr = reinterpret_cast<pointer>(storage_ptr);
                    this->uninitialized_fill(ptr, ptr + count, value);

                    _get_heap().ptr = ptr;
                    _get_heap().cap = cap;
                }
                else {
                    _construct_stack_storage();
                    this->uninitialized_fill(_get_stack().get_data(),
                                             _get_stack().get_data() + StackN,
                                             value);
                }
                _set_size(count);
            }

            explicit small_vector(size_type count)
            {
                if (!can_be_small(count)) {
                    _construct_heap_storage();
                    auto cap = next_pow2(count);
                    auto storage_ptr = new stack_storage_type[count];
                    auto ptr = reinterpret_cast<pointer>(storage_ptr);
                    this->uninitialized_fill_default_construct<T>(ptr,
                                                                  ptr + count);

                    _get_heap().ptr = ptr;
                    _get_heap().cap = cap;
                }
                else {
                    _construct_stack_storage();
                    this->uninitialized_fill_default_construct<T>(
                        _get_stack().get_data(),
                        _get_stack().get_data() + count);
                }
                _set_size(count);
            }

            small_vector(const small_vector& other)
            {
                if (other.empty()) {
                    _construct_stack_storage();
                    return;
                }

                auto size = other.size();
                if (!other.is_small()) {
                    _construct_heap_storage();
                    auto cap = other.capacity();
                    auto optr = other.data();

                    auto storage_ptr = new stack_storage_type[cap];
                    auto ptr = reinterpret_cast<pointer>(storage_ptr);
                    this->uninitialized_copy(optr, optr + size, ptr);

                    _get_heap().ptr = ptr;
                    _get_heap().cap = cap;
                }
                else {
                    _construct_stack_storage();
                    auto optr = other.data();
                    this->uninitialized_copy(optr, optr + size,
                                             _get_stack().get_data());
                }
                _set_size(size);
            }
            small_vector(small_vector&& other) noexcept
            {
                if (other.empty()) {
                    _construct_stack_storage();
                    return;
                }

                auto size = other.size();
                if (other.m_heap) {
                    _construct_heap_storage();
                    _get_heap().ptr = other.data();
                    _get_heap().cap = other.capacity();

                    other._get_heap().ptr = nullptr;
                    other._get_heap().size = 0;
                    other._get_heap().cap = 0;
                }
                else {
                    _construct_stack_storage();
                    auto optr = other.data();
                    this->uninitialized_move(optr, optr + size,
                                             _get_stack().get_data());
                    _get_stack().size = size;

                    other._destruct_elements();
                    other._get_stack().size = 0;
                }
                _set_size(size);
            }

            small_vector& operator=(const small_vector& other)
            {
                _destruct_elements();

                if (other.empty()) {
                    return *this;
                }

                // this other
                // s s      false || true
                // s h      false || false second
                // h s      true || true
                // h h      true || false
                if (!is_small() || other.is_small()) {
                    this->uninitialized_copy(
                        other.data(), other.data() + other.size(), data());
                }
                else {
                    _destruct_stack_storage();
                    _construct_heap_storage();

                    auto cap = next_pow2(other.size());
                    auto storage_ptr = new stack_storage_type[cap];
                    auto ptr = reinterpret_cast<pointer>(storage_ptr);
                    this->uninitialized_copy(other.data(),
                                             other.data() + other.size(), ptr);
                    _get_heap().ptr = ptr;
                }
                _set_size(other.size());
                return *this;
            }

            small_vector& operator=(small_vector&& other) noexcept
            {
                _destruct_elements();

                if (other.empty()) {
                    return *this;
                }

                if (!is_small() && !other.is_small()) {
                    if (!is_small()) {
                        if (capacity() != 0) {
                            delete[] reinterpret_cast<stack_storage_type*>(
                                _get_heap().ptr);
                        }
                    }

                    _get_heap().ptr = other.data();
                    _get_heap().cap = other.capacity();
                    _get_heap().size = other.size();
                }
                else if (!is_small() || other.is_small()) {
                    this->uninitialized_move(
                        other.data(), other.data() + other.size(), data());
                    _set_size(other.size());
                    other._destruct_elements();
                }
                else {
                    _destruct_stack_storage();
                    _construct_heap_storage();

                    _get_heap().ptr = other.data();
                    _get_heap().cap = other.capacity();
                    _get_heap().size = other.size();
                }

                other._set_size(0);
                if (!other.is_small()) {
                    other._get_heap().ptr = nullptr;
                    other._get_heap().cap = 0;
                    other._destruct_heap_storage();
                    other._construct_stack_storage();
                }

                return *this;
            }

            ~small_vector()
            {
                _destruct();
            }

            pointer data() noexcept
            {
                if (is_small()) {
                    return _get_stack().get_data();
                }
                return _get_heap().ptr;
            }
            const_pointer data() const noexcept
            {
                if (is_small()) {
                    return _get_stack().get_data();
                }
                return _get_heap().ptr;
            }
            size_type size() const noexcept
            {
                if (is_small()) {
                    return _get_stack().size;
                }
                return _get_heap().size;
            }
            size_type capacity() const noexcept
            {
                if (is_small()) {
                    return StackN;
                }
                return _get_heap().cap;
            }

            bool empty() const noexcept
            {
                return size() == 0;
            }

            SCN_CONSTEXPR bool is_small() const noexcept
            {
                return !m_heap;
            }
            SCN_CONSTEXPR static bool can_be_small(size_type n) noexcept
            {
                return n <= StackN;
            }

            reference operator[](size_type pos)
            {
                return *(begin() + pos);
            }
            const_reference operator[](size_type pos) const
            {
                return *(begin() + pos);
            }

            reference front()
            {
                return *begin();
            }
            const_reference front() const
            {
                return *begin();
            }

            reference back()
            {
                return *(end() - 1);
            }
            const_reference back() const
            {
                return *(end() - 1);
            }

            iterator begin() noexcept
            {
                if (is_small()) {
                    return _get_stack().get_data();
                }
                return _get_heap().ptr;
            }
            const_iterator begin() const noexcept
            {
                if (is_small()) {
                    return _get_stack().get_data();
                }
                return _get_heap().ptr;
            }
            const_iterator cbegin() const noexcept
            {
                return begin();
            }

            iterator end() noexcept
            {
                if (is_small()) {
                    return _get_stack().get_data() + size();
                }
                return _get_heap().ptr + size();
            }
            const_iterator end() const noexcept
            {
                if (is_small()) {
                    return _get_stack().get_data() + size();
                }
                return _get_heap().ptr + size();
            }
            const_iterator cend() const noexcept
            {
                return end();
            }

            reverse_iterator rbegin() noexcept
            {
                return make_reverse_iterator(end());
            }
            const_reverse_iterator rbegin() const noexcept
            {
                return make_reverse_iterator(end());
            }
            const_reverse_iterator crbegin() const noexcept
            {
                return rbegin();
            }

            reverse_iterator rend() noexcept
            {
                return make_reverse_iterator(begin());
            }
            const_reverse_iterator rend() const noexcept
            {
                return make_reverse_iterator(begin());
            }
            const_reverse_iterator crend() const noexcept
            {
                return rend();
            }

            size_type max_size() const noexcept
            {
                return std::numeric_limits<size_type>::max();
            }

            void make_small() noexcept
            {
                if (is_small() || !can_be_small(size())) {
                    return;
                }

                stack_storage s;
                this->uninitialized_move(begin(), end(), s.get_data());
                s.size = size();

                _destruct();
                _construct_stack_storage();
                this->uninitialized_move(s.get_data(), s.get_data() + s.size,
                                         _get_stack().get_data());
                _set_size(s.size);

                for (size_t i = 0; i != s.size; ++i) {
                    _get_stack().get_data()[i].~T();
                }
            }

            void reserve(size_type new_cap)
            {
                if (new_cap <= capacity()) {
                    return;
                }
                _realloc(next_pow2(new_cap));
            }

            void shrink_to_fit()
            {
                if (is_small()) {
                    return;
                }
                if (!can_be_small(size())) {
                    _realloc(size());
                }
                else {
                    make_small();
                }
            }

            void clear() noexcept
            {
                _destruct_elements();
            }

            iterator erase(const_iterator pos)
            {
                if (pos == end()) {
                    pos->~T();
                    --_get_stack().size;
                    return end();
                }
                else {
                    for (auto it = pos; it != end(); ++it) {
                        it->~T();
                        ::new (static_cast<void*>(it)) T(std::move(*(it + 1)));
                    }
                    (end() - 1)->~T();
                    return pos;
                }
            }

            void push_back(const T& value)
            {
                ::new (_prepare_push_back()) T(value);
                _set_size(size() + 1);
            }
            void push_back(T&& value)
            {
                ::new (_prepare_push_back()) T(std::move(value));
                _set_size(size() + 1);
            }

            template <typename... Args>
            reference emplace_back(Args&&... args)
            {
                ::new (_prepare_push_back()) T(std::forward<Args>(args)...);
                _set_size(size() + 1);
                return back();
            }

            void pop_back()
            {
                back().~T();
                _set_size(size() - 1);
            }

            void resize(size_type count)
            {
                if (count > size()) {
                    return;
                }
                for (auto it = begin() + count; it != end(); ++it) {
                    it->~T();
                }
                _set_size(count);
            }

            void swap(small_vector& other) noexcept
            {
                small_vector tmp{std::move(other)};
                other = std::move(*this);
                *this = std::move(tmp);
            }

        private:
            void _construct_stack_storage() noexcept
            {
                ::new (static_cast<void*>(std::addressof(m_storage)))
                    stack_storage();
                m_heap = false;
            }
            void _construct_heap_storage() noexcept
            {
                ::new (static_cast<void*>(std::addressof(m_storage)))
                    heap_storage();
                m_heap = true;
            }

            void _destruct_stack_storage() noexcept
            {
                _get_stack().~stack_storage();
            }
            void _destruct_heap_storage() noexcept
            {
                if (capacity() != 0) {
                    delete[] reinterpret_cast<stack_storage_type*>(
                        _get_heap().ptr);
                }
                _get_heap().~heap_storage();
            }

            void _destruct_elements() noexcept
            {
                if (m_heap) {
                    for (size_type i = 0; i != _get_heap().size; ++i) {
                        _get_heap().ptr[i].~T();
                    }
                }
                else {
                    for (size_type i = 0; i != _get_stack().size; ++i) {
                        _get_stack().get_data()[i].~T();
                    }
                }
                _set_size(0);
            }

            void _destruct() noexcept
            {
                _destruct_elements();
                if (m_heap) {
                    _destruct_heap_storage();
                }
                else {
                    _destruct_stack_storage();
                }
            }

            void _set_size(size_t n) noexcept
            {
                if (is_small()) {
                    _get_stack().size = n;
                }
                else {
                    _get_heap().size = n;
                }
            }

            void _realloc(size_type new_cap)
            {
                auto storage_ptr = new stack_storage_type[new_cap];
                auto ptr = reinterpret_cast<pointer>(storage_ptr);
                auto n = size();
                this->uninitialized_move(begin(), end(), ptr);
                _destruct();
                if (is_small()) {
                    _construct_heap_storage();
                }
                _get_heap().ptr = ptr;
                _get_heap().size = n;
                _get_heap().cap = new_cap;
            }

            void* _prepare_push_back()
            {
                if (size() == capacity()) {
                    _realloc(next_pow2(size() + 1));
                }
                if (is_small()) {
                    return _get_stack().data + size();
                }
                return _get_heap().ptr + size();
            }

            stack_storage& _get_stack() noexcept
            {
                return *reinterpret_cast<stack_storage*>(
                    std::addressof(m_storage));
            }
            const stack_storage& _get_stack() const noexcept
            {
                return *reinterpret_cast<const stack_storage*>(
                    std::addressof(m_storage));
            }

            heap_storage& _get_heap() noexcept
            {
                return *reinterpret_cast<heap_storage*>(
                    std::addressof(m_storage));
            }
            const heap_storage& _get_heap() const noexcept
            {
                return *reinterpret_cast<const heap_storage*>(
                    std::addressof(m_storage));
            }

            storage_type m_storage;
            bool m_heap{false};
        };

        template <typename T, size_t N>
        void swap(small_vector<T, N>& l,
                  small_vector<T, N>& r) noexcept(noexcept(l.swap(r)))
        {
            l.swap(r);
        }

        SCN_CLANG_POP
    }  // namespace detail
}  // namespace scn

#endif  // SCN_DETAIL_SMALL_VECTOR_H
