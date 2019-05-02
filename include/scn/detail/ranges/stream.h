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

#ifndef SCN_DETAIL_RANGES_STREAM_H
#define SCN_DETAIL_RANGES_STREAM_H

#include "../context.h"
#include "../erased_stream.h"

#include "config.h"

namespace scn {
    namespace ranges {
        SCN_BEGIN_NAMESPACE

        namespace detail {
            template <typename CharT>
            class erased_range_stream_base {
            public:
                using char_type = CharT;

                erased_range_stream_base(const erased_range_stream_base&) =
                    delete;
                erased_range_stream_base& operator=(
                    const erased_range_stream_base&) = delete;
                erased_range_stream_base(erased_range_stream_base&&) = default;
                erased_range_stream_base& operator=(
                    erased_range_stream_base&&) = default;

                virtual ~erased_range_stream_base() = default;

                virtual size_t chars_read() const = 0;

            protected:
                erased_range_stream_base() = default;
            };

            template <typename Stream>
            class erased_range_stream_impl
                : public erased_range_stream_base<typename Stream::char_type> {
                using base = ::scn::detail::erased_stream_impl<Stream>;

            public:
                using char_type = typename base::char_type;

                erased_range_stream_impl(Stream& s)
                    : m_stream(std::addressof(s))
                {
                }

                size_t chars_read() const override
                {
                    return m_stream->chars_read();
                }

            private:
                Stream* m_stream;
            };
        }  // namespace detail

        template <typename CharT, bool Sized>
        class basic_erased_range_stream
            : public std::conditional_t<Sized,
                                        erased_sized_stream<CharT>,
                                        erased_stream<CharT>> {
            using base = std::conditional_t<Sized,
                                            erased_sized_stream<CharT>,
                                            erased_stream<CharT>>;

        public:
            using char_type = CharT;
            using is_sized_stream = std::integral_constant<bool, Sized>;

            template <typename Stream>
            basic_erased_range_stream(Stream s)
                : base(std::move(s)),
                  m_stream(::scn::detail::make_unique<
                           detail::erased_range_stream_impl<Stream>>(
                      base::template get_as<Stream>().get()))
            {
            }

            size_t chars_read() const
            {
                return m_stream->chars_read();
            }

        private:
            ::scn::detail::unique_ptr<detail::erased_range_stream_base<CharT>>
                m_stream;
        };

        template <typename CharT>
        using erased_range_stream = basic_erased_range_stream<CharT, false>;
        template <typename CharT>
        using erased_sized_range_stream =
            basic_erased_range_stream<CharT, true>;

        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wpadded")

        template <typename Range>
        class basic_bidirectional_range_stream : public stream_base {
        public:
            using range_type = Range;
            using underlying_iterator = SCN_RANGES_NS::iterator_t<range_type>;
            using underlying_sentinel = SCN_RANGES_NS::sentinel_t<range_type>;

            using iterator = underlying_iterator;
            using char_type = SCN_RANGES_NS::value_type_t<iterator>;

            constexpr basic_bidirectional_range_stream(range_type& r) noexcept
                : m_range(std::addressof(r)),
                  m_begin(SCN_RANGES_NS::begin(*m_range)),
                  m_next(m_begin)
            {
            }

            constexpr expected<char_type> read_char() noexcept
            {
                if (m_next == SCN_RANGES_NS::end(*m_range)) {
                    return error(error::end_of_stream, "EOF");
                }
                auto ch = *m_next;
                ++m_next;
                return ch;
            }
            constexpr error putback(char_type) noexcept
            {
                SCN_EXPECT(m_next != m_begin);
                --m_next;
                return {};
            }

            size_t chars_read() const noexcept
            {
                return static_cast<size_t>(
                    std::distance(SCN_RANGES_NS::begin(*m_range), m_next));
            }

        protected:
            range_type* m_range;
            iterator m_begin, m_next;
        };

        SCN_CLANG_POP

