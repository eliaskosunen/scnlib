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

#include <scn/scan.h>

#if !SCN_DISABLE_IOSTREAM

#if defined(SCN_MODULE) && defined(SCN_IMPORT_STD)
import std;
#else
#include <istream>
#endif

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
template <typename T, typename CharT, typename Enable = void>
struct is_streamable_impl : std::false_type {};

template <typename T, typename CharT>
struct is_streamable_impl<
    T,
    CharT,
    std::enable_if_t<sizeof(SCN_DECLVAL(std::basic_istream<CharT>&)
                            << std::declval<T>()) != 0>> : std::true_type {};

template <typename CharT>
struct dummy_context_for_is_streamamble {
    using char_type = CharT;
};

template <typename T, typename CharT>
struct is_streamable
    : std::conditional_t<
          std::is_convertible_v<
              std::add_lvalue_reference_t<
                  decltype(arg_mapper<dummy_context_for_is_streamamble<CharT>>::
                               map(SCN_DECLVAL(T&)))>,
              unscannable&>,
          is_streamable_impl<T, CharT>,
          std::false_type> {};

/**
 * Wraps `SourceRange`, and makes it a `std::basic_streambuf`.
 *
 * Used by `basic_istream_scanner`.
 */
template <typename SourceRange>
class basic_range_streambuf
    : public std::basic_streambuf<detail::char_t<SourceRange>> {
    using base = std::basic_streambuf<detail::char_t<SourceRange>>;

public:
    using range_type = SourceRange;
    using iterator = ranges::iterator_t<SourceRange>;
    using char_type = typename base::char_type;
    using traits_type = typename base::traits_type;
    using int_type = typename base::int_type;

    explicit basic_range_streambuf(range_type range)
        : m_range(range), m_begin(ranges::begin(m_range)), m_begin_prev(m_begin)
    {
    }

    iterator begin() const
        noexcept(std::is_nothrow_copy_constructible_v<iterator>)
    {
        return m_begin;
    }
    iterator begin_prev() const
        noexcept(std::is_nothrow_copy_constructible_v<iterator>)
    {
        return m_begin_prev;
    }
    int_type last_char() const noexcept
    {
        return m_ch;
    }

private:
    int_type underflow() override
    {
        // Already read
        if (!traits_type::eq_int_type(m_ch, traits_type::eof())) {
            return m_ch;
        }

        if (m_begin == ranges::end(m_range)) {
            return traits_type::eof();
        }
        m_begin_prev = m_begin;
        SCN_CLANG_PUSH_IGNORE_UNSAFE_BUFFER_USAGE
        m_ch = traits_type::to_int_type(*m_begin++);
        SCN_CLANG_POP_IGNORE_UNSAFE_BUFFER_USAGE
        return m_ch;
    }

    int_type uflow() override
    {
        auto ret = underflow();
        if (ret != traits_type::eof()) {
            m_ch = traits_type::eof();
        }
        return ret;
    }

    std::streamsize showmanyc() override
    {
        return traits_type::eq_int_type(m_ch, traits_type::eof()) ? 0 : 1;
    }

    int_type pbackfail(int_type c) override
    {
        SCN_EXPECT(traits_type::eq_int_type(c, traits_type::eof()));
        SCN_EXPECT(!m_has_put_back);
        m_has_put_back = true;

        m_begin = m_begin_prev;

        if (m_begin == ranges::end(m_range)) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(0);
    }

    range_type m_range;
    iterator m_begin;
    iterator m_begin_prev;
    int_type m_ch{traits_type::eof()};
    bool m_has_put_back{false};
};

using range_streambuf = basic_range_streambuf<scan_context::range_type>;
using wrange_streambuf = basic_range_streambuf<wscan_context::range_type>;
}  // namespace detail

/**
 * Implements the `scn::scanner` interface, by reading the value with
 * `operator>>`.
 *
 * Example:
 *
 * \code{.cpp}
 * #include <scn/istream.h> // required for basic_istream_scanner
 *
 * struct mytype {
 *   friend std::istream& operator>>(std::istream&, const mytype&);
 * };
 *
 * // Use mytype::operator>> for scanning with scnlib
 * template <typename CharT>
 * struct scn::scanner<mytype> : scn::basic_istream_scanner<CharT> {};
 *
 * auto [result, myvalue] = scn::scan<mytype>(...);
 * \endcode
 */
