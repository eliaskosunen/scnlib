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

#include <scn/impl/reader/float_reader.h>
#include <scn/impl/reader/integer_reader.h>

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wold-style-cast")
SCN_GCC_IGNORE("-Wnoexcept")
SCN_GCC_IGNORE("-Wundef")
SCN_GCC_IGNORE("-Wsign-conversion")

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wold-style-cast")
SCN_CLANG_IGNORE("-Wdeprecated")
SCN_CLANG_IGNORE("-Wcomma")
SCN_CLANG_IGNORE("-Wundef")
SCN_CLANG_IGNORE("-Wdocumentation-unknown-command")

#if SCN_CLANG >= SCN_COMPILER(16, 0, 0)
SCN_CLANG_IGNORE("-Wunsafe-buffer-usage")
#endif

#if SCN_CLANG >= SCN_COMPILER(8, 0, 0)
SCN_CLANG_IGNORE("-Wextra-semi-stmt")
#endif

#if SCN_CLANG >= SCN_COMPILER(13, 0, 0)
SCN_CLANG_IGNORE("-Wreserved-identifier")
#endif

#include <fast_float/fast_float.h>

SCN_CLANG_POP
SCN_GCC_POP

#include <clocale>
#include <limits>
#include <sstream>
#include <string_view>

#if SCN_HAS_FLOAT_CHARCONV
#include <charconv>
#endif

#define SCN_XLOCALE_POSIX    0
#define SCN_XLOCALE_MSVC     1
#define SCN_XLOCALE_OTHER    2
#define SCN_XLOCALE_DISABLED 3

#if SCN_DISABLE_LOCALE
#define SCN_XLOCALE SCN_XLOCALE_DISABLED
#elif SCN_HAS_INCLUDE(<xlocale.h>)
#include <xlocale.h>
#define SCN_XLOCALE SCN_XLOCALE_POSIX

#elif defined(_MSC_VER)
#define SCN_XLOCALE SCN_XLOCALE_MSVC

#elif defined(__GLIBC__)
// glibc

#include <features.h>

#if !((__GLIBC__ > 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ > 25)))
#include <xlocale.h>
#define SCN_XLOCALE SCN_XLOCALE_POSIX
#endif  // __GLIBC__ <= 2.25

#elif defined(__FreeBSD_version) && __FreeBSD_version >= 1000010

// FreeBSD
#include <xlocale.h>
#define SCN_XLOCALE SCN_XLOCALE_POSIX

#endif  // ^^^ else

#ifndef SCN_XLOCALE
#define SCN_XLOCALE SCN_XLOCALE_OTHER
#endif

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
namespace {
SCN_GCC_COMPAT_PUSH
SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")
constexpr bool is_float_zero(float f)
{
    return f == 0.0f || f == -0.0f;
}
constexpr bool is_float_zero(double d)
{
    return d == 0.0 || d == -0.0;
}
SCN_MAYBE_UNUSED constexpr bool is_float_zero(long double ld)
{
    return ld == 0.0L || ld == -0.0L;
}
SCN_GCC_COMPAT_POP

struct impl_base {
    float_reader_base::float_kind m_kind;
    unsigned m_options;
};

template <typename CharT>
struct impl_init_data {
    contiguous_range_factory<CharT>& input;
    float_reader_base::float_kind kind;
    unsigned options;

    constexpr impl_base base() const
    {
        return {kind, options};
    }
};

////////////////////////////////////////////////////////////////////
// strtod-based implementation
// Fallback for all CharT and FloatT, if allowed
////////////////////////////////////////////////////////////////////

#if !SCN_DISABLE_STRTOD
template <typename T>
class strtod_impl_base : impl_base {
protected:
    strtod_impl_base(impl_base base) : impl_base{base} {}

    template <typename CharT, typename Strtod>
    scan_expected<std::ptrdiff_t> parse(T& value,
                                        const CharT* src,
                                        Strtod strtod_cb)
    {
        CharT* end{};
        errno = 0;
        value = strtod_cb(src, &end);
        const auto saved_errno = errno;
        auto chars_read = end - src;

        if (auto e = this->check_error(chars_read, saved_errno, value);
            SCN_UNLIKELY(!e)) {
            return unexpected(e);
        }

        if (m_kind == float_reader_base::float_kind::hex_without_prefix &&
            chars_read >= 2) {
            chars_read -= 2;
        }

        return chars_read;
    }

