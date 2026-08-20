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

#include <scn/chrono.h>
#include <scn/impl.h>
#include <scn/istream.h>

#include <mutex>

#if !SCN_DISABLE_LOCALE
#include <locale>
#include <sstream>
#endif

#if !SCN_DISABLE_IOSTREAM
#include <iostream>
#endif

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wunused-macros")

#define SCN_XLOCALE_POSIX 0
#define SCN_XLOCALE_MSVC  1
#define SCN_XLOCALE_OTHER 2

SCN_CLANG_POP

#if SCN_DISABLE_LOCALE
#define SCN_XLOCALE SCN_XLOCALE_OTHER
#elif (!defined(__ANDROID_API__) || __ANDROID_API__ >= 28) && \
    !defined(__EMSCRIPTEN__) && defined(__has_include) && __has_include(<xlocale.h>)
#include <xlocale.h>
#define SCN_XLOCALE SCN_XLOCALE_POSIX

#elif defined(_MSC_VER)
#define SCN_XLOCALE SCN_XLOCALE_MSVC

#elif defined(__GLIBC__) && !defined(__ANDROID_API__) && \
    !defined(__EMSCRIPTEN__)
// glibc

#include <features.h>

#define SCN_XLOCALE SCN_XLOCALE_POSIX

#if !((__GLIBC__ > 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ > 25)))
// xlocale.h was removed in glibc 2.26
#include <xlocale.h>
#else
#include <clocale>
#endif  // __GLIBC__ <= 2.25

#elif defined(__FreeBSD_version) && __FreeBSD_version >= 1000010

// FreeBSD
#include <xlocale.h>
#define SCN_XLOCALE SCN_XLOCALE_POSIX

#endif  // SCN_DISABLE_LOCALE, others

#ifndef SCN_XLOCALE
#define SCN_XLOCALE SCN_XLOCALE_OTHER
#endif

namespace scn {
SCN_BEGIN_NAMESPACE

/////////////////////////////////////////////////////////////////
// Whitespace finders
/////////////////////////////////////////////////////////////////

namespace impl {
namespace {
template <typename R>
bool has_nonascii_char_64(R source)
{
    static_assert(sizeof(*source.data()) == 1);
    SCN_EXPECT(source.size() <= 8);
    uint64_t word{};
    std::memcpy(&word, source.data(), source.size());

    return has_byte_greater(word, 127) != 0;
}

template <typename CuCb, typename CpCb>
std::string_view::iterator find_classic_impl(std::string_view source,
                                             CuCb cu_cb,
                                             CpCb cp_cb)
{
    auto it = source.begin();

    while (it != source.end()) {
        auto sv =
            detail::make_string_view_from_iterators<char>(it, source.end())
                .substr(0, 8);

        if (!has_nonascii_char_64(sv)) {
            auto tmp_it = std::find_if(sv.begin(), sv.end(), cu_cb);
            it = detail::make_string_view_iterator(source, tmp_it);
            if (tmp_it != sv.end()) {
                break;
            }
            continue;
        }

        for (std::size_t i = 0; i < sv.size(); ++i) {
            auto tmp =
                detail::make_string_view_from_iterators<char>(it, source.end());
            auto res = get_next_code_point(tmp);
            if (cp_cb(res.value)) {
                return it;
            }
            i += static_cast<std::size_t>(
                ranges::distance(tmp.data(), detail::to_address(res.iterator)));
            it = detail::make_string_view_iterator(source, res.iterator);
            SCN_ENSURE(it <= source.end());
        }
    }

    return detail::make_string_view_iterator(source, it);
}

bool is_decimal_digit(char ch) noexcept
{
    static constexpr std::array<bool, 256> lookup = {
        {false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, true,  true,
         true,  true,  true,  true,  true,  true,  true,  true,  false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false}};

    return lookup[static_cast<size_t>(static_cast<unsigned char>(ch))];
}

std::string_view::iterator find_nondecimal_digit_simple_impl(
    std::string_view source)
{
    return std::find_if(source.begin(), source.end(),
                        [](char ch) noexcept { return !is_decimal_digit(ch); });
}
}  // namespace

SCN_PUBLIC std::string_view::iterator find_classic_space_narrow_fast(
    std::string_view source)
{
    return find_classic_impl(
        source, [](char ch) { return is_ascii_space(ch); },
        [](char32_t cp) { return detail::is_cp_space(cp); });
}

SCN_PUBLIC std::string_view::iterator find_classic_nonspace_narrow_fast(
    std::string_view source)
{
    return find_classic_impl(
        source, [](char ch) { return !is_ascii_space(ch); },
        [](char32_t cp) { return !detail::is_cp_space(cp); });
}

SCN_PUBLIC std::string_view::iterator find_nondecimal_digit_narrow_fast(
    std::string_view source)
{
    return find_nondecimal_digit_simple_impl(source);
}
}  // namespace impl

/////////////////////////////////////////////////////////////////
// Scanner implementations
/////////////////////////////////////////////////////////////////

namespace detail {
template <typename T, typename Context>
scan_expected<typename Context::iterator>
scanner_scan_for_builtin_type(T& val, Context& ctx, const format_specs& specs)
{
    if constexpr (!detail::is_type_disabled<T>) {
        return impl::arg_reader<Context>{ctx.range(), specs, {}}(val);
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
}

template <typename Range>
scan_expected<ranges::iterator_t<Range>> internal_skip_classic_whitespace(
    Range r,
    bool allow_exhaustion)
{
    return impl::skip_classic_whitespace(r, allow_exhaustion)
        .transform_error(impl::make_eof_scan_error);
}

#define SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(T, Context)     \
    template SCN_PUBLIC scan_expected<Context::iterator> \
    scanner_scan_for_builtin_type(T&, Context&, const format_specs&);

#define SCN_DEFINE_SCANNER_SCAN_FOR_CTX(Context)                               \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(Context::char_type, Context)              \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(signed char, Context)                     \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(short, Context)                           \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(int, Context)                             \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long, Context)                            \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long long, Context)                       \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned char, Context)                   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned short, Context)                  \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned int, Context)                    \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned long, Context)                   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned long long, Context)              \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(float, Context)                           \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(double, Context)                          \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long double, Context)                     \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::string, Context)                     \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::wstring, Context)                    \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::string_view, Context)                \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::wstring_view, Context)               \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(regex_matches, Context)                   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(wregex_matches, Context)                  \
    template SCN_PUBLIC scan_expected<ranges::iterator_t<Context::range_type>> \
    internal_skip_classic_whitespace(Context::range_type, bool);

SCN_DEFINE_SCANNER_SCAN_FOR_CTX(scan_context)
SCN_DEFINE_SCANNER_SCAN_FOR_CTX(wscan_context)

/////////////////////////////////////////////////////////////////
// File support
/////////////////////////////////////////////////////////////////

using stdio_file_buffer_interface =
    impl::file_buffer_interface<char, impl::stdio_file_interface>;

SCN_PUBLIC scan_cfile_buffer::scan_cfile_buffer(std::FILE* file)
    : base(base::non_contiguous_tag{}), m_file(file)
{
    auto f = impl::stdio_file_interface{m_file};
    stdio_file_buffer_interface::construct(f, this->m_source_error);
}

SCN_PUBLIC scan_cfile_buffer::~scan_cfile_buffer()
{
    auto f = impl::stdio_file_interface{m_file};
    stdio_file_buffer_interface::destruct(f);
}

SCN_PUBLIC bool scan_cfile_buffer::do_fill()
{
    auto f = impl::stdio_file_interface{m_file};
    return stdio_file_buffer_interface::fill(
        f, m_current_view, m_putback_buffer, m_source_error, m_latest);
}

SCN_PUBLIC bool scan_cfile_buffer::do_sync(std::ptrdiff_t position)
{
    auto f = impl::stdio_file_interface{m_file};
    return stdio_file_buffer_interface::sync(f, position, *this, m_current_view,
                                             m_putback_buffer,
                                             true) == position;
}

SCN_PUBLIC scan_file_buffer::scan_file_buffer(scan_file& file)
    : base(scan_file_access::get_handle(file)),
      m_prelude(scan_file_access::get_prelude(file))
{
    this->m_current_view = std::string_view{m_prelude.data(), m_prelude.size()};
}

SCN_PUBLIC scan_file_buffer::~scan_file_buffer() = default;

SCN_PUBLIC bool scan_file_buffer::do_fill()
{
    if (scan_cfile_buffer::do_fill()) {
        m_prelude.clear();
        return true;
    }
    return false;
}

SCN_PUBLIC bool scan_file_buffer::do_sync(std::ptrdiff_t position)
{
    auto f = impl::stdio_file_interface{m_file};
    if (auto i = stdio_file_buffer_interface::sync(
            f, position, *this, m_current_view, m_putback_buffer,
            m_prelude.empty());
        i != position) {
        detail::set_prelude_after_sync(m_prelude, position, i, m_current_view,
                                       m_putback_buffer);
    }
    return true;
}

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wexit-time-destructors")

static std::mutex stdin_mutex{};

#if !SCN_DISABLE_IOSTREAM
static auto& get_stdin_buffer()
{
    static std::optional<scan_istream_buffer> buf{};
    return buf;
}

void prompt_print(const char* msg)
{
    std::cout << msg << std::flush;
}
#else
static auto& get_stdin_buffer()
{
    static std::optional<scan_cfile_buffer> buf{};
    return buf;
}

void prompt_print(const char* msg)
{
    std::printf("%s", msg);
    std::fflush(stdout);
}
#endif

SCN_CLANG_POP

SCN_PUBLIC void stdin_acquire() SCN_THREADSAFETY_NO_ANALYSIS
{
    stdin_mutex.lock();
#if !SCN_DISABLE_IOSTREAM
    get_stdin_buffer().emplace(std::cin);
#else
    get_stdin_buffer().emplace(stdin);
#endif
}

SCN_PUBLIC void stdin_release() SCN_THREADSAFETY_NO_ANALYSIS
{
    get_stdin_buffer().reset();
    stdin_mutex.unlock();
}

SCN_PUBLIC scan_buffer& make_scan_buffer(stdin_tag_t,
                                         make_scan_buffer_tag) noexcept
{
    SCN_ASSERT(get_stdin_buffer(),
               "stdin not locked, stdin_buffer not created");
    return *get_stdin_buffer();
}

}  // namespace detail

/////////////////////////////////////////////////////////////////
// locale implementations
/////////////////////////////////////////////////////////////////

#if !SCN_DISABLE_LOCALE

namespace detail {
template <typename Locale>
locale_ref::locale_ref(const Locale& loc) : m_locale(&loc)
{
    static_assert(std::is_same_v<Locale, std::locale>);
}

template <typename Locale>
Locale locale_ref::get() const
{
    static_assert(std::is_same_v<Locale, std::locale>);
    return m_locale ? *static_cast<const std::locale*>(m_locale)
                    : std::locale{};
}

template SCN_PUBLIC locale_ref::locale_ref(const std::locale&);
template SCN_PUBLIC auto locale_ref::get() const -> std::locale;
}  // namespace detail

#endif

namespace detail {
SCN_PUBLIC scan_error handle_error(scan_error e)
{
    SCN_UNLIKELY_ATTR
    return e;
}
}  // namespace detail

/////////////////////////////////////////////////////////////////
// Floating-point reader implementation
/////////////////////////////////////////////////////////////////

namespace impl::float_conversion {

namespace {

// Detect +0.0, -0.0, +inf, and -inf, despite stuff like -ffast-math

SCN_GCC_COMPAT_PUSH
SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")

template <typename T>
SCN_NODISCARD bool is_float_any_zero(T value)
{
    return value == static_cast<T>(0.0) || value == static_cast<T>(-0.0);
}

template <typename T>
SCN_NODISCARD bool is_float_positive_zero(T value)
{
#if defined(__NO_SIGNED_ZEROS__) && __NO_SIGNED_ZEROS__
    using repr = typename float_traits<T>::value_repr;

    repr expected{};

    repr received{};
    std::memcpy(&received, &value, sizeof(repr));
    received.clear_padding();

    return std::memcmp(&received, &expected, sizeof(repr)) == 0;
#else
    return value == static_cast<T>(0.0);
#endif
}
template <typename T>
SCN_NODISCARD bool is_float_negative_zero(T value)
{
#if defined(__NO_SIGNED_ZEROS__) && __NO_SIGNED_ZEROS__
    using repr = typename float_traits<T>::value_repr;

    repr expected{};
    expected.sign = 1;

    repr received{};
    std::memcpy(&received, &value, sizeof(repr));
    received.clear_padding();

    return std::memcmp(&received, &expected, sizeof(repr)) == 0;
#else
    return value == static_cast<T>(-0.0);
#endif
}

template <typename T>
SCN_NODISCARD bool is_float_positive_infinity(T value)
{
    if constexpr (std::numeric_limits<T>::has_infinity) {
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
        using repr = typename float_traits<T>::value_repr;

        repr expected{};
        expected.apply_exponent((1u << float_traits<T>::exponent_bits) - 1u);
        expected.apply_significand(0u);

        repr received{};
        std::memcpy(&received, &value, sizeof(repr));
        received.clear_padding();

        return std::memcmp(&received, &expected, sizeof(repr)) == 0;
#else
        return value == std::numeric_limits<T>::infinity();
#endif
    }
    else {
        return false;
    }
}
template <typename T>
SCN_NODISCARD bool is_float_negative_infinity(T value)
{
    if constexpr (std::numeric_limits<T>::has_infinity) {
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
        using repr = typename float_traits<T>::value_repr;

        repr expected{};
        expected.apply_exponent((1u << float_traits<T>::exponent_bits) - 1u);
        expected.apply_significand(0);
        expected.sign = 1;

        repr received{};
        std::memcpy(&received, &value, sizeof(repr));
        received.clear_padding();

        return std::memcmp(&received, &expected, sizeof(repr)) == 0;
#else
        return value == -std::numeric_limits<T>::infinity();
#endif
    }
    else {
        return false;
    }
}

SCN_GCC_COMPAT_POP  // -Wfloat-equal

}  // namespace

////////////////////////////////////////////////////////////////////
// strtod-based implementation
// Fallback for all CharT and standard FloatT,
// for FloatT that don't follow IEEE 754
// (IEEE floats can always be handled by convert_custom)
////////////////////////////////////////////////////////////////////

#if !SCN_DISABLE_STRTOD

namespace {

template <typename T>
struct convert_strtod {
    convert_strtod(kind_type kind) : m_kind(kind) {}

