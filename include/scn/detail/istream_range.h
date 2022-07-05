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

#include <scn/fwd.h>

#if SCN_USE_IOSTREAMS

#include <scn/detail/caching_view.h>
#include <scn/detail/ranges.h>

#include <iosfwd>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename CharT>
        class basic_input_istreambuf_view : public ranges::view_base {
        public:
            using char_type = CharT;
            using traits_type = std::char_traits<CharT>;
            using int_type = typename traits_type::int_type;
            using difference_type = std::ptrdiff_t;
            using streambuf_type = std::basic_streambuf<char_type, traits_type>;
            using istream_type = std::basic_istream<char_type, traits_type>;

            class iterator;
            using sentinel = ranges_std::default_sentinel_t;

            basic_input_istreambuf_view(istream_type& is)
                : basic_input_istreambuf_view(is.rdbuf())
            {
            }

            basic_input_istreambuf_view(streambuf_type* s) : m_streambuf(s)
            {
                SCN_EXPECT(s != nullptr);
            }

            ~basic_input_istreambuf_view() = default;

            basic_input_istreambuf_view(const basic_input_istreambuf_view&) =
                delete;
            basic_input_istreambuf_view& operator=(
                const basic_input_istreambuf_view&) = delete;

            basic_input_istreambuf_view(basic_input_istreambuf_view&&) =
                default;
            basic_input_istreambuf_view& operator=(
                basic_input_istreambuf_view&&) = default;

            iterator begin() const SCN_NOEXCEPT;

            sentinel end() const SCN_NOEXCEPT
            {
                return ranges_std::default_sentinel;
            }

            streambuf_type* rdbuf()
            {
                return m_streambuf;
            }
            const streambuf_type* rdbuf() const
            {
                return m_streambuf;
            }

        private:
            bool read_next_char() const;

            void read_next_char_checked() const
            {
                const auto result = read_next_char();
                SCN_ENSURE(result);
            }

            streambuf_type* m_streambuf;
            mutable int_type m_last_char{traits_type::eof()};
            mutable bool m_end_reached{false};
        };

        template <typename CharT>
        class basic_input_istreambuf_view<CharT>::iterator {
        public:
            using view_type = basic_input_istreambuf_view<CharT>;
            using traits_type = std::char_traits<CharT>;

            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = CharT;
            using reference = CharT;
            using pointer = CharT*;

            friend view_type;

            iterator() = default;

            iterator& operator++()
            {
                SCN_EXPECT(m_view);
                m_view->m_last_char = traits_type::eof();
                return *this;
            }
            void operator++(int)
            {
                operator++();
            }

            value_type operator*() const
            {
                SCN_EXPECT(m_view);
                m_view->read_next_char_checked();
                return traits_type::to_char_type(m_view->m_last_char);
            }

            view_type& view()
            {
                SCN_EXPECT(m_view);
                return const_cast<view_type&>(*m_view);
            }
            const view_type& view() const
            {
                SCN_EXPECT(m_view);
                return *m_view;
            }

            friend bool operator==(const iterator& x,
                                   ranges_std::default_sentinel_t)
            {
                return x.is_at_end();
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

            bool operator==(const iterator& o) const
            {
                return m_view == o.m_view;
            }
            bool operator!=(const iterator& o) const
            {
                return !(*this == o);
            }

        private:
            iterator(const view_type& view) : m_view(&view) {}

            bool is_at_end() const;

            mutable const view_type* m_view{nullptr};
        };

        template <typename CharT>
        auto basic_input_istreambuf_view<CharT>::begin() const
            SCN_NOEXCEPT->iterator
        {
            return {*this};
        }

        extern template bool basic_input_istreambuf_view<char>::read_next_char()
            const;
        extern template bool
        basic_input_istreambuf_view<wchar_t>::read_next_char() const;

        extern template bool
        basic_input_istreambuf_view<char>::iterator::is_at_end() const;
        extern template bool
        basic_input_istreambuf_view<wchar_t>::iterator::is_at_end() const;
    }  // namespace detail

    template <typename CharT>
    class basic_istreambuf_view
        : public detail::basic_caching_view<
              detail::basic_input_istreambuf_view<CharT>> {
        using istreambuf_view = detail::basic_input_istreambuf_view<CharT>;
        using base = detail::basic_caching_view<istreambuf_view>;

    public:
        using char_type = CharT;
        using traits_type = std::char_traits<CharT>;
        using streambuf_type = std::basic_streambuf<CharT>;
        using istream_type = std::basic_istream<CharT>;
        using iterator = typename base::iterator;

        basic_istreambuf_view(istream_type& is) : base(istreambuf_view{is}) {}
        basic_istreambuf_view(streambuf_type* s) : base(istreambuf_view{s}) {}

        void sync(iterator it);
    };

    template <typename CharT>
    class basic_istreambuf_subrange
        : public ranges::subrange<
              ranges::iterator_t<basic_istreambuf_view<CharT>>,
              ranges_std::default_sentinel_t,
              ranges::subrange_kind::unsized> {
        using iterator = ranges::iterator_t<basic_istreambuf_view<CharT>>;
        using base = ranges::subrange<iterator,
                                      ranges_std::default_sentinel_t,
                                      ranges::subrange_kind::unsized>;

    public:
        using base::base;

        basic_istreambuf_subrange(const base& o) : base(o.begin(), o.end()) {}

        void sync(iterator it);
    };

    extern template void basic_istreambuf_view<char>::sync(iterator);
    extern template void basic_istreambuf_view<wchar_t>::sync(iterator);

    extern template void basic_istreambuf_subrange<char>::sync(iterator);
    extern template void basic_istreambuf_subrange<wchar_t>::sync(iterator);

    SCN_END_NAMESPACE
}  // namespace scn

#if SCN_STD_RANGES

namespace std::ranges {
    template <typename CharT>
    inline constexpr bool enable_view<scn::basic_istreambuf_view<CharT>> = true;
    template <typename CharT>
    inline constexpr bool enable_view<scn::basic_istreambuf_subrange<CharT>> =
        true;

    template <typename CharT>
    inline constexpr bool
        enable_borrowed_range<scn::basic_istreambuf_subrange<CharT>> = true;
}  // namespace std::ranges

#else

namespace nano {
    template <typename CharT>
    inline constexpr bool enable_view<scn::basic_istreambuf_view<CharT>> = true;
    template <typename CharT>
    inline constexpr bool enable_view<scn::basic_istreambuf_subrange<CharT>> =
        true;

    template <typename CharT>
    inline constexpr bool
        enable_borrowed_range<scn::basic_istreambuf_subrange<CharT>> = true;
}  // namespace nano

#endif  // SCN_STD_RANGES

#endif  // SCN_USE_IOSTREAMS
