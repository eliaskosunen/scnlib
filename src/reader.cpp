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

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY
#define SCN_READER_CPP
#endif

#include <scn/detail/args.h>
#include <scn/detail/reader.h>

#include <cerrno>
#include <clocale>

#if SCN_HAS_FLOAT_CHARCONV
#include <charconv>
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename CharT, typename T>
        struct read_float_impl;

        template <typename T, typename CharT, typename F>
        expected<T> read_float_cstd(F&& f_strtod,
                                    const CharT* str,
                                    size_t& chars)
        {
            // Get current C locale
            const auto loc = std::setlocale(LC_NUMERIC, nullptr);
            // For whatever reason, this cannot be stored in the heap if
            // setlocale hasn't been called before, or msan errors with
            // 'use-of-unitialized-value' when resetting the locale back.
            // POSIX specifies that the content of loc may not be static, so we
            // need to save it ourselves
            char locbuf[256] = {0};
            std::strcpy(locbuf, loc);

            std::setlocale(LC_NUMERIC, "C");

            CharT* end{};
            errno = 0;
            T f = f_strtod(str, &end);
            chars = static_cast<size_t>(end - str);
            auto err = errno;
            // Reset locale
            std::setlocale(LC_NUMERIC, locbuf);
            errno = 0;
            if (err == ERANGE) {
                return error(error::value_out_of_range,
                             "Floating-point value out of range");
            }
            return f;
        }

#if SCN_HAS_FLOAT_CHARCONV
        template <typename T>
        struct read_float_impl<char, T> {
            static expected<T> get(const char* str, size_t& chars)
            {
                const auto len = std::strlen(str);
                T value{};
                const auto result = std::from_chars(
                    str, str + len, value,
                    std::chars_format::general | std::chars_format::hex);
                if (result.ec == std::errc::invalid_argument) {
                    return error(error::invalid_scanned_value, "from_chars");
                }
                if (result.ec == std::errc::result_out_of_range) {
                    return error(error::value_out_of_range,
                                 "strtof range error");
                }
                chars = static_cast<size_t>(result.ptr - str);
                return value;
            }
        };
#else
        template <>
        struct read_float_impl<char, float> {
            static expected<float> get(const char* str, size_t& chars)
            {
                auto f = read_float_cstd<float>(strtof, str, chars);
                if (!f) {
                    return f;
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (f.value() == 0.0f && chars == 0) {
                    return error(error::invalid_scanned_value, "strtof");
                }
                SCN_GCC_POP
                return f;
            }
        };

        template <>
        struct read_float_impl<char, double> {
            static expected<double> get(const char* str, size_t& chars)
            {
                auto d = read_float_cstd<double>(strtod, str, chars);
                if (!d) {
                    return d;
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (d.value() == 0.0 && chars == 0) {
                    return error(error::invalid_scanned_value, "strtod");
                }
                SCN_GCC_POP
                return d;
            }
        };

        template <>
        struct read_float_impl<char, long double> {
            static expected<long double> get(const char* str, size_t& chars)
            {
                auto ld = read_float_cstd<long double>(strtold, str, chars);
                if (!ld) {
                    return ld;
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (ld.value() == 0.0l && chars == 0) {
                    return error(error::invalid_scanned_value, "strtold");
                }
                SCN_GCC_POP
                return ld;
            }
        };
#endif  // <charconv>

        template <>
        struct read_float_impl<wchar_t, float> {
            static expected<float> get(const wchar_t* str, size_t& chars)
            {
                auto f = read_float_cstd<float>(wcstof, str, chars);
                if (!f) {
                    return f;
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (f.value() == 0.0f && chars == 0) {
                    return error(error::invalid_scanned_value, "wcstof");
                }
                SCN_GCC_POP
                return f;
            }
        };
        template <>
        struct read_float_impl<wchar_t, double> {
            static expected<double> get(const wchar_t* str, size_t& chars)
            {
                auto d = read_float_cstd<double>(wcstod, str, chars);
                if (!d) {
                    return d;
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (d.value() == 0.0 && chars == 0) {
                    return error(error::invalid_scanned_value, "wcstod");
                }
                SCN_GCC_POP
                return d;
            }
        };
        template <>
        struct read_float_impl<wchar_t, long double> {
            static expected<long double> get(const wchar_t* str, size_t& chars)
            {
                auto ld = read_float_cstd<long double>(wcstold, str, chars);
                if (!ld) {
                    return ld;
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (ld.value() == 0.0l && chars == 0) {
                    return error(error::invalid_scanned_value, "wcstold");
                }
                SCN_GCC_POP
                return ld;
            }
        };

        template <typename T>
        template <typename CharT>
        expected<T> float_scanner<T>::_read_float_impl(const CharT* str,
                                                       size_t& chars)
        {
            return read_float_impl<CharT, T>::get(str, chars);
        }

        template expected<float> float_scanner<float>::_read_float_impl(
            const char*,
            size_t&);
        template expected<double> float_scanner<double>::_read_float_impl(
            const char*,
            size_t&);
        template expected<long double>
        float_scanner<long double>::_read_float_impl(const char*, size_t&);
        template expected<float> float_scanner<float>::_read_float_impl(
            const wchar_t*,
            size_t&);
        template expected<double> float_scanner<double>::_read_float_impl(
            const wchar_t*,
            size_t&);
        template expected<long double>
        float_scanner<long double>::_read_float_impl(const wchar_t*, size_t&);
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
