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

#include <scn/impl/reader/float/value_reader.h>

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

#if SCN_CLANG >= SCN_COMPILER(8, 0, 0)
SCN_CLANG_IGNORE("-Wextra-semi-stmt")
#endif

#if SCN_CLANG >= SCN_COMPILER(13, 0, 0)
SCN_CLANG_IGNORE("-Wreserved-identifier")
#endif

SCN_GCC_COMPAT_IGNORE("-Wundef")

#define FASTFLOAT_SKIP_WHITE_SPACE    0
#define FASTFLOAT_ALLOWS_LEADING_PLUS 1

#include <fast_float/fast_float.h>

SCN_CLANG_POP
SCN_GCC_POP

#include <limits>
#include <sstream>

#if SCN_HAS_FLOAT_CHARCONV
#include <charconv>
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        namespace {
            template <typename CharT>
            constexpr bool is_hexfloat(std::basic_string_view<CharT> str)
                SCN_NOEXCEPT
            {
                if (str.size() < 3) {
                    return false;
                }
                if (str[0] == CharT{'-'}) {
                    str = str.substr(1);
                }
                return str[0] == CharT{'0'} &&
                       (str[1] == CharT{'x'} || str[1] == CharT{'X'});
            }

            template <typename CharT>
            bool is_input_inf(std::basic_string_view<CharT> str)
            {
                if (str.empty()) {
                    return false;
                }
                if (str[0] == CharT{'-'}) {
                    str = str.substr(1);
                }

                if (str.size() >= 3) {
                    if ((str[0] == CharT{'i'} || str[0] == CharT{'I'}) &&
                        (str[1] == CharT{'n'} || str[1] == CharT{'N'}) &&
                        (str[2] == CharT{'f'} || str[2] == CharT{'F'})) {
                        return true;
                    }
                }
                return false;
            }

            template <typename CharT>
            bool _skip_zeroes(
                std::basic_string_view<CharT> str,
                typename std::basic_string_view<CharT>::iterator& it)
            {
                for (; it != str.end(); ++it) {
                    if (*it == CharT{'0'}) {
                        continue;
                    }
                    if (*it == CharT{'.'} || *it == CharT{'e'} ||
                        *it == CharT{'E'} || *it == CharT{'p'} ||
                        *it == CharT{'P'}) {
                        break;
                    }
                    return false;
                }
                return true;
            }

            template <typename CharT>
            bool is_input_hexzero(std::basic_string_view<CharT> str)
            {
                if (str[0] == CharT{'-'}) {
                    str = str.substr(3);
                }
                else {
                    str = str.substr(2);
                }

                auto it = str.begin();
                if (!_skip_zeroes(str, it)) {
                    return false;
                }

                if (it == str.end() || *it != CharT{'.'}) {
                    return true;
                }
                ++it;
                if (it == str.end()) {
                    return true;
                }
                if (!_skip_zeroes(str, it)) {
                    return false;
                }
                return true;
            }

            template <typename CharT>
            bool is_input_zero(std::basic_string_view<CharT> str)
            {
                if (is_hexfloat(str)) {
                    return is_input_hexzero(str);
                }

                if (str.empty()) {
                    return false;
                }
                if (str[0] == CharT{'-'}) {
                    str = str.substr(1);
                }

                auto it = str.begin();
                if (!_skip_zeroes(str, it)) {
                    return false;
                }
                if (it == str.end() || *it != CharT{'.'}) {
                    return true;
                }
                ++it;

                if (!_skip_zeroes(str, it)) {
                    return false;
                }
                return true;
            }

            SCN_GCC_COMPAT_PUSH
            SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")
            constexpr bool is_float_zero(float f)
            {
                return f == 0.0f;
            }
            constexpr bool is_float_zero(double d)
            {
                return d == 0.0;
            }
            constexpr bool is_float_zero(long double ld)
            {
                return ld == 0.0L;
            }

            template <typename F>
            constexpr bool is_float_max(F f)
            {
                return f == std::numeric_limits<F>::max();
            }
            SCN_GCC_COMPAT_POP
        }  // namespace

        ////////////////////////////////////////////////////////////////////////
        // C standard library based implementation
        // Fallback for all CharT and FloatT
        ////////////////////////////////////////////////////////////////////////

        namespace {
            struct float_classic_value_reader_cstd_impl_base {
                const float_value_reader_base& reader;
            };
        }  // namespace

        template <typename CharT, typename T>
        class cstd_reader_impl
            : public float_classic_value_reader_cstd_impl_base {
        public:
            explicit cstd_reader_impl(const float_value_reader_base& r)
                : float_classic_value_reader_cstd_impl_base{r}
            {
            }

            scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>
            operator()(std::basic_string_view<CharT> source, T& value)
            {
                clocale_restorer lr{LC_NUMERIC};
                std::setlocale(LC_NUMERIC, "C");

                std::basic_string<CharT> null_terminated_source{};
                auto first_space = find_classic_space_narrow_fast(source);
                null_terminated_source.assign(source.begin(), first_space);

                T tmp{};
                return impl(null_terminated_source.c_str(), tmp)
                    .transform([&](size_t chars_read) SCN_NOEXCEPT {
                        value = tmp;
                        return source.begin() + chars_read;
                    })
                    .transform_error([&](scan_error err) SCN_NOEXCEPT {
                        if (err.code() == scan_error::value_out_of_range) {
                            value = tmp;
                        }
                        return err;
                    });
            }

        private:
            scan_expected<std::size_t> impl(const CharT* src,
                                            T& tmp_value) const
            {
                CharT* end{};
                errno = 0;
                tmp_value = cstd_strtod(src, &end);
                const auto chars_read = end - src;
                errno = 0;

                if (auto e = check_error(src, chars_read, tmp_value);
                    SCN_UNLIKELY(!e)) {
                    return unexpected(e);
                }

                return static_cast<size_t>(chars_read);
            }

            SCN_NODISCARD scan_error
            check_error(std::basic_string_view<CharT> source,
                        std::ptrdiff_t chars_read,
                        T& value) const
            {
                // No conversion
                if (is_float_zero(value) && chars_read == 0) {
                    SCN_UNLIKELY_ATTR
                    return {scan_error::invalid_scanned_value,
                            "strtod failed: no conversion"};
                }

                // Unexpected hex float
                if (is_hexfloat(source) &&
                    (reader.m_options &
                     float_classic_value_reader<CharT>::allow_hex) == 0) {
                    SCN_UNLIKELY_ATTR
                    return {scan_error::invalid_scanned_value,
                            "Parsed a hex float, which was "
                            "not allowed by the format string"};
                }

                // Musl libc doesn't set errno to ERANGE on range error,
                // so we can't rely on that

                // Underflow:
                // returned 0, and input is not 0
                if (is_float_zero(value) &&
                    !is_input_zero(
                        source.substr(0, static_cast<size_t>(chars_read)))) {
                    SCN_UNLIKELY_ATTR
                    return {scan_error::value_out_of_range,
                            "strtod failed: float underflow"};
                }

                // Overflow:
                // returned inf (HUGE_VALUE), and input is not "inf"
                if (std::isinf(value) &&
                    !is_input_inf(
                        source.substr(0, static_cast<size_t>(chars_read)))) {
                    SCN_UNLIKELY_ATTR
                    return {scan_error::value_out_of_range,
                            "strtod failed: float overflow"};
                }

                return {};
            }

            T cstd_strtod(const CharT* str, CharT** str_end) const
            {
                if constexpr (std::is_same_v<CharT, char>) {
                    if constexpr (std::is_same_v<T, float>) {
                        return std::strtof(str, str_end);
                    }
                    else if constexpr (std::is_same_v<T, double>) {
                        return std::strtod(str, str_end);
                    }
                    else if constexpr (std::is_same_v<T, long double>) {
                        return std::strtold(str, str_end);
                    }
                }
                else if constexpr (std::is_same_v<CharT, wchar_t>) {
                    if constexpr (std::is_same_v<T, float>) {
                        return std::wcstof(str, str_end);
                    }
                    else if constexpr (std::is_same_v<T, double>) {
                        return std::wcstod(str, str_end);
                    }
                    else if constexpr (std::is_same_v<T, long double>) {
                        return std::wcstold(str, str_end);
                    }
                }
            }
        };

        ////////////////////////////////////////////////////////////////////////
        // std::from_chars based implementation
        // Only for CharT=char
        ////////////////////////////////////////////////////////////////////////

