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
#include <fstream>
#include <istream>
#include <sstream>
#if defined(__cpp_lib_spanstream)
#include <spanstream>
#endif
#endif

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
/**
 * Wraps `SourceRange`, and makes it a `std::basic_streambuf`.
 *
 * Used by `basic_istream_scanner`.
 */
template <typename SourceRange>
class basic_range_streambuf
    : public std::basic_streambuf<detail::char_t<SourceRange>> {
    using base = std::basic_streambuf<detail::char_t<SourceRange>>;

    static_assert(ranges::bidirectional_range<SourceRange>);

public:
    using range_type = SourceRange;
    using iterator = ranges::iterator_t<SourceRange>;
    using char_type = typename base::char_type;
    using traits_type = typename base::traits_type;
    using int_type = typename base::int_type;

    explicit basic_range_streambuf(range_type range)
        : m_range(range), m_begin(ranges::begin(m_range))
    {
    }

    iterator begin() const
        noexcept(std::is_nothrow_copy_constructible_v<iterator>)
    {
        return m_begin;
    }

private:
    // There's no get area: every read goes through these virtual functions,
    // and `m_begin` always points to the next character to be read

    int_type underflow() override
    {
        if (m_begin == ranges::end(m_range)) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(*m_begin);
    }

    int_type uflow() override
    {
        auto ret = underflow();
        if (!traits_type::eq_int_type(ret, traits_type::eof())) {
            ++m_begin;
        }
        return ret;
    }

    std::streamsize showmanyc() override
    {
        return m_begin == ranges::end(m_range) ? -1 : 1;
    }

    int_type pbackfail(int_type c) override
    {
        if (m_begin == ranges::begin(m_range)) {
            return traits_type::eof();
        }
        --m_begin;

        const auto previous = traits_type::to_int_type(*m_begin);
        if (traits_type::eq_int_type(c, traits_type::eof())) {
            return traits_type::not_eof(previous);
        }
        // The source can't be modified, so only putting back the character
        // that was there can succeed
        if (!traits_type::eq_int_type(c, previous)) {
            ++m_begin;
            return traits_type::eof();
        }
        return c;
    }

    range_type m_range;
    iterator m_begin;
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
 *   friend std::istream& operator>>(std::istream&, mytype&);
 * };
 *
 * // Use mytype::operator>> for scanning with scnlib
 * template <typename CharT>
 * struct scn::scanner<mytype, CharT> : scn::basic_istream_scanner<CharT> {};
 *
 * auto result = scn::scan<mytype>(...);
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
        std::basic_istream<CharT> stream(std::addressof(streambuf));
#if !SCN_DISABLE_LOCALE
        if (ctx.locale()) {
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
            stream.imbue(ctx.locale().template get<std::locale>());
            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
        }
#endif

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

        return streambuf.begin();
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
            return x.at_end();
        }
        friend bool operator!=(const iterator& x, default_sentinel_t)
        {
            return !(x == default_sentinel);
        }

        friend bool operator==(default_sentinel_t, const iterator& x)
        {
            return x == default_sentinel;
        }
        friend bool operator!=(default_sentinel_t, const iterator& x)
        {
            return x != default_sentinel;
        }

    private:
        friend basic_istreambuf_view;

        explicit iterator(const basic_istreambuf_view& parent) noexcept
            : m_parent(&parent)
        {
        }

        // A friend of a nested class can't access the private members of the
        // enclosing class, but a member can
        bool at_end() const
        {
            SCN_EXPECT(m_parent);
            return !m_parent->m_current.has_value();
        }

        const basic_istreambuf_view* m_parent{nullptr};
    };

    explicit basic_istreambuf_view(streambuf_type& buf) : m_buf(&buf) {}

    SCN_NODISCARD iterator begin() const
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

    void _read() const
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
    mutable std::optional<CharT> m_current{};
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
struct scan_istream_access;
}

