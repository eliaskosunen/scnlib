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
#define SCN_LOCALE_CPP
#endif

#include <scn/detail/core.h>
#include <scn/detail/locale.h>

#include <cctype>
#include <cwchar>
#include <locale>
#include <sstream>

namespace scn {
    namespace detail {
        template <typename CharT>
        std::locale get_locale(basic_locale_ref<CharT> ref)
        {
            if (ref.is_default()) {
                return std::locale();
            }
            return *static_cast<const std::locale*>(ref.get_ptr());
        }
    }  // namespace detail

    template <>
    SCN_FUNC basic_locale_ref<char>::basic_locale_ref()
        : m_locale(nullptr),
          m_truename("true"),
          m_falsename("false"),
          m_decimal_point('.'),
          m_thousands_separator(',')
    {
    }
    template <>
    SCN_FUNC basic_locale_ref<wchar_t>::basic_locale_ref()
        : m_locale(nullptr),
          m_truename(L"true"),
          m_falsename(L"false"),
          m_decimal_point(L'.'),
          m_thousands_separator(L',')
    {
    }
    template <typename CharT>
    basic_locale_ref<CharT>::basic_locale_ref(const void* loc)
        : m_locale(loc),
          m_truename(
              std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
                  .truename()),
          m_falsename(
              std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
                  .falsename()),
          m_decimal_point(
              std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
                  .decimal_point()),
          m_thousands_separator(
              std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
                  .thousands_sep())
    {
    }

    template <typename CharT>
    bool basic_locale_ref<CharT>::is_space(CharT ch) const
    {
        if (is_default()) {
            return std::isspace(ch) != 0;
        }
        return std::isspace(ch, detail::get_locale(*this));
    }
    template <typename CharT>
    bool basic_locale_ref<CharT>::is_digit(CharT ch) const
    {
        if (is_default()) {
            return std::isdigit(ch) != 0;
        }
        return std::isdigit(ch, detail::get_locale(*this));
    }
    template <typename CharT>
    CharT basic_locale_ref<CharT>::decimal_point() const
    {
        return m_decimal_point;
    }
    template <typename CharT>
    CharT basic_locale_ref<CharT>::thousands_separator() const
    {
        return m_thousands_separator;
    }
    template <typename CharT>
    auto basic_locale_ref<CharT>::truename() const -> string_view_type
    {
        return string_view_type(m_truename.data(), m_truename.size());
    }
    template <typename CharT>
    auto basic_locale_ref<CharT>::falsename() const -> string_view_type
    {
        return string_view_type(m_falsename.data(), m_falsename.size());
    }

    namespace detail {
        template <typename CharT>
        struct default_widen;
        template <>
        struct default_widen<char> {
            static char widen(char ch)
            {
                return ch;
            }
        };
        template <>
        struct default_widen<wchar_t> {
            static wchar_t widen(char ch)
            {
                auto ret = std::btowc(static_cast<int>(ch));
                if (ret == WEOF) {
                    return static_cast<wchar_t>(-1);
                }
                return static_cast<wchar_t>(ret);
            }
        };
    }  // namespace detail

    template <typename CharT>
    CharT basic_locale_ref<CharT>::widen(char ch) const
    {
        if (is_default()) {
            return detail::default_widen<CharT>::widen(ch);
        }
        return std::use_facet<std::ctype<CharT>>(detail::get_locale(*this))
            .widen(ch);
    }

    namespace detail {
        template <typename CharT>
        struct default_narrow;
        template <>
        struct default_narrow<char> {
            static char narrow(char ch, char)
            {
                return ch;
            }
        };
        template <>
        struct default_narrow<wchar_t> {
            static char narrow(wchar_t ch, char def)
            {
                auto ret = std::wctob(static_cast<wint_t>(ch));
                if (ret == EOF) {
                    return def;
                }
                return static_cast<char>(ret);
            }
        };
    }  // namespace detail

    template <typename CharT>
    char basic_locale_ref<CharT>::narrow(char_type ch, char def) const
    {
        if (is_default()) {
            return detail::default_narrow<CharT>::narrow(ch, def);
        }
        return std::use_facet<std::ctype<CharT>>(detail::get_locale(*this))
            .narrow(ch, def);
    }

    namespace detail {
        template <typename T, typename CharT>
        result<size_t> read_num_impl(T& val,
                                     const std::locale& loc,
                                     const std::basic_string<CharT>& buf)
        {
            std::basic_istringstream<CharT> ss(buf);
            ss.imbue(loc);
            std::ios_base::iostate err = std::ios_base::goodbit;

            try {
                typename decltype(ss)::sentry s(ss);
                if (s) {
                    std::use_facet<std::num_get<CharT>>(ss.getloc())
                        .get(ss, {}, ss, err, val);
                }
            }
            catch (const std::ios_base::failure&) {
                return error::invalid_scanned_value;
            }
            return static_cast<size_t>(ss.gcount());
        }

