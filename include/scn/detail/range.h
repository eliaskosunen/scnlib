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
        template <typename Iterator, typename = void>
        struct extract_char_type;
        template <typename Iterator>
        struct extract_char_type<
            Iterator,
            typename std::enable_if<std::is_integral<
                ranges::iter_value_t<Iterator>>::value>::type> {
            using type = ranges::iter_value_t<Iterator>;
        };
        template <typename Iterator>
        struct extract_char_type<
            Iterator,
            void_t<typename std::enable_if<!std::is_integral<
                       ranges::iter_value_t<Iterator>>::value>::type,
                   typename ranges::iter_value_t<Iterator>::success_type>> {
            using type = typename ranges::iter_value_t<Iterator>::success_type;
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

            view_type& get()
            {
                return r;
            }
            const view_type& get() const
            {
                return r;
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
#if 0
        template <typename Range>
        class range_reference_return_type {
        public:
            using range_type = Range;
            using iterator = ranges::iterator_t<const Range>;
            using sentinel = ranges::sentinel_t<const Range>;

            template <typename R>
            range_reference_return_type(R&& r) : m_range(std::forward<R>(r))
            {
            }

            Range& get()
            {
                return m_range;
            }
            const Range& get() const
            {
                return m_range;
            }

            auto begin() -> decltype(ranges::begin(get()))
            {
                return ranges::begin(m_range);
            }
            auto begin() const -> decltype(ranges::begin(get()))
            {
                return ranges::begin(m_range);
            }
            auto cbegin() const -> decltype(ranges::cbegin(get()))
            {
                return ranges::cbegin(m_range);
            }

            auto end() -> decltype(ranges::end(get()))
            {
                return ranges::end(m_range);
            }
            auto end() const -> decltype(ranges::end(get()))
            {
                return ranges::end(m_range);
            }
            auto cend() const -> decltype(ranges::cend(get()))
            {
                return ranges::cend(m_range);
            }

        private:
            range_type m_range;
        };
#endif

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

        template <typename Range, typename = void>
        struct is_direct_impl
            : std::is_integral<ranges::range_value_t<const Range>> {
        };
        template <typename Range, typename = void>
        struct is_contiguous_impl : ranges::contiguous_range<Range> {
        };
        template <typename Range, typename = void>
        struct provides_buffer_access_impl : std::false_type {
        };

        namespace _reset_to_rollback_point {
            struct fn {
            private:
                template <typename Range, typename Iterator>
                static auto impl(
                    const Range& r,
                    Iterator& begin,
                    Iterator& rollback,
                    priority_tag<
                        2>) noexcept(noexcept(r
                                                  .reset_to_rollback_point(
                                                      begin,
                                                      rollback)))
                    -> decltype(r.reset_to_rollback_point(begin, rollback))
                {
                    return r.reset_to_rollback_point(begin, rollback);
                }
                template <typename Range, typename Iterator>
                static auto impl(
                    const Range& r,
                    Iterator& begin,
                    Iterator& rollback,
                    priority_tag<
                        1>) noexcept(noexcept(reset_to_rollback_point(r,
                                                                      begin,
                                                                      rollback)))
                    -> decltype(reset_to_rollback_point(r, begin, rollback))
                {
                    return reset_to_rollback_point(r, begin, rollback);
                }
                template <typename Range, typename Iterator>
                static error impl(
                    const Range& r,
                    Iterator& begin,
                    Iterator& rollback,
                    priority_tag<0>) noexcept(noexcept(begin != rollback,
                                                       (void)(--begin),
                                                       (void)(begin ==
                                                              ranges::end(r))))
                {
                    while (begin != rollback) {
                        --begin;
                        if (begin == ranges::end(r)) {
                            return error(error::unrecoverable_stream_error,
                                         "Putback failed");
                        }
                    }
                    return {};
                }

            public:
                template <typename Range, typename Iterator>
                auto operator()(const Range& r,
                                Iterator& begin,
                                Iterator& rollback) const
                    noexcept(noexcept(
                        fn::impl(r, begin, rollback, priority_tag<2>{})))
                        -> decltype(
                            fn::impl(r, begin, rollback, priority_tag<2>{}))
                {
                    return fn::impl(r, begin, rollback, priority_tag<2>{});
                }
            };
        }  // namespace _reset_to_rollback_point
        namespace {
            static SCN_CONSTEXPR auto& reset_to_rollback_point =
                static_const<_reset_to_rollback_point::fn>::value;
        }

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
            using char_type = typename extract_char_type<iterator>::type;

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
            auto data() const
                noexcept(noexcept(*std::declval<ranges::iterator_t<const R>>()))
                    -> decltype(std::addressof(
                        *std::declval<ranges::iterator_t<const R>>()))
            {
                return std::addressof(*m_begin);
            }
            template <typename R = Range,
                      typename std::enable_if<
                          ranges::sized_range<R>::value>::type* = nullptr>
            auto size() const noexcept(noexcept(
                ranges::distance(std::declval<ranges::iterator_t<const R>>(),
                                 std::declval<ranges::sentinel_t<const R>>())))
                -> decltype(ranges::distance(
                    std::declval<ranges::iterator_t<const R>>(),
                    std::declval<ranges::sentinel_t<const R>>()))
            {
                return ranges::distance(m_begin, end());
            }

            error reset_to_rollback_point()
            {
                return ::scn::detail::reset_to_rollback_point(range(), begin(),
                                                              m_rollback);
            }
            void set_rollback_point()
            {
                m_rollback = m_begin;
            }

            // iterator value type is a character
            static SCN_CONSTEXPR bool is_direct = is_direct_impl<Range>::value;
            // can call .data() and memcpy
            static SCN_CONSTEXPR bool is_contiguous =
                is_contiguous_impl<Range>::value;
            // provides mechanism to get a pointer to memcpy from
            static SCN_CONSTEXPR bool provides_buffer_access =
                provides_buffer_access_impl<Range>::value;

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
            using char_type = typename extract_char_type<iterator>::type;

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
                return ::scn::detail::reset_to_rollback_point(range(), begin(),
                                                              m_rollback);
            }
            void set_rollback_point()
            {
                m_rollback = m_begin;
            }

            static SCN_CONSTEXPR bool is_direct = is_direct_impl<Range>::value;
            static SCN_CONSTEXPR bool is_contiguous =
                is_contiguous_impl<Range>::value;
            static SCN_CONSTEXPR bool provides_buffer_access =
                provides_buffer_access_impl<Range>::value;

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
            using char_type = typename extract_char_type<iterator>::type;

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
            auto data() const
                noexcept(noexcept(*std::declval<ranges::iterator_t<const R>>()))
                    -> decltype(std::addressof(
                        *std::declval<ranges::iterator_t<const R>>()))
            {
                return std::addressof(*m_begin);
            }
            template <typename R = Range,
                      typename std::enable_if<
                          ranges::sized_range<R>::value>::type* = nullptr>
            auto size() const noexcept(noexcept(
                ranges::distance(std::declval<ranges::iterator_t<const R>>(),
                                 std::declval<ranges::sentinel_t<const R>>())))
                -> decltype(ranges::distance(
                    std::declval<ranges::iterator_t<const R>>(),
                    std::declval<ranges::sentinel_t<const R>>()))
            {
                return ranges::distance(m_begin, end());
            }

            error reset_to_rollback_point()
            {
                return ::scn::detail::reset_to_rollback_point(range(), begin(),
                                                              m_rollback);
            }
            void set_rollback_point()
            {
                m_rollback = m_begin;
            }

            static SCN_CONSTEXPR bool is_direct = is_direct_impl<Range>::value;
            static SCN_CONSTEXPR bool is_contiguous =
                is_contiguous_impl<Range>::value;
            static SCN_CONSTEXPR bool provides_buffer_access =
                provides_buffer_access_impl<Range>::value;

        private:
            Range m_range;
            iterator m_begin;
            iterator m_rollback;
        };