    template <typename CharT>
    scan_expected<void> operator()(const CharT* source, T& value)
    {
        CharT* end{};
        errno = 0;

        if constexpr (std::is_same_v<CharT, char>) {
            value = generic_narrow_strtod(source, &end);
        }
        else if constexpr (std::is_same_v<CharT, wchar_t>) {
            value = generic_wide_strtod(source, &end);
        }
        else {
            static_assert(detail::dependent_false<CharT>::value, "");
        }

        const auto saved_errno = errno;
        auto chars_read = end - source;
        SCN_TRY_DISCARD(check_error(chars_read, saved_errno, value));

        return {};
    }

private:
    SCN_NODISCARD static T generic_narrow_strtod(const char* str,
                                                 char** str_end)
    {
#if SCN_HAS_STD_F16 && defined(__HAVE_FLOAT16) && __HAVE_FLOAT16
        if constexpr (std::is_same_v<T, std::float16_t>) {
            set_clocale_classic_guard clocale_guard{LC_NUMERIC};
            return static_cast<std::float16_t>(::strtof16(str, str_end));
        }
#endif

#if SCN_XLOCALE == SCN_XLOCALE_POSIX
        static locale_t cloc = ::newlocale(LC_ALL_MASK, "C", nullptr);
        if constexpr (std::is_same_v<T, float>) {
            return ::strtof_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return ::strtod_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return ::strtold_l(str, str_end, cloc);
        }
#elif SCN_XLOCALE == SCN_XLOCALE_MSVC
        static _locale_t cloc = ::_create_locale(LC_ALL, "C");
        if constexpr (std::is_same_v<T, float>) {
            return ::_strtof_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return ::_strtod_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return ::_strtold_l(str, str_end, cloc);
        }
#else
        set_clocale_classic_guard clocale_guard{LC_NUMERIC};
        if constexpr (std::is_same_v<T, float>) {
            return std::strtof(str, str_end);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return std::strtod(str, str_end);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return std::strtold(str, str_end);
        }
#endif

        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    SCN_NODISCARD static T generic_wide_strtod(const wchar_t* str,
                                               wchar_t** str_end)
    {
#if SCN_HAS_STD_F16 && defined(__HAVE_FLOAT16) && __HAVE_FLOAT16
        if constexpr (std::is_same_v<T, std::float16_t>) {
            set_clocale_classic_guard clocale_guard{LC_NUMERIC};
            return static_cast<std::float16_t>(::wcstof16(str, str_end));
        }
#endif

#if SCN_XLOCALE == SCN_XLOCALE_POSIX
        static locale_t cloc = ::newlocale(LC_ALL_MASK, "C", nullptr);
        if constexpr (std::is_same_v<T, float>) {
            return ::wcstof_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return ::wcstod_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return ::wcstold_l(str, str_end, cloc);
        }
#elif SCN_XLOCALE == SCN_XLOCALE_MSVC
        static _locale_t cloc = ::_create_locale(LC_ALL, "C");
        if constexpr (std::is_same_v<T, float>) {
            return ::_wcstof_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return ::_wcstod_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return ::_wcstold_l(str, str_end, cloc);
        }
#else
        set_clocale_classic_guard clocale_guard{LC_NUMERIC};
        if constexpr (std::is_same_v<T, float>) {
            return std::wcstof(str, str_end);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return std::wcstod(str, str_end);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return std::wcstold(str, str_end);
        }
#endif

        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    SCN_NODISCARD scan_expected<void> check_error(std::ptrdiff_t chars_read,
                                                  int c_errno,
                                                  T value) const
    {
        if (is_float_any_zero(value) && chars_read == 0) {
            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "strtod failed: No conversion");
        }

        if (c_errno == ERANGE && is_float_positive_zero(value)) {
            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::value_positive_underflow,
                "strtod failed: Value too small");
        }
        if (c_errno == ERANGE && is_float_negative_zero(value)) {
            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::value_negative_underflow,
                "strtod failed: Value too small");
        }

        // Infinities are handled separately up the call stack,
        // so getting an infinity here always means overflow
        // (no need to even check errno,
        // which actually isn't even set on all C stdlib implementations)
        if (is_float_positive_infinity(value)) {
            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::value_positive_overflow,
                "strtod failed: Value too large");
        }
        if (is_float_negative_infinity(value)) {
            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::value_negative_overflow,
                "strtod failed: Value too large");
        }

        return {};
    }

    kind_type m_kind{};
};

}  // namespace

template <typename CharT, typename T>
auto convert_strtod_traits::convert(const CharT* source,
                                    T& value,
                                    kind_type kind,
                                    bool& can_fallback) -> scan_expected<void>
{
    if constexpr (enabled<CharT, T>) {
        can_fallback = false;
        return convert_strtod<T>{kind}(source, value);
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
}

#define SCN_DEFINE_STRTOD_CONVERT(T)                               \
    template SCN_PUBLIC auto convert_strtod_traits::convert(       \
        const char*, T&, kind_type, bool&) -> scan_expected<void>; \
    template SCN_PUBLIC auto convert_strtod_traits::convert(       \
        const wchar_t*, T&, kind_type, bool&) -> scan_expected<void>;

#if !SCN_DISABLE_TYPE_FLOAT
SCN_DEFINE_STRTOD_CONVERT(float)
#endif

#if !SCN_DISABLE_TYPE_DOUBLE
SCN_DEFINE_STRTOD_CONVERT(double)
#endif

#if !SCN_DISABLE_TYPE_LONG_DOUBLE
SCN_DEFINE_STRTOD_CONVERT(long double)
#endif

#if SCN_HAS_STD_F16 && !SCN_DISABLE_TYPE_FLOAT16
SCN_DEFINE_STRTOD_CONVERT(std::float16_t)
#endif

#if SCN_HAS_STD_F32 && !SCN_DISABLE_TYPE_FLOAT32
SCN_DEFINE_STRTOD_CONVERT(std::float32_t)
#endif

#if SCN_HAS_STD_F64 && !SCN_DISABLE_TYPE_FLOAT64
SCN_DEFINE_STRTOD_CONVERT(std::float64_t)
#endif

#if SCN_HAS_STD_F128 && !SCN_DISABLE_TYPE_FLOAT128
SCN_DEFINE_STRTOD_CONVERT(std::float128_t)
#endif

#if SCN_HAS_STD_BF16 && !SCN_DISABLE_TYPE_BFLOAT16
SCN_DEFINE_STRTOD_CONVERT(std::bfloat16_t)
#endif

#undef SCN_DEFINE_STRTOD_CONVERT

#endif  // !SCN_DISABLE_STRTOD

////////////////////////////////////////////////////////////////////
// Custom implementation using Simple Decimal Conversion
// Fallback for all CharT and IEEE FloatT
////////////////////////////////////////////////////////////////////

namespace {

template <typename CharT>
float_parts<CharT> parse_float_parts(
    std::basic_string_view<CharT> source,
    const typename source_reader<CharT>::state_type& state)
{
    float_parts<CharT> parts{};

    parts.before_radix_point = source.substr(0, state.whole_part_digits_count);
    auto idx = state.whole_part_digits_count;

    if (state.has_radix_point) {
        ++idx;  // skip '.'
        parts.after_radix_point =
            source.substr(idx, state.fractional_part_digits_count);
        idx += state.fractional_part_digits_count;
    }

    if (state.exponent_digits_count > 0) {
        ++idx;  // skip 'e'/'p'
        if (state.has_exponent_sign) {
            parts.exponent_positive = source[idx] == CharT{'+'};
            ++idx;
        }
        parts.exponent = source.substr(idx, state.exponent_digits_count);
        idx += state.exponent_digits_count;
    }

    SCN_ENSURE(idx == source.size());
    parts.end = source.end();
    return parts;
}

struct convert_custom_dec {
    convert_custom_dec(kind_type kind)
    {
        SCN_EXPECT(kind == kind_type::fixed || kind == kind_type::scientific);
    }

    template <typename CharT, typename T>
    scan_expected<void> operator()(const float_parts<CharT>& parts, T& value)
    {
        float_rounding_guard guard{};

        high_precision_decimal hpd{};
        if (auto res = high_precision_decimal::populate(hpd, parts); !res) {
            return unexpected(res.error());
        }
        if (auto res = simple_decimal_conversion(value, hpd); !res) {
            return unexpected(res.error());
        }
        return {};
    }
};

struct convert_custom_hex {
    convert_custom_hex(kind_type kind)
    {
        SCN_EXPECT(kind == kind_type::hex_with_prefix ||
                   kind == kind_type::hex_without_prefix);
    }

    template <typename CharT, typename T>
    scan_expected<void> operator()(const float_parts<CharT>& parts, T& value)
    {
        using significand_int_type =
            typename float_traits<T>::significand_int_type;
        // The entire significand, including a leading "1.",
        // must be storable in `significand_int_type`
        static_assert(float_traits<T>::fraction_bits + 1 <=
                          sizeof(significand_int_type) * 8,
                      "");

        // Parse all hex digits (before and after the radix point) into
        // hex_sig, an integer in significand_int_type, maintaining the
        // invariant:
        //   actual_value ≈ hex_sig * 2^exp2
        // with sticky capturing any non-zero bits dropped below hex_sig.
        //
        // When hex_sig is full (top 4 bits non-zero), further digits are less
        // significant: they go to sticky, and exp2 is incremented by 4 to
        // keep the invariant.

        static constexpr auto total_bits =
            static_cast<int>(sizeof(significand_int_type) * 8);
        significand_int_type significand{0u};
        int dropped_significand_digits{};
        bool sticky = false;

        auto add_digit = [&](uint8_t d) {
            if ((significand >> static_cast<unsigned>(total_bits - 4)) !=
                significand_int_type{0u}) {
                // significand is full, this digit is a dropped lower-order bit
                sticky |= (d != 0);
                ++dropped_significand_digits;
            }
            else {
                significand <<= significand_int_type{4u};
                significand |= significand_int_type{d};
            }
        };
        for (CharT c : parts.before_radix_point) {
            add_digit(char_to_int(c));
        }
        for (CharT c : parts.after_radix_point) {
            add_digit(char_to_int(c));
        }

        // Handle zero
        if (significand == significand_int_type{0u}) {
            value = static_cast<T>(0.0);
            return {};
        }

        // Handle explicit binary exponent
        int binary_exponent = 0;
        if (!parts.exponent.empty()) {
            auto exp_opt =
                parse_exponent(parts.exponent, parts.exponent_positive);
            if (!exp_opt) {
                return detail::unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Invalid hexfloat binary exponent");
            }
            binary_exponent = *exp_opt;
        }

        auto exp2 = binary_exponent -
                    4 * static_cast<int>(parts.after_radix_point.size()) +
                    4 * dropped_significand_digits;

        // Normalize value to 1.fraction * 2^exponent (for normal values)
        //
        // Find bit-width of significand: MSB is at bit position N-1.
        // value
        //   = hex_sig * 2^exp2
        //   = (1 + frac/2^(N - 1)) * 2^(N - 1 + exp2)
        const auto N = total_bits -
                       static_cast<unsigned>(count_leading_zeroes(significand));

        static constexpr int exp_bias =
            (1 << (float_traits<T>::exponent_bits - 1)) - 1;
        static constexpr int fraction_bits = float_traits<T>::fraction_bits;
        static constexpr int max_biased_exp =
            (1 << float_traits<T>::exponent_bits) - 1;

        auto biased_exp = static_cast<int>(N) - 1 + exp2 + exp_bias;

        // Check for overflow (value too large → infinity)
        if (biased_exp >= max_biased_exp) {
            return detail::unexpected_scan_error(
                scan_error::value_positive_overflow,
                "Hexfloat value too large");
        }

        // Step 5: Truncate and/or round the fraction to fit in the value.
        //
        // Right-shift `val` by `n` bits with round-to-nearest-even.
        // Any bits shifted away are OR-ed into the outer `sticky` flag.
        auto right_shift_round = [&](significand_int_type val, unsigned n) {
            SCN_EXPECT(n >= 1 && n < total_bits);
            const auto round_bit =
                static_cast<bool>((val >> (n - 1)) & significand_int_type{1u});
            if (n >= 2) {
                const auto sticky_mask = (significand_int_type{1u} << (n - 1)) -
                                         significand_int_type{1u};
                sticky |= ((val & sticky_mask) != 0);
            }
            auto result = val >> n;
            if (round_bit && (sticky || (result & significand_int_type{1u}))) {
                result += significand_int_type{1u};
            }
            return static_cast<significand_int_type>(result);
        };

        if (biased_exp > 0) {
            // Normal: strip the implicit leading 1, then shift the remaining
            // N-1 fraction bits to fill the fraction_bits field.
            significand ^= significand_int_type{1u} << (N - 1);

            const int shift = static_cast<int>(N) - 1 - fraction_bits;
            if (shift > 0) {
                significand = right_shift_round(significand,
                                                static_cast<unsigned>(shift));
                // If rounding carried out of the fraction field, adjust
                if (significand == (significand_int_type{1u}
                                    << static_cast<unsigned>(fraction_bits))) {
                    significand = significand_int_type{0u};
                    if (++biased_exp >= max_biased_exp) {
                        return detail::unexpected_scan_error(
                            scan_error::value_positive_overflow,
                            "Hexfloat value too large after rounding");
                    }
                }
            }
            else {
                significand <<= static_cast<unsigned>(-shift);
            }
        }
        else {
            // Subnormal (biased_exp <= 0): shift the full significand
            // (including the leading 1) to fill the fraction_bits field.
            // significand_stored = significand * 2^(biased_exp + fraction_bits
            // - N)
            const int e_shift =
                biased_exp + fraction_bits - static_cast<int>(N);
            biased_exp = 0;  // subnormals are stored with biased_exp = 0

            if (e_shift < 0) {
                const auto shift = static_cast<unsigned>(-e_shift);
                if (shift >= total_bits) {
                    return detail::unexpected_scan_error(
                        scan_error::value_positive_underflow,
                        "Hexfloat value too small");
                }
                significand = right_shift_round(significand, shift);
                // If rounding carried up to the minimum normal, adjust
                if (significand == (significand_int_type{1u}
                                    << static_cast<unsigned>(fraction_bits))) {
                    significand = significand_int_type{0u};
                    biased_exp = 1;
                }
            }
            else {
                significand <<= static_cast<unsigned>(e_shift);
            }

            if (significand == significand_int_type{0u} && biased_exp == 0) {
                return detail::unexpected_scan_error(
                    scan_error::value_positive_underflow,
                    "Hexfloat value rounds to zero");
            }
        }

        typename float_traits<T>::value_repr repr{};
        SCN_ENSURE(biased_exp >= 0);
        repr.apply_exponent(biased_exp);
        repr.apply_significand(significand);
        std::memcpy(&value, &repr, sizeof(repr));

        return {};
    }

private:
    template <typename CharT>
    static std::optional<int> parse_exponent(
        std::basic_string_view<CharT> input,
        bool is_positive)
    {
        if (input.empty()) {
            return 0;
        }

        int value{};
        auto r = parse_integer_value(
            input, value,
            is_positive ? sign_type::plus_sign : sign_type::minus_sign, 10);
        if (!r || *r != input.end()) {
            return std::nullopt;
        }
        SCN_ENSURE(r.value() == input.end());
        return value;
    }
};

}  // namespace

template <typename CharT, typename T>
scan_expected<void> convert_custom_traits::convert(
    std::basic_string_view<CharT> source,
    T& value,
    kind_type kind,
    const typename source_reader<CharT>::state_type& state,
    bool& can_fallback)
{
    if constexpr (enabled<CharT, T>) {
        can_fallback = false;

        if (kind == kind_type::hex_with_prefix) {
            auto parts = parse_float_parts(source.substr(2), state);
            return convert_custom_hex{kind}(parts, value);
        }

        auto parts = parse_float_parts(source, state);
        if (kind == kind_type::hex_without_prefix) {
            return convert_custom_hex{kind}(parts, value);
        }
        return convert_custom_dec{kind}(parts, value);
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
}

#define SCN_DEFINE_CUSTOM_CONVERT(T)                                           \
    template SCN_PUBLIC auto convert_custom_traits::convert(                   \
        std::string_view, T&, kind_type,                                       \
        const source_reader<char>::state_type&, bool&) -> scan_expected<void>; \
    template SCN_PUBLIC auto convert_custom_traits::convert(                   \
        std::wstring_view, T&, kind_type,                                      \
        const source_reader<wchar_t>::state_type&, bool&)                      \
        -> scan_expected<void>;

#if !SCN_DISABLE_TYPE_FLOAT
SCN_DEFINE_CUSTOM_CONVERT(float)
#endif

#if !SCN_DISABLE_TYPE_DOUBLE
SCN_DEFINE_CUSTOM_CONVERT(double)
#endif

#if !SCN_DISABLE_TYPE_LONG_DOUBLE
SCN_DEFINE_CUSTOM_CONVERT(long double)
#endif

#if SCN_HAS_STD_F16 && !SCN_DISABLE_TYPE_FLOAT16
SCN_DEFINE_CUSTOM_CONVERT(std::float16_t)
#endif

#if SCN_HAS_STD_F32 && !SCN_DISABLE_TYPE_FLOAT32
SCN_DEFINE_CUSTOM_CONVERT(std::float32_t)
#endif

#if SCN_HAS_STD_F64 && !SCN_DISABLE_TYPE_FLOAT64
SCN_DEFINE_CUSTOM_CONVERT(std::float64_t)
#endif

#if SCN_HAS_STD_F128 && !SCN_DISABLE_TYPE_FLOAT128
SCN_DEFINE_CUSTOM_CONVERT(std::float128_t)
#endif

#if SCN_HAS_STD_BF16 && !SCN_DISABLE_TYPE_BFLOAT16
SCN_DEFINE_CUSTOM_CONVERT(std::bfloat16_t)
#endif

#undef SCN_DEFINE_CUSTOM_CONVERT

////////////////////////////////////////////////////////////////////
// std::from_chars-based implementation
// Only for CharT=char, if available
////////////////////////////////////////////////////////////////////

#if SCN_HAS_FLOAT_CHARCONV && !SCN_DISABLE_FROM_CHARS

namespace {
struct convert_from_chars {
    convert_from_chars(kind_type kind, unsigned options)
        : m_kind(kind), m_options(options)
    {
    }

    template <typename T>
    auto operator()(std::string_view source, T& value, bool& can_fallback)
        -> scan_expected<std::string_view::iterator>
    {
        std::chars_format flags{};
        std::size_t offset{0};
        if (m_kind == kind_type::hex_with_prefix) {
            offset = 2;
            flags = std::chars_format::hex;
        }
        else if (m_kind == kind_type::hex_without_prefix) {
            flags = std::chars_format::hex;
        }
        else {
            if (m_options & allow_fixed) {
                flags |= std::chars_format::fixed;
            }
            if (m_options & allow_scientific) {
                flags |= std::chars_format::scientific;
            }
        }

        const auto result =
            std::from_chars(source.data() + offset,
                            source.data() + source.size(), value, flags);

        if (SCN_UNLIKELY(result.ec == std::errc::invalid_argument)) {
            can_fallback = false;
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "std::from_chars: invalid_argument");
        }
        if (result.ec == std::errc::result_out_of_range) {
            // std::from_chars doesn't give us a way to distinguish between
            // different kinds of over-/underflow:
            // per the standard, `value` is unmodified.
            // Fall back to try to determine what's up.

            can_fallback = true;
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "std::from_chars: Unknown result_out_of_range error");
        }

        return detail::make_string_view_iterator_from_pointer(source,
                                                              result.ptr);
    }

private:
    kind_type m_kind{};
    unsigned m_options{};
};

}  // namespace

