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

#include <scn/impl.h>

#include <locale>

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

#if SCN_HAS_FLOAT_CHARCONV
#include <charconv>
#endif

#define SCN_XLOCALE_POSIX    0
#define SCN_XLOCALE_MSVC     1
#define SCN_XLOCALE_OTHER    2
#define SCN_XLOCALE_DISABLED 3

#if SCN_DISABLE_LOCALE
#define SCN_XLOCALE SCN_XLOCALE_DISABLED
#elif (!defined(__ANDROID_API__) || __ANDROID_API__ >= 28) && \
    SCN_HAS_INCLUDE(<xlocale.h>)
#include <xlocale.h>
#define SCN_XLOCALE SCN_XLOCALE_POSIX

#elif defined(_MSC_VER)
#define SCN_XLOCALE SCN_XLOCALE_MSVC

#elif defined(__GLIBC__) && !defined(__ANDROID_API__)
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

        for (size_t i = 0; i < sv.size(); ++i) {
            auto tmp =
                detail::make_string_view_from_iterators<char>(it, source.end());
            auto res = get_next_code_point(tmp);
            if (cp_cb(res.value)) {
                return it;
            }
            i += ranges::distance(tmp.data(), detail::to_address(res.iterator));
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

std::string_view::iterator find_classic_space_narrow_fast(
    std::string_view source)
{
    return find_classic_impl(
        source, [](char ch) { return is_ascii_space(ch); },
        [](char32_t cp) { return detail::is_cp_space(cp); });
}

std::string_view::iterator find_classic_nonspace_narrow_fast(
    std::string_view source)
{
    return find_classic_impl(
        source, [](char ch) { return !is_ascii_space(ch); },
        [](char32_t cp) { return !detail::is_cp_space(cp); });
}

std::string_view::iterator find_nondecimal_digit_narrow_fast(
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

#define SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(T, Context)                         \
    template scan_expected<Context::iterator> scanner_scan_for_builtin_type( \
        T&, Context&, const format_specs&);

#define SCN_DEFINE_SCANNER_SCAN_FOR_CTX(Context)                    \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(Context::char_type, Context)   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(signed char, Context)          \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(short, Context)                \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(int, Context)                  \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long, Context)                 \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long long, Context)            \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned char, Context)        \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned short, Context)       \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned int, Context)         \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned long, Context)        \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned long long, Context)   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(float, Context)                \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(double, Context)               \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long double, Context)          \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::string, Context)          \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::wstring, Context)         \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::string_view, Context)     \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::wstring_view, Context)    \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(regex_matches, Context)        \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(wregex_matches, Context)       \
    template scan_expected<ranges::iterator_t<Context::range_type>> \
    internal_skip_classic_whitespace(Context::range_type, bool);

SCN_DEFINE_SCANNER_SCAN_FOR_CTX(scan_context)
SCN_DEFINE_SCANNER_SCAN_FOR_CTX(wscan_context)

/////////////////////////////////////////////////////////////////
// scan_buffer implementations
/////////////////////////////////////////////////////////////////

namespace {
struct file_wrapper_impl_base {
private:
    static auto fgetc_impl(std::FILE* file)
    {
#if SCN_POSIX
        return getc_unlocked(file);
#elif SCN_WINDOWS
        return _fgetc_nolock(file);
#else
        return std::fgetc(file);
#endif
    }

    static auto ungetc_impl(std::FILE* file, int ch)
    {
#if SCN_WINDOWS && !SCN_MINGW
        return _ungetc_nolock(ch, file);
#else
        return std::ungetc(ch, file);
#endif
    }

public:
    static void lock(std::FILE* file)
    {
#if SCN_POSIX
        ::flockfile(file);
#elif SCN_WINDOWS
        ::_lock_file(file);
#else
        SCN_UNUSED(file);
#endif
    }
    static void unlock(std::FILE* file)
    {
#if SCN_POSIX
        ::funlockfile(file);
#elif SCN_WINDOWS
        ::_unlock_file(file);
#else
        SCN_UNUSED(file);
#endif
    }

    static void lock_for_unget(std::FILE* file)
    {
#if SCN_WINDOWS && !SCN_MINGW
        SCN_UNUSED(file);
#else
        lock(file);
#endif
    }

    static void unlock_for_unget(std::FILE* file)
    {
#if SCN_WINDOWS && !SCN_MINGW
        SCN_UNUSED(file);
#else
        unlock(file);
#endif
    }

    static std::optional<char> read(std::FILE* file)
    {
        auto res = fgetc_impl(file);
        if (res == EOF) {
            return std::nullopt;
        }
        return static_cast<char>(res);
    }

    static void unget(std::FILE* file, char ch)
    {
        auto res = ungetc_impl(file, static_cast<unsigned char>(ch));
        SCN_ENSURE(res != EOF);
    }

    static std::optional<char> peek(std::FILE* file)
    {
        if (auto res = read(file); res) {
            unget(file, *res);
            return res;
        }
        return std::nullopt;
    }
};

struct default_file_tag {};
struct gnu_file_tag {};
struct bsd_file_tag {};

template <typename F, typename Tag>
struct file_wrapper_impl;

template <typename F>
struct file_wrapper_impl<F, default_file_tag> : file_wrapper_impl_base {
    constexpr static std::string_view get_current_buffer(F*)
    {
        return {};
    }

    constexpr static bool has_buffering()
    {
        return false;
    }

    static bool fill_buffer(F*)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    constexpr static void unsafe_advance_to_buffer_end(F*) {}

    static void unsafe_advance_n(F*, std::ptrdiff_t)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
};

// GNU libc (Linux)
template <typename F>
struct file_wrapper_impl<F, gnu_file_tag> : file_wrapper_impl_base {
    static std::string_view get_current_buffer(F* file)
    {
        return make_string_view_from_pointers(file->_IO_read_ptr,
                                              file->_IO_read_end);
    }

    constexpr static bool has_buffering()
    {
        return true;
    }

    static bool fill_buffer(F* file)
    {
        return peek(file).has_value();
    }

    static void unsafe_advance_to_buffer_end(F* file)
    {
        SCN_EXPECT(file->_IO_read_ptr && file->_IO_read_end);
        file->_IO_read_ptr = file->_IO_read_end;
    }

    static void unsafe_advance_n(F* file, std::ptrdiff_t n)
    {
        SCN_EXPECT(file->_IO_read_ptr);
        SCN_EXPECT(file->_IO_read_end - file->_IO_read_ptr >= n);
        file->_IO_read_ptr += n;
    }

    static std::optional<char> peek(F* file)
    {
        if (file->_IO_read_ptr != file->_IO_read_end) {
            return file->_IO_read_ptr[0];
        }
        if (auto res = read(file); res) {
            --file->_IO_read_ptr;
            return res;
        }
        return std::nullopt;
    }
};

// BSD libc (Apple)
template <typename F>
struct file_wrapper_impl<F, bsd_file_tag> : file_wrapper_impl_base {
    static std::string_view get_current_buffer(F* file)
    {
        return {reinterpret_cast<const char*>(file->_p),
                static_cast<std::size_t>(file->_r)};
    }

    constexpr static bool has_buffering()
    {
        return true;
    }

    static bool fill_buffer(F* file)
    {
        return peek(file).has_value();
    }

    static void unsafe_advance_to_buffer_end(F* file)
    {
        SCN_EXPECT(file->_p != nullptr);
        file->_p += file->_r;
        file->_r = 0;
    }

    static void unsafe_advance_n(F* file, std::ptrdiff_t n)
    {
        SCN_EXPECT(file->_p != nullptr);
        SCN_EXPECT(file->_r >= n);
        file->_p += n;
        file->_r -= n;
    }

    static std::optional<char> peek(F* file)
    {
        if (file->_p != nullptr && file->_r != 0) {
            return static_cast<char>(*file->_p);
        }
        if (auto res = read(file); res) {
            --file->_p;
            ++file->_r;
            return res;
        }
        return std::nullopt;
    }
};

// Non-pretty workaround for MSVC silliness
template <typename F, typename = void>
inline constexpr bool is_gnu_file = false;
template <typename F>
inline constexpr bool
    is_gnu_file<F,
                std::void_t<decltype(SCN_DECLVAL(F)._IO_read_ptr),
                            decltype(SCN_DECLVAL(F)._IO_read_end)>> = true;

template <typename F, typename = void>
inline constexpr bool is_bsd_file = false;
template <typename F>
inline constexpr bool is_bsd_file<
    F,
    std::void_t<decltype(SCN_DECLVAL(F)._p), decltype(SCN_DECLVAL(F)._r)>> =
    true;

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wunneeded-internal-declaration")

constexpr auto get_file_tag()
{
    if constexpr (is_gnu_file<std::FILE>) {
        return detail::tag_type<gnu_file_tag>{};
    }
    else if constexpr (is_bsd_file<std::FILE>) {
        return detail::tag_type<bsd_file_tag>{};
    }
    else {
        return detail::tag_type<default_file_tag>{};
    }
}

using file_wrapper =
    file_wrapper_impl<std::FILE, decltype(get_file_tag())::type>;

SCN_CLANG_POP

bool fill_with_buffering(std::FILE* file, std::string_view& current_view)
{
    SCN_EXPECT(file_wrapper::has_buffering());

    if (!current_view.empty()) {
        file_wrapper::unsafe_advance_to_buffer_end(file);
    }

    if (!file_wrapper::fill_buffer(file)) {
        current_view = {};
        return false;
    }

    current_view = file_wrapper::get_current_buffer(file);
    return true;
}

bool fill_without_buffering(std::FILE* file,
                            std::string_view& current_view,
                            std::optional<char>& latest)
{
    latest = file_wrapper::read(file);
    if (!latest) {
        current_view = {};
        return false;
    }
    current_view = {&*latest, 1};
    return true;
}
}  // namespace

scan_file_buffer::scan_file_buffer(std::FILE* file)
    : base(base::non_contiguous_tag{}), m_file(file)
{
    file_wrapper::lock(file);
}

scan_file_buffer::~scan_file_buffer()
{
    file_wrapper::unlock(m_file);
}

bool scan_file_buffer::fill()
{
    SCN_EXPECT(m_file);

    if (!this->m_current_view.empty()) {
        this->m_putback_buffer.insert(this->m_putback_buffer.end(),
                                      this->m_current_view.begin(),
                                      this->m_current_view.end());
    }

    if (file_wrapper::has_buffering()) {
        return fill_with_buffering(m_file, this->m_current_view);
    }

    return fill_without_buffering(m_file, this->m_current_view, this->m_latest);
}

namespace {
struct file_unlocker_for_unget {
    file_unlocker_for_unget(std::FILE* f) : file(f)
    {
        file_wrapper::unlock_for_unget(file);
    }
    ~file_unlocker_for_unget()
    {
        file_wrapper::lock_for_unget(file);
    }

    std::FILE* file;
};
}  // namespace

void scan_file_buffer::sync(std::ptrdiff_t position)
{
    SCN_EXPECT(m_file);

    if (file_wrapper::has_buffering()) {
        if (position <
            static_cast<std::ptrdiff_t>(this->putback_buffer().size())) {
            file_unlocker_for_unget unlocker{m_file};
            auto putback_segment = this->get_segment_starting_at(position);
            for (auto rit = putback_segment.rbegin();
                 rit != putback_segment.rend(); ++rit) {
                file_wrapper::unget(m_file, *rit);
            }
            return;
        }

        file_wrapper::unsafe_advance_n(
            m_file, position - static_cast<std::ptrdiff_t>(
                                   this->putback_buffer().size()));
        return;
    }

    const auto chars_avail = this->chars_available();
    if (position == chars_avail) {
        return;
    }

    file_unlocker_for_unget unlocker{m_file};
    SCN_EXPECT(m_current_view.size() == 1);
    file_wrapper::unget(m_file, m_current_view.front());

    auto putback_segment =
        std::string_view{this->putback_buffer()}.substr(position);
    for (auto rit = putback_segment.rbegin(); rit != putback_segment.rend();
         ++rit) {
        file_wrapper::unget(m_file, *rit);
    }
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

template locale_ref::locale_ref(const std::locale&);
template auto locale_ref::get() const -> std::locale;
}  // namespace detail

#endif

namespace detail {
scan_error handle_error(scan_error e)
{
    return e;
}
}  // namespace detail

/////////////////////////////////////////////////////////////////
// Floating-point reader implementation
/////////////////////////////////////////////////////////////////

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
        if constexpr (sizeof(CharT) == 1) {
            return m_input.view();
        }
        else if constexpr (sizeof(CharT) == 2) {
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

/////////////////////////////////////////////////////////////////
// Integer reader implementation
/////////////////////////////////////////////////////////////////

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

SCN_FORCE_INLINE constexpr size_t min_safe_u64(int base)
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

template <typename T>
constexpr T store_result(uint64_t u64val, bool is_negative)
{
    if (is_negative) {
        SCN_MSVC_PUSH
        SCN_MSVC_IGNORE(4146)
        return static_cast<T>(
            -std::numeric_limits<T>::max() -
            static_cast<T>(u64val - std::numeric_limits<T>::max()));
        SCN_MSVC_POP
    }

    return static_cast<T>(u64val);
}

template <typename T>
auto parse_decimal_integer_fast(std::string_view input,
                                T& val,
                                bool is_negative) -> scan_expected<const char*>
{
    uint64_t u64val{};
    auto ptr = parse_decimal_integer_fast_impl(
        input.data(), input.data() + input.size(), u64val);

    auto digits_count = static_cast<size_t>(ptr - input.data());
    if (SCN_UNLIKELY(
            check_integer_overflow<T>(u64val, digits_count, 10, is_negative))) {
        return unexpected_scan_error(scan_error::value_out_of_range,
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
        return unexpected_scan_error(scan_error::value_out_of_range,
                                     "Integer overflow");
    }

    val = store_result<T>(u64val, is_negative);
    return begin;
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
        return unexpected_scan_error(scan_error::invalid_scanned_value,
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
            value = 0;
            return ranges::next(source.begin(),
                                ranges::distance(source.data(), start));
        }
    }

    if constexpr (std::is_same_v<CharT, char>) {
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

#define SCN_DEFINE_INTEGER_READER_TEMPLATE(CharT, IntT)                      \
    template auto parse_integer_value(std::basic_string_view<CharT> source,  \
                                      IntT& value, sign_type sign, int base) \
        -> scan_expected<typename std::basic_string_view<CharT>::iterator>;

#if !SCN_DISABLE_TYPE_SCHAR
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, signed char)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, signed char)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   signed char&);
#endif
#if !SCN_DISABLE_TYPE_SHORT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, short)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, short)
template void parse_integer_value_exhaustive_valid(std::string_view, short&);
#endif
#if !SCN_DISABLE_TYPE_INT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, int)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, int)
template void parse_integer_value_exhaustive_valid(std::string_view, int&);
#endif
#if !SCN_DISABLE_TYPE_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, long)
template void parse_integer_value_exhaustive_valid(std::string_view, long&);
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, long long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, long long)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   long long&);
#endif
#if !SCN_DISABLE_TYPE_UCHAR
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned char)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned char)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned char&);
#endif
#if !SCN_DISABLE_TYPE_USHORT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned short)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned short)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned short&);
#endif
#if !SCN_DISABLE_TYPE_UINT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned int)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned int)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned int&);
#endif
#if !SCN_DISABLE_TYPE_ULONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned long&);
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned long long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long long)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned long long&);
#endif

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
    basic_scan_args<basic_scan_context<CharT>> args,
    basic_scan_arg<basic_scan_context<CharT>> arg,
    detail::locale_ref loc = {})
{
    if (SCN_UNLIKELY(!arg)) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Argument #0 not found");
    }

    auto reader =
        impl::default_arg_reader<impl::basic_contiguous_scan_context<CharT>>{
            ranges::subrange<const CharT*>{source.data(),
                                           source.data() + source.size()},
            SCN_MOVE(args), loc};
    SCN_TRY(it, visit_scan_arg(SCN_MOVE(reader), arg));
    return ranges::distance(source.data(), it);
}
template <typename CharT>
scan_expected<std::ptrdiff_t> scan_simple_single_argument(
    detail::basic_scan_buffer<CharT>& source,
    basic_scan_args<basic_scan_context<CharT>> args,
    basic_scan_arg<basic_scan_context<CharT>> arg,
    detail::locale_ref loc = {})
{
    if (SCN_UNLIKELY(!arg)) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Argument #0 not found");
    }

    if (SCN_LIKELY(source.is_contiguous())) {
        auto reader = impl::default_arg_reader<
            impl::basic_contiguous_scan_context<CharT>>{source.get_contiguous(),
                                                        SCN_MOVE(args), loc};
        SCN_TRY(it, visit_scan_arg(SCN_MOVE(reader), arg));
        return ranges::distance(source.get_contiguous().begin(), it);
    }

    auto reader = impl::default_arg_reader<basic_scan_context<CharT>>{
        source.get(), SCN_MOVE(args), loc};
    SCN_TRY(it, visit_scan_arg(SCN_MOVE(reader), arg));
    return it.position();
}