#if SCN_HAS_FLOAT_CHARCONV
        namespace {
            template <typename = void>
            struct float_classic_value_reader_from_chars_impl_base {
                scan_expected<std::chars_format> get_flags(
                    std::string_view& source,
                    bool& has_negative_sign) const
                {
                    if ((reader.m_options &
                         float_value_reader_base::allow_hex) != 0 &&
                        is_hexfloat(source)) {
                        if (source[0] == '-') {
                            has_negative_sign = true;
                            source = source.substr(3);
                        }
                        else if (source[0] == '+') {
                            source = source.substr(3);
                        }
                        else {
                            source = source.substr(2);
                        }
                        return {std::chars_format::hex};
                    }

                    std::chars_format flags{};
                    if ((reader.m_options &
                         float_value_reader_base::allow_fixed) != 0) {
                        flags |= std::chars_format::fixed;
                    }
                    if ((reader.m_options &
                         float_value_reader_base::allow_scientific) != 0) {
                        flags |= std::chars_format::scientific;
                    }

                    if (SCN_UNLIKELY(flags == std::chars_format{})) {
                        SCN_EXPECT((reader.m_options &
                                    float_value_reader_base::allow_hex) != 0);
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "from_chars failed: Expected a hexfloat");
                    }

                    return {flags};
                }

                const float_value_reader_base& reader;
            };
        }  // namespace

        template <typename CharT, typename T>
        class from_chars_reader_impl
            : public float_classic_value_reader_from_chars_impl_base<> {
        public:
            explicit from_chars_reader_impl(const float_value_reader_base& r)
                : float_classic_value_reader_from_chars_impl_base{r}
            {
            }

            scan_expected<ranges::iterator_t<std::string_view>> operator()(
                std::string_view source,
                T& value) const
            {
                const auto original_source = source;
                bool has_negative_sign = false;
                const auto flags = get_flags(source, has_negative_sign);
                if (SCN_UNLIKELY(!flags)) {
                    return unexpected(flags.error());
                }

                T tmp{};
                const auto result = std::from_chars(
                    source.data(), source.data() + source.size(), tmp, *flags);

                if (SCN_UNLIKELY(result.ec == std::errc::invalid_argument)) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "from_chars failed: invalid_argument");
                }
                if (result.ec == std::errc::result_out_of_range) {
                    // Out of range, may be subnormal -> fall back to strtod
                    // On gcc, std::from_chars doesn't parse subnormals
                    return cstd_reader_impl<CharT, T>{reader}(original_source,
                                                              value);
                }

                if (has_negative_sign) {
                    value = -tmp;
                }
                else {
                    value = tmp;
                }
                return {source.begin() + (result.ptr - source.data())};
            }
        };