/**
 * Non-owning view into a `std::basic_istream`, for scanning
 *
 * A `std::basic_streambuf` is only required to support putting back the
 * characters it has returned, and many don't even do that.
 * Thus, a scan can fail to give the characters it read back to the stream.
 * `basic_scan_istream` stores these unsynced characters,
 * and the next scan from `*this` reads them first.
 * Destroying a `basic_scan_istream` with unsynced characters drops them.
 *
 * `CharT` must be `char` or `wchar_t`.
 *
 * If a stream buffer can't put all the characters back,
 * scnlib seeks it back instead,
 * and verifies the seek by reading the characters again.
 * This isn't done on Windows.
 * For a stream passed to the constructor as a `std::basic_ifstream` or
 * `std::basic_fstream`, the seek is done only if the file is a regular file
 * or a block device.
 * The file type can be checked with libc++,
 * and with libstdc++ 14 or later if scnlib is built as C++26.
 * Otherwise, and for a stream passed as a `std::basic_istream`,
 * a seek on a terminal could appear to succeed, and verifying it could block.
 *
 * `std::basic_istringstream`, `std::basic_stringstream`, and, if available,
 * `std::basic_ispanstream` and `std::basic_spanstream` never fail to put
 * characters back.
 * They can be scanned from directly, without a `basic_scan_istream`.
 * `scn::ranges::istreambuf_view` is an alternative that never puts back.
 *
 * \ingroup scannable
 */
template <typename CharT>
class basic_scan_istream {
    static_assert(detail::is_valid_char_type<CharT>,
                  "basic_scan_istream only supports char and wchar_t");

    friend struct detail::scan_istream_access;

public:
    explicit basic_scan_istream(std::basic_istream<CharT>& stream) noexcept
        : m_stream(&stream)
    {
    }

    /**
     * Reads through the `std::basic_filebuf` of `stream`,
     * even if the stream buffer of the base class has been replaced.
     * With this overload, the file type can be checked before seeking
     * (see above).
     */
    explicit basic_scan_istream(std::basic_ifstream<CharT>& stream) noexcept
        : m_stream(&stream), m_filebuf(stream.rdbuf())
    {
    }
    /// Like the constructor taking a `std::basic_ifstream`
    explicit basic_scan_istream(std::basic_fstream<CharT>& stream) noexcept
        : m_stream(&stream), m_filebuf(stream.rdbuf())
    {
    }

    ~basic_scan_istream() = default;

    basic_scan_istream(const basic_scan_istream&) = delete;
    basic_scan_istream& operator=(const basic_scan_istream&) = delete;

    basic_scan_istream(basic_scan_istream&& other) noexcept
        : m_unsynced(SCN_MOVE(other.m_unsynced)),
          m_stream(other.m_stream),
          m_filebuf(other.m_filebuf)
    {
        other.m_unsynced = {};
        other.m_stream = nullptr;
        other.m_filebuf = nullptr;
    }
    basic_scan_istream& operator=(basic_scan_istream&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        m_unsynced = SCN_MOVE(other.m_unsynced);
        m_stream = other.m_stream;
        m_filebuf = other.m_filebuf;
        other.m_unsynced = {};
        other.m_stream = nullptr;
        other.m_filebuf = nullptr;
        return *this;
    }

    /**
     * Returns the stream associated with `*this`.
     * If `*this` has unsynced characters, returns `std::nullopt`.
     * They can be retrieved with `unsynced()` or `release_unsynced()`,
     * or given back to the stream with `sync()`.
     * If `*this` has a null stream (i.e., it has been moved from),
     * returns a null pointer.
     * A scan fails with `scan_error::invalid_source_state` if `*this` has a
     * null stream, or if the stream has no stream buffer
     * (`rdbuf() == nullptr`).
     */
    SCN_NODISCARD std::optional<std::basic_istream<CharT>*> stream() const
    {
        if (m_unsynced.empty()) {
            return m_stream;
        }
        return std::nullopt;
    }