template <typename Context, typename ID, typename Handler>
auto get_arg(Context& ctx, ID id, Handler& handler) ->
    typename Context::arg_type
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
    using arg_type = typename Context::arg_type;

    constexpr specs_handler(detail::format_specs& specs,
                            basic_scan_parse_context<char_type>& parse_ctx,
                            Context& ctx)
        : detail::specs_setter(specs), m_parse_ctx(parse_ctx), m_ctx(ctx)
    {
    }

private:
    constexpr arg_type get_arg(auto_id)
    {
        return get_arg(m_ctx, m_parse_ctx.next_arg_id(), *this);
    }

    constexpr arg_type get_arg(std::size_t arg_id)
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
                return on_error("Argument list not exhausted");
            }
        }

        if (args_count < 64) {
            return;
        }

        auto last_args_count = args_count - 64;
        for (auto it = visited_args_upper.begin();
             it != visited_args_upper.end() - 1; ++it) {
            if (*it != std::numeric_limits<uint8_t>::max()) {
                return on_error("Argument list not exhausted");
            }
            last_args_count -= 8;
        }

        const auto mask = static_cast<uint8_t>(1u << last_args_count) - 1;
        if (visited_args_upper.back() != mask) {
            return on_error("Argument list not exhausted");
        }
    }

    void on_error(const char* msg)
    {
        SCN_UNLIKELY_ATTR
        error = scan_error{scan_error::invalid_format_string, msg};
    }
    void on_error(scan_error err)
    {
        if (SCN_UNLIKELY(err != scan_error::good)) {
            error = err;
        }
    }

    explicit constexpr operator bool() const
    {
        return static_cast<bool>(error);
    }
    SCN_NODISCARD scan_error get_error() const
    {
        return error;
    }

    SCN_NODISCARD bool has_arg_been_visited(size_t id)
    {
        if (SCN_UNLIKELY(id >= args_count)) {
            on_error("Invalid out-of-range argument ID");
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
            on_error("Invalid out-of-range argument ID");
            return;
        }

        if (SCN_UNLIKELY(has_arg_been_visited(id))) {
            return on_error("Argument with this ID has already been scanned");
        }

        if (SCN_LIKELY(id < 64)) {
            visited_args_lower64 |= (1ull << id);
            return;
        }

        id -= 64;
        visited_args_upper[id / 8] |= (1ull << (id % 8));
    }

    std::size_t args_count;
    scan_error error{};
    uint64_t visited_args_lower64{0};
    std::vector<uint8_t> visited_args_upper{};
};

