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

#ifndef SCN_DETAIL_RANGE_H
#define SCN_DETAIL_RANGE_H

#include "ranges.h"
#include "result.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename Range, typename = void>
        struct extract_char_type;
        template <typename Range>
        struct extract_char_type<
            Range,
            typename std::enable_if<
                std::is_integral<ranges::range_value_t<Range>>::value>::type> {
            using type = ranges::range_value_t<Range>;
        };
        template <typename Range>
        struct extract_char_type<
            Range,
            void_t<typename std::enable_if<!std::is_integral<
                       ranges::range_value_t<Range>>::value>::type,
                   typename ranges::range_value_t<Range>::success_type>> {
            using type = typename ranges::range_value_t<Range>::sucess_type;
        };

        template <typename Range>
        class reconstructed_return_type {
        public:
            using range_type = Range;
            using view_type = Range;
            using iterator = ranges::iterator_t<const range_type>;
            using sentinel = ranges::sentinel_t<const range_type>;

            reconstructed_return_type(iterator i, sentinel s)
                : r(std::move(i), std::move(s))
            {
            }

            Range& get() &
            {
                return r;
            }
            const Range& get() const&
            {
                return r;
            }
            Range&& get() &&
            {
                return std::move(r);
            }

            auto begin() -> decltype(ranges::begin(get()))
            {
                return ranges::begin(r);
            }
            auto begin() const -> decltype(ranges::begin(get()))
            {
                return ranges::begin(r);
            }
            auto cbegin() const -> decltype(ranges::cbegin(get()))
            {
                return ranges::cbegin(r);
            }

            auto end() -> decltype(ranges::end(get()))
            {
                return ranges::end(r);
            }
            auto end() const -> decltype(ranges::end(get()))
            {
                return ranges::end(r);
            }
            auto cend() const -> decltype(ranges::cend(get()))
            {
                return ranges::cend(r);
            }

        private:
            Range r;
        };
        template <typename Range>
        class subrange_return_type {
        public:
            using range_type = Range;
            using iterator = ranges::iterator_t<const range_type>;
            using sentinel = ranges::sentinel_t<const range_type>;
            using view_type = ranges::subrange<iterator, sentinel>;

            subrange_return_type(iterator i, sentinel s)
                : r(std::move(i), std::move(s))
            {
            }

            view_type& get() &
            {
                return r;
            }
            const view_type& get() const&
            {
                return r;
            }
            view_type&& get() &&
            {
                return std::move(r);
            }

            auto begin() -> decltype(ranges::begin(get()))
            {
                return ranges::begin(r);
            }
            auto begin() const -> decltype(ranges::begin(get()))
            {
                return ranges::begin(r);
            }
            auto cbegin() const -> decltype(ranges::cbegin(get()))
            {
                return ranges::cbegin(r);
            }

            auto end() -> decltype(ranges::end(get()))
            {
                return ranges::end(r);
            }
            auto end() const -> decltype(ranges::end(get()))
            {
                return ranges::end(r);
            }
            auto cend() const -> decltype(ranges::cend(get()))
            {
                return ranges::cend(r);
            }

        private:
            view_type r;
        };

        template <typename Range>
        struct lvalue_range_wrapper_return_type {
            using type = subrange_return_type<Range>;

            template <typename I, typename S>
            static type make(I&& i, S&& s)
            {
                return {std::forward<I>(i), std::forward<S>(s)};
            }
        };
        template <typename CharT, typename Traits, typename Allocator>
        struct lvalue_range_wrapper_return_type<
            std::basic_string<CharT, Traits, Allocator>> {
            using type = reconstructed_return_type<basic_string_view<CharT>>;

            template <typename I, typename S>
            static type make(I i, S s)
            {
                return {std::addressof(*i), std::addressof(*s)};
            }
        };

        // lvalue range owning its contents
        // const std::string&
        template <typename Range>
        class lvalue_range_wrapper {
        public:
            using range_type = Range;
            using storage_type = const Range&;
#if 0
            using return_type = typename std::conditional<
                ranges::pair_reconstructible_range<Range>::value,
                reconstructed_return_type<Range>,
                view_return_type<Range>>::type;
#endif
            using return_type =
                typename lvalue_range_wrapper_return_type<Range>::type;
            using iterator = ranges::iterator_t<const Range>;
            using sentinel = ranges::sentinel_t<const Range>;
            using char_type = typename extract_char_type<Range>::type;

            lvalue_range_wrapper(const Range& r)
                : m_range(r),
                  m_begin(ranges::begin(m_range)),
                  m_rollback(m_begin)
            {
            }

            return_type get_return() const
            {
                return lvalue_range_wrapper_return_type<Range>::make(
                    m_begin, ranges::end(m_range));
            }

            const Range& range() const noexcept
            {
                return m_range;
            }

            SCN_NODISCARD iterator& begin() noexcept
            {
                return m_begin;
            }
            const iterator& begin() const noexcept
            {
                return m_begin;
            }
            sentinel end() const
                noexcept(noexcept(ranges::end(std::declval<const Range&>())))
            {
                return ranges::end(m_range);
            }

            template <typename R = Range,
                      typename std::enable_if<
                          ranges::contiguous_range<R>::value>::type* = nullptr>
            auto data() const noexcept(noexcept(*std::declval<iterator>()))
                -> decltype(std::addressof(*std::declval<iterator>()))
            {
                return std::addressof(*m_begin);
            }
            template <typename R = Range,
                      typename std::enable_if<
                          ranges::sized_range<R>::value>::type* = nullptr>
            auto size() const
                noexcept(noexcept(ranges::distance(std::declval<iterator>(),
                                                   std::declval<sentinel>())))
                    -> decltype(ranges::distance(std::declval<iterator>(),
                                                 std::declval<sentinel>()))
            {
                return ranges::distance(m_begin, end());
            }

            error reset_to_rollback_point()
            {
                while (m_begin != m_rollback) {
                    --m_begin;
                    if (m_begin == end()) {
                        return error(error::unrecoverable_stream_error,
                                     "Putback failed");
                    }
                }
                return {};
            }
            void set_rollback_point()
            {
                m_rollback = m_begin;
            }

            // iterator value type is a character
            static SCN_CONSTEXPR bool is_direct()
            {
                return std::is_integral<ranges::iter_value_t<iterator>>::value;
            }
            // can call .data() and memcpy
            static SCN_CONSTEXPR bool is_contiguous()
            {
                return ranges::contiguous_range<Range>::value;
            }
            // provides mechanism to get a pointer to memcpy from
            static SCN_CONSTEXPR bool provides_buffer_access()
            {
                return false;
            }

        private:
            storage_type m_range;
            iterator m_begin;
            iterator m_rollback;
        };
        // rvalue range owning its contents
        // std::string&&
        template <typename Range>
        class rvalue_range_wrapper {
        public:
            using range_type = Range;
            using storage_type = Range;
            using return_type = reconstructed_return_type<Range>;
            using iterator = ranges::iterator_t<const Range>;
            using sentinel = ranges::sentinel_t<const Range>;
            using char_type = typename extract_char_type<Range>::type;

            rvalue_range_wrapper(Range&& r)
                : m_range(std::move(r)),
                  m_begin(ranges::begin(m_range)),
                  m_rollback(m_begin)
            {
            }

            rvalue_range_wrapper(const rvalue_range_wrapper& other) = delete;

            rvalue_range_wrapper(rvalue_range_wrapper&& other)
            {
                const auto begin_offset = ranges::distance(
                    ranges::begin(other.m_range), other.m_begin);
                const auto rollback_offset = ranges::distance(
                    ranges::begin(other.m_range), other.m_rollback);
                m_range = std::move(other.m_range);
                m_begin = _iterator_at_offset(m_range, begin_offset);
                m_rollback = _iterator_at_offset(m_range, rollback_offset);
            }

            ~rvalue_range_wrapper() = default;

            return_type get_return() const
            {
                return {m_begin, ranges::end(m_range)};
            }

            const Range& range() const noexcept
            {
                return m_range;
            }

            SCN_NODISCARD iterator& begin() noexcept
            {
                return m_begin;
            }
            const iterator& begin() const noexcept
            {
                return m_begin;
            }
            sentinel end() const
                noexcept(noexcept(ranges::end(std::declval<const Range&>())))
            {
                return ranges::end(m_range);
            }

            template <typename R = Range,
                      typename std::enable_if<
                          ranges::contiguous_range<R>::value>::type* = nullptr>
            auto data() const noexcept(noexcept(*std::declval<iterator>()))
                -> decltype(std::addressof(*std::declval<iterator>()))
            {
                return std::addressof(*m_begin);
            }
            template <typename R = Range,
                      typename std::enable_if<
                          ranges::sized_range<R>::value>::type* = nullptr>
            auto size() const
                noexcept(noexcept(ranges::distance(std::declval<iterator>(),
                                                   std::declval<sentinel>())))
                    -> decltype(ranges::distance(std::declval<iterator>(),
                                                 std::declval<sentinel>()))
            {
                return ranges::distance(m_begin, end());
            }

            error reset_to_rollback_point()
            {
                while (m_begin != m_rollback) {
                    --m_begin;
                    if (m_begin == end()) {
                        return error(error::unrecoverable_stream_error,
                                     "Putback failed");
                    }
                }
                return {};
            }
            void set_rollback_point()
            {
                m_rollback = m_begin;
            }

            static SCN_CONSTEXPR bool is_direct()
            {
                return std::is_integral<ranges::iter_value_t<iterator>>::value;
            }
            static SCN_CONSTEXPR bool is_contiguous()
            {
                return ranges::contiguous_range<Range>::value;
            }
            static SCN_CONSTEXPR bool provides_buffer_access()
            {
                return false;
            }

        private:
            template <typename R>
            static iterator _iterator_at_offset(R& r,
                                                ranges::range_difference_t<R> n)
            {
                auto b = ranges::begin(r);
                ranges::advance(b, n);
                return b;
            }

            Range m_range;
            iterator m_begin;
            iterator m_rollback;
        };
        // view range
        // string_view
        template <typename Range>
        class view_range_wrapper {
        public:
            using range_type = Range;
            using storage_type = Range;
            using return_type = reconstructed_return_type<Range>;
            using iterator = ranges::iterator_t<const Range>;
            using sentinel = ranges::sentinel_t<const Range>;
            using char_type = typename extract_char_type<Range>::type;

            view_range_wrapper(Range r)
                : m_range(std::move(r)),
                  m_begin(ranges::begin(m_range)),
                  m_rollback(m_begin)
            {
            }

            return_type get_return() const
            {
                return {m_begin, ranges::end(m_range)};
            }

            Range range() const noexcept
            {
                return m_range;
            }

            SCN_NODISCARD iterator& begin() noexcept
            {
                return m_begin;
            }
            const iterator& begin() const noexcept
            {
                return m_begin;
            }
            sentinel end() const
                noexcept(noexcept(ranges::end(std::declval<const Range&>())))
            {
                return ranges::end(m_range);
            }

            template <typename R = Range,
                      typename std::enable_if<
                          ranges::contiguous_range<R>::value>::type* = nullptr>
            auto data() const noexcept(noexcept(*std::declval<iterator>()))
                -> decltype(std::addressof(*std::declval<iterator>()))
            {
                return std::addressof(*m_begin);
            }
            template <typename R = Range,
                      typename std::enable_if<
                          ranges::sized_range<R>::value>::type* = nullptr>
            auto size() const
                noexcept(noexcept(ranges::distance(std::declval<iterator>(),
                                                   std::declval<sentinel>())))
                    -> decltype(ranges::distance(std::declval<iterator>(),
                                                 std::declval<sentinel>()))
            {
                return ranges::distance(m_begin, end());
            }

            error reset_to_rollback_point()
            {
                while (m_begin != m_rollback) {
                    --m_begin;
                    if (m_begin == end()) {
                        return error(error::unrecoverable_stream_error,
                                     "Putback failed");
                    }
                }
                return {};
            }
            void set_rollback_point()
            {
                m_rollback = m_begin;
            }

            static SCN_CONSTEXPR bool is_direct()
            {
                return std::is_integral<ranges::iter_value_t<iterator>>::value;
            }
            static SCN_CONSTEXPR bool is_contiguous()
            {
                return ranges::contiguous_range<Range>::value;
            }
            static SCN_CONSTEXPR bool provides_buffer_access()
            {
                return false;
            }

        private:
            Range m_range;
            iterator m_begin;
            iterator m_rollback;
        };

        namespace _make_range_wrapper {
            struct fn {
            private:
                template <typename Range>
                static view_range_wrapper<Range> impl(
                    view_range_wrapper<Range> r,
                    priority_tag<3>)
                {
                    return r;
                }
                template <typename Range>
                static lvalue_range_wrapper<Range> impl(
                    lvalue_range_wrapper<Range> r,
                    priority_tag<3>)
                {
                    return r;
                }
                template <typename Range>
                static rvalue_range_wrapper<Range> impl(
                    rvalue_range_wrapper<Range> r,
                    priority_tag<3>)
                {
                    return r;
                }

                // string literal -> string_view
                template <typename CharT,
                          std::size_t N,
                          typename std::enable_if<
                              std::is_integral<CharT>::value>::type* = nullptr>
                static view_range_wrapper<basic_string_view<CharT>> impl(
                    const CharT (&str)[N],
                    priority_tag<2>)
                {
                    return view_range_wrapper<basic_string_view<CharT>>(str);
                }
                template <typename CharT,
                          std::size_t N,
                          typename std::enable_if<
                              std::is_integral<CharT>::value>::type* = nullptr>
                static view_range_wrapper<basic_string_view<CharT>> impl(
                    CharT(&&str)[N],
                    priority_tag<2>) = delete;

                // std::string -> string_view
                template <typename CharT, typename Traits, typename Allocator>
                static view_range_wrapper<basic_string_view<CharT>> impl(
                    const std::basic_string<CharT, Traits, Allocator>& str,
                    priority_tag<2>)
                {
                    return view_range_wrapper<basic_string_view<CharT>>(
                        {str.data(), str.size()});
                }
                template <typename CharT, typename Traits, typename Allocator>
                static rvalue_range_wrapper<
                    std::basic_string<CharT, Traits, Allocator>>
                impl(std::basic_string<CharT, Traits, Allocator>&& str,
                     priority_tag<2>)
                {
                    return {std::move(str)};
                }

                // span -> string_view
                template <typename CharT>
                static view_range_wrapper<
                    basic_string_view<typename std::remove_const<CharT>::type>>
                impl(span<CharT> s, priority_tag<2>)
                {
                    return {s.data(), s.size()};
                }

                template <typename Range,
                          typename std::enable_if<ranges::view<
                              remove_cvref_t<Range>>::value>::type* = nullptr>
                static view_range_wrapper<remove_cvref_t<Range>> impl(
                    Range&& r,
                    priority_tag<1>)
                {
                    return view_range_wrapper<remove_cvref_t<Range>>(
                        std::forward<Range>(r));
                }

                template <typename Range,
                          typename std::enable_if<!ranges::view<
                              remove_cvref_t<Range>>::value>::type* = nullptr>
                static lvalue_range_wrapper<Range> impl(const Range& r,
                                                        priority_tag<0>)
                {
                    return lvalue_range_wrapper<Range>(r);
                }
                template <
                    typename Range,
                    typename std::enable_if<
                        !std::is_reference<Range>::value &&
                        !ranges::view<remove_cvref_t<Range>>::value>::type* =
                        nullptr>
                static rvalue_range_wrapper<Range> impl(Range&& r,
                                                        priority_tag<0>)
                {
                    return rvalue_range_wrapper<Range>(std::move(r));
                }

            public:
                template <typename Range>
                auto operator()(Range&& r) const
                    -> decltype(fn::impl(std::forward<Range>(r),
                                         priority_tag<3>{}))
                {
                    return fn::impl(std::forward<Range>(r), priority_tag<3>{});
                }
            };
        }  // namespace _make_range_wrapper
        namespace {
            static SCN_CONSTEXPR auto& make_range_wrapper =
                static_const<_make_range_wrapper::fn>::value;
        }

        template <typename Range>
        using range_wrapper_for_t =
            decltype(make_range_wrapper(std::declval<Range>()));

        namespace _wrap {
            struct fn {
            private:
                template <
                    typename Range,
                    typename std::enable_if<
                        ranges::view<Range>::value &&
                        !std::is_reference<Range>::value>::type* = nullptr>
                static auto impl(Range&& r, priority_tag<1>)
                    -> decltype(make_range_wrapper(std::forward<Range>(r)))
                {
                    return make_range_wrapper(std::forward<Range>(r));
                }
                template <typename Range>
                static auto impl(const Range& r, priority_tag<1>)
                    -> decltype(make_range_wrapper(r))
                {
                    return make_range_wrapper(r);
                }
                template <
                    typename Range,
                    typename std::enable_if<
                        !ranges::view<Range>::value &&
                        !std::is_reference<Range>::value>::type* = nullptr>
                static auto impl(Range&& r, priority_tag<0>) -> decltype(
                    make_range_wrapper(std::forward<Range>(r))) = delete;

            public:
                template <typename Range>
                auto operator()(Range&& r) const
                    noexcept(noexcept(fn::impl(std::forward<Range>(r),
                                               priority_tag<1>{})))
                        -> decltype(fn::impl(std::forward<Range>(r),
                                             priority_tag<1>{}))
                {
                    return fn::impl(std::forward<Range>(r), priority_tag<1>{});
                }
            };
        }  // namespace _wrap
    }      // namespace detail
    namespace {
        static SCN_CONSTEXPR auto& wrap =
            detail::static_const<detail::_wrap::fn>::value;
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_RANGE_H
