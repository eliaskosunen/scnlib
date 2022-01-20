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
#define SCN_READER_INT_CPP
#endif

#include <scn/detail/args.h>
#include <scn/detail/reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename T>
        template <typename CharT>
        expected<std::ptrdiff_t> integer_scanner<T>::_parse_int(
            T& val,
            span<const CharT> s)
        {
            SCN_MSVC_PUSH
            SCN_MSVC_IGNORE(4244)
            SCN_MSVC_IGNORE(4127)  // conditional expression is constant

            if (std::is_unsigned<T>::value) {
                if (s[0] == detail::ascii_widen<CharT>('-')) {
                    return error(error::value_out_of_range,
                                 "Unexpected sign '-' when scanning an "
                                 "unsigned integer");
                }
            }

            SCN_MSVC_POP

            T tmp = 0;
            bool minus_sign = false;
            auto it = s.begin();

            if (s[0] == ascii_widen<CharT>('-') ||
                s[0] == ascii_widen<CharT>('+')) {
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wsign-conversion")
                minus_sign = s[0] == ascii_widen<CharT>('-');
                ++it;
                SCN_GCC_POP
            }
            if (SCN_UNLIKELY(it == s.end())) {
                return error(error::invalid_scanned_value,
                             "Expected number after sign");
            }

            if (*it == ascii_widen<CharT>('0')) {
                ++it;
                if (it == s.end()) {
                    val = 0;
                    return ranges::distance(s.begin(), it);
                }
                if (*it == ascii_widen<CharT>('x') ||
                    *it == ascii_widen<CharT>('X')) {
                    if (SCN_UNLIKELY(base != 0 && base != 16)) {
                        val = 0;
                        return ranges::distance(s.begin(), it);
                    }
                    ++it;
                    if (SCN_UNLIKELY(it == s.end())) {
                        --it;
                        val = 0;
                        return ranges::distance(s.begin(), it);
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

            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
            auto r = _parse_int_impl(tmp, minus_sign,
                                     make_span(it, s.end()).as_const());
            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
            if (!r) {
                return r.error();
            }
            it = r.value();
            if (s.begin() == it) {
                return error(error::invalid_scanned_value, "custom::read_int");
            }
            val = tmp;
            return ranges::distance(s.begin(), it);

            SCN_CLANG_POP
            SCN_GCC_POP
        }

        template <typename T>
        template <typename CharT>
        expected<typename span<const CharT>::iterator>
        integer_scanner<T>::_parse_int_impl(T& val,
                                            bool minus_sign,
                                            span<const CharT> buf) const
        {
            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wconversion")
            SCN_GCC_IGNORE("-Wsign-conversion")
            SCN_GCC_IGNORE("-Wsign-compare")

            SCN_CLANG_PUSH
            SCN_CLANG_IGNORE("-Wconversion")
            SCN_CLANG_IGNORE("-Wsign-conversion")
            SCN_CLANG_IGNORE("-Wsign-compare")

            SCN_MSVC_PUSH
            SCN_MSVC_IGNORE(4018)  // > signed/unsigned mismatch
            SCN_MSVC_IGNORE(4389)  // == signed/unsigned mismatch
            SCN_MSVC_IGNORE(4244)  // lossy conversion

            using utype = typename std::make_unsigned<T>::type;

            const auto ubase = static_cast<utype>(base);
            SCN_ASSUME(ubase > 0);

            constexpr auto uint_max = static_cast<utype>(-1);
            constexpr auto int_max = static_cast<utype>(uint_max >> 1);
            constexpr auto abs_int_min = static_cast<utype>(int_max + 1);

            const auto cut = div(
                [&]() -> utype {
                    if (std::is_signed<T>::value) {
                        if (minus_sign) {
                            return abs_int_min;
                        }
                        return int_max;
                    }
                    return uint_max;
                }(),
                ubase);
            const auto cutoff = cut.first;
            const auto cutlim = cut.second;

            auto it = buf.begin();
            const auto end = buf.end();
            utype tmp = 0;
            for (; it != end; ++it) {
                const auto digit = _char_to_int(*it);
                if (digit >= ubase) {
                    break;
                }
                if (SCN_UNLIKELY(tmp > cutoff ||
                                 (tmp == cutoff && digit > cutlim))) {
                    if (!minus_sign) {
                        return error(error::value_out_of_range,
                                     "Out of range: integer overflow");
                    }
                    return error(error::value_out_of_range,
                                 "Out of range: integer underflow");
                }
                tmp = tmp * ubase + digit;
            }
            if (minus_sign) {
                // special case: signed int minimum's absolute value can't
                // be represented with the same type
                //
                // For example, short int -- range is [-32768, 32767], 32768
                // can't be represented
                //
                // In that case, -static_cast<T>(tmp) would trigger UB
                if (SCN_UNLIKELY(tmp == abs_int_min)) {
                    val = std::numeric_limits<T>::min();
                }
                else {
                    val = -static_cast<T>(tmp);
                }
            }
            else {
                val = static_cast<T>(tmp);
            }
            return it;

            SCN_MSVC_POP
            SCN_CLANG_POP
            SCN_GCC_POP
        }

#define SCN_DEFINE_INTEGER_SCANNER_MEMBERS_IMPL(CharT, T)             \
    template expected<std::ptrdiff_t> integer_scanner<T>::_parse_int( \
        T& val, span<const CharT> s);                                 \
    template expected<typename span<const CharT>::iterator>           \
    integer_scanner<T>::_parse_int_impl(T& val, bool minus_sign,      \
                                        span<const CharT> buf) const;

#define SCN_DEFINE_INTEGER_SCANNER_MEMBERS(Char)                  \
    SCN_DEFINE_INTEGER_SCANNER_MEMBERS_IMPL(Char, short)          \
    SCN_DEFINE_INTEGER_SCANNER_MEMBERS_IMPL(Char, int)            \
    SCN_DEFINE_INTEGER_SCANNER_MEMBERS_IMPL(Char, long)           \
    SCN_DEFINE_INTEGER_SCANNER_MEMBERS_IMPL(Char, long long)      \
    SCN_DEFINE_INTEGER_SCANNER_MEMBERS_IMPL(Char, unsigned short) \
    SCN_DEFINE_INTEGER_SCANNER_MEMBERS_IMPL(Char, unsigned int)   \
    SCN_DEFINE_INTEGER_SCANNER_MEMBERS_IMPL(Char, unsigned long)  \
    SCN_DEFINE_INTEGER_SCANNER_MEMBERS_IMPL(Char, unsigned long long)

        SCN_DEFINE_INTEGER_SCANNER_MEMBERS(char)
        SCN_DEFINE_INTEGER_SCANNER_MEMBERS(wchar_t)

    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