template <typename T>
scan_expected<std::string_view::iterator> convert_from_chars_traits::convert(
    std::string_view source,
    T& value,
    kind_type kind,
    unsigned options,
    bool& can_fallback)
{
    if constexpr (enabled<char, T>) {
        return convert_from_chars{kind, options}(source, value, can_fallback);
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
}

#define SCN_DEFINE_FROM_CHARS_CONVERT(T)                         \
    template SCN_PUBLIC auto convert_from_chars_traits::convert( \
        std::string_view, T&, kind_type, unsigned, bool&)        \
        -> scan_expected<std::string_view::iterator>;

#if !SCN_DISABLE_TYPE_FLOAT
SCN_DEFINE_FROM_CHARS_CONVERT(float)
#endif

#if !SCN_DISABLE_TYPE_DOUBLE
SCN_DEFINE_FROM_CHARS_CONVERT(double)
#endif

#if !SCN_DISABLE_TYPE_LONG_DOUBLE
SCN_DEFINE_FROM_CHARS_CONVERT(long double)
#endif

#if SCN_HAS_STD_F16 && !SCN_DISABLE_TYPE_FLOAT16
SCN_DEFINE_FROM_CHARS_CONVERT(std::float16_t)
#endif

#if SCN_HAS_STD_F32 && !SCN_DISABLE_TYPE_FLOAT32
SCN_DEFINE_FROM_CHARS_CONVERT(std::float32_t)
#endif

#if SCN_HAS_STD_F64 && !SCN_DISABLE_TYPE_FLOAT64
SCN_DEFINE_FROM_CHARS_CONVERT(std::float64_t)
#endif

#if SCN_HAS_STD_F128 && !SCN_DISABLE_TYPE_FLOAT128
SCN_DEFINE_FROM_CHARS_CONVERT(std::float128_t)
#endif

#if SCN_HAS_STD_BF16 && !SCN_DISABLE_TYPE_BFLOAT16
SCN_DEFINE_FROM_CHARS_CONVERT(std::bfloat16_t)
#endif

#undef SCN_DEFINE_FROM_CHARS_CONVERT

#endif  // SCN_HAS_FLOAT_CHARCONV && !SCN_DISABLE_FROM_CHARS

////////////////////////////////////////////////////////////////////
// fast_float-based implementation
// Only for FloatT=(float OR double OR extended-float)
////////////////////////////////////////////////////////////////////

#if !SCN_DISABLE_FAST_FLOAT

namespace {
struct convert_fast_float {
    convert_fast_float(kind_type kind, unsigned options) : m_options(options)
    {
        SCN_EXPECT(kind == kind_type::decimal || kind == kind_type::fixed ||
                   kind == kind_type::scientific);
    }

    template <typename CharT, typename T>
    auto operator()(std::basic_string_view<CharT> source,
                    T& value,
                    bool& can_fallback)
        -> scan_expected<typename std::basic_string_view<CharT>::iterator>
    {
        auto flags =
            static_cast<std::uint64_t>(fast_float::chars_format::no_infnan);
        if (m_options & allow_fixed) {
            flags |=
                static_cast<std::uint64_t>(fast_float::chars_format::fixed);
        }
        if (m_options & allow_scientific) {
            flags |= static_cast<std::uint64_t>(
                fast_float::chars_format::scientific);
        }

        const auto view = cast_source(source);
        const auto result = fast_float::from_chars(
            view.data(), view.data() + view.size(), value,
            static_cast<fast_float::chars_format>(flags));

        if (SCN_UNLIKELY(result.ec == std::errc::invalid_argument)) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "fast_float: invalid_argument");
        }
        if (SCN_UNLIKELY(is_float_positive_infinity(value))) {
            // Infinity, but input couldn't've been infinity
            //  -> definitely overflow
            can_fallback = false;
            return detail::unexpected_scan_error(
                scan_error::value_positive_overflow,
                "fast_float: value too large");
        }
        if (SCN_UNLIKELY(result.ec == std::errc::result_out_of_range)) {
            if (SCN_UNLIKELY(is_float_any_zero(value))) {
                can_fallback = false;
                return detail::unexpected_scan_error(
                    scan_error::value_positive_underflow,
                    "fast_float: value too small");
            }
            can_fallback = true;
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "fast_float: Unknown result_out_of_range error");
        }

        return detail::make_string_view_iterator_from_pointer(
            source, reinterpret_cast<const CharT*>(result.ptr));
    }

private:
    template <typename CharT>
    static auto cast_source(std::basic_string_view<CharT> source)
    {
        if constexpr (sizeof(CharT) == 1) {
            return source;
        }
        else if constexpr (sizeof(CharT) == 2) {
            return std::u16string_view{
                reinterpret_cast<const char16_t*>(source.data()),
                source.size()};
        }
        else {
            return std::u32string_view{
                reinterpret_cast<const char32_t*>(source.data()),
                source.size()};
        }
    }

    unsigned m_options{};
};

}  // namespace

template <typename CharT, typename T>
scan_expected<typename std::basic_string_view<CharT>::iterator>
convert_fast_float_traits::convert(std::basic_string_view<CharT> source,
                                   T& value,
                                   kind_type kind,
                                   unsigned options,
                                   bool& can_fallback)
{
    if constexpr (enabled<CharT, T>) {
        return convert_fast_float{kind, options}(source, value, can_fallback);
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
}

#define SCN_DEFINE_FAST_FLOAT_CONVERT(T)                         \
    template SCN_PUBLIC auto convert_fast_float_traits::convert( \
        std::string_view, T&, kind_type, unsigned, bool&)        \
        -> scan_expected<std::string_view::iterator>;            \
    template SCN_PUBLIC auto convert_fast_float_traits::convert( \
        std::wstring_view, T&, kind_type, unsigned, bool&)       \
        -> scan_expected<std::wstring_view::iterator>;

#if !SCN_DISABLE_TYPE_FLOAT
SCN_DEFINE_FAST_FLOAT_CONVERT(float)
#endif

#if !SCN_DISABLE_TYPE_DOUBLE
SCN_DEFINE_FAST_FLOAT_CONVERT(double)
#endif

#if !SCN_DISABLE_TYPE_LONG_DOUBLE
SCN_DEFINE_FAST_FLOAT_CONVERT(long double)
#endif

#if SCN_HAS_STD_F16 && !SCN_DISABLE_TYPE_FLOAT16
SCN_DEFINE_FAST_FLOAT_CONVERT(std::float16_t)
#endif

#if SCN_HAS_STD_F32 && !SCN_DISABLE_TYPE_FLOAT32
SCN_DEFINE_FAST_FLOAT_CONVERT(std::float32_t)
#endif

#if SCN_HAS_STD_F64 && !SCN_DISABLE_TYPE_FLOAT64
SCN_DEFINE_FAST_FLOAT_CONVERT(std::float64_t)
#endif

#if SCN_HAS_STD_F128 && !SCN_DISABLE_TYPE_FLOAT128
SCN_DEFINE_FAST_FLOAT_CONVERT(std::float128_t)
#endif

#if SCN_HAS_STD_BF16 && !SCN_DISABLE_TYPE_BFLOAT16
SCN_DEFINE_FAST_FLOAT_CONVERT(std::bfloat16_t)
#endif

#undef SCN_DEFINE_FAST_FLOAT_CONVERT

#endif  // !SCN_DISABLE_FAST_FLOAT

}  // namespace impl::float_conversion

/////////////////////////////////////////////////////////////////
// Integer reader implementation
/////////////////////////////////////////////////////////////////

