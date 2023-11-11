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

#include <scn/detail/ranges.h>
#include <scn/util/meta.h>
#include <scn/util/span.h>

#include <array>
#include <memory>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename T>
        struct is_erased_range_or_subrange : std::false_type {};
        template <typename T>
        struct is_erased_range_iterator : std::false_type {};
    }  // namespace detail

#if !SCN_DISABLE_ERASED_RANGE
    namespace detail {
        class erased_range_impl_base {
        public:
            erased_range_impl_base() = default;
            virtual ~erased_range_impl_base();

            erased_range_impl_base(const erased_range_impl_base&) = delete;
            erased_range_impl_base(erased_range_impl_base&&) = delete;
            erased_range_impl_base& operator=(const erased_range_impl_base&) =
                delete;
            erased_range_impl_base& operator=(erased_range_impl_base&&) =
                delete;

            virtual void reset_current_to_begin() = 0;

            SCN_NODISCARD std::ptrdiff_t get_current_index() const
            {
                return m_current_index;
            }

            SCN_NODISCARD std::ptrdiff_t get_end_index() const
            {
                return m_end_index;
            }

        protected:
            mutable std::ptrdiff_t m_current_index{-1}, m_end_index{-1};
        };

        template <typename CharT>
        class basic_erased_range_impl_base : public erased_range_impl_base {
        public:
            SCN_NODISCARD virtual std::optional<CharT> prime_first_element()
                const = 0;
            SCN_NODISCARD virtual std::optional<CharT>
            increment_single_and_check_end() = 0;
            SCN_NODISCARD virtual std::optional<CharT>
            increment_multiple_and_check_end(std::ptrdiff_t n) = 0;

            SCN_NODISCARD std::optional<CharT> get_cached_current() const
            {
                return m_cached_current;
            }

            void increment_until_index(std::ptrdiff_t i)
            {
                if (this->m_current_index < 0) {
                    prime_first_element();
                }
                if (i > this->m_current_index) {
                    (void)increment_multiple_and_check_end(
                        i - this->m_current_index);
                }
            }

            CharT deref_at_index(std::ptrdiff_t i)
            {
                if (i == this->m_current_index) {
                    SCN_EXPECT(m_cached_current);
                    return *m_cached_current;
                }

                if (i < this->m_current_index) {
                    this->reset_current_to_begin();
                }
                increment_until_index(i);
                SCN_EXPECT(m_cached_current);
                return *m_cached_current;
            }

        protected:
            mutable std::optional<CharT> m_cached_current{std::nullopt};
        };

        template <typename Range>
        class basic_erased_range_impl : public basic_erased_range_impl_base<
                                            ranges::range_value_t<Range>> {
            using base =
                basic_erased_range_impl_base<ranges::range_value_t<Range>>;

            static_assert(ranges::forward_range<Range>);

        public:
            using range_type = Range;
            using char_type = ranges::range_value_t<Range>;
            using iterator = ranges::iterator_t<range_type>;
            using sentinel = ranges::sentinel_t<range_type>;

            template <typename R>
            basic_erased_range_impl(R&& r)
                : m_range(SCN_FWD(r)), m_current(ranges::begin(m_range))
            {
            }

            void reset_current_to_begin() override
            {
                m_current = ranges::begin(m_range);
                this->m_current_index = -1;
                this->m_cached_current.reset();
            }

            SCN_NODISCARD std::optional<char_type> prime_first_element()
                const override
            {
                SCN_EXPECT(this->m_current_index < 0);
                SCN_EXPECT(m_current == ranges::begin(m_range));
                SCN_EXPECT(!this->m_cached_current);

                this->m_current_index = 0;
                if (m_current != ranges::end(m_range)) {
                    this->m_cached_current = *m_current;
                }
                else {
                    this->m_end_index = 0;
                }
                return this->m_cached_current;
            }

            SCN_NODISCARD std::optional<char_type>
            increment_single_and_check_end() override
            {
                SCN_EXPECT(m_current != ranges::end(m_range));
                ++m_current;
                ++this->m_current_index;
                return _deref_current();
            }

            SCN_NODISCARD std::optional<char_type>
            increment_multiple_and_check_end(std::ptrdiff_t n) override
            {
                SCN_EXPECT(n >= 0);
                SCN_EXPECT(m_current != ranges::end(m_range));
                ranges::advance(m_current, n);
                this->m_current_index += n;
                return _deref_current();
            }

        private:
            SCN_NODISCARD std::optional<char_type> _deref_current() const
            {
                if (m_current == ranges::end(m_range)) {
                    this->m_cached_current = std::nullopt;
                    this->m_end_index = this->m_current_index;
                    return std::nullopt;
                }

                this->m_cached_current = *m_current;
                return this->m_cached_current;
            }

            Range m_range;
            iterator m_current;
        };
    }  // namespace detail

    /**
     * Type-erased `forward_range`.
     */
    template <typename CharT>
    class basic_erased_range {
    public:
        using char_type = CharT;
        using value_type = CharT;

        class iterator;
        using sentinel = ranges_std::default_sentinel_t;

        /**
         * Construct a `basic_erased_range` containing `range`.
         */
        template <typename Range>
        explicit basic_erased_range(Range&& range)
        {
            using erased_impl_type = detail::basic_erased_range_impl<
                ranges_polyfill::views::all_t<Range&&>>;
            if constexpr (sizeof(erased_impl_type) > small_buffer_size) {
                m_ptr = new erased_impl_type(SCN_FWD(range));
                m_destroy = [](impl_base* ptr) {
                    SCN_EXPECT(ptr);
                    delete ptr;
                };
            }
            else {
                m_ptr = ::new (static_cast<void*>(m_memory.data()))
                    erased_impl_type(SCN_FWD(range));
                m_destroy = [](impl_base* ptr) {
                    SCN_EXPECT(ptr);
                    static_cast<erased_impl_type*>(ptr)->~erased_impl_type();
                };
            }
        }

        basic_erased_range(const basic_erased_range&) = delete;
        basic_erased_range(basic_erased_range&&) = delete;
        basic_erased_range& operator=(const basic_erased_range&) = delete;
        basic_erased_range& operator=(basic_erased_range&&) = delete;

        ~basic_erased_range()
        {
            SCN_EXPECT(m_destroy);
            m_destroy(m_ptr);
        }

        /**
         * \return An `iterator` pointing to the beginning of `*this`.
         */
        iterator begin() const SCN_NOEXCEPT;

        /**
         * \return A `sentinel` corresponding to the end of `*this`.
         */
        SCN_NODISCARD sentinel end() const SCN_NOEXCEPT
        {
            return ranges_std::default_sentinel;
        }

    private:
        using impl_base = detail::basic_erased_range_impl_base<CharT>;
        using destroy_t = void (*)(impl_base*);

        static constexpr size_t small_buffer_size = 64;
        alignas(std::max_align_t)
            std::array<unsigned char, small_buffer_size> m_memory;
        impl_base* m_ptr;
        destroy_t m_destroy;
    };

    template <typename R>
    basic_erased_range(R) -> basic_erased_range<detail::char_t<R>>;

    template <typename CharT>
    class basic_erased_range<CharT>::iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = CharT;
        using reference = CharT;
        using pointer = CharT*;

        friend class basic_erased_range<char_type>;

        iterator() = default;

        difference_type distance_from_begin() const
        {
            return m_current;
        }

        iterator& operator++()
        {
            SCN_EXPECT(m_impl);
            ++m_current;
            m_impl->increment_until_index(m_current);
            return *this;
        }

        iterator operator++(int)
        {
            auto copy = *this;
            operator++();
            return copy;
        }

        CharT operator*() const
        {
            SCN_EXPECT(m_impl);
            return m_impl->deref_at_index(m_current);
        }

        friend bool operator==(const iterator& x,
                               ranges_std::default_sentinel_t)
        {
            return x._is_end();
        }

        friend bool operator==(ranges_std::default_sentinel_t s,
                               const iterator& x)
        {
            return x == s;
        }

        friend bool operator!=(const iterator& x,
                               ranges_std::default_sentinel_t s)
        {
            return !(x == s);
        }

        friend bool operator!=(ranges_std::default_sentinel_t s,
                               const iterator& x)
        {
            return !(x == s);
        }

        friend bool operator==(const iterator& a, const iterator& b)
        {
            if (a._is_end() == b._is_end() && a._is_end()) {
                return true;
            }
            return a.m_current == b.m_current;
        }

        friend bool operator!=(const iterator& a, const iterator& b)
        {
            return !(a == b);
        }

        friend bool operator<(const iterator& a, const iterator& b)
        {
            if (a._is_end() && b._is_end()) {
                return false;
            }
            if (a._is_end() && !b._is_end()) {
                return false;
            }
            if (!a._is_end() && b._is_end()) {
                return true;
            }
            SCN_EXPECT(a.m_impl == b.m_impl);
            return a.m_current < b.m_current;
        }
        friend bool operator>(const iterator& a, const iterator& b)
        {
            return b < a;
        }
        friend bool operator<=(const iterator& a, const iterator& b)
        {
            return !(a > b);
        }
        friend bool operator>=(const iterator& a, const iterator& b)
        {
            return !(a < b);
        }

    private:
        iterator(detail::basic_erased_range_impl_base<char_type>* impl)
            : m_impl(impl)
        {
            SCN_EXPECT(impl);
        }

        bool _is_end() const
        {
            if (m_impl == nullptr) {
                return true;
            }
            if (m_impl->get_current_index() < 0) {
                (void)m_impl->prime_first_element();
            }
            return m_current == m_impl->get_end_index();
        }

        detail::basic_erased_range_impl_base<char_type>* m_impl{nullptr};
        mutable difference_type m_current{0};
    };

    template <typename CharT>
    auto basic_erased_range<CharT>::begin() const SCN_NOEXCEPT->iterator
    {
        SCN_EXPECT(m_ptr);
        return {m_ptr};
    }

    /**
     * A subrange into a `basic_erased_range`.
     *
     * Not a type alias to avoid long template names.
     */
    template <typename CharT>
    struct basic_erased_subrange
        : public ranges::subrange<ranges::iterator_t<basic_erased_range<CharT>>,
                                  ranges_std::default_sentinel_t,
                                  ranges::subrange_kind::unsized> {
        using iterator = ranges::iterator_t<basic_erased_range<CharT>>;
        using sentinel = ranges_std::default_sentinel_t;
        using base = ranges::
            subrange<iterator, sentinel, ranges::subrange_kind::unsized>;
        using base::base;

        basic_erased_subrange(const basic_erased_range<CharT>& other)
            : basic_erased_subrange(other.begin(), other.end())
        {
        }

        basic_erased_subrange(const base& o) : base(o.begin(), o.end()) {}
    };

    /**
     * Erase the given range, if necessary.
     *
     * The given range must model `forward_range`. `caching_view` can be used to
     * turn `input_range`s into `forward_range`s, if necessary.
     *
     * If the given range models `contiguous_range` and `sized_range`, returns
     * the given range as-is.
     *
     * Otherwise, returns the given range wrapped in a `basic_erased_range`,
     * with an appropriate character type.
     */
    template <typename Range, typename CharT = detail::char_t<Range>>
    auto erase_range(Range&& r)
    {
        static_assert(ranges::forward_range<Range>,
                      "erase_range can only erase forward_ranges");

        if constexpr (ranges::contiguous_range<detail::remove_cvref_t<Range>> &&
                      ranges::sized_range<detail::remove_cvref_t<Range>>) {
            return r;
        }
        else {
            return basic_erased_range<CharT>{SCN_FWD(r)};
        }
    }

    template <typename CharT>
    ranges::dangling erase_range(basic_erased_range<CharT>& r)
    {
        SCN_UNUSED(r);
        return {};
    }

    template <typename CharT>
    basic_erased_range<CharT>&& erase_range(basic_erased_range<CharT>&& r)
    {
        return SCN_MOVE(r);
    }

    namespace detail {
        template <typename CharT>
        struct is_erased_range_or_subrange<basic_erased_range<CharT>>
            : std::true_type {};
        template <typename CharT>
        struct is_erased_range_or_subrange<basic_erased_subrange<CharT>>
            : std::true_type {};

        template <>
        struct is_erased_range_iterator<basic_erased_range<char>::iterator>
            : std::true_type {};
        template <>
        struct is_erased_range_iterator<basic_erased_range<wchar_t>::iterator>
            : std::true_type {};

    }  // namespace detail

#endif  // !SCN_DISABLE_ERASED_RANGE

    SCN_END_NAMESPACE
}  // namespace scn

#if !SCN_DISABLE_ERASED_RANGE

#if SCN_STD_RANGES

namespace std::ranges {
    template <typename CharT>
    inline constexpr bool enable_view<scn::basic_erased_range<CharT>> = true;
    template <typename CharT>
    inline constexpr bool enable_view<scn::basic_erased_subrange<CharT>> = true;

    template <typename CharT>
    inline constexpr bool
        enable_borrowed_range<scn::basic_erased_subrange<CharT>> = true;
}  // namespace std::ranges

#else

NANO_BEGIN_NAMESPACE

template <typename CharT>
inline constexpr bool enable_view<scn::basic_erased_range<CharT>> = true;
template <typename CharT>
inline constexpr bool enable_view<scn::basic_erased_subrange<CharT>> = true;

template <typename CharT>
inline constexpr bool enable_borrowed_range<scn::basic_erased_subrange<CharT>> =
    true;

NANO_END_NAMESPACE

#endif  // SCN_STD_RANGES

#endif  // !SCN_DISABLE_ERASED_RANGE