    template <typename CharT>
    const CharT* get_null_terminated_source(
        contiguous_range_factory<CharT>& input)
    {
        if (!input.stores_allocated_string()) {
            // TODO: call float_reader::read_source?
            auto first_space = read_until_classic_space(input.view());
            input.assign(
                std::basic_string<CharT>{input.view().begin(), first_space});
        }

        if (this->m_kind == float_reader_base::float_kind::hex_without_prefix) {
            if constexpr (std::is_same_v<CharT, char>) {
                input.get_allocated_string().insert(0, "0x");
            }
            else {
                input.get_allocated_string().insert(0, L"0x");
            }
        }

        return input.get_allocated_string().c_str();
    }

    SCN_NODISCARD scan_error check_error(std::ptrdiff_t chars_read,
                                         int c_errno,
                                         T value) const
    {
        if (is_float_zero(value) && chars_read == 0) {
            SCN_UNLIKELY_ATTR
            return {scan_error::invalid_scanned_value,
                    "strtod failed: No conversion"};
        }

        if (m_kind == float_reader_base::float_kind::hex_with_prefix &&
            (m_options & float_reader_base::allow_hex) == 0) {
            SCN_UNLIKELY_ATTR
            return {scan_error::invalid_scanned_value,
                    "Hexfloats disallowed by format string"};
        }

        if (c_errno == ERANGE && is_float_zero(value)) {
            SCN_UNLIKELY_ATTR
            return {scan_error::value_out_of_range, "strtod failed: underflow"};
        }

        SCN_GCC_COMPAT_PUSH
        SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")

        if (m_kind != float_reader_base::float_kind::inf_short &&
            m_kind != float_reader_base::float_kind::inf_long &&
            std::abs(value) == std::numeric_limits<T>::infinity()) {
            SCN_UNLIKELY_ATTR
            return {scan_error::value_out_of_range, "strtod failed: overflow"};
        }

        SCN_GCC_COMPAT_POP  // -Wfloat-equal

            return {};
    }

    static T generic_narrow_strtod(const char* str, char** str_end)
    {
#if SCN_XLOCALE == SCN_XLOCALE_POSIX
        static locale_t cloc = ::newlocale(LC_ALL_MASK, "C", NULL);
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
    }

    static T generic_wide_strtod(const wchar_t* str, wchar_t** str_end)
    {
#if SCN_XLOCALE == SCN_XLOCALE_POSIX
        static locale_t cloc = ::newlocale(LC_ALL_MASK, "C", NULL);
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
    }
};

template <typename CharT, typename T>
class strtod_impl : public strtod_impl_base<T> {
public:
    explicit strtod_impl(impl_init_data<CharT> data)
        : strtod_impl_base<T>(data.base()), m_input(data.input)
    {
    }

    scan_expected<std::ptrdiff_t> operator()(T& value)
    {
        return this->parse(value, this->get_null_terminated_source(m_input),
                           generic_strtod);
    }

private:
    static T generic_strtod(const CharT* str, CharT** str_end)
    {
        if constexpr (std::is_same_v<CharT, char>) {
            return strtod_impl_base<T>::generic_narrow_strtod(str, str_end);
        }
        else {
            return strtod_impl_base<T>::generic_wide_strtod(str, str_end);
        }
    }

    contiguous_range_factory<CharT>& m_input;
};
#endif

////////////////////////////////////////////////////////////////////
// std::from_chars-based implementation
// Only for CharT=char, if available
////////////////////////////////////////////////////////////////////

#if SCN_HAS_FLOAT_CHARCONV && !SCN_DISABLE_FROM_CHARS
template <typename Float, typename = void>
struct has_charconv_for : std::false_type {};

template <typename Float>
struct has_charconv_for<
    Float,
    std::void_t<decltype(std::from_chars(SCN_DECLVAL(const char*),
                                         SCN_DECLVAL(const char*),
                                         SCN_DECLVAL(Float&)))>>
    : std::true_type {};

#if SCN_STDLIB_GLIBCXX
// libstdc++ has buggy std::from_chars for long double
template <>
struct has_charconv_for<long double, void> : std::false_type {};
#endif

struct SCN_MAYBE_UNUSED from_chars_impl_base : impl_base {
    SCN_MAYBE_UNUSED from_chars_impl_base(impl_init_data<char> data)
        : impl_base{data.base()}, m_input(data.input)
    {
    }

protected:
    SCN_MAYBE_UNUSED scan_expected<std::chars_format> get_flags(
        std::string_view& input) const
    {
        auto flags = map_options_to_flags();

        if ((flags & std::chars_format::hex) != std::chars_format{}) {
            if (m_kind == float_reader_base::float_kind::hex_without_prefix) {
                return std::chars_format::hex;
            }
            else if (m_kind == float_reader_base::float_kind::hex_with_prefix) {
                input = input.substr(2);
                return std::chars_format::hex;
            }

            flags &= ~std::chars_format::hex;
            if (flags == std::chars_format{}) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "from_chars: Expected a hexfloat");
            }
        }