namespace impl {

namespace {
uint64_t get_eight_digits_word(const char* input)
{
    uint64_t val{};
    std::memcpy(&val, input, sizeof(uint64_t));
    if constexpr (SCN_IS_BIG_ENDIAN) {
        val = byteswap(val);
    }
    return val;
}

constexpr uint32_t parse_eight_decimal_digits_unrolled_fast(uint64_t word)
{
    constexpr uint64_t mask = 0x000000FF000000FF;
    constexpr uint64_t mul1 = 0x000F424000000064;  // 100 + (1000000ULL << 32)
    constexpr uint64_t mul2 = 0x0000271000000001;  // 1 + (10000ULL << 32)
    word -= 0x3030303030303030;
    word = (word * 10) + (word >> 8);  // val = (val * 2561) >> 8;
    word = (((word & mask) * mul1) + (((word >> 16) & mask) * mul2)) >> 32;
    return static_cast<uint32_t>(word);
}

constexpr bool is_word_made_of_eight_decimal_digits_fast(uint64_t word)
{
    return !((((word + 0x4646464646464646) | (word - 0x3030303030303030)) &
              0x8080808080808080));
}

void loop_parse_if_eight_decimal_digits(const char*& p,
                                        const char* const end,
                                        uint64_t& val)
{
    while (
        std::distance(p, end) >= 8 &&
        is_word_made_of_eight_decimal_digits_fast(get_eight_digits_word(p))) {
        val = val * 100'000'000 + parse_eight_decimal_digits_unrolled_fast(
                                      get_eight_digits_word(p));
        p += 8;
    }
}

const char* parse_decimal_integer_fast_impl(const char* begin,
                                            const char* const end,
                                            uint64_t& val)
{
    loop_parse_if_eight_decimal_digits(begin, end, val);

    while (begin != end) {
        const auto digit = char_to_int(*begin);
        if (digit >= 10) {
            break;
        }
        val = 10ull * val + static_cast<uint64_t>(digit);
        ++begin;
    }

    return begin;
}

constexpr size_t maxdigits_u64_table[] = {
    0,  0,  64, 41, 32, 28, 25, 23, 22, 21, 20, 19, 18, 18, 17, 17, 16, 16, 16,
    16, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13};

SCN_FORCE_INLINE constexpr size_t maxdigits_u64(int base)
{
    SCN_EXPECT(base >= 2 && base <= 36);
    return maxdigits_u64_table[static_cast<size_t>(base)];
}

constexpr uint64_t min_safe_u64_table[] = {0,
                                           0,
                                           9223372036854775808ull,
                                           12157665459056928801ull,
                                           4611686018427387904,
                                           7450580596923828125,
                                           4738381338321616896,
                                           3909821048582988049,
                                           9223372036854775808ull,
                                           12157665459056928801ull,
                                           10000000000000000000ull,
                                           5559917313492231481,
                                           2218611106740436992,
                                           8650415919381337933,
                                           2177953337809371136,
                                           6568408355712890625,
                                           1152921504606846976,
                                           2862423051509815793,
                                           6746640616477458432,
                                           15181127029874798299ull,
                                           1638400000000000000,
                                           3243919932521508681,
                                           6221821273427820544,
                                           11592836324538749809ull,
                                           876488338465357824,
                                           1490116119384765625,
                                           2481152873203736576,
                                           4052555153018976267,
                                           6502111422497947648,
                                           10260628712958602189ull,
                                           15943230000000000000ull,
                                           787662783788549761,
                                           1152921504606846976,
                                           1667889514952984961,
                                           2386420683693101056,
                                           3379220508056640625,
                                           4738381338321616896};

SCN_FORCE_INLINE constexpr uint64_t min_safe_u64(int base)
{
    SCN_EXPECT(base >= 2 && base <= 36);
    return min_safe_u64_table[static_cast<size_t>(base)];
}

template <typename T>
constexpr bool check_integer_overflow(uint64_t val,
                                      size_t digits_count,
                                      int base,
                                      bool is_negative)
{
    static_assert(sizeof(T) <= sizeof(uint64_t));
    SCN_UNUSED(is_negative);  // not really

    auto max_digits = maxdigits_u64(base);
    if (digits_count > max_digits) {
        return true;
    }
    if (digits_count == max_digits && val < min_safe_u64(base)) {
        return true;
    }
    if constexpr (!std::is_same_v<T, uint64_t>) {
        if (val > static_cast<uint64_t>(std::numeric_limits<T>::max()) +
                      static_cast<uint64_t>(is_negative)) {
            SCN_UNLIKELY_ATTR
            return true;
        }
    }

    return false;
}

template <typename T, typename Acc>
constexpr T store_result(Acc acc, bool is_negative)
{
    if constexpr (std::is_signed_v<T>) {
        if (is_negative) {
            SCN_MSVC_PUSH
            SCN_MSVC_IGNORE(4146)
            return static_cast<T>(
                -std::numeric_limits<T>::max() -
                static_cast<T>(acc - std::numeric_limits<T>::max()));
            SCN_MSVC_POP
        }
    }

    return static_cast<T>(acc);
}

template <typename T>
auto parse_decimal_integer_fast(std::string_view input,
                                T& val,
                                bool is_negative) -> scan_expected<const char*>
{
    static_assert(sizeof(T) <= sizeof(std::uint64_t));

    uint64_t u64val{};
    auto ptr = parse_decimal_integer_fast_impl(
        input.data(), input.data() + input.size(), u64val);

    auto digits_count = static_cast<size_t>(ptr - input.data());
    if (SCN_UNLIKELY(
            check_integer_overflow<T>(u64val, digits_count, 10, is_negative))) {
        return detail::unexpected_scan_error(
            is_negative ? scan_error::value_negative_overflow
                        : scan_error::value_positive_overflow,
            "Integer overflow");
    }

    val = store_result<T>(u64val, is_negative);
    return ptr;
}

template <typename CharT, typename T>
auto parse_regular_integer(std::basic_string_view<CharT> input,
                           T& val,
                           int base,
                           bool is_negative) -> scan_expected<const CharT*>
{
    uint64_t u64val{};
    const CharT* begin = input.data();
    const CharT* const end = input.data() + input.size();

    while (begin != end) {
        const auto digit = char_to_int(*begin);
        if (digit >= base) {
            break;
        }
        u64val =
            static_cast<uint64_t>(base) * u64val + static_cast<uint64_t>(digit);
        ++begin;
    }

    auto digits_count = static_cast<size_t>(begin - input.data());
    if (SCN_UNLIKELY(check_integer_overflow<T>(u64val, digits_count, base,
                                               is_negative))) {
        return detail::unexpected_scan_error(
            is_negative ? scan_error::value_negative_overflow
                        : scan_error::value_positive_overflow,
            "Integer overflow");
    }

    val = store_result<T>(u64val, is_negative);
    return begin;
}

// int128 is parsed with a different algorithm,
// going over the input char-by-char and checking for overflow on each step.
// It's slower, but simpler,
// and we don't have to build separate lookup tables for it.
template <typename SignedT, typename UnsignedT, typename CharT, typename T>
[[maybe_unused]]
auto parse_int128(std::basic_string_view<CharT> input,
                  T& val,
                  int base,
                  bool is_negative) -> scan_expected<const CharT*>
{
    constexpr UnsignedT uint_max = std::numeric_limits<UnsignedT>::max();
    [[maybe_unused]] constexpr UnsignedT int_max = uint_max >> UnsignedT{1u};
    [[maybe_unused]] constexpr UnsignedT abs_int_min = int_max + UnsignedT{1u};

    SCN_GCC_COMPAT_PUSH
    SCN_GCC_COMPAT_IGNORE("-Wsign-conversion")
    auto const [limit_val,
                max_digit] = [&]() -> std::pair<UnsignedT, UnsignedT> {
        if constexpr (std::is_same_v<T, SignedT>) {
            if (is_negative) {
                return {abs_int_min / base, abs_int_min % static_cast<T>(base)};
            }
            return {int_max / base, int_max % static_cast<T>(base)};
        }
        else if constexpr (!std::is_same_v<T, uint128_polyfill>) {
            return {uint_max / base, uint_max % static_cast<T>(base)};
        }
        else {
            SCN_EXPECT(base == 10 || base == 2 || base == 8 || base == 16);
            if (base == 2) {
                return {uint_max >> T{1u}, uint_max & T{1u}};
            }
            if (base == 8) {
                return {uint_max >> T{3u}, uint_max & T{7u}};
            }
            if (base == 16) {
                return {uint_max >> T{4u}, uint_max & T{15u}};
            }
            // Can't be bothered to figure out the math for base-10,
            // using base-16, this is fine because this will only ever be used
            // for filling NaN payloads that don't use the full 128 bits anyway
            return {uint_max >> T{4u}, uint_max & T{15u}};
        }
    }();

    const CharT* begin = input.data();
    const CharT* const end = input.data() + input.size();
    UnsignedT acc{};

    while (begin != end) {
        const auto digit = char_to_int(*begin);
        if (SCN_UNLIKELY(digit >= base)) {
            break;
        }
        if (acc < limit_val || (acc == limit_val && digit <= max_digit)) {
            SCN_LIKELY_ATTR
            if constexpr (std::is_same_v<T, uint128_polyfill>) {
                if (base == 2) {
                    acc = (acc << T{1u}) + static_cast<T>(digit);
                }
                else if (base == 8) {
                    acc = (acc << T{3u}) + static_cast<T>(digit);
                }
                else if (base == 16) {
                    acc = (acc << T{4u}) + static_cast<T>(digit);
                }
                else {
                    acc =
                        (acc << T{3u}) + (acc << T{1u}) + static_cast<T>(digit);
                }
            }
            else {
                acc = acc * static_cast<T>(base) + static_cast<T>(digit);
            }
        }
        else {
            return detail::unexpected_scan_error(
                is_negative ? scan_error::value_negative_overflow
                            : scan_error::value_positive_overflow,
                "Integer overflow");
        }
        ++begin;
    }
    SCN_GCC_COMPAT_POP

    val = store_result<T>(acc, is_negative);
    return begin;
}

#if SCN_HAS_INT128
template <typename CharT>
auto parse_regular_integer(std::basic_string_view<CharT> input,
                           int128& val,
                           int base,
                           bool is_negative) -> scan_expected<const CharT*>
{
    return parse_int128<int128, uint128>(input, val, base, is_negative);
}

template <typename CharT>
auto parse_regular_integer(std::basic_string_view<CharT> input,
                           uint128& val,
                           int base,
                           bool is_negative) -> scan_expected<const CharT*>
{
    SCN_EXPECT(!is_negative);
    return parse_int128<int128, uint128>(input, val, base, is_negative);
}
#endif

template <typename CharT>
[[maybe_unused]] auto parse_regular_integer(std::basic_string_view<CharT> input,
                                            uint128_polyfill& val,
                                            int base,
                                            bool is_negative)
    -> scan_expected<const CharT*>
{
    SCN_EXPECT(!is_negative);
    return parse_int128<void, uint128_polyfill>(input, val, base, is_negative);
}

}  // namespace

template <typename CharT, typename T>
auto parse_integer_value(std::basic_string_view<CharT> source,
                         T& value,
                         sign_type sign,
                         int base)
    -> scan_expected<typename std::basic_string_view<CharT>::iterator>
{
    SCN_EXPECT(!source.empty());
    SCN_EXPECT(std::is_signed_v<T> || sign == sign_type::plus_sign);
    SCN_EXPECT(sign != sign_type::default_sign);
    SCN_EXPECT(base > 0);

    if (char_to_int(source[0]) >= base) {
        SCN_UNLIKELY_ATTR
        return detail::unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Invalid integer value");
    }

    // Skip leading zeroes
    auto start = source.data();
    const auto end = source.data() + source.size();
    {
        for (; start != end; ++start) {
            if (*start != CharT{'0'}) {
                break;
            }
        }
        if (SCN_UNLIKELY(start == end || char_to_int(*start) >= base)) {
            value = T{0};
            return ranges::next(source.begin(),
                                ranges::distance(source.data(), start));
        }
    }

    if constexpr (std::is_same_v<CharT, char> &&
                  sizeof(T) <= sizeof(std::uint64_t)) {
        if (base == 10) {
            SCN_TRY(ptr, parse_decimal_integer_fast(
                             detail::make_string_view_from_pointers(start, end),
                             value, sign == sign_type::minus_sign));
            return ranges::next(source.begin(),
                                ranges::distance(source.data(), ptr));
        }
    }

    SCN_TRY(ptr, parse_regular_integer(
                     detail::make_string_view_from_pointers(start, end), value,
                     base, sign == sign_type::minus_sign));
    return ranges::next(source.begin(), ranges::distance(source.data(), ptr));
}

template <typename T>
void parse_integer_value_exhaustive_valid(std::string_view source, T& value)
{
    SCN_EXPECT(!source.empty());

    bool negative_sign = false;
    if constexpr (std::is_signed_v<T>) {
        if (source.front() == '-') {
            source = source.substr(1);
            negative_sign = true;
        }
    }
    SCN_EXPECT(!source.empty());
    SCN_EXPECT(char_to_int(source.front()) < 10);

    const char* p = source.data();
    const char* const end = source.data() + source.size();

    uint64_t u64val{};
    while (std::distance(p, end) >= 8) {
        SCN_EXPECT(is_word_made_of_eight_decimal_digits_fast(
            get_eight_digits_word(p)));
        u64val =
            u64val * 100'000'000 +
            parse_eight_decimal_digits_unrolled_fast(get_eight_digits_word(p));
        p += 8;
    }

    while (p != end) {
        const auto digit = char_to_int(*p);
        SCN_EXPECT(digit < 10);
        u64val = 10ull * u64val + static_cast<uint64_t>(digit);
        ++p;
    }
    SCN_EXPECT(p == end);

    {
        auto digits_count = static_cast<size_t>(p - source.data());
        SCN_UNUSED(digits_count);
        SCN_EXPECT(check_integer_overflow<T>(u64val, digits_count, 10,
                                             negative_sign) == false);
    }

    value = store_result<T>(u64val, negative_sign);
}

#define SCN_DEFINE_INTEGER_READER_TEMPLATE(CharT, IntT)                    \
    template SCN_PUBLIC auto parse_integer_value(                          \
        std::basic_string_view<CharT> source, IntT& value, sign_type sign, \
        int base)                                                          \
        -> scan_expected<typename std::basic_string_view<CharT>::iterator>;

#if !SCN_DISABLE_TYPE_SCHAR
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, signed char)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, signed char)
template SCN_PUBLIC void parse_integer_value_exhaustive_valid(std::string_view,
                                                              signed char&);
#endif
#if !SCN_DISABLE_TYPE_SHORT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, short)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, short)
template SCN_PUBLIC void parse_integer_value_exhaustive_valid(std::string_view,
                                                              short&);
#endif
#if !SCN_DISABLE_TYPE_INT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, int)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, int)
template SCN_PUBLIC void parse_integer_value_exhaustive_valid(std::string_view,
                                                              int&);
#endif
#if !SCN_DISABLE_TYPE_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, long)
template SCN_PUBLIC void parse_integer_value_exhaustive_valid(std::string_view,
                                                              long&);
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, long long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, long long)
template SCN_PUBLIC void parse_integer_value_exhaustive_valid(std::string_view,
                                                              long long&);
#endif
#if !SCN_DISABLE_TYPE_UCHAR
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned char)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned char)
template SCN_PUBLIC void parse_integer_value_exhaustive_valid(std::string_view,
                                                              unsigned char&);
#endif
#if !SCN_DISABLE_TYPE_USHORT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned short)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned short)
template SCN_PUBLIC void parse_integer_value_exhaustive_valid(std::string_view,
                                                              unsigned short&);
#endif
#if !SCN_DISABLE_TYPE_UINT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned int)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned int)
template SCN_PUBLIC void parse_integer_value_exhaustive_valid(std::string_view,
                                                              unsigned int&);
#endif
#if !SCN_DISABLE_TYPE_ULONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long)
template SCN_PUBLIC void parse_integer_value_exhaustive_valid(std::string_view,
                                                              unsigned long&);
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned long long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long long)
template SCN_PUBLIC void parse_integer_value_exhaustive_valid(
    std::string_view,
    unsigned long long&);
#endif

#if SCN_HAS_INT128

#if !SCN_DISABLE_TYPE_INT128
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, int128)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, int128)
// no parse_integer_value_exhaustive_valid
#endif

#if !SCN_DISABLE_TYPE_UINT128
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, uint128)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, uint128)
// no parse_integer_value_exhaustive_valid
#endif

#endif  // SCN_HAS_INT128

#undef SCN_DEFINE_INTEGER_READER_TEMPLATE
}  // namespace impl

/////////////////////////////////////////////////////////////////
// vscan implementation
/////////////////////////////////////////////////////////////////

namespace {
template <typename CharT>
constexpr bool is_simple_single_argument_format_string(
    std::basic_string_view<CharT> format)
{
    if (format.size() != 2) {
        return false;
    }
    return format[0] == CharT{'{'} && format[1] == CharT{'}'};
}

template <typename CharT>
scan_expected<std::ptrdiff_t> scan_simple_single_argument(
    std::basic_string_view<CharT> source,
    basic_scan_args<detail::default_context<CharT>> args,
    basic_scan_arg<detail::default_context<CharT>> arg,
    detail::locale_ref loc = {})
{
    if (SCN_UNLIKELY(!arg)) {
        return detail::unexpected_scan_error(scan_error::invalid_format_string,
                                             "Argument #0 not found");
    }

    auto reader =
        impl::default_arg_reader<impl::basic_contiguous_scan_context<CharT>>{
            ranges::subrange<const CharT*>{source.data(),
                                           source.data() + source.size()},
            SCN_MOVE(args), loc};
    SCN_TRY(it, arg.visit(SCN_MOVE(reader)));
    return ranges::distance(source.data(), it);
}

template <typename CharT>
scan_expected<std::ptrdiff_t> scan_simple_single_argument(
    typename detail::basic_scan_buffer<
        detail::type_identity_t<CharT>>::range_type source,
    basic_scan_args<detail::default_context<CharT>> args,
    basic_scan_arg<detail::default_context<CharT>> arg,
    detail::locale_ref loc = {})
{
    if (SCN_UNLIKELY(!arg)) {
        return detail::unexpected_scan_error(scan_error::invalid_format_string,
                                             "Argument #0 not found");
    }

    SCN_EXPECT(source.begin().stores_parent());
    auto reader = impl::default_arg_reader<detail::default_context<CharT>>{
        source, SCN_MOVE(args), loc};
    SCN_TRY(it, arg.visit(SCN_MOVE(reader)));
    return it.position();
}

template <typename Context, typename ID, typename Handler>
auto get_arg(Context& ctx, ID id, Handler& handler)
{
    auto arg = ctx.arg(id);
    if (SCN_UNLIKELY(!arg)) {
        handler.on_error("Failed to find argument with ID");
    }
    return arg;
}

struct auto_id {};

template <typename Context>
class specs_handler : public detail::specs_setter {
public:
    using char_type = typename Context::char_type;

    constexpr specs_handler(detail::format_specs& specs,
                            basic_scan_parse_context<char_type>& parse_ctx,
                            Context& ctx)
        : detail::specs_setter(specs), m_parse_ctx(parse_ctx), m_ctx(ctx)
    {
    }

private:
    constexpr auto get_arg(auto_id)
    {
        return get_arg(m_ctx, m_parse_ctx.next_arg_id(), *this);
    }

    constexpr auto get_arg(std::size_t arg_id)
    {
        m_parse_ctx.check_arg_id(arg_id);
        return get_arg(m_ctx, arg_id, *this);
    }

    basic_scan_parse_context<char_type>& m_parse_ctx;
    Context& m_ctx;
};

struct format_handler_base {
    format_handler_base(size_t argcount) : args_count(argcount)
    {
        if (SCN_UNLIKELY(args_count >= 64)) {
            visited_args_upper.resize((args_count - 64) / 8);
        }
    }