#if 0
        // input reference range
        // file_view
        // input range (ever only single valid instance)
        // begin() returns a reference
        template <typename Range>
        class input_reference_range_wrapper {
        public:
            using range_type = Range;
            using storage_type = Range;
            using return_type = range_reference_return_type<Range>;
            using iterator = ranges::iterator_t<const Range>;
            using sentinel = ranges::sentinel_t<const Range>;
            using char_type = typename extract_char_type<iterator>::type;

            template <typename R>
            input_reference_range_wrapper(R&& r) : m_range(std::forward<R>(r))
            {
            }

            return_type get_return() const
            {
                return {m_range};
            }

            const Range& range() const noexcept
            {
                return m_range;
            }

            SCN_NODISCARD iterator& begin() noexcept
            {
                return m_range.begin();
            }
            const iterator& begin() const noexcept
            {
                return m_range.begin();
            }
            sentinel end() const
                noexcept(noexcept(ranges::end(std::declval<const Range&>())))
            {
                return m_range.end();
            }

            template <typename R = Range,
                      typename std::enable_if<
                          ranges::contiguous_range<R>::value>::type* = nullptr>
            auto data() const noexcept(noexcept(*std::declval<iterator>()))
                -> decltype(std::addressof(*std::declval<iterator>()))
            {
                return std::addressof(*begin());
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
                return ranges::distance(begin(), end());
            }

            error reset_to_rollback_point()
            {
                return ::scn::detail::reset_to_rollback_point(range(), begin(),
                                                              end());
            }
            void set_rollback_point() {}

            static SCN_CONSTEXPR bool is_direct = is_direct_impl<Range>::value;
            static SCN_CONSTEXPR bool is_contiguous =
                is_contiguous_impl<Range>::value;
            static SCN_CONSTEXPR bool provides_buffer_access =
                provides_buffer_access_impl<Range>::value;

        private:
            Range m_range;
        };