        return flags;
    }

    contiguous_range_factory<char>& m_input;

private:
    std::chars_format map_options_to_flags() const
    {
        std::chars_format flags{};

        if (m_options & float_reader_base::allow_fixed) {
            flags |= std::chars_format::fixed;
        }
        if (m_options & float_reader_base::allow_scientific) {
            flags |= std::chars_format::scientific;
        }
        if (m_options & float_reader_base::allow_hex) {
            flags |= std::chars_format::hex;
        }

        return flags;
    }
};

template <typename T>
class from_chars_impl : public from_chars_impl_base {
public:
    using from_chars_impl_base::from_chars_impl_base;

    scan_expected<std::ptrdiff_t> operator()(T& value) const
    {
        auto input_view = m_input.view();
        const auto flags = get_flags(input_view);
        if (SCN_UNLIKELY(!flags)) {
            return unexpected(flags.error());
        }

        const auto result = std::from_chars(
            input_view.data(), input_view.data() + input_view.size(), value,
            *flags);

        if (SCN_UNLIKELY(result.ec == std::errc::invalid_argument)) {
            return unexpected_scan_error(scan_error::invalid_scanned_value,
                                         "from_chars: invalid_argument");
        }
        if (result.ec == std::errc::result_out_of_range) {
#if !SCN_DISABLE_STRTOD
            // May be subnormal:
            // at least libstdc++ gives out_of_range for subnormals
            //  -> fall back to strtod
            return strtod_impl<char, T>{{ m_input, m_kind, m_options }}(value);
#else
            return unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "from_chars: invalid_argument, fallback to strtod "
                "disabled");
#endif
        }

        return result.ptr - m_input.view().data();
    }
};
#endif  // SCN_HAS_FLOAT_CHARCONV && !SCN_DISABLE_FROM_CHARS

////////////////////////////////////////////////////////////////////
// fast_float-based implementation
// Only for FloatT=(float OR double)
////////////////////////////////////////////////////////////////////

template <typename CharT, typename T>
scan_expected<std::ptrdiff_t> fast_float_fallback(impl_init_data<CharT> data,
                                                  T& value)
{
#if SCN_HAS_FLOAT_CHARCONV && !SCN_DISABLE_FROM_CHARS
    if constexpr (std::is_same_v<CharT, has_charconv_for<T>>) {
        return from_chars_impl<T>{data}(value);
    }
    else
#endif
    {
#if !SCN_DISABLE_STRTOD
        return strtod_impl<CharT, T>{data}(value);
#else
        return unexpected_scan_error(
            scan_error::invalid_scanned_value,
            "fast_float failed, and fallbacks are disabled");
#endif
    }
}

struct fast_float_impl_base : impl_base {
    fast_float::chars_format get_flags() const
    {
        unsigned format_flags{};
        if ((m_options & float_reader_base::allow_fixed) != 0) {
            format_flags |= fast_float::fixed;
        }
        if ((m_options & float_reader_base::allow_scientific) != 0) {
            format_flags |= fast_float::scientific;
        }

        return static_cast<fast_float::chars_format>(format_flags);
    }
};

template <typename CharT, typename T>
struct fast_float_impl : fast_float_impl_base {
    fast_float_impl(impl_init_data<CharT> data)
        : fast_float_impl_base{data.base()}, m_input(data.input)
    {
    }

    scan_expected<std::ptrdiff_t> operator()(T& value) const
    {
        if (m_kind == float_reader_base::float_kind::hex_without_prefix ||
            m_kind == float_reader_base::float_kind::hex_with_prefix) {
            // fast_float doesn't support hexfloats
            return fast_float_fallback<CharT>({m_input, m_kind, m_options},
                                              value);
        }

        const auto flags = get_flags();
        const auto view = get_view();
        const auto result = fast_float::from_chars(
            view.data(), view.data() + view.size(), value, flags);

        if (SCN_UNLIKELY(result.ec == std::errc::invalid_argument)) {
            return unexpected_scan_error(scan_error::invalid_scanned_value,
                                         "fast_float: invalid_argument");
        }
        if (SCN_UNLIKELY(result.ec == std::errc::result_out_of_range)) {
            // may just be very large: fall back
            return fast_float_fallback<CharT>({m_input, m_kind, m_options},
                                              value);
        }

        return result.ptr - view.data();
    }

private:
    auto get_view() const
    {
        if constexpr (get_encoding<CharT>() == encoding::utf8) {
            return m_input.view();
        }
        else if constexpr (get_encoding<CharT>() == encoding::utf16) {
            return std::u16string_view{
                reinterpret_cast<const char16_t*>(m_input.view().data()),
                m_input.view().size()};
        }
        else {
            return std::u32string_view{
                reinterpret_cast<const char32_t*>(m_input.view().data()),
                m_input.view().size()};
        }
    }

