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

namespace scn {
    namespace detail {
        namespace sto {
            template <typename CharT>
            struct str_to_int<CharT, long long> {
                static either<long long> get(
                    const std::basic_string<CharT>& str,
                    size_t& chars,
                    int base)
                {
                    return std::stoll(str, &chars, base);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, long> {
                static either<long> get(const std::basic_string<CharT>& str,
                                        size_t& chars,
                                        int base)
                {
                    return std::stol(str, &chars, base);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, int> {
                static either<int> get(const std::basic_string<CharT>& str,
                                       size_t& chars,
                                       int base)
                {
                    return std::stoi(str, &chars, base);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, short> {
                static either<short> get(const std::basic_string<CharT>& str,
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
                static either<unsigned long long> get(
                    const std::basic_string<CharT>& str,
                    size_t& chars,
                    int base)
                {
                    return std::stoull(str, &chars, base);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, unsigned long> {
                static either<unsigned long> get(
                    const std::basic_string<CharT>& str,
                    size_t& chars,
                    int base)
                {
                    return std::stoul(str, &chars, base);
                }
            };
            template <typename CharT>
            struct str_to_int<CharT, unsigned int> {
                static either<unsigned int> get(
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
                static either<unsigned short> get(
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

        namespace strto {
            template <>
            struct str_to_int<char, long long> {
                static either<long long> get(const char* str,
                                             size_t& chars,
                                             int base)
                {
                    char* end{};
                    auto ret = std::strtoll(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    return ret;
                }
            };
            template <>
            struct str_to_int<wchar_t, long long> {
                static either<long long> get(const wchar_t* str,
                                             size_t& chars,
                                             int base)
                {
                    wchar_t* end{};
                    auto ret = std::wcstoll(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    return ret;
                }
            };

            template <>
            struct str_to_int<char, long> {
                static either<long> get(const char* str,
                                        size_t& chars,
                                        int base)
                {
                    char* end{};
                    auto ret = std::strtol(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    return ret;
                }
            };
            template <>
            struct str_to_int<wchar_t, long> {
                static either<long> get(const wchar_t* str,
                                        size_t& chars,
                                        int base)
                {
                    wchar_t* end{};
                    auto ret = std::wcstol(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    return ret;
                }
            };

            template <typename Char>
            struct str_to_int<Char, int> {
                static either<int> get(const Char* str, size_t& chars, int base)
                {
                    auto tmp =
                        str_to_int<Char, long long>::get(str, chars, base);
                    if (!tmp) {
                        return tmp.get_error();
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
                static either<short> get(const Char* str,
                                         size_t& chars,
                                         int base)
                {
                    auto tmp = str_to_int<Char, long>::get(str, chars, base);
                    if (!tmp) {
                        return tmp.get_error();
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
                static either<unsigned long long> get(const char* str,
                                                      size_t& chars,
                                                      int base)
                {
                    char* end{};
                    auto ret = std::strtoull(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    return ret;
                }
            };
            template <>
            struct str_to_int<wchar_t, unsigned long long> {
                static either<unsigned long long> get(const wchar_t* str,
                                                      size_t& chars,
                                                      int base)
                {
                    wchar_t* end{};
                    auto ret = std::wcstoull(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    return ret;
                }
            };

            template <>
            struct str_to_int<char, unsigned long> {
                static either<unsigned long> get(const char* str,
                                                 size_t& chars,
                                                 int base)
                {
                    char* end{};
                    auto ret = std::strtoul(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    return ret;
                }
            };
            template <>
            struct str_to_int<wchar_t, unsigned long> {
                static either<unsigned long> get(const wchar_t* str,
                                                 size_t& chars,
                                                 int base)
                {
                    wchar_t* end{};
                    auto ret = std::wcstoul(str, &end, base);
                    chars = static_cast<size_t>(end - str);
                    return ret;
                }
            };

            template <typename Char>
            struct str_to_int<Char, unsigned int> {
                static either<unsigned int> get(const Char* str,
                                                size_t& chars,
                                                int base)
                {
                    auto tmp = str_to_int<Char, unsigned long long>::get(
                        str, chars, base);
                    if (!tmp) {
                        return tmp.get_error();
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
                static either<unsigned short> get(const Char* str,
                                                  size_t& chars,
                                                  int base)
                {
                    auto tmp =
                        str_to_int<Char, unsigned long>::get(str, chars, base);
                    if (!tmp) {
                        return tmp.get_error();
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

        namespace strto {
            template <>
            struct str_to_float<char, float> {
                static either<float> get(const char* str, size_t& chars)
                {
                    char* end{};
                    float f = std::strtof(str, &end);
                    chars = static_cast<size_t>(end - str);
                    return f;
                }
            };
            template <>
            struct str_to_float<wchar_t, float> {
                static either<float> get(const wchar_t* str, size_t& chars)
                {
                    wchar_t* end{};
                    float f = std::wcstof(str, &end);
                    chars = static_cast<size_t>(end - str);
                    return f;
                }
            };

            template <>
            struct str_to_float<char, double> {
                static either<double> get(const char* str, size_t& chars)
                {
                    char* end{};
                    double d = std::strtod(str, &end);
                    chars = static_cast<size_t>(end - str);
                    return d;
                }
            };
            template <>
            struct str_to_float<wchar_t, double> {
                static either<double> get(const wchar_t* str, size_t& chars)
                {
                    wchar_t* end{};
                    double d = std::wcstod(str, &end);
                    chars = static_cast<size_t>(end - str);
                    return d;
                }
            };

            template <>
            struct str_to_float<char, long double> {
                static either<long double> get(const char* str, size_t& chars)
                {
                    char* end{};
                    long double ld = std::strtold(str, &end);
                    chars = static_cast<size_t>(end - str);
                    return ld;
                }
            };
            template <>
            struct str_to_float<wchar_t, long double> {
                static either<long double> get(const wchar_t* str,
                                               size_t& chars)
                {
                    wchar_t* end{};
                    long double ld = std::wcstold(str, &end);
                    chars = static_cast<size_t>(end - str);
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

        template <typename CharT, typename T>
        either<size_t> integer_scanner<CharT, T>::_read_sto(
            T& val,
            const std::basic_string<CharT>& buf,
            int base)
        {
            try {
                size_t chars = 0;
                auto ret = sto::str_to_int<CharT, T>::get(buf, chars, base);
                if (!ret) {
                    return ret.get_error();
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
        }
        template <typename CharT, typename T>
        either<size_t> integer_scanner<CharT, T>::_read_strto(
            T& val,
            const std::basic_string<CharT>& buf,
            int base)
        {
            size_t chars = 0;
            errno = 0;
            auto ret = strto::str_to_int<CharT, T>::get(&buf[0], chars, base);
            if (!ret && (ret.get_error() == error::value_out_of_range ||
                         errno != ERANGE)) {
                return ret.get_error();
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
        either<size_t> integer_scanner<CharT, T>::_read_from_chars(
            T& val,
            const std::basic_string<CharT>& buf,
            int base)
        {
#if SCN_HAS_INTEGER_CHARCONV
            auto begin = buf.data();
            auto end = begin + buf.size();
            auto result = std::from_chars(begin, end, val, base);
            if (result.ec == std::errc::result_out_of_range) {
                return make_error(error::value_out_of_range);
            }
            if (result.ec == std::errc::invalid_argument) {
                return make_error(error::invalid_scanned_value);
            }
            return static_cast<size_t>(std::distance(buf.data(), result.ptr));
#else
            SCN_UNUSED(val);
            SCN_UNUSED(buf);
            SCN_UNUSED(base);
            return error(error::invalid_operation,
                         "from_chars is not a supported integer scanning "
                         "method with this platform");
#endif
        }

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

        template <typename CharT, typename T>
        either<size_t> float_scanner<CharT, T>::_read_sto(
            T& val,
            const std::basic_string<CharT>& buf)
        {
            try {
                size_t chars;
                val = sto::str_to_float<CharT, T>::get(buf, chars);
                return chars;
            }
            catch (const std::invalid_argument& e) {
                return error(error::invalid_scanned_value, e.what());
            }
            catch (const std::out_of_range& e) {
                return error(error::value_out_of_range, e.what());
            }
        }
        template <typename CharT, typename T>
        either<size_t> float_scanner<CharT, T>::_read_strto(
            T& val,
            const std::basic_string<CharT>& buf)
        {
            size_t chars;
            auto ret = strto::str_to_float<CharT, T>::get(&buf[0], chars);
            if (!ret) {
                return ret.get_error();
            }
            val = ret.value();
            return chars;
        }
        template <typename CharT, typename T>
        either<size_t> float_scanner<CharT, T>::_read_from_chars(
            T& val,
            const std::basic_string<CharT>& buf)
        {
#if SCN_HAS_FLOAT_CHARCONV
            auto begin = buf.data();
            auto end = begin + buf.size();
            auto result = std::from_chars(begin, end, val);
            if (result.ec == std::errc::result_out_of_range) {
                return make_error(error::value_out_of_range);
            }
            if (result.ec == std::errc::invalid_argument) {
                return make_error(error::invalid_scanned_value);
            }
            return static_cast<size_t>(std::distance(buf.data(), result.ptr));
#else
            SCN_UNUSED(val);
            SCN_UNUSED(buf);
            return error(error::invalid_operation,
                         "from_chars is not a supported floating-point "
                         "scanning method with this platform");
#endif
        }

        template struct float_scanner<char, float>;
        template struct float_scanner<char, double>;
        template struct float_scanner<char, long double>;
        template struct float_scanner<wchar_t, float>;
        template struct float_scanner<wchar_t, double>;
        template struct float_scanner<wchar_t, long double>;
    }  // namespace detail
}  // namespace scn