#endif

        namespace _make_range_wrapper {
            struct fn {
            private:
                template <typename Range>
                static view_range_wrapper<Range> impl(
                    view_range_wrapper<Range> r,
                    priority_tag<5>)
                {
                    return r;
                }
                template <typename Range>
                static lvalue_range_wrapper<Range> impl(
                    lvalue_range_wrapper<Range> r,
                    priority_tag<5>)
                {
                    return r;
                }
                template <typename Range>
                static rvalue_range_wrapper<Range> impl(
                    rvalue_range_wrapper<Range> r,
                    priority_tag<5>)
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
                    priority_tag<4>)
                {
                    return view_range_wrapper<basic_string_view<CharT>>(str);
                }
                template <typename CharT,
                          std::size_t N,
                          typename std::enable_if<
                              std::is_integral<CharT>::value>::type* = nullptr>
                static view_range_wrapper<basic_string_view<CharT>> impl(
                    CharT(&&str)[N],
                    priority_tag<4>) = delete;

                // std::string -> string_view
                template <typename CharT, typename Traits, typename Allocator>
                static view_range_wrapper<basic_string_view<CharT>> impl(
                    const std::basic_string<CharT, Traits, Allocator>& str,
                    priority_tag<4>)
                {
                    return view_range_wrapper<basic_string_view<CharT>>(
                        {str.data(), str.size()});
                }
                template <typename CharT, typename Traits, typename Allocator>
                static rvalue_range_wrapper<
                    std::basic_string<CharT, Traits, Allocator>>
                impl(std::basic_string<CharT, Traits, Allocator>&& str,
                     priority_tag<4>)
                {
                    return {std::move(str)};
                }

                // span -> string_view
                template <typename CharT>
                static view_range_wrapper<
                    basic_string_view<typename std::remove_const<CharT>::type>>
                impl(span<CharT> s, priority_tag<4>)
                {
                    return {s.data(), s.size()};
                }

                template <typename Range>
                static auto impl(Range& r, priority_tag<3>) noexcept(noexcept(
                    r.make_range_wrapper())) -> decltype(r.make_range_wrapper())
                {
                    return r.make_range_wrapper();
                }
                template <typename Range>
                static auto impl(Range&& r, priority_tag<2>) noexcept(
                    noexcept(make_range_wrapper(std::forward<Range>(r))))
                    -> decltype(make_range_wrapper(std::forward<Range>(r)))
                {
                    return make_range_wrapper(std::forward<Range>(r));
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
                    noexcept(noexcept(fn::impl(std::forward<Range>(r),
                                               priority_tag<5>{})))
                        -> decltype(fn::impl(std::forward<Range>(r),
                                             priority_tag<5>{}))
                {
                    return fn::impl(std::forward<Range>(r), priority_tag<5>{});
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

    namespace detail {
        template <typename CharT>
        expected<CharT> wrap_deref(CharT ch)
        {
            return {ch};
        }
        template <typename CharT>
        expected<CharT> wrap_deref(expected<CharT> e)
        {
            return e;
        }
    }  // namespace detail

    template <typename Iterator>
    class backtracking_iterator {
    public:
        using iterator = Iterator;
        using underlying_value_type = detail::ranges::iter_value_t<iterator>;
        using char_type = typename detail::extract_char_type<iterator>::type;
        using traits = std::char_traits<char_type>;

        using value_type = expected<char_type>;
        using reference = expected<char_type>;
        using pointer = expected<char_type>*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        backtracking_iterator(iterator it) : m_it(std::move(it)) {}

        expected<char_type> operator*()
        {
            if (m_n > 0) {
                return *(m_buf.end() - m_n);
            }
            if (m_latest) {
                if (m_latest.value() == traits::eof()) {
                    return _read_next();
                }
                return traits::to_char_type(m_latest.value());
            }
            return m_latest.error();
        }
        backtracking_iterator& operator++()
        {
            if (m_n > 0) {
                --m_n;
            }
            else {
                _read_next();
            }
            return *this;
        }
        backtracking_iterator& operator--()
        {
            ++m_n;
            return *this;
        }

        bool operator==(const backtracking_iterator& o) const
        {
            return m_it == o.m_it && m_n == o.m_n;
        }
        bool operator!=(const backtracking_iterator& o) const
        {
            return !operator==(o);
        }

        template <typename Sentinel>
        bool operator==(const Sentinel& o) const
        {
            if (m_n != 0) {
                return false;
            }
            return m_it == o;
        }
        template <typename Sentinel>
        bool operator!=(const Sentinel& o) const
        {
            return !operator==(o);
        }

        const iterator& base() const
        {
            return m_it;
        }

        template <typename F>
        bool sync(F s)
        {
            if (m_n == 0) {
                return true;
            }
            if (s(span<char_type>(std::addressof(*(m_buf.end() - m_n)),
                                  std::addressof(*(m_buf.end() - 1)) + 1))) {
                m_buf.clear();
                m_n = 0;
                return true;
            }
            return false;
        }

    private:
        expected<char_type> _read_next()
        {
            m_buf.push_back(traits::to_char_type(m_latest.value()));
            ++m_it;
            auto next = detail::wrap_deref(*m_it);
            if (next) {
                m_latest = {traits::to_int_type(next.value())};
            }
            else {
                m_latest = {next.error()};
            }
            return next;
        }

        iterator m_it;
        std::ptrdiff_t m_n{0};
        std::basic_string<char_type> m_buf{};
        expected<typename traits::int_type> m_latest{traits::eof()};
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_RANGE_H