        template <typename T, typename CharT>
        struct read_num {
            static result<size_t> read(T& val,
                                       const std::locale& loc,
                                       const std::basic_string<CharT>& buf)
            {
                return read_num_impl(val, loc, buf);
            }
        };
        template <typename CharT>
        struct read_num<short, CharT> {
            static result<size_t> read(short& val,
                                       const std::locale& loc,
                                       const std::basic_string<CharT>& buf)
            {
                long long tmp{};
                auto ret = read_num_impl(tmp, loc, buf);
                if (!ret) {
                    return ret;
                }
                if (tmp >
                    static_cast<long long>(std::numeric_limits<short>::max())) {
                    return error::value_out_of_range;
                }
                if (tmp <
                    static_cast<long long>(std::numeric_limits<short>::min())) {
                    return error::value_out_of_range;
                }
                val = static_cast<short>(tmp);
                return ret;
            }
        };
        template <typename CharT>
        struct read_num<int, CharT> {
            static result<size_t> read(int& val,
                                       const std::locale& loc,
                                       const std::basic_string<CharT>& buf)
            {
                long long tmp{};
                auto ret = read_num_impl(tmp, loc, buf);
                if (!ret) {
                    return ret;
                }
                if (tmp >
                    static_cast<long long>(std::numeric_limits<int>::max())) {
                    return error::value_out_of_range;
                }
                if (tmp <
                    static_cast<long long>(std::numeric_limits<int>::min())) {
                    return error::value_out_of_range;
                }
                val = static_cast<int>(tmp);
                return ret;
            }
        };
    }  // namespace detail

    template <typename CharT>
    template <typename T>
    result<size_t> basic_locale_ref<CharT>::read_num(T& val,
                                                     const string_type& buf)
    {
        return detail::read_num<T, CharT>::read(val, detail::get_locale(*this),
                                                buf);
    }

#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
    template class basic_locale_ref<char>;
    template class basic_locale_ref<wchar_t>;
#if SCN_CLANG
#pragma clang diagnostic pop
#endif

    template result<size_t> basic_locale_ref<char>::read_num<short>(
        short&,
        const string_type&);
    template result<size_t> basic_locale_ref<char>::read_num<int>(
        int&,
        const string_type&);
    template result<size_t> basic_locale_ref<char>::read_num<long>(
        long&,
        const string_type&);
    template result<size_t> basic_locale_ref<char>::read_num<long long>(
        long long&,
        const string_type&);
    template result<size_t> basic_locale_ref<char>::read_num<unsigned short>(
        unsigned short&,
        const string_type&);
    template result<size_t> basic_locale_ref<char>::read_num<unsigned int>(
        unsigned int&,
        const string_type&);
    template result<size_t> basic_locale_ref<char>::read_num<unsigned long>(
        unsigned long&,
        const string_type&);
    template result<size_t>
    basic_locale_ref<char>::read_num<unsigned long long>(unsigned long long&,
                                                         const string_type&);
    template result<size_t> basic_locale_ref<char>::read_num<float>(
        float&,
        const string_type&);
    template result<size_t> basic_locale_ref<char>::read_num<double>(
        double&,
        const string_type&);
    template result<size_t> basic_locale_ref<char>::read_num<long double>(
        long double&,
        const string_type&);

    template result<size_t> basic_locale_ref<wchar_t>::read_num<short>(
        short&,
        const string_type&);
    template result<size_t> basic_locale_ref<wchar_t>::read_num<int>(
        int&,
        const string_type&);
    template result<size_t> basic_locale_ref<wchar_t>::read_num<long>(
        long&,
        const string_type&);
    template result<size_t> basic_locale_ref<wchar_t>::read_num<long long>(
        long long&,
        const string_type&);
    template result<size_t> basic_locale_ref<wchar_t>::read_num<unsigned short>(
        unsigned short&,
        const string_type&);
    template result<size_t> basic_locale_ref<wchar_t>::read_num<unsigned int>(
        unsigned int&,
        const string_type&);
    template result<size_t> basic_locale_ref<wchar_t>::read_num<unsigned long>(
        unsigned long&,
        const string_type&);
    template result<size_t>
    basic_locale_ref<wchar_t>::read_num<unsigned long long>(unsigned long long&,
                                                            const string_type&);
    template result<size_t> basic_locale_ref<wchar_t>::read_num<float>(
        float&,
        const string_type&);
    template result<size_t> basic_locale_ref<wchar_t>::read_num<double>(
        double&,
        const string_type&);
    template result<size_t> basic_locale_ref<wchar_t>::read_num<long double>(
        long double&,
        const string_type&);
}  // namespace scn
