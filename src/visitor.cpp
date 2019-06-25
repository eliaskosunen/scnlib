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
#define SCN_VISITOR_CPP
#endif

#include <scn/detail/visitor.h>

#include <cerrno>
#include <string>

#if SCN_HAS_INTEGER_CHARCONV || SCN_HAS_FLOAT_CHARCONV
#include <charconv>
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        //
        // sto
        // integers
        //
        namespace sto {
            template <typename CharT>
            struct str_to_int<CharT, long long> {
                static expected<long long> get(
                    const std::basic_string<CharT>& str,
                    size_t& chars,
                    int base)
                {
                    return std::stoll(str, &chars, base);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, long> {
                static expected<long> get(const std::basic_string<CharT>& str,
                                          size_t& chars,
                                          int base)
                {
                    return std::stol(str, &chars, base);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, int> {
                static expected<int> get(const std::basic_string<CharT>& str,
                                         size_t& chars,
                                         int base)
                {
                    return std::stoi(str, &chars, base);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, short> {
                static expected<short> get(const std::basic_string<CharT>& str,
                                           size_t& chars,
                                           int base)
                {
                    auto i = std::stoi(str, &chars, base);
                    if (i >
                        static_cast<int>(std::numeric_limits<short>::max())) {
                        return error(error::value_out_of_range,
                                     "Scanned integer out of range for a short "
                                     "int: overflow");
                    }
                    if (i <
                        static_cast<int>(std::numeric_limits<short>::min())) {
                        return error(error::value_out_of_range,
                                     "Scanned integer out of range for a short "
                                     "int: underflow");
                    }
                    return static_cast<short>(i);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, unsigned long long> {
                static expected<unsigned long long> get(
                    const std::basic_string<CharT>& str,
                    size_t& chars,
                    int base)
                {
                    return std::stoull(str, &chars, base);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, unsigned long> {
                static expected<unsigned long> get(
                    const std::basic_string<CharT>& str,
                    size_t& chars,
                    int base)
                {
                    return std::stoul(str, &chars, base);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, unsigned int> {
                static expected<unsigned int> get(
                    const std::basic_string<CharT>& str,
                    size_t& chars,
                    int base)
                {
                    auto i = std::stoul(str, &chars, base);
                    if (i > static_cast<unsigned long>(
                                std::numeric_limits<unsigned int>::max())) {
                        return error(
                            error::value_out_of_range,
                            "Scanned integer out of range for an unsigned "
                            "int: overflow");
                    }
                    return static_cast<unsigned int>(i);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, unsigned short> {
                static expected<unsigned short> get(
                    const std::basic_string<CharT>& str,
                    size_t& chars,
                    int base)
                {
                    auto i = std::stoul(str, &chars, base);
                    if (i > static_cast<unsigned long>(
                                std::numeric_limits<unsigned short>::max())) {
                        return error(
                            error::value_out_of_range,
                            "Scanned integer out of range for an unsigned "
                            "short: overflow");
                    }
                    return static_cast<unsigned short>(i);
                }
            };

            template struct str_to_int<char, short>;
            template struct str_to_int<char, int>;
            template struct str_to_int<char, long>;
            template struct str_to_int<char, long long>;
            template struct str_to_int<char, unsigned short>;
            template struct str_to_int<char, unsigned int>;
            template struct str_to_int<char, unsigned long>;
            template struct str_to_int<char, unsigned long long>;
            template struct str_to_int<wchar_t, short>;
            template struct str_to_int<wchar_t, int>;
            template struct str_to_int<wchar_t, long>;
            template struct str_to_int<wchar_t, long long>;
            template struct str_to_int<wchar_t, unsigned short>;
            template struct str_to_int<wchar_t, unsigned int>;
            template struct str_to_int<wchar_t, unsigned long>;
            template struct str_to_int<wchar_t, unsigned long long>;
        }  // namespace sto

        //
        // strto
        // integers
        //
        namespace strto {
            template <>
            struct str_to_int<char, long long> {
                static expected<long long> get(const char* str,
                                               size_t& chars,
                                               int base)
                {
                    char* end{};
                    errno = 0;
                    auto ret = std::strtoll(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    if (errno == ERANGE) {
                        errno = 0;
                        return error(error::value_out_of_range,
                                     "strtoll range error");
                    }
                    if (ret == 0 && str == end) {
                        return error(error::invalid_scanned_value, "strtoll");
                    }
                    return ret;
                }
            };
            template <>
            struct str_to_int<wchar_t, long long> {
                static expected<long long> get(const wchar_t* str,
                                               size_t& chars,
                                               int base)
                {
                    wchar_t* end{};
                    errno = 0;
                    auto ret = std::wcstoll(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    if (errno == ERANGE) {
                        errno = 0;
                        return error(error::value_out_of_range,
                                     "wcstoll range error");
                    }
                    if (ret == 0 && str == end) {
                        return error(error::invalid_scanned_value, "wcstoll");
                    }
                    return ret;
                }
            };

            template <>
            struct str_to_int<char, long> {
                static expected<long> get(const char* str,
                                          size_t& chars,
                                          int base)
                {
                    char* end{};
                    errno = 0;
                    auto ret = std::strtol(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    if (errno == ERANGE) {
                        errno = 0;
                        return error(error::value_out_of_range,
                                     "strtol range error");
                    }
                    if (ret == 0 && str == end) {
                        return error(error::invalid_scanned_value, "strtol");
                    }
                    return ret;
                }
            };
            template <>
            struct str_to_int<wchar_t, long> {
                static expected<long> get(const wchar_t* str,
                                          size_t& chars,
                                          int base)
                {
                    wchar_t* end{};
                    errno = 0;
                    auto ret = std::wcstol(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    if (errno == ERANGE) {
                        errno = 0;
                        return error(error::value_out_of_range,
                                     "wcstol range error");
                    }
                    if (ret == 0 && str == end) {
                        return error(error::invalid_scanned_value, "wcstol");
                    }
                    return ret;
                }
            };

            template <typename Char>
            struct str_to_int<Char, int> {
                static expected<int> get(const Char* str,
                                         size_t& chars,
                                         int base)
                {
                    auto tmp =
                        str_to_int<Char, long long>::get(str, chars, base);
                    if (!tmp) {
                        return tmp.error();
                    }
                    if (tmp.value() > static_cast<long long>(
                                          std::numeric_limits<int>::max())) {
                        return error(error::value_out_of_range,
                                     "Scanned integer out of range for an int: "
                                     "overflow");
                    }
                    if (tmp.value() < static_cast<long long>(
                                          std::numeric_limits<int>::min())) {
                        return error(error::value_out_of_range,
                                     "Scanned integer out of range for an int: "
                                     "underflow");
                    }
                    return static_cast<int>(tmp.value());
                }
            };
            template <typename Char>
            struct str_to_int<Char, short> {
                static expected<short> get(const Char* str,
                                           size_t& chars,
                                           int base)
                {
                    auto tmp = str_to_int<Char, long>::get(str, chars, base);
                    if (!tmp) {
                        return tmp.error();
                    }
                    if (tmp.value() >
                        static_cast<long>(std::numeric_limits<short>::max())) {
                        return error(error::value_out_of_range,
                                     "Scanned integer out of range for a short "
                                     "int: overflow");
                    }
                    if (tmp.value() <
                        static_cast<long>(std::numeric_limits<short>::min())) {
                        return error(error::value_out_of_range,
                                     "Scanned integer out of range for a short "
                                     "int: underflow");
                    }
                    return static_cast<short>(tmp.value());
                }
            };

            template <>
            struct str_to_int<char, unsigned long long> {
                static expected<unsigned long long> get(const char* str,
                                                        size_t& chars,
                                                        int base)
                {
                    char* end{};
                    errno = 0;
                    auto ret = std::strtoull(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    if (errno == ERANGE) {
                        errno = 0;
                        return error(error::value_out_of_range,
                                     "strtoull range error");
                    }
                    if (ret == 0 && str == end) {
                        return error(error::invalid_scanned_value, "strtoull");
                    }
                    return ret;
                }
            };
            template <>
            struct str_to_int<wchar_t, unsigned long long> {
                static expected<unsigned long long> get(const wchar_t* str,
                                                        size_t& chars,
                                                        int base)
                {
                    wchar_t* end{};
                    errno = 0;
                    auto ret = std::wcstoull(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    if (errno == ERANGE) {
                        errno = 0;
                        return error(error::value_out_of_range,
                                     "wcstoull range error");
                    }
                    if (ret == 0 && str == end) {
                        return error(error::invalid_scanned_value, "wcstoull");
                    }
                    return ret;
                }
            };

            template <>
            struct str_to_int<char, unsigned long> {
                static expected<unsigned long> get(const char* str,
                                                   size_t& chars,
                                                   int base)
                {
                    char* end{};
                    errno = 0;
                    auto ret = std::strtoul(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    if (errno == ERANGE) {
                        errno = 0;
                        return error(error::value_out_of_range,
                                     "strtoul range error");
                    }
                    if (ret == 0 && str == end) {
                        return error(error::invalid_scanned_value, "strtoul");
                    }
                    return ret;
                }
            };
            template <>
            struct str_to_int<wchar_t, unsigned long> {
                static expected<unsigned long> get(const wchar_t* str,
                                                   size_t& chars,
                                                   int base)
                {
                    wchar_t* end{};
                    errno = 0;
                    auto ret = std::wcstoul(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    if (errno == ERANGE) {
                        errno = 0;
                        return error(error::value_out_of_range,
                                     "wcstoul range error");
                    }
                    if (ret == 0 && str == end) {
                        return error(error::invalid_scanned_value, "wcstoul");
                    }
                    return ret;
                }
            };

            template <typename Char>
            struct str_to_int<Char, unsigned int> {
                static expected<unsigned int> get(const Char* str,
                                                  size_t& chars,
                                                  int base)
                {
                    auto tmp = str_to_int<Char, unsigned long long>::get(
                        str, chars, base);
                    if (!tmp) {
                        return tmp.error();
                    }
                    if (tmp.value() >
                        static_cast<unsigned long long>(
                            std::numeric_limits<unsigned int>::max())) {
                        return error(error::value_out_of_range,
                                     "Scanned integer out of range for an "
                                     "unsigned int: overflow");
                    }
                    return static_cast<unsigned>(tmp.value());
                }
            };
            template <typename Char>
            struct str_to_int<Char, unsigned short> {
                static expected<unsigned short> get(const Char* str,
                                                    size_t& chars,
                                                    int base)
                {
                    auto tmp =
                        str_to_int<Char, unsigned long>::get(str, chars, base);
                    if (!tmp) {
                        return tmp.error();
                    }
                    if (tmp.value() >
                        static_cast<unsigned long>(
                            std::numeric_limits<unsigned short>::max())) {
                        return error(error::value_out_of_range,
                                     "Scanned integer out of range for an "
                                     "unsigned short: overflow");
                    }
                    return static_cast<unsigned short>(tmp.value());
                }
            };

            template struct str_to_int<char, short>;
            template struct str_to_int<char, int>;
            // template struct str_to_int<char, long>;
            // template struct str_to_int<char, long long>;
            template struct str_to_int<char, unsigned short>;
            template struct str_to_int<char, unsigned int>;
            // template struct str_to_int<char, unsigned long>;
            // template struct str_to_int<char, unsigned long long>;
            template struct str_to_int<wchar_t, short>;
            template struct str_to_int<wchar_t, int>;
            // template struct str_to_int<wchar_t, long>;
            // template struct str_to_int<wchar_t, long long>;
            template struct str_to_int<wchar_t, unsigned short>;
            template struct str_to_int<wchar_t, unsigned int>;
            // template struct str_to_int<wchar_t, unsigned long>;
            // template struct str_to_int<wchar_t, unsigned long long>;
        }  // namespace strto

        //
        // custom
        // integers
        //
        namespace custom {
            template <typename CharT>
            bool is_base_digit(CharT ch, int base)
            {
                if (base <= 10) {
                    return ch >= ascii_widen<CharT>('0') &&
                           ch <= ascii_widen<CharT>('0') + base - 1;
                }
                return is_base_digit(ch, 10) ||
                       (ch >= ascii_widen<CharT>('a') &&
                        ch <= ascii_widen<CharT>('a') + base - 1) ||
                       (ch >= ascii_widen<CharT>('A') &&
                        ch <= ascii_widen<CharT>('A') + base - 1);
            }
            template <typename T, typename CharT>
            T char_to_int(CharT ch, int base)
            {
                if (base <= 10) {
                    return static_cast<T>(ch - ascii_widen<CharT>('0'));
                }
                if (ch <= ascii_widen<CharT>('9')) {
                    return static_cast<T>(ch - ascii_widen<CharT>('0'));
                }
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wconversion")
                if (ch >= ascii_widen<CharT>('a') &&
                    ch <= ascii_widen<CharT>('z')) {
                    return 10 + static_cast<T>(ch - ascii_widen<CharT>('a'));
                }
                return 10 + static_cast<T>(ch - ascii_widen<CharT>('A'));
                SCN_GCC_POP
            }

            template <typename T, typename CharT>
            expected<typename span<const CharT>::iterator> read_signed(
                T& val,
                T sign,
                span<const CharT> buf,
                int base,
                CharT thsep)
            {
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wconversion")
                SCN_GCC_IGNORE("-Wsign-conversion")
                SCN_GCC_IGNORE("-Wsign-compare")

                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wconversion")
                SCN_CLANG_IGNORE("-Wsign-conversion")
                SCN_CLANG_IGNORE("-Wsign-compare")

                SCN_ASSUME(sign != 0);

                using utype = typename std::make_unsigned<T>::type;

                utype cutoff_tmp = sign == -1
                                       ? -static_cast<unsigned long long>(
                                             std::numeric_limits<T>::min())
                                       : std::numeric_limits<T>::max();
                const auto cutlim =
                    detail::ascii_widen<CharT>(cutoff_tmp % base + '0');
                const utype cutoff = cutoff_tmp / base;

                auto it = buf.begin();
                for (; it != buf.end(); ++it) {
                    if (SCN_LIKELY(is_base_digit(*it, base))) {
                        if (SCN_UNLIKELY(val > cutoff ||
                                         (val == cutoff && *it > cutlim))) {
                            if (sign == 1) {
                                return error(error::value_out_of_range,
                                             "Out of range: integer overflow");
                            }
                            return error(error::value_out_of_range,
                                         "Out of range: integer underflow");
                        }
                        else {
                            val = val * base + char_to_int<T>(*it, base);
                        }
                    }
                    else {
                        if (thsep != 0 && *it == thsep) {
                            continue;
                        }
                        break;
                    }
                }
                val = val * sign;
                return it;

                SCN_CLANG_POP
                SCN_GCC_POP
            }
            template <typename T, typename CharT>
            expected<typename span<const CharT>::iterator>
            read_unsigned(T& val, span<const CharT> buf, int base, CharT thsep)
            {
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wconversion")
                SCN_GCC_IGNORE("-Wsign-conversion")
                SCN_GCC_IGNORE("-Wsign-compare")

                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wconversion")
                SCN_CLANG_IGNORE("-Wsign-conversion")
                SCN_CLANG_IGNORE("-Wsign-compare")

                const T cutoff = std::numeric_limits<T>::max() / base;
                const auto cutlim = detail::ascii_widen<CharT>(
                    std::numeric_limits<T>::max() % base + '0');

                auto it = buf.begin();
                for (; it != buf.end(); ++it) {
                    if (SCN_LIKELY(is_base_digit(*it, base))) {
                        if (SCN_UNLIKELY(val > cutoff ||
                                         (val == cutoff && *it > cutlim))) {
                            return error(error::value_out_of_range,
                                         "Out of range: integer overflow");
                        }
                        else {
                            val = val * base + char_to_int<T>(*it, base);
                        }
                    }
                    else {
                        if (thsep != 0 && *it == thsep) {
                            continue;
                        }
                        break;
                    }
                }
                return it;

                SCN_CLANG_POP
                SCN_GCC_POP
            }
        }  // namespace custom

        //
        // sto
        // floats
        //
        namespace sto {
            template <typename CharT>
            struct str_to_float<CharT, float> {
                static float get(const std::basic_string<CharT>& str,
                                 size_t& chars)
                {
                    return std::stof(str, &chars);
                }
            };
            template <typename CharT>
            struct str_to_float<CharT, double> {
                static double get(const std::basic_string<CharT>& str,
                                  size_t& chars)
                {
                    return std::stod(str, &chars);
                }
            };
            template <typename CharT>
            struct str_to_float<CharT, long double> {
                static long double get(const std::basic_string<CharT>& str,
                                       size_t& chars)
                {
                    return std::stold(str, &chars);
                }
            };

            template struct str_to_float<char, float>;
            template struct str_to_float<char, double>;
            template struct str_to_float<char, long double>;
            template struct str_to_float<wchar_t, float>;
            template struct str_to_float<wchar_t, double>;
            template struct str_to_float<wchar_t, long double>;
        }  // namespace sto

        //
        // strto
        // floats
        //
        namespace strto {
            template <>
            struct str_to_float<char, float> {
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
            struct str_to_float<wchar_t, float> {
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
            struct str_to_float<char, double> {
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
            struct str_to_float<wchar_t, double> {
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
            struct str_to_float<char, long double> {
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
            struct str_to_float<wchar_t, long double> {
                static expected<long double> get(const wchar_t* str,
                                                 size_t& chars)
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

            // template struct str_to_float<char, float>;
            // template struct str_to_float<char, double>;
            // template struct str_to_float<char, long double>;
            // template struct str_to_float<wchar_t, float>;
            // template struct str_to_float<wchar_t, double>;
            // template struct str_to_float<wchar_t, long double>;
        }  // namespace strto

        namespace from_chars {
#if SCN_HAS_INTEGER_CHARCONV
            template <typename T>
            expected<const char*> str_to_int(const char* begin,
                                             const char* end,
                                             T& value,
                                             int base)
            {
                auto r = std::from_chars(begin, end, value, base);
                if (r.ec == std::errc::result_out_of_range) {
                    return error(error::value_out_of_range,
                                 "from_chars: value out of range");
                }
                if (r.ec == std::errc::invalid_argument) {
                    return error(error::invalid_scanned_value,
                                 "from_chars: invalid scanned value");
                }
                return r.ptr;
            }

            template <typename T>
            expected<const wchar_t*> str_to_int(const wchar_t*,
                                                const wchar_t*,
                                                T&,
                                                int)
            {
                return error(error::invalid_operation,
                             "from_chars is not a supported integer scanning "
                             "method for wide streams");
            }

            template <typename T>
            expected<const char*> str_to_float(const char* begin,
                                               const char* end,
                                               T& value)
            {
                auto r = std::from_chars(begin, end, value);
                if (r.ec == std::errc::result_out_of_range) {
                    return error(error::value_out_of_range,
                                 "from_chars: value out of range");
                }
                if (r.ec == std::errc::invalid_argument) {
                    return error(error::invalid_scanned_value,
                                 "from_chars: invalid scanned value");
                }
                return r.ptr;
            }

            template <typename T>
            expected<const wchar_t*> str_to_float(const wchar_t*,
                                                  const wchar_t*,
                                                  T&)
            {
                return error(error::invalid_operation,
                             "from_chars is not a supported integer scanning "
                             "method for wide streams");
            }
#endif
        }  // namespace from_chars

        template <typename CharT, typename T>
        expected<size_t> integer_scanner<CharT, T>::_read_sto(
            T& val,
            span<const CharT> buf,
            int base,
            CharT)
        {
#if SCN_HAS_EXCEPTIONS
            try {
                size_t chars = 0;
                std::basic_string<CharT> str(buf.data(), buf.size());
                auto ret = sto::str_to_int<CharT, T>::get(str, chars, base);
                if (!ret) {
                    return ret.error();
                }
                val = ret.value();
                return chars;
            }
            catch (const std::invalid_argument& e) {
                return error(error::invalid_scanned_value, e.what());
            }
            catch (const std::out_of_range& e) {
                return error(error::value_out_of_range, e.what());
            }
#else
            SCN_UNUSED(val);
            SCN_UNUSED(buf);
            SCN_UNUSED(base);
            return error(
                error::exceptions_required,
                "sto scanning method is only supported with exceptions "
                "enabled. Use strto or from_chars instead.");
#endif
        }
        template <typename CharT, typename T>
        expected<size_t> integer_scanner<CharT, T>::_read_strto(
            T& val,
            span<const CharT> buf,
            int base,
            CharT)
        {
            size_t chars = 0;
            errno = 0;
            std::basic_string<CharT> str(buf.data(), buf.size());
            auto ret =
                strto::str_to_int<CharT, T>::get(str.data(), chars, base);
            if (!ret &&
                (ret.error() == error::value_out_of_range || errno != ERANGE)) {
                return ret.error();
            }
            if (errno == ERANGE) {
                return error(error::value_out_of_range,
                             "Scanned integer out of range");
            }
            if (chars == 0) {
                return error(error::invalid_scanned_value,
                             "Could not parse integer");
            }
            val = ret.value();

            return chars;
        }

        template <typename CharT, typename T>
        expected<size_t> integer_scanner<CharT, T>::_read_from_chars(
            T& val,
            span<const CharT> buf,
            int base,
            CharT)
        {
#if SCN_HAS_INTEGER_CHARCONV
            auto begin = buf.data();
            auto end = begin + buf.size();
            auto result = from_chars::str_to_int(begin, end, val, base);
            if (!result) {
                return result.error();
            }
            return static_cast<size_t>(
                std::distance(buf.data(), result.value()));
#else
            SCN_UNUSED(val);
            SCN_UNUSED(buf);
            SCN_UNUSED(base);
            return error(error::invalid_operation,
                         "from_chars is not a supported integer scanning "
                         "method with this platform");
#endif
        }

        template <typename CharT, typename T>
        expected<size_t> integer_scanner<CharT, T>::_read_custom(
            T& val,
            span<const CharT> buf,
            int base,
            CharT thsep)
        {
            T tmp = 0;
            T sign = 1;
            auto it = buf.begin();
            if (buf[0] == ascii_widen<CharT>('-') ||
                buf[0] == ascii_widen<CharT>('+')) {
                SCN_GCC_PUSH
                // integer_scanner::scan ensures that unsigned values have no
                // '-' sign
                SCN_GCC_IGNORE("-Wsign-conversion")
                sign = 1 - 2 * (buf[0] == ascii_widen<CharT>('-'));
                ++it;
                SCN_GCC_POP
            }
            if (SCN_UNLIKELY(it == buf.end())) {
                return error(error::invalid_scanned_value,
                             "Expected number after sign");
            }

            if (*it == ascii_widen<CharT>('0')) {
                ++it;
                if (it == buf.end()) {
                    val = 0;
                    return static_cast<size_t>(std::distance(buf.begin(), it));
                }
                if (*it == ascii_widen<CharT>('x') ||
                    *it == ascii_widen<CharT>('X')) {
                    if (SCN_UNLIKELY(base != 0 && base != 16)) {
                        val = 0;
                        return static_cast<size_t>(
                            std::distance(buf.begin(), it));
                    }
                    ++it;
                    if (SCN_UNLIKELY(it == buf.end())) {
                        --it;
                        val = 0;
                        return static_cast<size_t>(
                            std::distance(buf.begin(), it));
                    }
                    if (base == 0) {
                        base = 16;
                    }
                }
                else if (base == 0) {
                    base = 8;
                }
            }
            if (base == 0) {
                base = 10;
            }

            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wconversion")
            SCN_GCC_IGNORE("-Wsign-conversion")
            SCN_GCC_IGNORE("-Wsign-compare")

            SCN_CLANG_PUSH
            SCN_CLANG_IGNORE("-Wconversion")
            SCN_CLANG_IGNORE("-Wsign-conversion")
            SCN_CLANG_IGNORE("-Wsign-compare")

            SCN_ASSUME(base > 0);
            SCN_ASSUME(sign != 0);

            if (std::is_signed<T>::value) {
                auto r = custom::read_signed(
                    tmp, sign, make_span(it, buf.end()), base, thsep);
                if (!r) {
                    return r.error();
                }
                it = r.value();
                if (buf.begin() == it) {
                    return error(error::invalid_scanned_value,
                                 "custom::read_signed");
                }
                val = tmp;
                return static_cast<size_t>(std::distance(buf.begin(), it));
            }
            auto r = custom::read_unsigned(tmp, make_span(it, buf.end()), base,
                                           thsep);
            if (!r) {
                return r.error();
            }
            it = r.value();
            if (buf.begin() == it) {
                return error(error::invalid_scanned_value,
                             "custom::read_unsigned");
            }
            val = tmp;
            return static_cast<size_t>(std::distance(buf.begin(), it));

            SCN_CLANG_POP
            SCN_GCC_POP
        }

        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wpadded")

        template class integer_scanner<char, short>;
        template class integer_scanner<char, int>;
        template class integer_scanner<char, long>;
        template class integer_scanner<char, long long>;
        template class integer_scanner<char, unsigned short>;
        template class integer_scanner<char, unsigned int>;
        template class integer_scanner<char, unsigned long>;
        template class integer_scanner<char, unsigned long long>;
        template class integer_scanner<wchar_t, short>;
        template class integer_scanner<wchar_t, int>;
        template class integer_scanner<wchar_t, long>;
        template class integer_scanner<wchar_t, long long>;
        template class integer_scanner<wchar_t, unsigned short>;
        template class integer_scanner<wchar_t, unsigned int>;
        template class integer_scanner<wchar_t, unsigned long>;
        template class integer_scanner<wchar_t, unsigned long long>;

        SCN_CLANG_POP

        template <typename CharT, typename T>
        expected<size_t> float_scanner<CharT, T>::_read_sto(
            T& val,
            span<const CharT> buf)
        {
#if SCN_HAS_EXCEPTIONS
            try {
                size_t chars;
                std::basic_string<CharT> str(buf.data(), buf.size());
                val = sto::str_to_float<CharT, T>::get(str, chars);
                return chars;
            }
            catch (const std::invalid_argument& e) {
                return error(error::invalid_scanned_value, e.what());
            }
            catch (const std::out_of_range& e) {
                return error(error::value_out_of_range, e.what());
            }
#else
            SCN_UNUSED(val);
            SCN_UNUSED(buf);
            return error(
                error::exceptions_required,
                "sto scanning method is only supported with exceptions "
                "enabled. Use strto or from_chars instead.");
#endif
        }
        template <typename CharT, typename T>
        expected<size_t> float_scanner<CharT, T>::_read_strto(
            T& val,
            span<const CharT> buf)
        {
            size_t chars;
            std::basic_string<CharT> str(buf.data(), buf.size());
            auto ret = strto::str_to_float<CharT, T>::get(str.data(), chars);
            if (!ret) {
                return ret.error();
            }
            val = ret.value();
            return chars;
        }
        template <typename CharT, typename T>
        expected<size_t> float_scanner<CharT, T>::_read_from_chars(
            T& val,
            span<const CharT> buf)
        {
#if SCN_HAS_FLOAT_CHARCONV
            auto begin = buf.data();
            auto end = begin + buf.size();
            auto result = from_chars::str_to_float(begin, end, val);
            if (!result) {
                return result.error();
            }
            return static_cast<size_t>(
                std::distance(buf.data(), result.value()));
#else
            SCN_UNUSED(val);
            SCN_UNUSED(buf);
            return error(error::invalid_operation,
                         "from_chars is not a supported integer scanning "
                         "method with this platform");
#endif
        }
        template <typename CharT, typename T>
        expected<size_t> float_scanner<CharT, T>::_read_custom(
            T& val,
            span<const CharT> buf)
        {
#if 0
            // TODO
            auto it = buf.begin();
            auto get_retval = [&]() {
                return static_cast<size_t>(std::distance(buf.begin(), it));
            };

            // sign
            int sign = 1;
            if (buf[0] == ascii_widen<CharT>('-') ||
                buf[0] == ascii_widen<CharT>('+')) {
                sign = 1 - 2 * (buf[0] == ascii_widen<CharT>('-'));
                ++it;
            }
            if (SCN_UNLIKELY(it == buf.end())) {
                return error(error::invalid_scanned_value,
                             "Expected number after sign");
            }

            using int_scanner = integer_scanner<CharT, long long>;
            auto make_float = [](long long whole, long long dec,
                                 long long exp) {
                auto f = static_cast<T>(whole);
                SCN_UNUSED(dec);
                SCN_UNUSED(exp);
                return f;
            };

            if (*it == ascii_widen<CharT>('0')) {
                ++it;
                if (it == buf.end()) {
                    val = static_cast<T>(0.0) * static_cast<T>(sign);
                }
                if (*it == ascii_widen<CharT>('x') ||
                    *it == ascii_widen<CharT>('X')) {
                    // hex
                    ++it;
                    if (SCN_UNLIKELY(it == buf.end())) {
                        return error(error::invalid_scanned_value,
                                     "Expected number after '0x'");
                    }

                    long long a{0}, b{0};
                    auto s = make_span(it, buf.end());
                    auto ret = int_scanner::_read_custom(a, s, 16, 0);
                    if (!ret) {
                        return ret;
                    }
                    it += ret.value();
                    if (it == buf.end()) {
                        val = make_float(a, b, 1);
                        return get_retval();
                    }

                    if (*it == ascii_widen<CharT>('.')) {
                        ++it;
                    }
                }
            }
#endif
            SCN_UNUSED(val);
            SCN_UNUSED(buf);
            return error(error::invalid_operation,
                         "custom is not a supported floating-point "
                         "scanning method");
        }

        template struct float_scanner<char, float>;
        template struct float_scanner<char, double>;
        template struct float_scanner<char, long double>;
        template struct float_scanner<wchar_t, float>;
        template struct float_scanner<wchar_t, double>;
        template struct float_scanner<wchar_t, long double>;
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