    contiguous_range_factory<CharT>& m_input;
};

////////////////////////////////////////////////////////////////////
// Dispatch implementation
////////////////////////////////////////////////////////////////////

template <typename CharT, typename T>
scan_expected<std::ptrdiff_t> dispatch_impl(
    impl_init_data<CharT> data,
    contiguous_range_factory<CharT>& nan_payload,
    T& value)
{
    if (data.kind == float_reader_base::float_kind::inf_short) {
        value = std::numeric_limits<T>::infinity();
        return 3;
    }
    if (data.kind == float_reader_base::float_kind::inf_long) {
        value = std::numeric_limits<T>::infinity();
        return 8;
    }
    if (data.kind == float_reader_base::float_kind::nan_simple) {
        value = std::numeric_limits<T>::quiet_NaN();
        return 3;
    }
    if (data.kind == float_reader_base::float_kind::nan_with_payload) {
        value = std::numeric_limits<T>::quiet_NaN();

        // TODO: use payload
#if 0
                    {
                        auto reader = integer_reader<CharT>{
                            integer_reader_base::only_unsigned, 0};
                        if (auto r = reader.read_source(
                                detail::tag_type<unsigned long long>{},
                                nan_payload.view());
                            SCN_UNLIKELY(!r)) {
                            return unexpected(r.error());
                        }

                        unsigned long long payload;
                        if (auto r = reader.parse_value(payload);
                            SCN_UNLIKELY(!r)) {
                            return unexpected(r.error());
                        }

                        constexpr auto mantissa_payload_len =
                            std::numeric_limits<T>::digits - 2;
                        payload &= ((1ull << mantissa_payload_len) - 1ull);


                    }
#endif
        SCN_UNUSED(nan_payload);

        return static_cast<std::ptrdiff_t>(5 + nan_payload.view().size());
    }

    SCN_EXPECT(!data.input.view().empty());
    if (data.kind == float_reader_base::float_kind::hex_without_prefix) {
        if (SCN_UNLIKELY(char_to_int(data.input.view().front()) >= 16)) {
            return unexpected_scan_error(scan_error::invalid_scanned_value,
                                         "Invalid floating-point digit");
        }
    }
    if (SCN_UNLIKELY(char_to_int(data.input.view().front()) >= 10)) {
        return unexpected_scan_error(scan_error::invalid_scanned_value,
                                     "Invalid floating-point digit");
    }

    if constexpr (std::is_same_v<T, long double>) {
        if constexpr (sizeof(double) == sizeof(long double)) {
            // If double == long double (true on Windows),
            // use fast_float with double
            double tmp{};
            auto ret = fast_float_impl<CharT, double>{data}(tmp);
            value = tmp;
            return ret;
        }
        else {
            // long doubles aren't supported by fast_float ->
            // fall back to from_chars or strtod
            return fast_float_fallback(data, value);
        }
    }
    else {
        // Default to fast_float
        return fast_float_impl<CharT, T>{data}(value);
    }
}
}  // namespace

template <typename CharT>
template <typename T>
scan_expected<std::ptrdiff_t> float_reader<CharT>::parse_value_impl(T& value)
{
    auto n = dispatch_impl<CharT>({this->m_buffer, m_kind, m_options},
                                  m_nan_payload_buffer, value);
    value = this->setsign(value);
    return n;
}

#define SCN_DEFINE_FLOAT_READER_TEMPLATE(CharT, FloatT)          \
    template auto float_reader<CharT>::parse_value_impl(FloatT&) \
        -> scan_expected<std::ptrdiff_t>;

#if !SCN_DISABLE_TYPE_FLOAT
SCN_DEFINE_FLOAT_READER_TEMPLATE(char, float)
SCN_DEFINE_FLOAT_READER_TEMPLATE(wchar_t, float)
#endif
#if !SCN_DISABLE_TYPE_DOUBLE
SCN_DEFINE_FLOAT_READER_TEMPLATE(char, double)
SCN_DEFINE_FLOAT_READER_TEMPLATE(wchar_t, double)
#endif
#if !SCN_DISABLE_TYPE_LONG_DOUBLE
SCN_DEFINE_FLOAT_READER_TEMPLATE(char, long double)
SCN_DEFINE_FLOAT_READER_TEMPLATE(wchar_t, long double)
#endif

#undef SCN_DEFINE_FLOAT_READER_TEMPLATE
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