    void check_args_exhausted()
    {
        {
            const auto args_count_lower64 = args_count >= 64 ? 64 : args_count;
            const uint64_t mask = args_count_lower64 == 64
                                      ? std::numeric_limits<uint64_t>::max()
                                      : (1ull << args_count_lower64) - 1;

            if (visited_args_lower64 != mask) {
                return on_error({scan_error::invalid_format_string,
                                 "Argument list not exhausted"});
            }
        }

        if (args_count < 64) {
            return;
        }

        auto last_args_count = args_count - 64;
        for (auto it = visited_args_upper.begin();
             it != visited_args_upper.end() - 1; ++it) {
            if (*it != std::numeric_limits<uint8_t>::max()) {
                return on_error({scan_error::invalid_format_string,
                                 "Argument list not exhausted"});
            }
            last_args_count -= 8;
        }

        const auto mask = static_cast<uint8_t>(1u << last_args_count) - 1;
        if (visited_args_upper.back() != mask) {
            return on_error({scan_error::invalid_format_string,
                             "Argument list not exhausted"});
        }
    }

    void on_error(const char* msg)
    {
        SCN_UNLIKELY_ATTR
        error = detail::unexpected_scan_error(scan_error::invalid_format_string,
                                              msg);
    }
    void on_error(scan_error err)
    {
        error = unexpected(err);
    }

    SCN_NODISCARD scan_expected<void> get_error() const
    {
        return error;
    }

    SCN_NODISCARD bool has_arg_been_visited(size_t id)
    {
        if (SCN_UNLIKELY(id >= args_count)) {
            on_error({scan_error::invalid_format_string,
                      "Argument ID out-of-range"});
            return false;
        }

        if (SCN_LIKELY(id < 64)) {
            return (visited_args_lower64 >> id) & 1ull;
        }

        id -= 64;
        return (visited_args_upper[id / 8] >> (id % 8)) & 1ull;
    }

    void set_arg_as_visited(size_t id)
    {
        if (SCN_UNLIKELY(id >= args_count)) {
            on_error({scan_error::invalid_format_string,
                      "Argument ID out-of-range"});
            return;
        }

        if (SCN_UNLIKELY(has_arg_been_visited(id))) {
            on_error({scan_error::invalid_format_string,
                      "Argument with this ID has already been scanned"});
        }

        if (SCN_LIKELY(id < 64u)) {
            visited_args_lower64 |= (1ull << id);
            return;
        }

        id -= 64u;
        visited_args_upper[id / 8u] |= static_cast<uint8_t>(1u << (id % 8u));
    }

    std::size_t args_count;
    scan_expected<void> error{};
    uint64_t visited_args_lower64{0};
    std::vector<uint8_t> visited_args_upper{};
};

template <typename CharT>
struct simple_context_wrapper {
    using context_type = detail::default_context<CharT>;

    simple_context_wrapper(
        typename detail::basic_scan_buffer<CharT>::range_type& source,
        basic_scan_args<detail::default_context<CharT>> args,
        detail::locale_ref loc)
        : ctx(source, SCN_MOVE(args), loc)
    {
    }

    detail::default_context<CharT>& get()
    {
        return ctx;
    }
    detail::default_context<CharT>& get_custom()
    {
        return ctx;
    }

    detail::default_context<CharT> ctx;
};

template <typename CharT>
struct contiguous_context_wrapper {
    using context_type = impl::basic_contiguous_scan_context<CharT>;

    contiguous_context_wrapper(
        ranges::subrange<const CharT*> source,
        basic_scan_args<detail::default_context<CharT>> args,
        detail::locale_ref loc)
        : contiguous_ctx(source, args, loc)
    {
    }

    context_type& get()
    {
        return contiguous_ctx;
    }
    detail::default_context<CharT>& get_custom()
    {
        using iterator = typename detail::basic_scan_buffer<CharT>::iterator;
        auto begin =
            iterator{detail::make_string_view_from_pointers<CharT>(
                         contiguous_ctx.original_begin(), contiguous_ctx.end()),
                     std::distance(contiguous_ctx.original_begin(),
                                   contiguous_ctx.begin())};
        if (!custom_ctx) {
            custom_ctx.emplace(begin, ranges::default_sentinel,
                               contiguous_ctx.args(), contiguous_ctx.locale());
        }
        else {
            custom_ctx->advance_to(begin);
        }
        return *custom_ctx;
    }

    context_type contiguous_ctx;
    std::optional<detail::default_context<CharT>> custom_ctx{std::nullopt};
};

template <bool Contiguous, typename CharT>
using context_wrapper_t = std::conditional_t<Contiguous,
                                             contiguous_context_wrapper<CharT>,
                                             simple_context_wrapper<CharT>>;

template <bool Contiguous, typename CharT>
struct format_handler : format_handler_base {
    using context_wrapper_type = context_wrapper_t<Contiguous, CharT>;
    using context_type = typename context_wrapper_type::context_type;
    using char_type = typename context_type::char_type;
    using format_type = std::basic_string_view<char_type>;

    using parse_context_type = typename context_type::parse_context_type;
    using args_type = basic_scan_args<detail::default_context<char_type>>;
    using arg_type = basic_scan_arg<detail::default_context<char_type>>;

    template <typename Source>
    format_handler(Source&& source,
                   format_type format,
                   args_type args,
                   detail::locale_ref loc,
                   std::size_t argcount)
        : format_handler_base{argcount},
          parse_ctx{source_tag<Source&&>, format},
          ctx{SCN_FWD(source), SCN_MOVE(args), SCN_MOVE(loc)}
    {
    }

    void on_literal_text(const char_type* begin, const char_type* end)
    {
        for (; begin != end; ++begin) {
            auto it = get_ctx().begin();
            if (impl::is_range_eof(it, get_ctx().end())) {
                SCN_UNLIKELY_ATTR
                return on_error(
                    {scan_error::invalid_literal, "Unexpected end of source"});
            }

            if (auto [after_space_it, cp, is_space] = impl::is_first_char_space(
                    detail::make_string_view_from_pointers(begin, end));
                cp == detail::invalid_code_point) {
                SCN_UNLIKELY_ATTR
                return on_error({scan_error::invalid_format_string,
                                 "Invalid encoding in format string"});
            }
            else if (is_space) {
                // Skip all whitespace in input
                get_ctx().advance_to(
                    impl::read_while_classic_space(get_ctx().range()));
                // And, skip all whitespace in the format string
                auto begin_it = impl::read_while_classic_space(
                    detail::make_string_view_from_pointers(
                        detail::to_address(after_space_it),
                        detail::to_address(end)));
                // (-1 because of the for loop ++begin)
                begin = detail::to_address(begin_it) - 1;
                continue;
            }

            if (*it != *begin) {
                SCN_UNLIKELY_ATTR
                return on_error({scan_error::invalid_literal,
                                 "Unexpected literal character in source"});
            }
            get_ctx().advance_to(ranges::next(it));
        }
    }

    constexpr std::size_t on_arg_id()
    {
        return parse_ctx.next_arg_id();
    }
    constexpr std::size_t on_arg_id(std::size_t id)
    {
        parse_ctx.check_arg_id(id);
        return id;
    }

    template <typename Visitor>
    void on_visit_scan_arg(Visitor&& visitor, arg_type arg)
    {
        if (!get_error() || !arg) {
            SCN_UNLIKELY_ATTR
            return;
        }

        auto r = arg.visit(SCN_FWD(visitor));
        if (SCN_UNLIKELY(!r)) {
            on_error(r.error());
        }
        else {
            get_ctx().advance_to(*r);
        }
    }

    void on_replacement_field(std::size_t arg_id, const char_type*)
    {
        auto arg = get_arg(get_ctx(), arg_id, *this);
        set_arg_as_visited(arg_id);

        on_visit_scan_arg(
            impl::default_arg_reader<context_type>{
                get_ctx().range(), get_ctx().args(), get_ctx().locale()},
            arg);
    }

    const char_type* on_format_specs(std::size_t arg_id,
                                     const char_type* begin,
                                     const char_type* end)
    {
        auto arg = get_arg(get_ctx(), arg_id, *this);
        set_arg_as_visited(arg_id);

        if (detail::get_arg_type(arg) == detail::arg_type::custom_type) {
            parse_ctx.advance_to(begin);
            on_visit_scan_arg(
                impl::custom_reader<detail::default_context<char_type>>{
                    parse_ctx, get_custom_ctx()},
                arg);
            return parse_ctx.begin();
        }

        auto specs = detail::format_specs{};
        detail::specs_checker<specs_handler<context_type>> handler{
            specs_handler<context_type>{specs, parse_ctx, get_ctx()},
            detail::get_arg_type(arg)};

        begin = detail::parse_format_specs(begin, end, handler);
        if (begin == end || *begin != char_type{'}'}) {
            SCN_UNLIKELY_ATTR
            on_error({scan_error::invalid_format_string,
                      "Missing '}' in format string"});
            return parse_ctx.begin();
        }
        if (SCN_UNLIKELY(!handler.get_error())) {
            return parse_ctx.begin();
        }
        parse_ctx.advance_to(begin);

        on_visit_scan_arg(
            impl::arg_reader<context_type>{get_ctx().range(), specs,
                                           get_ctx().locale()},
            arg);
        return parse_ctx.begin();
    }

    context_type& get_ctx()
    {
        return ctx.get();
    }
    auto& get_custom_ctx()
    {
        return ctx.get_custom();
    }

    parse_context_type parse_ctx;
    context_wrapper_type ctx;
};

template <typename CharT, typename Handler>
scan_expected<std::ptrdiff_t> vscan_parse_format_string(
    std::basic_string_view<CharT> format,
    Handler& handler)
{
    const auto beg = handler.get_ctx().begin();
    detail::parse_format_string<false>(format, handler);
    if (auto err = handler.get_error(); SCN_UNLIKELY(!err)) {
        return unexpected(err.error());
    }
    return ranges::distance(beg, handler.get_ctx().begin());
}

template <typename CharT>
scan_expected<std::ptrdiff_t> vscan_internal(
    std::basic_string_view<CharT> source,
    std::basic_string_view<CharT> format,
    basic_scan_args<detail::default_context<CharT>> args,
    detail::locale_ref loc = {})
{
    const auto argcount = args.size();
    if (is_simple_single_argument_format_string(format) && argcount == 1) {
        auto arg = args.get(0);
        return scan_simple_single_argument(source, SCN_MOVE(args), arg);
    }

    auto handler = format_handler<true, CharT>{
        ranges::subrange<const CharT*>{source.data(),
                                       source.data() + source.size()},
        format, SCN_MOVE(args), SCN_MOVE(loc), argcount};
    return vscan_parse_format_string(format, handler);
}

template <typename CharT>
scan_expected<std::ptrdiff_t> vscan_internal(
    typename detail::basic_scan_buffer<
        detail::type_identity_t<CharT>>::range_type source,
    std::basic_string_view<CharT> format,
    basic_scan_args<detail::default_context<CharT>> args,
    detail::locale_ref loc = {})
{
    if (!source.begin().stores_parent()) {
        return vscan_internal(source.begin().contiguous_segment(), format, args,
                              loc);
    }

    auto& buffer = *source.begin().parent();
    if (auto e = buffer.get_source_error(); SCN_UNLIKELY(!e)) {
        return unexpected(e.error());
    }

    const auto begin_position = source.begin().position();
    const auto end_position = [&]() {
        const auto argcount = args.size();
        if (is_simple_single_argument_format_string(format) && argcount == 1) {
            auto arg = args.get(0);
            return scan_simple_single_argument(source, SCN_MOVE(args), arg);
        }

        auto handler = format_handler<false, CharT>{
            source, format, SCN_MOVE(args), loc, argcount};
        return vscan_parse_format_string(format, handler);
    }();

    if (SCN_LIKELY(end_position)) {
        if (SCN_UNLIKELY(!buffer.sync(*end_position))) {
            return detail::unexpected_scan_error(
                scan_error::invalid_source_state,
                "Failed to sync with underlying source");
        }
    }
    else {
        if (SCN_UNLIKELY(!buffer.sync(begin_position))) {
            return detail::unexpected_scan_error(
                scan_error::invalid_source_state,
                "Failed to sync with underlying source");
        }
    }

    if (auto e = buffer.get_source_error(); SCN_UNLIKELY(!e)) {
        return unexpected(e.error());
    }

    return end_position;
}

template <typename Source, typename CharT>
scan_expected<std::ptrdiff_t> vscan_value_internal(
    Source&& source,
    basic_scan_arg<detail::default_context<CharT>> arg)
{
    return scan_simple_single_argument(SCN_FWD(source), {}, arg);
}
}  // namespace

namespace detail {
template <typename T>
auto scan_int_impl(std::string_view source, T& value, int base)
    -> scan_expected<std::string_view::iterator>
{
    SCN_TRY(beg, impl::skip_classic_whitespace(source).transform_error(
                     impl::make_eof_scan_error));
    auto reader = impl::reader_impl_for_int<char>{};
    return reader.read_default_with_base(ranges::subrange{beg, source.end()},
                                         value, base);
}

template <typename T>
auto scan_int_exhaustive_valid_impl(std::string_view source) -> T
{
    T value{};
    impl::parse_integer_value_exhaustive_valid(source, value);
    return value;
}

}  // namespace detail

SCN_PUBLIC vscan_result<stdin_tag_t> vinput(std::string_view format,
                                            scan_args args)
{
    return detail::vscan_generic(stdin_tag, format, SCN_MOVE(args));
}

template <typename Locale, typename>
SCN_PUBLIC vscan_result<stdin_tag_t> vinput(const Locale& loc,
                                            std::string_view format,
                                            scan_args args)
{
    return detail::vscan_localized_generic(loc, stdin_tag, format,
                                           SCN_MOVE(args));
}

namespace detail {

SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_impl(std::string_view source,
                                                    std::string_view format,
                                                    scan_args args)
{
    return vscan_internal(source, format, args);
}
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_impl(
    scan_buffer::range_type source,
    std::string_view format,
    scan_args args)
{
    return vscan_internal(source, format, args);
}

SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_impl(std::wstring_view source,
                                                    std::wstring_view format,
                                                    wscan_args args)
{
    return vscan_internal(source, format, args);
}
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_impl(
    wscan_buffer::range_type source,
    std::wstring_view format,
    wscan_args args)
{
    return vscan_internal(source, format, args);
}

#if !SCN_DISABLE_LOCALE
template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   std::string_view source,
                                                   std::string_view format,
                                                   scan_args args)
{
    return vscan_internal(source, format, args, detail::locale_ref{loc});
}
template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(
    const Locale& loc,
    scan_buffer::range_type source,
    std::string_view format,
    scan_args args)
{
    return vscan_internal(source, format, args, detail::locale_ref{loc});
}

template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   std::wstring_view source,
                                                   std::wstring_view format,
                                                   wscan_args args)
{
    return vscan_internal(source, format, args, detail::locale_ref{loc});
}
template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(
    const Locale& loc,
    wscan_buffer::range_type source,
    std::wstring_view format,
    wscan_args args)
{
    return vscan_internal(source, format, args, detail::locale_ref{loc});
}

template SCN_PUBLIC auto vscan_localized_impl<std::locale>(const std::locale&,
                                                           std::string_view,
                                                           std::string_view,
                                                           scan_args)
    -> scan_expected<std::ptrdiff_t>;