#endif

        ////////////////////////////////////////////////////////////////////////
        // fast_float based implementation
        // Only for CharT=char AND FloatT=(float OR double)
        ////////////////////////////////////////////////////////////////////////

        struct float_classic_value_reader_fast_float_impl_base {
            fast_float::parse_options get_flags() const
            {
                unsigned format_flags{};
                if ((reader.m_options & float_value_reader_base::allow_fixed) !=
                    0) {
                    format_flags |= fast_float::fixed;
                }
                if ((reader.m_options &
                     float_value_reader_base::allow_scientific) != 0) {
                    format_flags |= fast_float::scientific;
                }

                return fast_float::parse_options{
                    static_cast<fast_float::chars_format>(format_flags)};
            }

            const float_value_reader_base& reader;
        };

        namespace {
#if SCN_HAS_FLOAT_CHARCONV
            template <typename Float, typename = void>
            struct has_charconv_for : std::false_type {};

            template <typename Float>
            struct has_charconv_for<
                Float,
                std::void_t<decltype(std::from_chars(SCN_DECLVAL(const char*),
                                                     SCN_DECLVAL(const char*),
                                                     SCN_DECLVAL(Float&)))>>
                : std::true_type {};
#endif

            template <typename T>
            auto fast_float_fallback(const float_value_reader_base& reader,
                                     std::string_view source,
                                     T& value)
            {
#if SCN_HAS_FLOAT_CHARCONV
                constexpr bool cond =
#if SCN_STDLIB_GLIBCXX
                    !std::is_same_v<T, long double> &&
#endif
                    has_charconv_for<T>::value;

                if constexpr (cond) {
                    return from_chars_reader_impl<char, T>{reader}(source,
                                                                   value);
                }
                else {
                    return cstd_reader_impl<char, T>{reader}(source, value);
                }
#else
                return cstd_reader_impl<char, T>{reader}(source, value);
#endif
            }
        }  // namespace

        template <typename CharT, typename T>
        class fast_float_reader_impl
            : float_classic_value_reader_fast_float_impl_base {
        public:
            explicit fast_float_reader_impl(const float_value_reader_base& r)
                : float_classic_value_reader_fast_float_impl_base{r}
            {
            }

            scan_expected<ranges::iterator_t<std::string_view>> operator()(
                std::string_view source,
                T& value) const
            {
                if ((reader.m_options & float_value_reader_base::allow_hex) !=
                    0) {
                    if (is_hexfloat(source)) {
                        // fast_float doesn't support hexfloats
                        return fast_float_fallback(reader, source, value);
                    }
                    if ((reader.m_options ^
                         float_value_reader_base::allow_hex) == 0) {
                        // only hex floats allowed
                        SCN_UNLIKELY_ATTR
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "float parsing failed: expected hexfloat");
                    }
                }

                const auto flags = get_flags();
                T tmp{};
                const auto result = fast_float::from_chars_advanced(
                    source.data(), source.data() + source.size(), tmp, flags);

                if (SCN_UNLIKELY(result.ec == std::errc::invalid_argument)) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "fast_float failed: invalid_argument");
                }
                if (SCN_UNLIKELY(result.ec == std::errc::result_out_of_range)) {
                    value = tmp;
                    return unexpected_scan_error(
                        scan_error::value_out_of_range,
                        "fast_float failed: result_out_of_range");
                }

                value = tmp;
                return {source.begin() + (result.ptr - source.data())};
            }
        };

        namespace {
            template <typename T>
            auto do_read(const float_value_reader_base& reader,
                         std::string_view source,
                         T& value)
                -> scan_expected<ranges::iterator_t<std::string_view>>
            {
                if constexpr (std::is_same_v<T, long double>) {
                    if constexpr (sizeof(double) == sizeof(long double)) {
                        // If long double is an alias to double (true on
                        // Windows), use fast_float with double
                        return fast_float_reader_impl<char, double>{reader}(
                            source, *reinterpret_cast<double*>(&value));
                    }
                    else {
                        // long doubles aren't supported by fast_float ->
                        // fall back to from_chars or cstd_impl
                        return fast_float_fallback(reader, source, value);
                    }
                }
                else {
                    // Default to fast_float
                    return fast_float_reader_impl<char, T>{reader}(source,
                                                                   value);
                }
            }

            scan_expected<std::string_view> make_utf8_string(
                std::wstring_view source,
                span<char> buffer)
            {
                return validate_and_count_transcoded_code_units<char>(source)
                    .transform([source, &buffer](std::size_t utf8_cu_count) {
                        buffer = buffer.first(utf8_cu_count);
                        const auto n = transcode_valid(source, buffer);
                        SCN_ENSURE(n == buffer.size());

                        return std::string_view{buffer.data(), buffer.size()};
                    });
            }

            ranges::iterator_t<std::wstring_view> get_corresponding_iterator(
                std::wstring_view wide_input,
                std::string_view utf8_input,
                std::size_t utf8_count)
            {
                auto n = count_valid_transcoded_code_units<wchar_t>(
                    std::string_view{utf8_input.data(), utf8_count});
                return wide_input.begin() + n;
            }

            template <typename T>
            auto do_read(const float_value_reader_base& reader,
                         std::wstring_view source,
                         T& value)
                -> scan_expected<ranges::iterator_t<std::wstring_view>>
            {
                // With wide strings, transcode to utf8, and use narrow impl

                auto limited_source = source.substr(0, 64);
                if constexpr (sizeof(wchar_t) == 2) {
                    while (utf16_code_point_length_by_starting_code_unit(
                               limited_source.back()) == 0) {
                        limited_source =
                            limited_source.substr(0, limited_source.size() - 1);
                    }
                }

                std::array<char, 256> buffer{};
                auto utf8_input = make_utf8_string(
                    limited_source, {buffer.data(), buffer.size()});
                if (SCN_UNLIKELY(!utf8_input)) {
                    return unexpected(utf8_input.error());
                }

                return do_read(reader, *utf8_input, value)
                    .transform([&source, utf8_input](auto it) {
                        return get_corresponding_iterator(
                            source, *utf8_input,
                            static_cast<std::size_t>(ranges::distance(
                                detail::to_address(utf8_input->begin()),
                                detail::to_address(it))));
                    });
            }
        }  // namespace

        template <typename CharT>
        template <typename T>
        auto float_classic_value_reader<CharT>::read(string_view_type source,
                                                     T& value)
            -> scan_expected<ranges::iterator_t<string_view_type>>
        {
            return do_read(*this, source, value);
        }

        template auto float_classic_value_reader<char>::read(string_view_type,
                                                             float&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_classic_value_reader<char>::read(string_view_type,
                                                             double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_classic_value_reader<char>::read(string_view_type,
                                                             long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;

        template auto float_classic_value_reader<wchar_t>::read(
            string_view_type,
            float&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_classic_value_reader<wchar_t>::read(
            string_view_type,
            double&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_classic_value_reader<wchar_t>::read(
            string_view_type,
            long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;

        ////////////////////////////////////////////////////////////////////////
        // localized implementation (std::num_get)
        ////////////////////////////////////////////////////////////////////////

        namespace {
            template <typename T, typename CharT>
            void range_error_value_localized(
                T& value,
                std::basic_string_view<CharT> source)
            {
                bool is_negative = false;
                if (source[0] == CharT{'-'}) {
                    is_negative = true;
                    source = source.substr(1);
                }
                else if (source[0] == CharT{'+'}) {
                    source = source.substr(1);
                }

                if (source.empty()) {
                    return;
                }

                if (source[0] == CharT{'0'}) {
                    value = static_cast<T>(0.0);
                    return;
                }

                auto sign =
                    is_negative ? static_cast<T>(-1.0) : static_cast<T>(1.0);
                value = std::copysign(std::numeric_limits<T>::infinity(), sign);
            }

            template <typename T, typename CharT>
            scan_error check_range_localized(
                T& value,
                std::ios_base::iostate err,
                std::basic_string_view<CharT> source)
            {
                if ((err & std::ios_base::failbit) == 0) {
                    // no error
                    return {};
                }

                if (is_float_max(value) || is_float_max(-value)) {
                    value = std::copysign(std::numeric_limits<T>::infinity(),
                                          value);
                }

                if (is_float_zero(value) &&
                    (err & std::ios_base::eofbit) == 0) {
                    SCN_UNLIKELY_ATTR
                    return {scan_error::invalid_scanned_value,
                            "Failed to scan float"};
                }

                if (std::isinf(value)) {
                    SCN_UNLIKELY_ATTR
                    return {scan_error::value_out_of_range,
                            "Out of range: float overflow"};
                }

                if (is_float_zero(value)) {
                    SCN_UNLIKELY_ATTR
                    // MSVC doesn't distinguish between overflow or underflow,
                    // but always returns 0
                    // We need to check ourselves
                    range_error_value_localized(value, source);
                    return {scan_error::value_out_of_range,
                            "Out of range: float overflow"};
                }

                // Not actually an error
                // libcxx gets here when parsing a subnormal
                return {};
            }
        }  // namespace

        template <typename CharT>
        template <typename T>
        auto float_localized_value_reader<CharT>::read(string_view_type source,
                                                       T& value)
            -> scan_expected<ranges::iterator_t<string_view_type>>
        {
            std::basic_istringstream<CharT> stream{};
            auto stdloc = m_locale.get<std::locale>();
            const auto& facet =
                get_or_add_facet<std::num_get<CharT, const CharT*>>(stdloc);

            std::ios_base::iostate err = std::ios_base::goodbit;

            T tmp{};
            auto it = facet.get(source.data(), source.data() + source.size(),
                                stream, err, tmp);
            if (auto e = check_range_localized(tmp, err, source);
                SCN_UNLIKELY(!e)) {
                value = tmp;
                return unexpected(e);
            }

            value = tmp;
            return detail::make_string_view_iterator(source, it);
        }

        template auto float_localized_value_reader<char>::read(string_view_type,
                                                               float&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_localized_value_reader<char>::read(string_view_type,
                                                               double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_localized_value_reader<char>::read(string_view_type,
                                                               long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;

        template auto float_localized_value_reader<wchar_t>::read(
            string_view_type,
            float&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_localized_value_reader<wchar_t>::read(
            string_view_type,
            double&) -> scan_expected<ranges::iterator_t<string_view_type>>;
        template auto float_localized_value_reader<wchar_t>::read(
            string_view_type,
            long double&)
            -> scan_expected<ranges::iterator_t<string_view_type>>;
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