    /**
     * Returns the characters read from the stream that could not be given
     * back to it, if any.
     */
    SCN_NODISCARD std::basic_string_view<CharT> unsynced() const
    {
        return m_unsynced;
    }

    /**
     * Returns the unsynced characters, and leaves `*this` without any.
     * After this, `stream()` returns the stream again.
     * The returned characters are no longer read by the next scan.
     */
    std::basic_string<CharT> release_unsynced() noexcept
    {
        return std::exchange(m_unsynced, std::basic_string<CharT>{});
    }

    /**
     * Gives the unsynced characters back to the stream,
     * like a scan does at its end.
     * Characters that can't be given back stay in `unsynced()`,
     * in the same order.
     * If `*this` has a null stream, or the stream has no stream buffer,
     * all of them stay.
     * Returns `true` if `unsynced()` is empty afterwards,
     * including when it was empty to begin with.
     * Otherwise, returns `false`.
     *
     * Call this before using `stream()` or the `std::basic_istream` directly
     * again, after a scan has failed to sync with the stream.
     */
    SCN_PUBLIC bool sync();

private:
    std::basic_string<CharT> m_unsynced{};
    std::basic_istream<CharT>* m_stream{nullptr};
    std::basic_filebuf<CharT>* m_filebuf{nullptr};
};

extern template SCN_PUBLIC bool basic_scan_istream<char>::sync();
extern template SCN_PUBLIC bool basic_scan_istream<wchar_t>::sync();

using scan_istream = basic_scan_istream<char>;
using wscan_istream = basic_scan_istream<wchar_t>;

namespace detail {

struct scan_istream_access {
    template <typename CharT>
    static std::basic_istream<CharT>* get_stream(basic_scan_istream<CharT>& s)
    {
        return s.m_stream;
    }

    template <typename CharT>
    static std::basic_filebuf<CharT>* get_filebuf(basic_scan_istream<CharT>& s)
    {
        return s.m_filebuf;
    }

    template <typename CharT>
    static std::basic_string<CharT>& get_unsynced(basic_scan_istream<CharT>& s)
    {
        return s.m_unsynced;
    }
};

/**
 * basic_scan_buffer implementation for std::basic_streambuf.
 * A null stream buffer is treated as being at the end of input.
 */
template <typename CharT>
class basic_scan_istream_buffer : public basic_scan_buffer<CharT> {
    using base = basic_scan_buffer<CharT>;
    using traits = typename std::basic_streambuf<CharT>::traits_type;

public:
    SCN_PUBLIC explicit basic_scan_istream_buffer(
        std::basic_streambuf<CharT>* streambuf,
        const std::basic_filebuf<CharT>* filebuf = nullptr) noexcept;
    SCN_PUBLIC ~basic_scan_istream_buffer() override;

protected:
    // Returns the position the stream buffer was synced to,
    // which is `position` if all the characters were given back
    SCN_PUBLIC std::ptrdiff_t sync_streambuf(std::ptrdiff_t position);

    std::basic_streambuf<CharT>* m_streambuf;
    // If not null, `m_streambuf` is this, and the type of the file is checked
    // before seeking
    const std::basic_filebuf<CharT>* m_filebuf;

private:
    SCN_PUBLIC bool do_fill() override;

    SCN_PUBLIC bool do_sync(std::ptrdiff_t position) override;