template SCN_PUBLIC auto vscan_localized_impl<std::locale>(
    const std::locale&,
    scan_buffer::range_type,
    std::string_view,
    scan_args) -> scan_expected<std::ptrdiff_t>;
template SCN_PUBLIC auto vscan_localized_impl<std::locale>(const std::locale&,
                                                           std::wstring_view,
                                                           std::wstring_view,
                                                           wscan_args)
    -> scan_expected<std::ptrdiff_t>;
template SCN_PUBLIC auto vscan_localized_impl<std::locale>(
    const std::locale&,
    wscan_buffer::range_type,
    std::wstring_view,
    wscan_args) -> scan_expected<std::ptrdiff_t>;
#endif

SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_value_impl(
    std::string_view source,
    basic_scan_arg<scan_context> arg)
{
    return vscan_value_internal(source, arg);
}
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_value_impl(
    scan_buffer::range_type source,
    basic_scan_arg<scan_context> arg)
{
    return vscan_value_internal(source, arg);
}

SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_value_impl(
    std::wstring_view source,
    basic_scan_arg<wscan_context> arg)
{
    return vscan_value_internal(source, arg);
}
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_value_impl(
    wscan_buffer::range_type source,
    basic_scan_arg<wscan_context> arg)
{
    return vscan_value_internal(source, arg);
}

#if !SCN_DISABLE_TYPE_SCHAR
template SCN_PUBLIC auto scan_int_impl(std::string_view, signed char&, int)
    -> scan_expected<std::string_view::iterator>;
template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> signed char;
#endif
#if !SCN_DISABLE_TYPE_SHORT
template SCN_PUBLIC auto scan_int_impl(std::string_view, short&, int)
    -> scan_expected<std::string_view::iterator>;
template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> short;
#endif
#if !SCN_DISABLE_TYPE_INT
template SCN_PUBLIC auto scan_int_impl(std::string_view, int&, int)
    -> scan_expected<std::string_view::iterator>;
template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> int;
#endif
#if !SCN_DISABLE_TYPE_LONG
template SCN_PUBLIC auto scan_int_impl(std::string_view, long&, int)
    -> scan_expected<std::string_view::iterator>;
template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> long;
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
template SCN_PUBLIC auto scan_int_impl(std::string_view, long long&, int)
    -> scan_expected<std::string_view::iterator>;
template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> long long;
#endif
#if !SCN_DISABLE_TYPE_UCHAR
template SCN_PUBLIC auto scan_int_impl(std::string_view, unsigned char&, int)
    -> scan_expected<std::string_view::iterator>;
template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned char;
#endif
#if !SCN_DISABLE_TYPE_USHORT
template SCN_PUBLIC auto scan_int_impl(std::string_view, unsigned short&, int)
    -> scan_expected<std::string_view::iterator>;
template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned short;
#endif
#if !SCN_DISABLE_TYPE_UINT
template SCN_PUBLIC auto scan_int_impl(std::string_view, unsigned int&, int)
    -> scan_expected<std::string_view::iterator>;
template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned int;
#endif
#if !SCN_DISABLE_TYPE_ULONG
template SCN_PUBLIC auto scan_int_impl(std::string_view, unsigned long&, int)
    -> scan_expected<std::string_view::iterator>;
template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned long;
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
template SCN_PUBLIC auto scan_int_impl(std::string_view,
                                       unsigned long long&,
                                       int)
    -> scan_expected<std::string_view::iterator>;
template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned long long;
#endif

#if SCN_HAS_INT128

#if !SCN_DISABLE_TYPE_INT128
template SCN_PUBLIC auto scan_int_impl(std::string_view, int128&, int)
    -> scan_expected<std::string_view::iterator>;
#endif

#if !SCN_DISABLE_TYPE_UINT128
template SCN_PUBLIC auto scan_int_impl(std::string_view, uint128&, int)
    -> scan_expected<std::string_view::iterator>;
#endif

#endif

///////////////////////////////////////////////////////////////////////////////
// <chrono> scanning
///////////////////////////////////////////////////////////////////////////////

#if !SCN_DISABLE_CHRONO

template <typename T>
struct datetime_setter;

template <typename T>
struct unreachable_datetime_setter {
    template <typename Handler>
    static void set_subsec(Handler&, T&, setter_state&, double)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    template <typename Handler>
    static void set_sec(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_min(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_hour24(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_hour12(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_mday(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_mon(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_full_year(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_century(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_short_year(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_wday(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_yday(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    template <typename Handler>
    static void set_tz_offset(Handler&, T&, setter_state&, std::chrono::minutes)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    template <typename Handler>
    static void set_tz_name(Handler&, T&, setter_state&, const std::string&)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    static void handle_am_pm(T&, setter_state&)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    static void handle_short_year_and_century(T&, setter_state&)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
};

template <>
struct datetime_setter<std::tm> {
    template <typename Handler>
    static void set_subsec(Handler& h, std::tm&, setter_state&, double)
    {
        h.set_error({scan_error::invalid_format_string,
                     "Subsecond precision not supported with std::tm"});
    }

    template <typename Handler>
    static void set_sec(Handler& h, std::tm& t, setter_state& st, int s)
    {
        if (SCN_UNLIKELY(s < 0 || s > 60)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_sec"});
        }
        t.tm_sec = s;
        st.set_sec(h);
    }
    template <typename Handler>
    static void set_min(Handler& h, std::tm& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 0 || m > 59)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_min"});
        }
        t.tm_min = m;
        st.set_min(h);
    }
    template <typename Handler>
    static void set_hour24(Handler& hdl, std::tm& t, setter_state& st, int h)
    {
        if (SCN_UNLIKELY(h < 0 || h > 23)) {
            return hdl.set_error({scan_error::invalid_scanned_value,
                                  "Invalid value for tm_hour"});
        }
        t.tm_hour = h;
        st.set_hour24(hdl);
    }
    template <typename Handler>
    static void set_hour12(Handler& hdl, std::tm& t, setter_state& st, int h)
    {
        if (SCN_UNLIKELY(h < 1 || h > 12)) {
            return hdl.set_error({scan_error::invalid_scanned_value,
                                  "Invalid value for 12-hour tm_hour"});
        }
        t.tm_hour = h;
        st.set_hour12(hdl);
    }
    template <typename Handler>
    static void set_mday(Handler& h, std::tm& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 1 || d > 31)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_mday"});
        }
        t.tm_mday = d;
        st.set_mday(h);
    }
    template <typename Handler>
    static void set_mon(Handler& h, std::tm& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 12)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_mon"});
        }
        t.tm_mon = m - 1;
        st.set_mon(h);
    }
    template <typename Handler>
    static void set_full_year(Handler& h, std::tm& t, setter_state& st, int y)
    {
        if (SCN_UNLIKELY(y < std::numeric_limits<int>::min() + 1900)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_year"});
        }
        t.tm_year = y - 1900;
        st.set_full_year(h);
    }
    template <typename Handler>
    static void set_century(Handler& h, std::tm&, setter_state& st, int c)
    {
        // TODO: range check
        st.century_value = static_cast<unsigned char>(c);
        st.set_century(h);
    }
    template <typename Handler>
    static void set_short_year(Handler& h, std::tm&, setter_state& st, int y)
    {
        if (SCN_UNLIKELY(y < 0 || y > 99)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_year"});
        }
        st.short_year_value = static_cast<unsigned char>(y);
        st.set_short_year(h);
    }
    template <typename Handler>
    static void set_wday(Handler& h, std::tm& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 0 || d > 6)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_wday"});
        }
        t.tm_wday = d;
        st.set_wday(h);
    }
    template <typename Handler>
    static void set_yday(Handler& h, std::tm& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 0 || d > 365)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_yday"});
        }
        t.tm_yday = d;
        st.set_yday(h);
    }

    template <typename Handler>
    static void set_tz_offset(Handler& h,
                              std::tm& t,
                              setter_state&,
                              std::chrono::minutes o)
    {
        if constexpr (mp_valid<has_tm_gmtoff_predicate, std::tm>::value) {
            assign_gmtoff(t,
                          std::chrono::duration_cast<std::chrono::seconds>(o));
        }
        else {
            return h.set_error(
                {scan_error::invalid_format_string, "tm_gmtoff not supported"});
        }
    }

    template <typename Handler>
    static void set_tz_name(Handler& h,
                            std::tm&,
                            setter_state&,
                            const std::string&)
    {
        return h.set_error(
            {scan_error::invalid_format_string, "tm_zone not supported"});
    }

    static void handle_am_pm(std::tm& t, setter_state& st)
    {
        return st.handle_am_pm(t.tm_hour);
    }

    static void handle_short_year_and_century(std::tm& t, setter_state& st)
    {
        st.handle_short_year_and_century(t.tm_year, 1900);
    }
};

template <>
struct datetime_setter<datetime_components> {
    template <typename Handler>
    static void set_subsec(Handler& h,
                           datetime_components& t,
                           setter_state& st,
                           double s)
    {
        assert(s >= 0.0 && s < 1.0);
        t.subsec = s;
        st.set_subsec(h);
    }
    template <typename Handler>
    static void set_sec(Handler& h,
                        datetime_components& t,
                        setter_state& st,
                        int s)
    {
        if (SCN_UNLIKELY(s < 0 || s > 60)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for seconds"});
        }
        t.sec = s;
        st.set_sec(h);
    }
    template <typename Handler>
    static void set_min(Handler& h,
                        datetime_components& t,
                        setter_state& st,
                        int m)
    {
        if (SCN_UNLIKELY(m < 0 || m > 59)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for minutes"});
        }
        t.min = m;
        st.set_min(h);
    }
    template <typename Handler>
    static void set_hour24(Handler& hdl,
                           datetime_components& t,
                           setter_state& st,
                           int h)
    {
        if (SCN_UNLIKELY(h < 0 || h > 23)) {
            return hdl.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for hours"});
        }
        t.hour = h;
        st.set_hour24(hdl);
    }
    template <typename Handler>
    static void set_hour12(Handler& hdl,
                           datetime_components& t,
                           setter_state& st,
                           int h)
    {
        if (SCN_UNLIKELY(h < 1 || h > 12)) {
            return hdl.set_error({scan_error::invalid_scanned_value,
                                  "Invalid value for hours (12-hour clock)"});
        }
        t.hour = h;
        st.set_hour12(hdl);
    }
    template <typename Handler>
    static void set_mday(Handler& h,
                         datetime_components& t,
                         setter_state& st,
                         int d)
    {
        if (SCN_UNLIKELY(d < 1 || d > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for mday"});
        }
        t.mday = d;
        st.set_mday(h);
    }
    template <typename Handler>
    static void set_mon(Handler& h,
                        datetime_components& t,
                        setter_state& st,
                        int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 12)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for mon"});
        }
        t.mon = month{static_cast<unsigned>(m)};
        st.set_mon(h);
    }
    template <typename Handler>
    static void set_full_year(Handler& h,
                              datetime_components& t,
                              setter_state& st,
                              int y)
    {
        t.year = y;
        st.set_full_year(h);
    }
    template <typename Handler>
    static void set_century(Handler& h,
                            datetime_components& t,
                            setter_state& st,
                            int c)
    {
        if (!t.year) {
            t.year = c * 100;
        }
        else {
            t.year = *t.year + c * 100;
        }
        st.set_century(h);
    }
    template <typename Handler>
    static void set_short_year(Handler& h,
                               datetime_components& t,
                               setter_state& st,
                               int y)
    {
        if (!t.year) {
            t.year = y;
        }
        else {
            t.year = *t.year + y;
        }
        st.set_short_year(h);
    }
    template <typename Handler>
    static void set_wday(Handler& h,
                         datetime_components& t,
                         setter_state& st,
                         int d)
    {
        if (SCN_UNLIKELY(d < 0 || d > 6)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for wday"});
        }
        t.wday = weekday{static_cast<unsigned>(d)};
        st.set_wday(h);
    }
    template <typename Handler>
    static void set_yday(Handler& h,
                         datetime_components& t,
                         setter_state& st,
                         int d)
    {
        if (SCN_UNLIKELY(d < 0 || d > 6)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for yday"});
        }
        t.yday = d;
        st.set_yday(h);
    }

    template <typename Handler>
    static void set_tz_offset(Handler& h,
                              datetime_components& t,
                              setter_state& st,
                              std::chrono::minutes o)
    {
        t.tz_offset = o;
        return st.set_tzoff(h);
    }

    template <typename Handler>
    static void set_tz_name(Handler& h,
                            datetime_components& t,
                            setter_state& st,
                            std::string n)
    {
        t.tz_name = std::move(n);
        return st.set_tzname(h);
    }

    static void handle_am_pm(datetime_components& t, setter_state& st)
    {
        assert(t.hour);
        st.handle_am_pm(*t.hour);
    }

    static void handle_short_year_and_century(datetime_components& t,
                                              setter_state& st)
    {
        assert(t.year);
        st.handle_short_year_and_century(*t.year, 0);
    }
};

template <>
struct datetime_setter<tm_with_tz> : datetime_setter<std::tm> {
    template <typename Handler>
    static void set_tz_offset(Handler& h,
                              tm_with_tz& t,
                              setter_state& st,
                              std::chrono::minutes o)
    {
        if constexpr (mp_valid<has_tm_gmtoff_predicate, std::tm>::value) {
            t.tz_offset = o;
            return datetime_setter<std::tm>::set_tz_offset(h, t, st, o);
        }
        else {
            t.tz_offset = o;
            return st.set_tzoff(h);
        }
    }

    template <typename Handler>
    static void set_tz_name(Handler& h,
                            tm_with_tz& t,
                            setter_state& st,
                            std::string n)
    {
        t.tz_name = std::move(n);
        return st.set_tzname(h);
    }
};

template <>
struct datetime_setter<weekday> : unreachable_datetime_setter<weekday> {
    template <typename Handler>
    static void set_wday(Handler& h, weekday& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 0 || d > 6)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for wday"});
        }
        t = weekday{static_cast<unsigned>(d)};
        st.set_wday(h);
    }
};

template <>
struct datetime_setter<day> : unreachable_datetime_setter<day> {
    template <typename Handler>
    static void set_mday(Handler& h, day& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 1 || d > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for mday"});
        }
        t = day{static_cast<unsigned>(d)};
        st.set_mday(h);
    }
};

template <>
struct datetime_setter<month> : unreachable_datetime_setter<month> {
    template <typename Handler>
    static void set_mon(Handler& h, month& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for month"});
        }
        t = month{static_cast<unsigned>(m)};
        st.set_mon(h);
    }
};

template <>
struct datetime_setter<year> : unreachable_datetime_setter<year> {
    template <typename Handler>
    static void set_full_year(Handler& h, year& t, setter_state& st, int y)
    {
        t = year{y};
        st.set_full_year(h);
    }
    template <typename Handler>
    static void set_century(Handler& h, year& t, setter_state& st, int c)
    {
        t = year{static_cast<int>(t) + c * 100};
        st.set_century(h);
    }
    template <typename Handler>
    static void set_short_year(Handler& h, year& t, setter_state& st, int y)
    {
        t = year{static_cast<int>(t) + y};
        st.set_short_year(h);
    }
};