        template <typename Range>
        class basic_sized_bidirectional_range_stream
            : public basic_bidirectional_range_stream<Range> {
            using base = basic_bidirectional_range_stream<Range>;

        public:
            using char_type = typename base::char_type;
            using is_sized_stream = std::true_type;

            basic_sized_bidirectional_range_stream(Range& r) : base(r) {}

            constexpr error read_sized(span<char_type> s) noexcept
            {
                if (chars_to_read() < s.size()) {
                    return error(error::end_of_stream,
                                 "Cannot complete read_sized: EOF encountered");
                }
                const auto ssize = static_cast<std::ptrdiff_t>(s.size());
                std::copy(base::m_next, base::m_next + ssize, s.begin());
                base::m_next += ssize;
                return {};
            }

            constexpr error putback_n(size_t n) noexcept
            {
                auto sn = static_cast<std::ptrdiff_t>(n);
                if (std::distance(base::m_begin, base::m_next) < sn) {
                    return error(error::invalid_argument,
                                 "Cannot putback more than chars read");
                }
                base::m_next -= sn;
                return {};
            }

            constexpr error set_roll_back() noexcept
            {
                base::m_begin = base::m_next;
                return {};
            }
            constexpr error roll_back() noexcept
            {
                base::m_next = base::m_begin;
                return {};
            }

            constexpr size_t chars_to_read() const noexcept
            {
                return static_cast<size_t>(std::distance(
                    base::m_next, SCN_RANGES_NS::end(*base::m_range)));
            }

            constexpr error skip(size_t n) noexcept
            {
                if (chars_to_read() < n) {
                    base::m_next = SCN_RANGES_NS::end(*base::m_range);
                    return error(error::end_of_stream, "EOF");
                }
                base::m_next += static_cast<std::ptrdiff_t>(n);
                return {};
            }
            constexpr error skip_all() noexcept
            {
                base::m_next = SCN_RANGES_NS::end(*base::m_range);
                return {};
            }
        };

        template <typename Range>
        class basic_forward_range_stream : public stream_base {
        public:
            using range_type = Range;
            using underlying_iterator = SCN_RANGES_NS::iterator_t<range_type>;
            using underlying_sentinel = SCN_RANGES_NS::sentinel_t<range_type>;

            using iterator = underlying_iterator;
            using char_type = SCN_RANGES_NS::value_type_t<iterator>;

            constexpr basic_forward_range_stream(range_type& r)
                : m_range(std::addressof(r)),
                  m_begin(SCN_RANGES_NS::begin(*m_range)),
                  m_next(m_begin)
            {
            }

            expected<char_type> read_char() noexcept
            {
                if (m_rollback.size() > 0) {
                    auto top = m_rollback.back();
                    m_rollback.pop_back();
                    return top;
                }
                if (m_begin == SCN_RANGES_NS::end(*m_range)) {
                    return error(error::end_of_stream, "EOF");
                }
                auto ch = *m_begin;
                ++m_begin;
                return ch;
            }
            error putback(char_type ch)
            {
                m_rollback.push_back(ch);
                return {};
            }

            size_t chars_read() const noexcept
            {
                return static_cast<size_t>(
                    std::distance(SCN_RANGES_NS::begin(*m_range), m_next));
            }

        protected:
            range_type* m_range;
            iterator m_begin, m_next;
            ::scn::detail::small_vector<char_type, 64> m_rollback{};
        };

        namespace detail {
            CPP_template(typename R)(
                requires SCN_RANGES_NS::BidirectionalRange<R> &&
                !SCN_RANGES_NS::SizedRange<R>)
                basic_bidirectional_range_stream<R> make_underlying_stream(R& r)
            {
                return {r};
            }
            CPP_template(typename R)(
                requires SCN_RANGES_NS::BidirectionalRange<R>&&
                    SCN_RANGES_NS::SizedRange<R>)
                basic_sized_bidirectional_range_stream<
                    R> make_underlying_stream(R& r)
            {
                return {r};
            }
            CPP_template(typename R)(requires SCN_RANGES_NS::ForwardRange<R> &&
                                     !SCN_RANGES_NS::BidirectionalRange<R>)
                basic_forward_range_stream<R> make_underlying_stream(R& r)
            {
                return {r};
            }

            template <typename R>
            struct erased_stream_for;
            template <typename R>
            struct erased_stream_for<basic_bidirectional_range_stream<R>> {
                template <typename CharT>
                using type = erased_range_stream<CharT>;
            };
            template <typename R>
            struct erased_stream_for<
                basic_sized_bidirectional_range_stream<R>> {
                template <typename CharT>
                using type = erased_sized_range_stream<CharT>;
            };
            template <typename R>
            struct erased_stream_for<basic_forward_range_stream<R>> {
                template <typename CharT>
                using type = erased_range_stream<CharT>;
            };
        }  // namespace detail

        template <typename R,
                  typename CharT =
                      SCN_RANGES_NS::value_type_t<SCN_RANGES_NS::iterator_t<R>>>
        auto make_stream(R& r)
        {
            return detail::make_underlying_stream(r);
        }

        CPP_template(typename R)(
            requires SCN_RANGES_NS::Range<R>) auto erase_stream(R& r)
        {
            using CharT =
                SCN_RANGES_NS::value_type_t<SCN_RANGES_NS::iterator_t<R>>;
            auto s = make_stream(r);
            return typename detail::erased_stream_for<decltype(
                s)>::template type<CharT>(std::move(s));
        }

        SCN_END_NAMESPACE
    }  // namespace ranges
}  // namespace scn

#endif  // SCN_DETAIL_RANGES_RANGES_H
