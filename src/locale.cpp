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
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename CharT>
        std::locale get_locale(const basic_locale_ref<CharT>& ref)
        {
            if (ref.is_default()) {
                return std::locale();
            }
            return *static_cast<const std::locale*>(ref.get_ptr());
        }

        template <typename CharT>
        truename_falsename_storage<CharT>::truename_falsename_storage(
            const void* loc)
            : m_truename(std::use_facet<std::numpunct<CharT>>(
                             *static_cast<const std::locale*>(loc))
                             .truename()),
              m_falsename(std::use_facet<std::numpunct<CharT>>(
                              *static_cast<const std::locale*>(loc))
                              .falsename())
        {
            SCN_EXPECT(loc != nullptr);
        }
    }  // namespace detail

    template <typename CharT>
    basic_locale_ref<CharT>::basic_locale_ref(const void* loc)
        : m_locale(loc),
          m_truefalse_storage(
              new detail::truename_falsename_storage<CharT>(loc)),
          m_truename(m_truefalse_storage->get_true_view()),
          m_falsename(m_truefalse_storage->get_false_view()),
          m_decimal_point(
              std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
                  .decimal_point()),
          m_thousands_separator(
              std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
                  .thousands_sep())
    {
        SCN_EXPECT(loc != nullptr);
    }

    template <typename CharT>
    bool basic_locale_ref<CharT>::_is_space(CharT ch) const
    {
        return std::isspace(ch, detail::get_locale(*this));
    }
    template <typename CharT>
    bool basic_locale_ref<CharT>::_is_digit(CharT ch) const
    {
        return std::isdigit(ch, detail::get_locale(*this));
    }

    template <typename CharT>
    CharT basic_locale_ref<CharT>::_widen(char ch) const
    {
        return std::use_facet<std::ctype<CharT>>(detail::get_locale(*this))
            .widen(ch);
    }

    template <typename CharT>
    char basic_locale_ref<CharT>::_narrow(char_type ch, char def) const
    {
        return std::use_facet<std::ctype<CharT>>(detail::get_locale(*this))
            .narrow(ch, def);
    }

    namespace detail {
        template <typename T, typename CharT>
        either<size_t> read_num_impl(T& val,
                                     const std::locale& loc,
                                     const std::basic_string<CharT>& buf)
        {
#if SCN_HAS_EXCEPTIONS
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
            catch (const std::ios_base::failure& f) {
                return error(error::invalid_scanned_value, f.what());
            }
            return static_cast<size_t>(ss.gcount());
#else
            SCN_UNUSED(val);
            SCN_UNUSED(loc);
            SCN_UNUSED(buf);
            return error(error::exceptions_required,
                         "Localized number reading is only supported with "
                         "exceptions enabled");
#endif
        }

        template <typename T, typename CharT>
        struct read_num {
            static either<size_t> read(T& val,
                                       const std::locale& loc,
                                       const std::basic_string<CharT>& buf)
            {
                return read_num_impl(val, loc, buf);
            }
        };
        template <typename CharT>
        struct read_num<int, CharT> {
            static either<size_t> read(int& val,
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
                    return error(
                        error::value_out_of_range,
                        "Scanned integer out of range for an int: overflow");
                }
                if (tmp <
                    static_cast<long long>(std::numeric_limits<int>::min())) {
                    return error(
                        error::value_out_of_range,
                        "Scanned integer out of range for an int: underflow");
                }
                val = static_cast<int>(tmp);
                return ret;
            }
        };
        template <typename CharT>
        struct read_num<short, CharT> {
            static either<size_t> read(short& val,
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
                    return error(error::value_out_of_range,
                                 "Scanned integer out of range for a short "
                                 "int: overflow");
                }
                if (tmp <
                    static_cast<long long>(std::numeric_limits<short>::min())) {
                    return error(error::value_out_of_range,
                                 "Scanned integer out of range for a short "
                                 "int: underflow");
                }
                val = static_cast<short>(tmp);
                return ret;
            }
        };
        template <typename CharT>
        struct read_num<unsigned int, CharT> {
            static either<size_t> read(unsigned int& val,
                                       const std::locale& loc,
                                       const std::basic_string<CharT>& buf)
            {
                unsigned long long tmp{};
                auto ret = read_num_impl(tmp, loc, buf);
                if (!ret) {
                    return ret;
                }
                if (tmp > static_cast<long long>(
                              std::numeric_limits<unsigned int>::max())) {
                    return error(error::value_out_of_range,
                                 "Scanned integer out of range for an unsigned "
                                 "int: overflow");
                }
                val = static_cast<unsigned int>(tmp);
                return ret;
            }
        };
        template <typename CharT>
        struct read_num<unsigned short, CharT> {
            static either<size_t> read(unsigned short& val,
                                       const std::locale& loc,
                                       const std::basic_string<CharT>& buf)
            {
                unsigned long long tmp{};
                auto ret = read_num_impl(tmp, loc, buf);
                if (!ret) {
                    return ret;
                }
                if (tmp > static_cast<long long>(
                              std::numeric_limits<unsigned short>::max())) {
                    return error(
                        error::value_out_of_range,
                        "Scanned integer out of range for a unsigned short "
                        "int: overflow");
                }
                val = static_cast<unsigned short>(tmp);
                return ret;
            }
        };
    }  // namespace detail

    template <typename CharT>
    template <typename T>
    either<size_t> basic_locale_ref<CharT>::read_num(T& val,
                                                     const string_type& buf)
    {
        return detail::read_num<T, CharT>::read(val, detail::get_locale(*this),
                                                buf);
    }

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")
    template class basic_locale_ref<char>;
    template class basic_locale_ref<wchar_t>;
    SCN_CLANG_POP

    template either<size_t> basic_locale_ref<char>::read_num<short>(
        short&,
        const string_type&);
    template either<size_t> basic_locale_ref<char>::read_num<int>(
        int&,
        const string_type&);
    template either<size_t> basic_locale_ref<char>::read_num<long>(
        long&,
        const string_type&);
    template either<size_t> basic_locale_ref<char>::read_num<long long>(
        long long&,
        const string_type&);
    template either<size_t> basic_locale_ref<char>::read_num<unsigned short>(
        unsigned short&,
        const string_type&);
    template either<size_t> basic_locale_ref<char>::read_num<unsigned int>(
        unsigned int&,
        const string_type&);
    template either<size_t> basic_locale_ref<char>::read_num<unsigned long>(
        unsigned long&,
        const string_type&);
    template either<size_t>
    basic_locale_ref<char>::read_num<unsigned long long>(unsigned long long&,
                                                         const string_type&);
    template either<size_t> basic_locale_ref<char>::read_num<float>(
        float&,
        const string_type&);
    template either<size_t> basic_locale_ref<char>::read_num<double>(
        double&,
        const string_type&);
    template either<size_t> basic_locale_ref<char>::read_num<long double>(
        long double&,
        const string_type&);

    template either<size_t> basic_locale_ref<wchar_t>::read_num<short>(
        short&,
        const string_type&);
    template either<size_t> basic_locale_ref<wchar_t>::read_num<int>(
        int&,
        const string_type&);
    template either<size_t> basic_locale_ref<wchar_t>::read_num<long>(
        long&,
        const string_type&);
    template either<size_t> basic_locale_ref<wchar_t>::read_num<long long>(
        long long&,
        const string_type&);
    template either<size_t> basic_locale_ref<wchar_t>::read_num<unsigned short>(
        unsigned short&,
        const string_type&);
    template either<size_t> basic_locale_ref<wchar_t>::read_num<unsigned int>(
        unsigned int&,
        const string_type&);
    template either<size_t> basic_locale_ref<wchar_t>::read_num<unsigned long>(
        unsigned long&,
        const string_type&);
    template either<size_t>
    basic_locale_ref<wchar_t>::read_num<unsigned long long>(unsigned long long&,
                                                            const string_type&);
    template either<size_t> basic_locale_ref<wchar_t>::read_num<float>(
        float&,
        const string_type&);
    template either<size_t> basic_locale_ref<wchar_t>::read_num<double>(
        double&,
        const string_type&);
    template either<size_t> basic_locale_ref<wchar_t>::read_num<long double>(
        long double&,
        const string_type&);

    SCN_END_NAMESPACE
}  // namespace scn
