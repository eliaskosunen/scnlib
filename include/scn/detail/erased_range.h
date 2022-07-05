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
#include <scn/util/unique_ptr.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

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

            std::ptrdiff_t get_current_index() const
            {
                return do_get_current_index();
            }

            void increment_current(std::ptrdiff_t n = 1)
            {
                SCN_EXPECT(!is_current_at_end());
                return do_increment_current(n);
            }
            void reset_current_to_begin()
            {
                return do_reset_current_to_begin();
            }

            bool is_current_at_end() const
            {
                return is_index_at_end(get_current_index());
            }
            bool is_index_at_end(std::ptrdiff_t i) const
            {
                return do_is_index_at_end(i);
            }

        protected:
            virtual std::ptrdiff_t do_get_current_index() const = 0;

            virtual void do_increment_current(std::ptrdiff_t n) = 0;
            virtual void do_reset_current_to_begin() = 0;

            SCN_NODISCARD virtual bool do_is_index_at_end(
                std::ptrdiff_t) const = 0;
        };

        template <typename CharT>
        class basic_erased_range_impl_base : public erased_range_impl_base {
        public:
            CharT get_current()
            {
                SCN_EXPECT(!is_current_at_end());
                return do_get_current();
            }

        protected:
            virtual CharT do_get_current() = 0;
        };

        template <typename Range, typename CharT>
        class basic_erased_range_impl
            : public basic_erased_range_impl_base<CharT> {
            using base = basic_erased_range_impl_base<CharT>;

        public:
            using char_type = CharT;
            using range_type = Range;
            using range_nocvref_type = remove_cvref_t<range_type>;
            using iterator = ranges::iterator_t<range_type>;
            using sentinel = ranges::sentinel_t<range_type>;

            basic_erased_range_impl(Range r)
                : m_range(SCN_FWD(r)), m_current(ranges::begin(m_range))
            {
            }

        private:
            std::ptrdiff_t do_get_current_index() const override
            {
                return m_current_index;
            }

            CharT do_get_current() override
            {
                return *m_current;
            }
            void do_increment_current(std::ptrdiff_t n) override
            {
                SCN_EXPECT(n >= 0);
                std::advance(m_current, n);
                m_current_index += n;
            }
            void do_reset_current_to_begin() override
            {
                m_current = ranges::begin(m_range);
                m_current_index = 0;
            }

            SCN_NODISCARD bool do_is_index_at_end(
                std::ptrdiff_t i) const override
            {
                SCN_EXPECT(i >= 0);
                if (i < m_current_index) {
                    return false;
                }
                return m_current == ranges::end(m_range);
            }

            ranges_polyfill::views::all_t<Range> m_range;
            iterator m_current;
            std::ptrdiff_t m_current_index{0};
        };

        template <typename CharT, typename Range>
        auto make_unique_erased_range_impl(Range&& r)
        {
            return ::scn::detail::make_unique<
                basic_erased_range_impl<Range, CharT>>(SCN_FWD(r));
        }
    }  // namespace detail

    template <typename CharT>
    class basic_erased_range {
    public:
        using char_type = CharT;
        using value_type = CharT;

        class iterator;
        using sentinel = ranges_std::default_sentinel_t;

        basic_erased_range() = default;

        template <typename R>
        explicit basic_erased_range(R&& r)
            : m_impl(detail::make_unique_erased_range_impl<CharT>(SCN_FWD(r)))
        {
        }

        iterator begin() const SCN_NOEXCEPT;

        SCN_NODISCARD sentinel end() const SCN_NOEXCEPT
        {
            return ranges_std::default_sentinel;
        }

    private:
        using impl_ptr_type =
            detail::unique_ptr<detail::basic_erased_range_impl_base<char_type>>;

        // TODO: Small object optimization?
        impl_ptr_type m_impl{nullptr};
    };

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
            advance_until_index(m_current);

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

            return m_impl->get_current();
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

    private:
        iterator(detail::basic_erased_range_impl_base<char_type>* impl)
            SCN_NOEXCEPT : m_impl(impl)
        {
            if (m_impl) {
                impl->reset_current_to_begin();
            }
        }

        iterator(detail::basic_erased_range_impl_base<char_type>* impl,
                 std::ptrdiff_t current) SCN_NOEXCEPT : m_impl(impl),
                                                        m_current(current)
        {
            if (m_impl) {
                advance_until_index(m_current);
            }
        }

        void advance_until_index(std::ptrdiff_t i) const
        {
            SCN_EXPECT(i == 0 || i >= m_impl->get_current_index());

            if (i == 0) {
                return m_impl->reset_current_to_begin();
            }

            const auto current = m_impl->get_current_index();
            if (i > current) {
                m_impl->increment_current(i - current);
            }
        }

        bool _is_end() const
        {
            if (m_impl == nullptr) {
                return true;
            }
            return m_impl->is_index_at_end(m_current);
        }

        detail::basic_erased_range_impl_base<char_type>* m_impl{nullptr};
        mutable difference_type m_current{0};
    };

    template <typename CharT>
    auto basic_erased_range<CharT>::begin() const SCN_NOEXCEPT->iterator
    {
        return {m_impl.get()};
    }

    template <typename CharT>
    struct basic_erased_subrange
        : public ranges::subrange<ranges::iterator_t<basic_erased_range<CharT>>,
                                  ranges_std::default_sentinel_t,
                                  ranges::subrange_kind::unsized> {
        using base =
            ranges::subrange<ranges::iterator_t<basic_erased_range<CharT>>,
                             ranges_std::default_sentinel_t,
                             ranges::subrange_kind::unsized>;
        using base::base;

        /*
        basic_erased_subrange(const basic_erased_range<CharT>& other)
            : basic_erased_subrange(other.begin(), other.end())
        {
        }
         */
        basic_erased_subrange(const base& o) : base(o.begin(), o.end()) {}
    };

    /**
     * Erase the given range, if necessary.
     *
     * The given range must model forward_range.
     *
     * If the given range models contiguous_range and sized_range, returns the
     * given range as-is.
     *
     * Otherwise, returns the given range wrapped in a basic_erased_range, with
     * an appropriate character type.
     */
    template <typename Range, typename CharT = ranges::range_value_t<Range>>
    auto erase_range(Range&& r)
    {
        static_assert(ranges::forward_range<Range>,
                      "erased_range can only erase forward_ranges");

        if constexpr (ranges::contiguous_range<detail::remove_cvref_t<Range>> &&
                      ranges::sized_range<detail::remove_cvref_t<Range>>) {
            return r;
        }
        else {
            return basic_erased_range<CharT>{SCN_FWD(r)};
        }
    }

    template <typename CharT>
    ranges::dangling erase_range(basic_erased_range<CharT>& r) = delete;

    template <typename CharT>
    basic_erased_range<CharT>&& erase_range(basic_erased_range<CharT>&& r)
    {
        return SCN_MOVE(r);
    }

    namespace detail {
        template <typename T>
        struct is_erased_range_or_subrange : std::false_type {};
        template <typename CharT>
        struct is_erased_range_or_subrange<basic_erased_range<CharT>>
            : std::true_type {};
        template <typename CharT>
        struct is_erased_range_or_subrange<basic_erased_subrange<CharT>>
            : std::true_type {};
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