    std::basic_string<CharT> m_buf;
    bool m_view_is_peeked{false};
};

extern template SCN_PUBLIC basic_scan_istream_buffer<
    char>::basic_scan_istream_buffer(std::streambuf*,
                                     const std::filebuf*) noexcept;
extern template SCN_PUBLIC basic_scan_istream_buffer<
    wchar_t>::basic_scan_istream_buffer(std::wstreambuf*,
                                        const std::wfilebuf*) noexcept;

/**
 * Returns whether the file of `filebuf` is a regular file or a block device.
 * A seek can be verified by reading only for these.
 * Returns `false` for a closed `filebuf`.
 * Returns `std::nullopt` if the file descriptor is not available
 * (on Windows, and in some standard libraries before C++26).
 */
template <typename CharT>
SCN_PUBLIC std::optional<bool> is_seekable_filebuf(
    const std::basic_filebuf<CharT>& filebuf);

extern template SCN_PUBLIC std::optional<bool> is_seekable_filebuf(
    const std::filebuf&);
extern template SCN_PUBLIC std::optional<bool> is_seekable_filebuf(
    const std::wfilebuf&);

extern template SCN_PUBLIC
    basic_scan_istream_buffer<char>::~basic_scan_istream_buffer();
extern template SCN_PUBLIC
    basic_scan_istream_buffer<wchar_t>::~basic_scan_istream_buffer();

/**
 * basic_scan_buffer implementation for basic_scan_istream.
 * Characters that can't be given back to the stream are stored in the
 * wrapper.
 */
template <typename CharT>
class basic_scan_istream_wrapper_buffer
    : public basic_scan_istream_buffer<CharT> {
    using base = basic_scan_istream_buffer<CharT>;

public:
    SCN_PUBLIC explicit basic_scan_istream_wrapper_buffer(
        basic_scan_istream<CharT>& strm) noexcept;
    SCN_PUBLIC ~basic_scan_istream_wrapper_buffer() override;

private:
    SCN_PUBLIC bool do_sync(std::ptrdiff_t position) override;

    static std::basic_streambuf<CharT>* get_streambuf(
        basic_scan_istream<CharT>& strm) noexcept;

    std::basic_string<CharT>& m_unsynced;
};

extern template SCN_PUBLIC basic_scan_istream_wrapper_buffer<
    char>::basic_scan_istream_wrapper_buffer(scan_istream&) noexcept;
extern template SCN_PUBLIC basic_scan_istream_wrapper_buffer<
    wchar_t>::basic_scan_istream_wrapper_buffer(wscan_istream&) noexcept;

extern template SCN_PUBLIC basic_scan_istream_wrapper_buffer<
    char>::~basic_scan_istream_wrapper_buffer();
extern template SCN_PUBLIC basic_scan_istream_wrapper_buffer<
    wchar_t>::~basic_scan_istream_wrapper_buffer();

template <typename CharT>
basic_scan_istream_wrapper_buffer<CharT> make_scan_buffer(
    basic_scan_istream<CharT>& stream,
    make_scan_buffer_tag) noexcept
{
    return basic_scan_istream_wrapper_buffer<CharT>{stream};
}

// The character type of the stream types that can be scanned from directly,
// if the stream is one
template <typename CharT, typename Alloc>
CharT string_stream_char_type_impl(
    const std::basic_istringstream<CharT, std::char_traits<CharT>, Alloc>*);
template <typename CharT, typename Alloc>
CharT string_stream_char_type_impl(
    const std::basic_stringstream<CharT, std::char_traits<CharT>, Alloc>*);
#if defined(__cpp_lib_spanstream)
template <typename CharT>
CharT string_stream_char_type_impl(const std::basic_ispanstream<CharT>*);
template <typename CharT>
CharT string_stream_char_type_impl(const std::basic_spanstream<CharT>*);
#endif

template <typename T>
using dt_string_stream_char_type = decltype(string_stream_char_type_impl(
    static_cast<const remove_cvref_t<T>*>(nullptr)));

template <typename T, typename = void>
inline constexpr bool is_scan_string_stream = false;
template <typename T>
inline constexpr bool is_scan_string_stream<
    T,
    std::enable_if_t<
        is_valid_char_type<mp_eval_or<void, dt_string_stream_char_type, T>>>> =
    true;

// Read through the derived class's rdbuf(), which is the string buffer even if
// the base class's stream buffer has been replaced
template <typename Stream,
          std::enable_if_t<is_scan_string_stream<Stream> &&
                           !std::is_const_v<Stream>>* = nullptr>
basic_scan_istream_buffer<dt_string_stream_char_type<Stream>> make_scan_buffer(
    Stream& stream,
    make_scan_buffer_tag) noexcept
{
    return basic_scan_istream_buffer<dt_string_stream_char_type<Stream>>{
        stream.rdbuf()};
}

// Any other std::basic_istream is rejected in make_scan_buffer.
// It's less specialized than the overloads above, so it's chosen only for the
// types they don't accept.
template <typename CharT, typename Traits>
std::conditional_t<std::is_same_v<Traits, std::char_traits<CharT>>,
                   unwrapped_istream_source<CharT>,
                   custom_char_traits_istream_source<CharT>>
make_scan_buffer(std::basic_istream<CharT, Traits>&,
                 make_scan_buffer_tag) noexcept
{
    return {};
}

// make_vscan_result rejects rvalue wrappers, so that the error is about a
// deleted function, not a missing `scan` overload
template <typename CharT>
basic_scan_istream_wrapper_buffer<CharT> make_scan_buffer(
    basic_scan_istream<CharT>&& stream,
    make_scan_buffer_tag) noexcept
{
    return basic_scan_istream_wrapper_buffer<CharT>{stream};
}

template <typename T>
using dt_char_type = typename T::char_type;

template <typename T>
using dt_traits_type = typename T::traits_type;

template <typename T>
inline constexpr bool is_derived_from_istream = std::is_base_of_v<
    std::basic_istream<
        mp_eval_or<char, dt_char_type, remove_cvref_t<T>>,
        mp_eval_or<std::char_traits<char>, dt_traits_type, remove_cvref_t<T>>>,
    remove_cvref_t<T>>;

template <typename T>
inline constexpr bool is_scan_istream_wrapper =
    is_specialization_of_v<T, basic_scan_istream>;

// Includes the stream types that are rejected by make_scan_buffer,
// so that computing the result type doesn't fail before the rejection does
template <typename T>
inline constexpr bool has_istream_result =
    is_derived_from_istream<T> || is_scan_istream_wrapper<T>;

template <typename T>
struct custom_scan_result<T, std::enable_if_t<has_istream_result<T>>> {
    using type = remove_cvref_t<T>*;
};

template <typename Source>
class scan_result_istream_storage {
    friend struct scan_result_source_access;

public:
    using source_type = Source;

