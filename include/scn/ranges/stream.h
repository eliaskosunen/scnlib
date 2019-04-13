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

#ifndef SCN_RANGES_STREAM_H
#define SCN_RANGES_STREAM_H

#include "../detail/context.h"
#include "../detail/stream.h"

#include <range/v3/core.hpp>

namespace scn {
    namespace ranges {
        template <typename CharT>
        class erased_range_stream_base {
        public:
            using char_type = CharT;

            erased_range_stream_base(const erased_range_stream_base&) = delete;
            erased_range_stream_base& operator=(
                const erased_range_stream_base&) = delete;
            erased_range_stream_base(erased_range_stream_base&&) = default;
            erased_range_stream_base& operator=(erased_range_stream_base&&) =
                default;

            virtual ~erased_range_stream_base() = default;

            virtual size_t chars_read() const = 0;

        protected:
            erased_range_stream_base() = default;
        };

        template <typename Stream>
        class erased_range_stream_impl
            : public erased_range_stream_base<typename Stream::char_type> {
            using base = erased_stream_impl<Stream>;

        public:
            using char_type = typename base::char_type;

            erased_range_stream_impl(Stream& s) : m_stream(std::addressof(s)) {}

            size_t chars_read() const override
            {
                return m_stream->chars_read();
            }

        private:
            Stream* m_stream;
        };

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

            template <typename Stream,
                      typename std::enable_if<
                          std::is_base_of<stream_base, Stream>::value>::type* =
                          nullptr>
            basic_erased_range_stream(Stream s)
                : base(std::move(s)),
                  m_stream(new erased_range_stream_impl<Stream>(
                      base::template get_as<Stream>().get()))
            {
            }

            size_t chars_read() const
            {
                return m_stream->chars_read();
            }

        private:
            detail::unique_ptr<erased_range_stream_base<CharT>> m_stream;
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
            using underlying_iterator = ::ranges::iterator_t<range_type>;
            using underlying_sentinel = ::ranges::sentinel_t<range_type>;

            using iterator = underlying_iterator;
            using char_type = ::ranges::value_type_t<iterator>;

            constexpr basic_bidirectional_range_stream(range_type& r) noexcept
                : m_range(std::addressof(r)),
                  m_begin(::ranges::begin(*m_range)),
                  m_next(m_begin)
            {
            }

            constexpr either<char_type> read_char() noexcept
            {
                if (m_next == ::ranges::end(*m_range)) {
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
                    std::distance(::ranges::begin(*m_range), m_next));
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
                return static_cast<size_t>(
                    std::distance(base::m_next, ::ranges::end(*base::m_range)));
            }

            constexpr error skip(size_t n) noexcept
            {
                if (chars_to_read() < n) {
                    base::m_next = ::ranges::end(*base::m_range);
                    return error(error::end_of_stream, "EOF");
                }
                base::m_next += static_cast<std::ptrdiff_t>(n);
                return {};
            }
            constexpr error skip_all() noexcept
            {
                base::m_next = ::ranges::end(*base::m_range);
                return {};
            }
        };

        template <typename Range>
        class basic_forward_range_stream : public stream_base {
        public:
            using range_type = Range;
            using underlying_iterator = ::ranges::iterator_t<range_type>;
            using underlying_sentinel = ::ranges::sentinel_t<range_type>;

            using iterator = underlying_iterator;
            using char_type = ::ranges::value_type_t<iterator>;

            constexpr basic_forward_range_stream(range_type& r)
                : m_range(std::addressof(r)),
                  m_begin(::ranges::begin(*m_range)),
                  m_next(m_begin)
            {
            }

            either<char_type> read_char() noexcept
            {
                if (m_rollback.size() > 0) {
                    auto top = m_rollback.back();
                    m_rollback.pop_back();
                    return top;
                }
                if (m_begin == ::ranges::end(*m_range)) {
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
                    std::distance(::ranges::begin(*m_range), m_next));
            }

        protected:
            range_type* m_range;
            iterator m_begin, m_next;
            detail::small_vector<char_type, 64> m_rollback{};
        };

        template <typename Range>
        class basic_sized_forward_range_stream
            : public basic_forward_range_stream<Range> {
            using base = basic_forward_range_stream<Range>;

        public:
            using char_type = typename base::char_type;
            using is_sized_stream = std::true_type;

            basic_sized_forward_range_stream(Range& r) : base(r) {}

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

            error set_roll_back() noexcept
            {
                base::m_rollback.clear();
                return {};
            }
            constexpr error roll_back() const noexcept
            {
                return {};
            }

            constexpr size_t chars_to_read() const noexcept
            {
                return static_cast<size_t>(std::distance(
                    base::m_begin, ::ranges::end(*base::m_range)));
            }

            constexpr error skip(size_t n) noexcept
            {
                if (chars_to_read() < n) {
                    base::m_begin = ::ranges::end(*base::m_range);
                    return error(error::end_of_stream, "EOF");
                }
                base::m_next += static_cast<std::ptrdiff_t>(n);
                return {};
            }
            constexpr error skip_all() noexcept
            {
                base::m_begin = ::ranges::end(*base::m_range);
                return {};
            }
        };

        CPP_template(
            typename R,
            typename CharT = ::ranges::value_type_t<::ranges::iterator_t<R>>)(
            requires ::ranges::BidirectionalRange<R> &&
            !::ranges::SizedRange<R>)
            erased_range_stream<CharT> make_stream(R& r)
        {
            auto s = basic_bidirectional_range_stream<R>(r);
            return {s};
        }
        CPP_template(
            typename R,
            typename CharT = ::ranges::value_type_t<::ranges::iterator_t<R>>)(
            requires ::ranges::BidirectionalRange<R>&& ::ranges::SizedRange<R>)
            erased_sized_range_stream<CharT> make_stream(R& r)
        {
            auto s = basic_sized_bidirectional_range_stream<R>(r);
            return {s};
        }
        CPP_template(
            typename R,
            typename CharT = ::ranges::value_type_t<::ranges::iterator_t<R>>)(
            requires ::ranges::ForwardRange<R> &&
            !::ranges::BidirectionalRange<R> && !::ranges::SizedRange<R>)
            erased_range_stream<CharT> make_stream(R& r)
        {
            auto s = basic_forward_range_stream<R>(r);
            return {s};
        }
        CPP_template(
            typename R,
            typename CharT = ::ranges::value_type_t<::ranges::iterator_t<R>>)(
            requires ::ranges::ForwardRange<R> &&
            !::ranges::BidirectionalRange<R> && ::ranges::SizedRange<R>)
            erased_sized_range_stream<CharT> make_stream(R& r)
        {
            auto s = basic_sized_forward_range_stream<R>(r);
            return {s};
        }
    }  // namespace ranges
}  // namespace scn

#endif  // SCN_RANGES_RANGES_H