template <typename CharT>
struct basic_istream_scanner {
    template <typename ParseContext>
    constexpr typename ParseContext::iterator parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename T, typename Context>
    scan_expected<typename Context::iterator> scan(T& val, Context& ctx) const
    {
        detail::basic_range_streambuf<typename Context::range_type> streambuf(
            ctx.range());
        using traits = typename decltype(streambuf)::traits_type;
        std::basic_istream<CharT> stream(std::addressof(streambuf));

        if (!(stream >> val)) {
            if (stream.eof()) {
                return detail::unexpected_scan_error(scan_error::end_of_input,
                                                     "EOF");
            }
            if (SCN_UNLIKELY(stream.bad())) {
                return detail::unexpected_scan_error(
                    scan_error::invalid_source_state,
                    "Bad std::istream after reading");
            }

            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Failed to read with std::istream");
        }

        if (traits::eq_int_type(streambuf.last_char(), traits::eof())) {
            return streambuf.begin();
        }
        return streambuf.begin_prev();
    }
};

namespace ranges {

// scnlib ranges extension:
// istreambuf_view

template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_istreambuf_view
    : public view_interface<basic_istreambuf_view<CharT, Traits>> {
    using streambuf_type = std::basic_streambuf<CharT, Traits>;

public:
    class iterator {
    public:
        using iterator_concept = input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = CharT;

        // Needed because of our ranges implementation,
        // not strictly true
        // (This is a C++20 input_iterator, not a Cpp17InputIterator)
        using iterator_category = input_iterator_tag;

        explicit iterator(basic_istreambuf_view& parent) noexcept
            : m_parent(&parent)
        {
        }

        iterator(const iterator&) = delete;
        iterator& operator=(const iterator&) = delete;

        iterator(iterator&&) = default;
        iterator& operator=(iterator&&) = default;

        ~iterator() = default;

        iterator& operator++()
        {
            SCN_EXPECT(m_parent);
            m_parent->_read();
            return *this;
        }
        void operator++(int)
        {
            ++*this;
        }

        SCN_NODISCARD CharT& operator*() const
        {
            SCN_EXPECT(m_parent);
            SCN_EXPECT(m_parent->m_current);
            return *m_parent->m_current;
        }

        friend bool operator==(const iterator& x, default_sentinel_t)
        {
            SCN_EXPECT(x.m_parent);
            return !x.m_parent->m_current.has_value();
        }
        friend bool operator!=(const iterator& x, default_sentinel_t)
        {
            return !(x == default_sentinel);
        }

    private:
        basic_istreambuf_view* m_parent{nullptr};
    };

    explicit basic_istreambuf_view(streambuf_type& buf) : m_buf(&buf) {}

    SCN_NODISCARD iterator begin() const noexcept
    {
        _read();
        return iterator{*this};
    }

    SCN_NODISCARD default_sentinel_t end() const noexcept
    {
        return default_sentinel;
    }

private:
    friend class iterator;

    void _read()
    {
        SCN_EXPECT(m_buf);
        auto val = m_buf->sbumpc();
        if (Traits::eq_int_type(val, Traits::eof())) {
            m_current.reset();
        }
        else {
            m_current = Traits::to_char_type(val);
        }
    }

    streambuf_type* m_buf{nullptr};
    std::optional<CharT> m_current{};
};

using istreambuf_view = basic_istreambuf_view<char>;
using wistreambuf_view = basic_istreambuf_view<wchar_t>;

static_assert(input_range<istreambuf_view>);

namespace views {

namespace istreambuf_ {

struct fn {
    template <typename CharT, typename Traits>
    auto operator()(std::basic_istream<CharT, Traits>& in) const
        -> basic_istreambuf_view<CharT, Traits>
    {
        return basic_istreambuf_view<CharT, Traits>{*in.rdbuf()};
    }
    template <typename CharT, typename Traits>
    auto operator()(std::basic_streambuf<CharT, Traits>& in) const
        -> basic_istreambuf_view<CharT, Traits>
    {
        return basic_istreambuf_view<CharT, Traits>{in};
    }
};

}  // namespace istreambuf_

inline constexpr auto istreambuf = istreambuf_::fn{};

}  // namespace views

}  // namespace ranges