    scan_result_istream_storage() = default;

    explicit scan_result_istream_storage(source_type& s) : m_source(&s) {}
    explicit scan_result_istream_storage(source_type* s) : m_source(s)
    {
        SCN_EXPECT(s);
    }

    SCN_NODISCARD source_type& source() const
    {
        SCN_EXPECT(m_source);
        return *m_source;
    }

    void set(source_type& s)
    {
        m_source = &s;
    }
    void set(source_type* s)
    {
        SCN_EXPECT(s);
        m_source = s;
    }

private:
    source_type* m_source{nullptr};
};

template <typename T>
struct custom_scan_result_source_storage<
    T,
    std::enable_if_t<has_istream_result<T>>> {
    using type = scan_result_istream_storage<remove_cvref_t<T>>;
};
template <typename T>
struct custom_scan_result_source_storage<
    T*,
    std::enable_if_t<has_istream_result<T>>> {
    using type = scan_result_istream_storage<remove_cvref_t<T>>;
};

template <typename T,
          typename CharT,
          std::enable_if_t<has_istream_result<T>>* = nullptr>
auto make_vscan_result(T& source,
                       const basic_scan_istream_buffer<CharT>&,
                       std::ptrdiff_t)
{
    return &source;
}
template <typename T,
          typename CharT,
          std::enable_if_t<!std::is_reference_v<T> && has_istream_result<T>>* =
              nullptr>
auto make_vscan_result(T&& source,
                       const basic_scan_istream_buffer<CharT>&,
                       std::ptrdiff_t) = delete;

}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn

#endif  // !SCN_DISABLE_IOSTREAM