template <typename CharT>
struct simple_context_wrapper {
    using context_type = basic_scan_context<CharT>;

    simple_context_wrapper(detail::basic_scan_buffer<CharT>& source,
                           basic_scan_args<basic_scan_context<CharT>> args,
                           detail::locale_ref loc)
        : ctx(source.get().begin(), SCN_MOVE(args), loc)
    {
    }

    basic_scan_context<CharT>& get()
    {
        return ctx;
    }
    basic_scan_context<CharT>& get_custom()
    {
        return ctx;
    }

    basic_scan_context<CharT> ctx;
};

template <typename CharT>
struct contiguous_context_wrapper {
    using context_type = impl::basic_contiguous_scan_context<CharT>;

    contiguous_context_wrapper(ranges::subrange<const CharT*> source,
                               basic_scan_args<basic_scan_context<CharT>> args,
                               detail::locale_ref loc)
        : contiguous_ctx(source, args, loc)
    {
    }

    impl::basic_contiguous_scan_context<CharT>& get()
    {
        return contiguous_ctx;
    }
    basic_scan_context<CharT>& get_custom()
    {
        if (!buffer) {
            buffer.emplace(detail::make_string_view_from_pointers(
                ranges::data(contiguous_ctx.underlying_range()),
                ranges::data(contiguous_ctx.underlying_range()) +
                    ranges::size(contiguous_ctx.underlying_range())));
        }
        auto it = buffer->get().begin();
        it.batch_advance_to(contiguous_ctx.begin_position());
        custom_ctx.emplace(it, contiguous_ctx.args(), contiguous_ctx.locale());
        return *custom_ctx;
    }