namespace detail {

/**
 * basic_scan_buffer implementation for std::basic_istream
 */
template <typename CharT>
class basic_scan_istream_buffer : public basic_scan_buffer<CharT> {
    using base = basic_scan_buffer<CharT>;
    using traits = typename std::basic_istream<CharT>::traits_type;

public:
    SCN_PUBLIC explicit basic_scan_istream_buffer(
        std::basic_istream<CharT>& strm);
    SCN_PUBLIC ~basic_scan_istream_buffer() override;

    SCN_PUBLIC bool fill() override;

    SCN_PUBLIC bool sync(std::ptrdiff_t position) override;

private:
    std::basic_istream<CharT>* m_stream;
    std::basic_string<CharT> m_buf;
};

using scan_istream_buffer = basic_scan_istream_buffer<char>;
using wscan_istream_buffer = basic_scan_istream_buffer<wchar_t>;

extern template basic_scan_istream_buffer<char>::basic_scan_istream_buffer(
    std::istream&);
extern template basic_scan_istream_buffer<wchar_t>::basic_scan_istream_buffer(
    std::wistream&);

extern template basic_scan_istream_buffer<char>::~basic_scan_istream_buffer();
extern template basic_scan_istream_buffer<
    wchar_t>::~basic_scan_istream_buffer();

inline scan_istream_buffer make_scan_buffer(std::istream& stream,
                                            make_scan_buffer_tag)
{
    return scan_istream_buffer{stream};
}
inline wscan_istream_buffer make_scan_buffer(std::wistream& stream,
                                             make_scan_buffer_tag)
{
    return wscan_istream_buffer{stream};
}

template <typename T>
using dt_char_type = typename T::char_type;

template <typename T>
inline constexpr bool is_derived_from_istream = std::is_base_of_v<
    std::basic_istream<mp_eval_or<char, dt_char_type, remove_cvref_t<T>>>,
    remove_cvref_t<T>>;

template <typename T>
struct custom_scan_result<T, std::enable_if_t<is_derived_from_istream<T>>> {
    using type = remove_cvref_t<T>*;
};

template <typename Stream>
class scan_result_istream_storage {
    friend struct scan_result_source_access;

public:
    using source_type = Stream;

    scan_result_istream_storage() = default;

    explicit scan_result_istream_storage(source_type& s) : m_stream(&s) {}
    explicit scan_result_istream_storage(source_type* s) : m_stream(s)
    {
        SCN_EXPECT(s);
    }

    SCN_NODISCARD source_type& stream()
    {
        SCN_EXPECT(m_stream);
        return *m_stream;
    }

private:
    void set(source_type& s)
    {
        m_stream = &s;
    }
    void set(source_type* s)
    {
        SCN_EXPECT(s);
        m_stream = s;
    }

    source_type* m_stream{nullptr};
};

template <typename T>
struct custom_scan_result_source_storage<
    T,
    std::enable_if_t<is_derived_from_istream<T>>> {
    using type = scan_result_istream_storage<remove_cvref_t<T>>;
};
template <typename T>
struct custom_scan_result_source_storage<
    T*,
    std::enable_if_t<is_derived_from_istream<T>>> {
    using type = scan_result_istream_storage<remove_cvref_t<T>>;
};

template <typename T,
          typename CharT,
          std::enable_if_t<is_derived_from_istream<T>>* = nullptr>
auto make_vscan_result(T& source,
                       const basic_scan_istream_buffer<CharT>&,
                       std::ptrdiff_t)
{
    return &source;
}
template <typename T,
          typename CharT,
          std::enable_if_t<!std::is_reference_v<T> &&
                           is_derived_from_istream<T>>* = nullptr>
auto make_vscan_result(T&& source,
                       const basic_scan_istream_buffer<CharT>&,
                       std::ptrdiff_t) = delete;

}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_USE_IOSTREAMS