template <>
struct datetime_setter<month_day> : unreachable_datetime_setter<month_day> {
    template <typename Handler>
    static void set_mon(Handler& h, month_day& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for month"});
        }
        t = month_day{month{static_cast<unsigned>(m)}, t.day()};
        st.set_mon(h);
    }

    template <typename Handler>
    static void set_mday(Handler& h, month_day& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 1 || d > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for mday"});
        }
        t = month_day{t.month(), day{static_cast<unsigned>(d)}};
        st.set_mday(h);
    }
};

template <>
struct datetime_setter<year_month> : unreachable_datetime_setter<year_month> {
    template <typename Handler>
    static void set_full_year(Handler& h,
                              year_month& t,
                              setter_state& st,
                              int y)
    {
        t = year_month{year{y}, t.month()};
        st.set_full_year(h);
    }
    template <typename Handler>
    static void set_century(Handler& h, year_month& t, setter_state& st, int c)
    {
        t = year_month{year{static_cast<int>(t.year()) + c * 100}, t.month()};
        st.set_century(h);
    }
    template <typename Handler>
    static void set_short_year(Handler& h,
                               year_month& t,
                               setter_state& st,
                               int y)
    {
        t = year_month{year{static_cast<int>(t.year()) + y}, t.month()};
        st.set_short_year(h);
    }
    template <typename Handler>
    static void set_mon(Handler& h, year_month& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for month"});
        }
        t = year_month{t.year(), month{static_cast<unsigned>(m)}};
        st.set_mon(h);
    }
};

template <>
struct datetime_setter<year_month_day>
    : unreachable_datetime_setter<year_month_day> {
    template <typename Handler>
    static void set_full_year(Handler& h,
                              year_month_day& t,
                              setter_state& st,
                              int y)
    {
        t = year_month_day{year{y}, t.month(), t.day()};
        st.set_full_year(h);
    }
    template <typename Handler>
    static void set_century(Handler& h,
                            year_month_day& t,
                            setter_state& st,
                            int c)
    {
        t = year_month_day{year{static_cast<int>(t.year()) + c * 100},
                           t.month(), t.day()};
        st.set_century(h);
    }
    template <typename Handler>
    static void set_short_year(Handler& h,
                               year_month_day& t,
                               setter_state& st,
                               int y)
    {
        t = year_month_day{year{static_cast<int>(t.year()) + y}, t.month(),
                           t.day()};
        st.set_short_year(h);
    }
    template <typename Handler>
    static void set_mon(Handler& h, year_month_day& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for month"});
        }
        t = year_month_day{t.year(), month{static_cast<unsigned>(m)}, t.day()};
        st.set_mon(h);
    }
    template <typename Handler>
    static void set_mday(Handler& h, year_month_day& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 1 || d > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for mday"});
        }
        t = year_month_day{t.year(), t.month(), day{static_cast<unsigned>(d)}};
        st.set_mday(h);
    }
};

template <typename T, typename Range, typename CharT>
class tm_reader {
public:
    using type = T;
    using setter = datetime_setter<T>;
    using iterator = ranges::iterator_t<Range>;

    tm_reader(Range r, T& t, locale_ref loc)
        : m_range(SCN_MOVE(r)),
          m_begin(ranges::begin(m_range)),
          m_tm(t),
          m_loc(loc)
    {
    }

    void on_text(const CharT* beg, const CharT* end)
    {
        while (beg != end) {
            if (m_begin == ranges::end(m_range)) {
                return set_error({scan_error::end_of_input, "EOF"});
            }
            if (*beg != *m_begin) {
                return on_error("Invalid literal character");
            }
            ++beg;
            ++m_begin;
        }
    }
    void on_whitespace()
    {
        if (auto res = internal_skip_classic_whitespace(
                ranges::subrange{m_begin, ranges::end(m_range)}, true);
            res) {
            m_begin = SCN_MOVE(*res);
        }
        else {
            set_error(res.error());
        }
    }

    void on_localized()
    {
        m_st.localized = true;
    }

    void on_full_year(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%EY", L"%EY")) {
                setter::set_full_year(*this, m_tm, m_st, t->tm_year + 1900);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int yr = read_classic_unsigned_integer(4, 4);
        setter::set_full_year(*this, m_tm, m_st, yr);
    }
    void on_short_year(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%Ey", L"%Ey")) {
                setter::set_short_year(*this, m_tm, m_st,
                                       (t->tm_year + 1900) % 100);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int yr = read_classic_unsigned_integer(2, 2);
        setter::set_short_year(*this, m_tm, m_st, yr);
    }
    void on_century(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%EC", L"%EC")) {
                setter::set_century(*this, m_tm, m_st,
                                    (t->tm_year + 1900) / 100);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int c = read_classic_unsigned_integer(2, 2);
        setter::set_century(*this, m_tm, m_st, c);
    }
    void on_iso_week_based_year()
    {
        unimplemented();
    }
    void on_iso_week_based_short_year()
    {
        unimplemented();
    }
    void on_loc_offset_year()
    {
        unimplemented();
    }

    void on_month_name()
    {
#if !SCN_DISABLE_FAST_FLOAT
        if (m_st.localized) {
            if (auto t = read_localized("%b", L"%b")) {
                setter::set_mon(*this, m_tm, m_st, t->tm_mon + 1);
            }
            return;
        }
#endif

        std::array<std::pair<std::string_view, int>, 12> long_mapping = {{
            {"January", 1},
            {"February", 2},
            {"March", 3},
            {"April", 4},
            {"May", 5},
            {"June", 6},
            {"July", 7},
            {"August", 8},
            {"September", 9},
            {"October", 10},
            {"November", 11},
            {"December", 12},
        }};
        if (auto m = try_one_of_str_nocase(long_mapping)) {
            return setter::set_mon(*this, m_tm, m_st, *m);
        }
        std::array<std::pair<std::string_view, int>, 11> short_mapping = {{
            {"Jan", 1},
            {"Feb", 2},
            {"Mar", 3},
            {"Apr", 4},
            {"Jun", 6},
            {"Jul", 7},
            {"Aug", 8},
            {"Sep", 9},
            {"Oct", 10},
            {"Nov", 11},
            {"Dec", 12},
        }};
        if (auto m = try_one_of_str_nocase(short_mapping)) {
            return setter::set_mon(*this, m_tm, m_st, *m);
        }
        set_error({scan_error::invalid_scanned_value, "Invalid month name"});
    }
    void on_dec_month(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%Om", L"%Om")) {
                setter::set_mon(*this, m_tm, m_st, t->tm_mon + 1);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int mon = read_classic_unsigned_integer(1, 2);
        setter::set_mon(*this, m_tm, m_st, mon);
    }

    void on_dec0_week_of_year(numeric_system = numeric_system::standard)
    {
        unimplemented();
    }
    void on_dec1_week_of_year()
    {
        unimplemented();
    }
    void on_iso_week_of_year()
    {
        unimplemented();
    }
    void on_day_of_year()
    {
        int yday = read_classic_unsigned_integer(1, 3);
        setter::set_yday(*this, m_tm, m_st, yday - 1);
    }
    void on_day_of_month(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%Od", L"%Od")) {
                setter::set_mday(*this, m_tm, m_st, t->tm_mday);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int mday = read_classic_unsigned_integer(1, 2);
        setter::set_mday(*this, m_tm, m_st, mday);
    }

    void on_weekday_name()
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized) {
            if (auto t = read_localized("%a", L"%a")) {
                setter::set_wday(*this, m_tm, m_st, t->tm_wday);
            }
            return;
        }