    impl::basic_contiguous_scan_context<CharT> contiguous_ctx;
    std::optional<detail::basic_scan_string_buffer<CharT>> buffer{std::nullopt};
    std::optional<basic_scan_context<CharT>> custom_ctx{std::nullopt};
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
    using args_type = basic_scan_args<basic_scan_context<char_type>>;

    template <typename Source>
    format_handler(Source&& source,
                   format_type format,
                   args_type args,
                   detail::locale_ref loc,
                   std::size_t argcount)
        : format_handler_base{argcount},
          parse_ctx{format},
          ctx{SCN_FWD(source), SCN_MOVE(args), SCN_MOVE(loc)}
    {
    }

    void on_literal_text(const char_type* begin, const char_type* end)
    {
        for (; begin != end; ++begin) {
            auto it = get_ctx().begin();
            if (impl::is_range_eof(it, get_ctx().end())) {
                SCN_UNLIKELY_ATTR
                return on_error("Unexpected end of source");
            }

            if (auto [after_space_it, cp, is_space] = impl::is_first_char_space(
                    detail::make_string_view_from_pointers(begin, end));
                cp == detail::invalid_code_point) {
                SCN_UNLIKELY_ATTR
                return on_error("Invalid encoding in format string");
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
                return on_error("Unexpected literal character in source");
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
    void on_visit_scan_arg(Visitor&& visitor,
                           typename context_type::arg_type arg)
    {
        if (!*this || !arg) {
            SCN_UNLIKELY_ATTR
            return;
        }

        auto r = visit_scan_arg(SCN_FWD(visitor), arg);
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

        if (arg.type() == detail::arg_type::custom_type) {
            parse_ctx.advance_to(begin);
            on_visit_scan_arg(
                impl::custom_reader<basic_scan_context<char_type>>{
                    parse_ctx, get_custom_ctx()},
                arg);
            return parse_ctx.begin();
        }

        auto specs = detail::format_specs{};
        detail::specs_checker<specs_handler<context_type>> handler{
            specs_handler<context_type>{specs, parse_ctx, get_ctx()},
            arg.type()};

        begin = detail::parse_format_specs(begin, end, handler);
        if (begin == end || *begin != char_type{'}'}) {
            SCN_UNLIKELY_ATTR
            on_error("Missing '}' in format string");
            return parse_ctx.begin();
        }
        if (SCN_UNLIKELY(!handler)) {
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
    if (SCN_UNLIKELY(!handler)) {
        return unexpected(handler.error);
    }
    return ranges::distance(beg, handler.get_ctx().begin());
}

template <typename CharT>
scan_expected<std::ptrdiff_t> vscan_internal(
    std::basic_string_view<CharT> source,
    std::basic_string_view<CharT> format,
    basic_scan_args<basic_scan_context<CharT>> args,
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
    detail::basic_scan_buffer<CharT>& buffer,
    std::basic_string_view<CharT> format,
    basic_scan_args<basic_scan_context<CharT>> args,
    detail::locale_ref loc = {})
{
    const auto argcount = args.size();
    if (is_simple_single_argument_format_string(format) && argcount == 1) {
        auto arg = args.get(0);
        return scan_simple_single_argument(buffer, SCN_MOVE(args), arg);
    }

    if (buffer.is_contiguous()) {
        auto handler = format_handler<true, CharT>{buffer.get_contiguous(),
                                                   format, SCN_MOVE(args),
                                                   SCN_MOVE(loc), argcount};
        return vscan_parse_format_string(format, handler);
    }

    SCN_UNLIKELY_ATTR
    {
        auto handler = format_handler<false, CharT>{
            buffer, format, SCN_MOVE(args), SCN_MOVE(loc), argcount};
        return vscan_parse_format_string(format, handler);
    }
}

template <typename Source, typename CharT>
scan_expected<std::ptrdiff_t> vscan_value_internal(
    Source&& source,
    basic_scan_arg<basic_scan_context<CharT>> arg)
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

scan_error vinput(std::string_view format, scan_args args)
{
    auto buffer = detail::make_file_scan_buffer(stdin);
    auto n = vscan_internal(buffer, format, args);
    if (n) {
        buffer.sync(*n);
        return {};
    }
    buffer.sync_all();
    return n.error();
}

namespace detail {
scan_expected<std::ptrdiff_t> vscan_impl(std::string_view source,
                                         std::string_view format,
                                         scan_args args)
{
    return vscan_internal(source, format, args);
}
scan_expected<std::ptrdiff_t> vscan_impl(scan_buffer& source,
                                         std::string_view format,
                                         scan_args args)
{
    auto n = vscan_internal(source, format, args);
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
}

scan_expected<std::ptrdiff_t> vscan_impl(std::wstring_view source,
                                         std::wstring_view format,
                                         wscan_args args)
{
    return vscan_internal(source, format, args);
}
scan_expected<std::ptrdiff_t> vscan_impl(wscan_buffer& source,
                                         std::wstring_view format,
                                         wscan_args args)
{
    auto n = vscan_internal(source, format, args);
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
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
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   scan_buffer& source,
                                                   std::string_view format,
                                                   scan_args args)
{
    auto n = vscan_internal(source, format, args, detail::locale_ref{loc});
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
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
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   wscan_buffer& source,
                                                   std::wstring_view format,
                                                   wscan_args args)
{
    auto n = vscan_internal(source, format, args, detail::locale_ref{loc});
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
}

template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                std::string_view,
                                                std::string_view,
                                                scan_args)
    -> scan_expected<std::ptrdiff_t>;
template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                scan_buffer&,
                                                std::string_view,
                                                scan_args)
    -> scan_expected<std::ptrdiff_t>;
template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                std::wstring_view,
                                                std::wstring_view,
                                                wscan_args)
    -> scan_expected<std::ptrdiff_t>;
template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                wscan_buffer&,
                                                std::wstring_view,
                                                wscan_args)
    -> scan_expected<std::ptrdiff_t>;
#endif

scan_expected<std::ptrdiff_t> vscan_value_impl(std::string_view source,
                                               basic_scan_arg<scan_context> arg)
{
    return vscan_value_internal(source, arg);
}
scan_expected<std::ptrdiff_t> vscan_value_impl(scan_buffer& source,
                                               basic_scan_arg<scan_context> arg)
{
    auto n = vscan_value_internal(source, arg);
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
}

scan_expected<std::ptrdiff_t> vscan_value_impl(
    std::wstring_view source,
    basic_scan_arg<wscan_context> arg)
{
    return vscan_value_internal(source, arg);
}
scan_expected<std::ptrdiff_t> vscan_value_impl(
    wscan_buffer& source,
    basic_scan_arg<wscan_context> arg)
{
    auto n = vscan_value_internal(source, arg);
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
}

#if !SCN_DISABLE_TYPE_SCHAR
template auto scan_int_impl(std::string_view, signed char&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> signed char;
#endif
#if !SCN_DISABLE_TYPE_SHORT
template auto scan_int_impl(std::string_view, short&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> short;
#endif
#if !SCN_DISABLE_TYPE_INT
template auto scan_int_impl(std::string_view, int&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> int;
#endif
#if !SCN_DISABLE_TYPE_LONG
template auto scan_int_impl(std::string_view, long&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> long;
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
template auto scan_int_impl(std::string_view, long long&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> long long;
#endif
#if !SCN_DISABLE_TYPE_UCHAR
template auto scan_int_impl(std::string_view, unsigned char&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> unsigned char;
#endif
#if !SCN_DISABLE_TYPE_USHORT
template auto scan_int_impl(std::string_view, unsigned short&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned short;
#endif
#if !SCN_DISABLE_TYPE_UINT
template auto scan_int_impl(std::string_view, unsigned int&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> unsigned int;
#endif
#if !SCN_DISABLE_TYPE_ULONG
template auto scan_int_impl(std::string_view, unsigned long&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> unsigned long;
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
template auto scan_int_impl(std::string_view, unsigned long long&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned long long;
#endif

}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
