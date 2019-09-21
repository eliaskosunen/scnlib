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

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY
#define SCN_READER_CPP
#endif

#include <scn/detail/reader.h>

#include <cerrno>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename CharT, typename T>
        struct read_float_impl;

        template <>
        struct read_float_impl<char, float> {
            static expected<float> get(const char* str, size_t& chars)
            {
                char* end{};
                errno = 0;
                float f = std::strtof(str, &end);
                chars = static_cast<size_t>(end - str);
                if (errno == ERANGE) {
                    errno = 0;
                    return error(error::value_out_of_range,
                                 "strtof range error");
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (f == 0.0f && end == str) {
                    return error(error::invalid_scanned_value, "strtof");
                }
                SCN_GCC_POP
                return f;
            }
        };
        template <>
        struct read_float_impl<wchar_t, float> {
            static expected<float> get(const wchar_t* str, size_t& chars)
            {
                wchar_t* end{};
                errno = 0;
                float f = std::wcstof(str, &end);
                chars = static_cast<size_t>(end - str);
                if (errno == ERANGE) {
                    errno = 0;
                    return error(error::value_out_of_range,
                                 "wcstof range error");
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (f == 0.0f && end == str) {
                    return error(error::invalid_scanned_value, "wcstof");
                }
                SCN_GCC_POP
                return f;
            }
        };

        template <>
        struct read_float_impl<char, double> {
            static expected<double> get(const char* str, size_t& chars)
            {
                char* end{};
                errno = 0;
                double d = std::strtod(str, &end);
                chars = static_cast<size_t>(end - str);
                if (errno == ERANGE) {
                    errno = 0;
                    return error(error::value_out_of_range,
                                 "strtod range error");
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (d == 0.0 && end == str) {
                    return error(error::invalid_scanned_value, "strtod");
                }
                SCN_GCC_POP
                return d;
            }
        };
        template <>
        struct read_float_impl<wchar_t, double> {
            static expected<double> get(const wchar_t* str, size_t& chars)
            {
                wchar_t* end{};
                errno = 0;
                double d = std::wcstod(str, &end);
                chars = static_cast<size_t>(end - str);
                if (errno == ERANGE) {
                    errno = 0;
                    return error(error::value_out_of_range,
                                 "wcstod range error");
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (d == 0.0 && end == str) {
                    return error(error::invalid_scanned_value, "wcstod");
                }
                SCN_GCC_POP
                return d;
            }
        };

        template <>
        struct read_float_impl<char, long double> {
            static expected<long double> get(const char* str, size_t& chars)
            {
                char* end{};
                errno = 0;
                long double ld = std::strtold(str, &end);
                chars = static_cast<size_t>(end - str);
                if (errno == ERANGE) {
                    errno = 0;
                    return error(error::value_out_of_range,
                                 "strtold range error");
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (ld == 0.0l && end == str) {
                    return error(error::invalid_scanned_value, "strtold");
                }
                SCN_GCC_POP
                return ld;
            }
        };
        template <>
        struct read_float_impl<wchar_t, long double> {
            static expected<long double> get(const wchar_t* str, size_t& chars)
            {
                wchar_t* end{};
                errno = 0;
                long double ld = std::wcstold(str, &end);
                chars = static_cast<size_t>(end - str);
                if (errno == ERANGE) {
                    errno = 0;
                    return error(error::value_out_of_range,
                                 "wcstold range error");
                }
                SCN_GCC_PUSH
                // bogus warning, == with 0.0 is safe
                SCN_GCC_IGNORE("-Wfloat-equal")
                if (ld == 0.0l && end == str) {
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