#endif

        std::array<std::pair<std::string_view, int>, 7> long_mapping = {{
            {"Sunday", 0},
            {"Monday", 1},
            {"Tuesday", 2},
            {"Wednesday", 3},
            {"Thursday", 4},
            {"Friday", 5},
            {"Saturday", 6},
        }};
        if (auto d = try_one_of_str_nocase(long_mapping)) {
            return setter::set_wday(*this, m_tm, m_st, *d);
        }
        std::array<std::pair<std::string_view, int>, 7> short_mapping = {{
            {"Sun", 0},
            {"Mon", 1},
            {"Tue", 2},
            {"Wed", 3},
            {"Thu", 4},
            {"Fri", 5},
            {"Sat", 6},
        }};
        if (auto d = try_one_of_str_nocase(short_mapping)) {
            return setter::set_wday(*this, m_tm, m_st, *d);
        }
        return set_error(
            {scan_error::invalid_scanned_value, "Invalid weekday name"});
    }
    void on_dec0_weekday(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%Ow", L"%Ow")) {
                setter::set_wday(*this, m_tm, m_st, t->tm_wday);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int wday = read_classic_unsigned_integer(1, 1);
        setter::set_wday(*this, m_tm, m_st, wday);
    }
    void on_dec1_weekday(numeric_system sys = numeric_system::standard)
    {
        auto adjust_wday = [](int d) {
            if (d == 0) {
                return 6;
            }
            return d - 1;
        };

#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%Ow", L"%Ow")) {
                setter::set_wday(*this, m_tm, m_st, adjust_wday(t->tm_wday));
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int wday = read_classic_unsigned_integer(1, 1);
        setter::set_wday(*this, m_tm, m_st, adjust_wday(wday));
    }

    void on_24_hour(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%OH", L"%OH")) {
                setter::set_hour24(*this, m_tm, m_st, t->tm_hour);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int hr = read_classic_unsigned_integer(1, 2);
        setter::set_hour24(*this, m_tm, m_st, hr);
    }
    void on_12_hour(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%OI", L"%OI")) {
                setter::set_hour12(*this, m_tm, m_st, t->tm_hour);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int hr = read_classic_unsigned_integer(1, 2);
        setter::set_hour12(*this, m_tm, m_st, hr);
    }
    void on_minute(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%OM", L"%OM")) {
                setter::set_min(*this, m_tm, m_st, t->tm_min);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int min = read_classic_unsigned_integer(1, 2);
        setter::set_min(*this, m_tm, m_st, min);
    }
    void on_second(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%OS", L"%OS")) {
                setter::set_sec(*this, m_tm, m_st, t->tm_sec);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int sec = read_classic_unsigned_integer(1, 2);
        setter::set_sec(*this, m_tm, m_st, sec);
    }
    void on_subsecond(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_TYPE_STRING && !SCN_DISABLE_TYPE_DOUBLE
        int whole = read_classic_unsigned_integer(1, 2);
        setter::set_sec(*this, m_tm, m_st, whole);

        if (!m_st.localized || sys == numeric_system::standard) {
            if (!consume_ch('.')) {
                return set_error({scan_error::invalid_scanned_value,
                                  "Expected `.` in subsecond value"});
            }
        }
#if !SCN_DISABLE_LOCALE
        else {
            auto& state = get_localized_read_state();
            CharT sep = state.numpunct_facet->decimal_point();
            if (!consume_code_unit(sep)) {
                return set_error(
                    {scan_error::invalid_scanned_value,
                     "Expected decimal separator in subsecond value"});
            }
        }
#endif  // !SCN_DISABLE_LOCALE

        auto str_res = scan<std::string>(
            ranges::subrange{m_begin, m_range.end()}, []() -> decltype(auto) {
                if constexpr (std::is_same_v<CharT, char>) {
                    return "{:[0-9]}";
                }
                else {
                    return L"{:[0-9]}";
                }
            }());
        if (!str_res) {
            return set_error(str_res.error());
        }
        if (str_res->begin() == m_begin) {
            return set_error({scan_error::invalid_scanned_value,
                              "Expected digits after `.` in subsecond value"});
        }
        m_begin = str_res->begin();

        auto subsecond_str = std::move(str_res->value());
        subsecond_str.insert(0, "0.");
        auto dbl_res = scan<double>(subsecond_str, "{}");
        if (!dbl_res) {
            return set_error(dbl_res.error());
        }

        setter::set_subsec(*this, m_tm, m_st, dbl_res->value());

#else  // !SCN_DISABLE_TYPE_STRING && !SCN_DISABLE_TYPE_DOUBLE
        SCN_UNUSED(sys);
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
#endif
    }

    void on_tz_offset(numeric_system sys = numeric_system::standard)
    {
        // [+|-]
        if (m_begin == ranges::end(m_range)) {
            return set_error({scan_error::end_of_input, "EOF"});
        }
        bool is_minus = false;
        {
            const auto sign_ch = *m_begin;
            if (sign_ch == CharT{'+'}) {
                ++m_begin;
            }
            else if (sign_ch == CharT{'-'}) {
                is_minus = true;
                ++m_begin;
            }
        }
        if (m_begin == ranges::end(m_range)) {
            return set_error({scan_error::end_of_input, "EOF"});
        }

        int hour = 0;
        int minute = 0;
        if (sys == numeric_system::standard) {
            // hh[[:]mm]
            hour = read_classic_unsigned_integer(2, 2);
            if (m_begin != ranges::end(m_range)) {
                auto it_before_colon = m_begin;
                if (*m_begin == CharT{':'}) {
                    ++m_begin;
                }
                if (m_begin == ranges::end(m_range) || *m_begin < CharT{'0'} ||
                    *m_begin > CharT{'9'}) {
                    m_begin = it_before_colon;
                }
                else {
                    minute = read_classic_unsigned_integer(2, 2);
                }
            }
        }
        else {
            // h[h][:mm]
            hour = read_classic_unsigned_integer(1, 2);
            if (m_begin != ranges::end(m_range)) {
                auto it_before_colon = m_begin;
                if (*m_begin == CharT{':'}) {
                    ++m_begin;
                    if (m_begin == ranges::end(m_range) ||
                        *m_begin < CharT{'0'} || *m_begin > CharT{'9'}) {
                        m_begin = it_before_colon;
                    }
                    else {
                        minute = read_classic_unsigned_integer(2, 2);
                    }
                }
            }
        }

        setter::set_tz_offset(
            *this, m_tm, m_st,
            std::chrono::minutes{(is_minus ? -1 : 1) * (hour * 60 + minute)});
    }
    void on_tz_name()
    {
        auto res = scan<std::string>(
            ranges::subrange{m_begin, m_range.end()}, []() -> decltype(auto) {
                if constexpr (std::is_same_v<CharT, char>) {
                    return "{:[a-zA-Z0-9-+_/]}";
                }
                else {
                    return L"{:[a-zA-Z0-9-+_/]}";
                }
            }());
        if (!res) {
            set_error(res.error());
        }
        else {
            setter::set_tz_name(*this, m_tm, m_st, std::move(res->value()));
        }
        m_begin = res->begin();
    }

    void on_loc_datetime(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized) {
            const auto t = [&]() {
                if (sys != numeric_system::standard) {
                    return read_localized("%Ec", L"%Ec");
                }
                return read_localized("%c", L"%c");
            }();

            if (t) {
                setter::set_full_year(*this, m_tm, m_st, t->tm_year + 1900);
                setter::set_mon(*this, m_tm, m_st, t->tm_mon + 1);
                setter::set_mday(*this, m_tm, m_st, t->tm_mday);
                setter::set_hour24(*this, m_tm, m_st, t->tm_hour);
                setter::set_min(*this, m_tm, m_st, t->tm_min);
                setter::set_sec(*this, m_tm, m_st, t->tm_sec);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif
        // %c == %a %b %d %H:%M:%S %Y
        constexpr CharT colon = ':';
        on_weekday_name();
        on_whitespace();
        on_month_name();
        on_whitespace();
        on_day_of_month();
        on_whitespace();
        on_24_hour();
        on_text(&colon, &colon + 1);
        on_minute();
        on_text(&colon, &colon + 1);
        on_second();
        on_whitespace();
        on_full_year();
    }
    void on_loc_date(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized) {
            const auto t = [&]() {
                if (sys != numeric_system::standard) {
                    return read_localized("%Ex", L"%Ex");
                }
                return read_localized("%x", L"%x");
            }();

            if (t) {
                setter::set_full_year(*this, m_tm, m_st, t->tm_year + 1900);
                setter::set_mon(*this, m_tm, m_st, t->tm_mon + 1);
                setter::set_mday(*this, m_tm, m_st, t->tm_mday);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        // %x == %m/%d/%Y
        constexpr CharT slash = '/';
        on_dec_month();
        on_text(&slash, &slash + 1);
        on_day_of_month();
        on_text(&slash, &slash + 1);
        on_full_year();
    }
    void on_loc_time(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized) {
            const auto t = [&]() {
                if (sys != numeric_system::standard) {
                    return read_localized("%EX", L"%EX");
                }
                return read_localized("%X", L"%X");
            }();

            if (t) {
                setter::set_hour24(*this, m_tm, m_st, t->tm_hour);
                setter::set_min(*this, m_tm, m_st, t->tm_min);
                setter::set_sec(*this, m_tm, m_st, t->tm_sec);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif
        // %X == %H:%M:%S
        on_iso_time();
    }
    void on_us_date()
    {
        // %m/%d/%y
        constexpr CharT slash = '/';
        on_dec_month();
        on_text(&slash, &slash + 1);
        on_day_of_month();
        on_text(&slash, &slash + 1);
        on_short_year();
    }
    void on_iso_date()
    {
        // %Y-%m-%d
        constexpr CharT dash = '-';
        on_full_year();
        on_text(&dash, &dash + 1);
        on_dec_month();
        on_text(&dash, &dash + 1);
        on_day_of_month();
    }
    void on_loc_12_hour_time()
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized) {
            if (auto t = read_localized("%r", L"%r")) {
                setter::set_hour24(*this, m_tm, m_st, t->tm_hour);
                setter::set_min(*this, m_tm, m_st, t->tm_min);
                setter::set_sec(*this, m_tm, m_st, t->tm_sec);
            }
            return;
        }
#endif
        // %r == %I:%M:%S %p
        constexpr CharT colon = ':';
        on_12_hour();
        on_text(&colon, &colon + 1);
        on_minute();
        on_text(&colon, &colon + 1);
        on_second();
        on_whitespace();
        on_am_pm();
    }
    void on_24_hour_time()
    {
        // %H:%M
        constexpr CharT colon = ':';
        on_24_hour();
        on_text(&colon, &colon + 1);
        on_minute();
    }
    void on_iso_time()
    {
        // %H:%M:%S
        constexpr CharT colon = ':';
        on_24_hour();
        on_text(&colon, &colon + 1);
        on_minute();
        on_text(&colon, &colon + 1);
        on_second();
    }
    void on_am_pm()
    {
        std::array<std::pair<std::string_view, bool>, 4> mapping = {{
            {"am", false},
            {"a.m.", false},
            {"pm", true},
            {"p.m.", true},
        }};
        if (auto b = try_one_of_str_nocase(mapping)) {
            m_st.is_pm = *b;
            return m_st.set_am_pm(*this);
        }
        set_error(
            {scan_error::invalid_scanned_value, "Invalid am/pm specifier"});
    }

    void on_epoch_offset()
    {
        unimplemented();
    }
    void on_duration_tick_count()
    {
        unimplemented();
    }
    void on_duration_suffix()
    {
        unimplemented();
    }

    void verify()
    {
        m_st.verify(*this);
        if (m_st.am_pm_set && m_st.hour12_set) {
            setter::handle_am_pm(m_tm, m_st);
        }
        if (!m_st.full_year_set && (m_st.short_year_set || m_st.century_set)) {
            setter::handle_short_year_and_century(m_tm, m_st);
        }
    }

    scan_expected<void> get_error() const
    {
        return m_error;
    }

    void on_error(const char* msg)
    {
        set_error({scan_error::invalid_format_string, msg});
    }

    void set_error(scan_error e)
    {
        if (m_error.has_value()) {
            m_error = unexpected(e);
        }
    }

    iterator get_iterator() const
    {
        return m_begin;
    }

private:
    void unimplemented()
    {
        on_error("Unimplemented");
    }

    int read_classic_unsigned_integer(int min_digits, int max_digits)
    {
        int digits_read = 0;
        int accumulator = 0;
        while (m_begin != m_range.end()) {
            const auto ch = *m_begin;
            if (ch < CharT{'0'} || ch > CharT{'9'}) {
                break;
            }
            ++m_begin;
            ++digits_read;
            accumulator = accumulator * 10 + static_cast<int>(ch - CharT{'0'});
            if (digits_read >= max_digits) {
                break;
            }
        }
        if (digits_read < min_digits) {
            set_error(scan_error{scan_error::invalid_scanned_value,
                                 "Too few integer digits"});
            return -1;
        }
        return accumulator;
    }

    bool consume_ch(char ch)
    {
        if (m_begin == m_range.end()) {
            return false;
        }
        if (*m_begin == static_cast<CharT>(ch)) {
            ++m_begin;
            return true;
        }
        return false;
    }

    bool consume_code_unit(CharT ch)
    {
        if (m_begin == m_range.end()) {
            return false;
        }
        if (*m_begin == ch) {
            ++m_begin;
            return true;
        }
        return false;
    }

    template <typename OptT, std::size_t N>
    std::optional<OptT> try_one_of_str_nocase(
        std::array<std::pair<std::string_view, OptT>, N>& options)
    {
        auto start_it = m_begin;
        std::size_t options_available = N;
        std::size_t chars_consumed = 0;
        while (options_available >= 1 &&
               options.front().first.size() > chars_consumed) {
            std::size_t i = 0;
            if (m_begin == m_range.end()) {
                options_available = 0;
                break;
            }
            const auto ch = *m_begin;
            ++m_begin;
            while (i < options_available) {
                const auto cmp = static_cast<unsigned>(
                    ch ^ options[i].first[chars_consumed]);
                if (options[i].first.size() <= chars_consumed ||
                    (cmp != 0 && cmp != 32)) {
                    std::rotate(
                        options.begin() + static_cast<std::ptrdiff_t>(i),
                        options.begin() + static_cast<std::ptrdiff_t>(i) + 1,
                        options.end());
                    --options_available;
                    continue;
                }
                ++i;
            }
            ++chars_consumed;
        }
        if (options_available != 1) {
            m_begin = start_it;
            return std::nullopt;
        }
        return options.front().second;
    }

#if !SCN_DISABLE_LOCALE
    struct localized_read_state {
        using facet_iterator_type =
            ranges::iterator_t<decltype(ranges::views::common(
                SCN_DECLVAL(Range&)))>;
        using time_facet_type = std::time_get<CharT, facet_iterator_type>;
        using numpunct_facet_type = std::numpunct<CharT>;

        std::locale locale;
        const time_facet_type* time_facet;
        const numpunct_facet_type* numpunct_facet;
        std::basic_stringstream<CharT> dummy_stream;
    };

    localized_read_state& get_localized_read_state()
    {
        if (!m_loc_state) {
            auto loc = [&]() {
                if (m_st.localized) {
                    return m_loc.get<std::locale>();
                }
                return std::locale::classic();
            }();
            if (!std::has_facet<typename localized_read_state::time_facet_type>(
                    loc)) {
                loc = std::locale(
                    loc, new typename localized_read_state::time_facet_type{});
            }
            if (!std::has_facet<
                    typename localized_read_state::numpunct_facet_type>(loc)) {
                loc = std::locale(
                    loc,
                    new typename localized_read_state::numpunct_facet_type{});
            }

            m_loc_state = localized_read_state{
                SCN_MOVE(loc),
                &std::use_facet<typename localized_read_state::time_facet_type>(
                    loc),
                &std::use_facet<
                    typename localized_read_state::numpunct_facet_type>(loc),
                std::basic_stringstream<CharT>{}};

            m_loc_state->dummy_stream.imbue(m_loc_state->locale);
        }

        return *m_loc_state;
    }

    std::optional<std::tm> do_read_localized(std::basic_string_view<CharT> fmt)
    {
        const auto& facet = *get_localized_read_state().time_facet;
        std::ios_base::iostate err{std::ios_base::goodbit};
        std::tm tm{};
        auto&& common_range = ranges::views::common(m_range);
        auto iter =
            facet.get(ranges::begin(common_range), ranges::end(common_range),
                      m_loc_state->dummy_stream, err, &tm, fmt.data(),
                      fmt.data() + fmt.size());
        if ((err & std::ios_base::failbit) != 0) {
            set_error({scan_error::invalid_scanned_value,
                       "Failed to scan localized datetime"});
            return std::nullopt;
        }
        if constexpr (ranges::common_range<Range>) {
            m_begin = SCN_MOVE(iter);
        }
        else {
            m_begin = SCN_MOVE(iter.base());
        }
        return tm;
    }

    std::optional<std::tm> read_localized(std::string_view fmt,
                                          std::wstring_view wfmt)
    {
        SCN_UNUSED(fmt);
        SCN_UNUSED(wfmt);
        if constexpr (std::is_same_v<CharT, char>) {
            return do_read_localized(fmt);
        }
        else {
            return do_read_localized(wfmt);
        }
    }

    std::optional<localized_read_state> m_loc_state;
#else
    std::optional<std::tm> read_localized(std::string_view, std::wstring_view)
    {
        set_error(
            {scan_error::invalid_format_string,
             "Failed to scan localized datetime with SCN_DISABLE_LOCALE on"});
        return std::nullopt;
    }
#endif  // !SCN_DISABLE_LOCALE

    Range m_range;
    iterator m_begin;
    T& m_tm;
    setter_state m_st{};
    locale_ref m_loc{};
    scan_expected<void> m_error{};
};

template <typename CharT, typename T, typename Context>
auto chrono_scan_inner_impl(std::basic_string_view<CharT> fmt,
                            T& t,
                            Context& ctx)
    -> scan_expected<typename Context::iterator>
{
    {
        SCN_TRY(it,
                detail::internal_skip_classic_whitespace(ctx.range(), false));
        ctx.advance_to(SCN_MOVE(it));
    }

    auto r = detail::tm_reader<T, typename Context::range_type, CharT>(
        ctx.range(), t, ctx.locale());
    detail::parse_chrono_format_specs(fmt.data(), fmt.data() + fmt.size(), r);
    if (auto e = r.get_error(); SCN_UNLIKELY(!e)) {
        return unexpected(e.error());
    }
    return r.get_iterator();
}

template <typename CharT, typename T, typename Context>
auto chrono_scan_impl(std::basic_string_view<CharT> fmt_str, T& t, Context& ctx)
    -> scan_expected<typename Context::iterator>
{
    if (!impl::is_entire_source_contiguous(ctx.range())) {
        // ctx.begin() stores parent (buffer) -> not contiguous
        return chrono_scan_inner_impl(fmt_str, t, ctx);
    }

    auto crange = impl::get_as_contiguous(ctx.range());
    auto contiguous_ctx = impl::basic_contiguous_scan_context<CharT>(
        ranges::subrange<const CharT*>(crange.data(),
                                       crange.data() + crange.size()),
        ctx.args(), ctx.locale());
    auto begin = contiguous_ctx.begin();
    SCN_TRY(it, chrono_scan_inner_impl(fmt_str, t, contiguous_ctx));
    return ctx.begin().batch_advance(std::distance(begin, it));
}

template auto chrono_scan_impl(std::string_view, std::tm&, scan_context&)
    -> scan_expected<scan_context::iterator>;
template auto chrono_scan_impl(std::string_view, tm_with_tz&, scan_context&)
    -> scan_expected<scan_context::iterator>;
template auto chrono_scan_impl(std::string_view,
                               datetime_components&,
                               scan_context&)
    -> scan_expected<scan_context::iterator>;

template auto chrono_scan_impl(std::wstring_view, std::tm&, wscan_context&)
    -> scan_expected<wscan_context::iterator>;
template auto chrono_scan_impl(std::wstring_view, tm_with_tz&, wscan_context&)
    -> scan_expected<wscan_context::iterator>;
template auto chrono_scan_impl(std::wstring_view,
                               datetime_components&,
                               wscan_context&)
    -> scan_expected<wscan_context::iterator>;

}  // namespace detail

#endif  // !SCN_DISABLE_CHRONO

#if !SCN_DISABLE_IOSTREAM

namespace detail {

template <typename CharT>
SCN_PUBLIC basic_scan_istream_buffer<CharT>::basic_scan_istream_buffer(
    std::basic_istream<CharT>& strm) noexcept
    : base(typename base::non_contiguous_tag{}), m_stream(&strm)
{
}

template <typename CharT>
SCN_PUBLIC basic_scan_istream_buffer<CharT>::~basic_scan_istream_buffer() =
    default;

template <typename CharT>
SCN_PUBLIC bool basic_scan_istream_buffer<CharT>::do_fill()
{
    SCN_EXPECT(m_stream);

    if (!this->m_current_view.empty()) {
        this->m_putback_buffer.append(this->m_current_view.begin(),
                                      this->m_current_view.end());
    }
    this->m_current_view = {};

    auto& streambuf = *m_stream->rdbuf();
    const auto peek_result = streambuf.sgetc();
    if (traits::eq_int_type(peek_result, traits::eof())) {
        return false;
    }

    const auto n_avail = streambuf.in_avail();
    if (n_avail == 0) {
        // Zero characters available, but we got one with `sgetc`.
        // Use that, and advance the sequence.
        const auto bump_result = streambuf.sbumpc();
        SCN_ENSURE(bump_result == peek_result);
        m_buf.resize(1);
        m_buf[0] = traits::to_char_type(peek_result);
        this->m_current_view = std::basic_string_view<CharT>{m_buf.data(), 1};
        return true;
    }
    if (n_avail < 0) {
        // EOF or error
        return false;
    }
    const auto n_avail_u = static_cast<std::size_t>(n_avail);
    m_buf.resize(n_avail_u);

    const auto n_read =
        static_cast<std::size_t>(streambuf.sgetn(m_buf.data(), n_avail));
    SCN_EXPECT(n_read == n_avail_u);
    this->m_current_view = std::basic_string_view<CharT>{m_buf.data(), n_read};
    return true;
}

template <typename CharT>
SCN_PUBLIC bool basic_scan_istream_buffer<CharT>::do_sync(
    std::ptrdiff_t position)
{
    SCN_EXPECT(m_stream);
    auto& streambuf = *m_stream->rdbuf();
    return impl::buffer_sync_helper(position, this->m_current_view,
                                    this->m_putback_buffer, [&](CharT ch) {
                                        return !traits::eq_int_type(
                                            streambuf.sputbackc(ch),
                                            traits::eof());
                                    }) == position;
}

template basic_scan_istream_buffer<char>::basic_scan_istream_buffer(
    std::istream&) noexcept;
template basic_scan_istream_buffer<wchar_t>::basic_scan_istream_buffer(
    std::wistream&) noexcept;

template basic_scan_istream_buffer<char>::~basic_scan_istream_buffer();
template basic_scan_istream_buffer<wchar_t>::~basic_scan_istream_buffer();

}  // namespace detail

#endif  // !SCN_DISABLE_IOSTREAM

SCN_END_NAMESPACE
}  // namespace scn
