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
                    priority_tag<0>) noexcept(noexcept(SCN_UNUSED(begin !=
                                                                  rollback),
                                                       (void)(--begin),
                                                       (void)(begin ==
                                                              ranges::end(r))))
                {
                    while (begin != rollback) {
                        --begin;
                        if (begin == ranges::end(r)) {
                            return error(error::unrecoverable_source_error,
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

        template <typename Range, typename It>
        void write_return(const Range&, It)
        {
        }
        template <typename Range, typename It>
        void write_return(Range&& r, It begin)
        {
            r = remove_cvref_t<Range>{begin,
                                      ranges::end(std::forward<Range>(r))};
        }

        template <typename Range>
        class range_wrapper {
        public:
            using range_type = Range;
            using iterator = ranges::iterator_t<const Range>;
            using sentinel = ranges::sentinel_t<const Range>;
            using char_type = typename extract_char_type<iterator>::type;
            using return_type = remove_cvref_t<range_type>;

            template <typename R,
                      typename std::enable_if<
                          std::is_same<remove_cvref_t<R>,
                                       return_type>::value>::type* = nullptr>
            range_wrapper(R&& r)
                : m_range(std::forward<R>(r)),
                  m_begin(ranges::begin(m_range)),
                  m_rollback(m_begin)
            {
            }

            remove_cvref_t<range_type> range() const
            {
                return {m_begin, ranges::end(m_range)};
            }
            remove_cvref_t<range_type> get_return()
            {
                write_return(m_range, m_begin);
                return range();
            }

            SCN_NODISCARD iterator& begin() noexcept
            {
                return m_begin;
            }
            iterator begin() const noexcept
            {
                return m_begin;
            }

            sentinel end() const
                noexcept(noexcept(ranges::end(std::declval<const Range&>())))
            {
                return ranges::end(m_range);
            }

            iterator begin_underlying() const
                noexcept(noexcept(ranges::begin(std::declval<const Range&>())))
            {
                return ranges::begin(m_range);
            }
            range_type range_underlying() const noexcept
            {
                return m_range;
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
            range_type m_range;
            iterator m_begin, m_rollback;
        };

        namespace _wrap {
            struct fn {
            private:
                template <typename Range>
                static range_wrapper<Range> impl(range_wrapper<Range> r,
                                                 priority_tag<3>) noexcept
                {
                    return r;
                }

                template <typename Range>
                static auto impl(Range&& r, priority_tag<2>) noexcept(
                    noexcept(std::forward<Range>(r)))
                    -> decltype(wrap(std::forward<Range>(r)))
                {
                    return wrap(std::forward<Range>(r));
                }

                template <typename CharT, std::size_t N>
                static auto impl(CharT (&str)[N], priority_tag<1>) noexcept
                    -> range_wrapper<
                        basic_string_view<typename std::remove_cv<CharT>::type>>
                {
                    return {
                        basic_string_view<typename std::remove_cv<CharT>::type>(
                            str, str + N - 1)};
                }

                template <typename Range>
                static auto impl(Range&& r, priority_tag<0>) noexcept
                    -> range_wrapper<Range>
                {
                    static_assert(ranges::view<remove_cvref_t<Range>>::value,
                                  "Cannot scan from a non-view!");

                    return {std::forward<Range>(r)};
                }

            public:
                template <typename Range>
                auto operator()(Range&& r) const
                    noexcept(noexcept(fn::impl(std::forward<Range>(r),
                                               priority_tag<3>{})))
                        -> decltype(fn::impl(std::forward<Range>(r),
                                             priority_tag<3>{}))
                {
                    return fn::impl(std::forward<Range>(r), priority_tag<3>{});
                }
            };
        }  // namespace _wrap
        namespace {
            static constexpr auto& wrap = static_const<_wrap::fn>::value;
        }

        template <typename Range>
        struct range_wrapper_for {
            using type = decltype(wrap(std::declval<Range>()));
        };
        template <typename Range>
        using range_wrapper_for_t = typename range_wrapper_for<Range>::type;
    }  // namespace detail

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

    namespace _make_view {
        struct fn {
        private:
            template <typename CharT, std::size_t N>
            static basic_string_view<typename std::remove_cv<CharT>::type> impl(
                CharT (&str)[N],
                detail::priority_tag<4>) noexcept
            {
                return {str, str + N - 1};  // damn you, null terminator!
            }

            template <typename CharT>
            static basic_string_view<CharT> impl(
                const std::basic_string<CharT>& str,
                detail::priority_tag<3>) noexcept
            {
                return {str.data(), str.size()};
            }

            template <typename Range>
            static auto impl(Range& r, detail::priority_tag<2>) noexcept(
                noexcept(make_view(r))) -> decltype(make_view(r))
            {
                return make_view(r);
            }
            template <typename Range>
            static auto impl(Range& r, detail::priority_tag<1>) noexcept(
                noexcept(r.make_view())) -> decltype(r.make_view())
            {
                return r.make_view();
            }

            template <typename Range>
            static Range impl(Range v, detail::priority_tag<0>) noexcept
            {
                static_assert(detail::ranges::view<Range>::value,
                              "Unknown value given to make_view");
                return v;
            }

        public:
            template <typename Range>
            auto operator()(Range&& r) const
                noexcept(noexcept(fn::impl(std::forward<Range>(r),
                                           detail::priority_tag<4>{})))
                    -> decltype(fn::impl(std::forward<Range>(r),
                                         detail::priority_tag<4>{}))
            {
                return fn::impl(std::forward<Range>(r),
                                detail::priority_tag<4>{});
            }
        };
    }  // namespace _make_view
    namespace {
        static constexpr auto& make_view =
            detail::static_const<_make_view::fn>::value;
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_RANGE_H
