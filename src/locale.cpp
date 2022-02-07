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
#define SCN_LOCALE_CPP
#endif

#include <scn/detail/locale.h>

#include <cctype>
#include <cmath>
#include <cwchar>
#include <iomanip>
#include <locale>
#include <sstream>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename CharT>
        const std::locale& to_locale(const basic_custom_locale_ref<CharT>& l)
        {
            SCN_EXPECT(l.get_locale());
            return *static_cast<const std::locale*>(l.get_locale());
        }

        template <typename CharT>
        basic_custom_locale_ref<CharT>::basic_custom_locale_ref()
        {
            m_global_locale = new std::locale{};
            _initialize();
        }
        template <typename CharT>
        basic_custom_locale_ref<CharT>::basic_custom_locale_ref(
            const void* locale)
            : m_locale(locale)
        {
            if (!locale) {
                m_global_locale = new std::locale{};
                m_locale = m_global_locale;
            }
            _initialize();
        }

        template <typename CharT>
        void basic_custom_locale_ref<CharT>::_initialize()
        {
            const auto& facet =
                std::use_facet<std::numpunct<CharT>>(to_locale(*this));
            m_truename = facet.truename();
            m_falsename = facet.falsename();
            m_decimal_point = facet.decimal_point();
            m_thousands_separator = facet.thousands_sep();
        }

        template <typename CharT>
        basic_custom_locale_ref<CharT>::basic_custom_locale_ref(
            basic_custom_locale_ref&& o)
        {
            if (o.m_global_locale) {
                m_global_locale = o.m_global_locale;
                m_locale = m_global_locale;
                o.m_global_locale = nullptr;
                o.m_locale = nullptr;
            }
            else {
                m_locale = o.m_locale;
                o.m_locale = nullptr;
            }
            _initialize();
        }
        template <typename CharT>
        auto basic_custom_locale_ref<CharT>::operator=(
            basic_custom_locale_ref&& o) -> basic_custom_locale_ref&
        {
            delete static_cast<std::locale*>(m_global_locale);

            if (o.m_global_locale) {
                m_global_locale = o.m_global_locale;
                m_locale = m_global_locale;
                o.m_global_locale = nullptr;
                o.m_locale = nullptr;
            }
            else {
                m_locale = o.m_locale;
                o.m_locale = nullptr;
            }
            _initialize();

            return *this;
        }

        template <typename CharT>
        basic_custom_locale_ref<CharT>::~basic_custom_locale_ref()
        {
            delete static_cast<std::locale*>(m_global_locale);
        }

        template <typename CharT>
        bool basic_custom_locale_ref<CharT>::do_is_space(char_type ch) const
        {
            return std::isspace(ch, to_locale(*this));
        }
        template <typename CharT>
        bool basic_custom_locale_ref<CharT>::do_is_digit(char_type ch) const
        {
            return std::isdigit(ch, to_locale(*this));
        }

#define SCN_DEFINE_CUSTOM_LOCALE_CTYPE(f)                           \
    template <typename CharT>                                       \
    bool basic_custom_locale_ref<CharT>::is_##f(char_type ch) const \
    {                                                               \
        return std::is##f(ch, to_locale(*this));                    \
    }
        SCN_DEFINE_CUSTOM_LOCALE_CTYPE(alnum)
        SCN_DEFINE_CUSTOM_LOCALE_CTYPE(alpha)
        SCN_DEFINE_CUSTOM_LOCALE_CTYPE(cntrl)
        SCN_DEFINE_CUSTOM_LOCALE_CTYPE(graph)
        SCN_DEFINE_CUSTOM_LOCALE_CTYPE(lower)
        SCN_DEFINE_CUSTOM_LOCALE_CTYPE(print)
        SCN_DEFINE_CUSTOM_LOCALE_CTYPE(punct)
        SCN_DEFINE_CUSTOM_LOCALE_CTYPE(upper)
        SCN_DEFINE_CUSTOM_LOCALE_CTYPE(xdigit)
#undef SCN_DEFINE_CUSTOM_LOCALE_CTYPE

        template <typename CharT>
        bool basic_custom_locale_ref<CharT>::is_blank(char_type ch) const
        {
            // For some reason, there's no isblank in libc++
            // return std::isblank(ch, to_locale(*this));

            return std::use_facet<std::ctype<CharT>>(to_locale(*this))
                .is(std::ctype_base::blank, ch);
        }

        template <typename T>
        auto read_num_check_range(T val) ->
            typename std::enable_if<std::is_integral<T>::value, error>::type
        {
            if (val == std::numeric_limits<T>::max()) {
                return error(error::value_out_of_range,
                             "Scanned number out of range: overflow");
            }
            if (val == std::numeric_limits<T>::min()) {
                return error(error::value_out_of_range,
                             "Scanned number out of range: underflow");
            }
            return error(error::invalid_scanned_value,
                         "Localized number read failed");
        }
        template <typename T>
        auto read_num_check_range(T val) ->
            typename std::enable_if<std::is_floating_point<T>::value,
                                    error>::type
        {
            SCN_GCC_COMPAT_PUSH
            SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")
            if (val == std::numeric_limits<T>::max() ||
                val == -std::numeric_limits<T>::max()) {
                return error(error::value_out_of_range,
                             "Scanned number out of range: overflow");
            }
            if (val == zero_value<T>::value) {
                return error(error::value_out_of_range,
                             "Scanned number out of range: underflow");
            }
            SCN_GCC_COMPAT_POP
            return error(error::invalid_scanned_value,
                         "Localized number read failed");
        }

        template <typename T, typename CharT>
        expected<std::ptrdiff_t> do_read_num(
            T& val,
            const std::locale& loc,
            const std::basic_string<CharT>& buf,
            int base)
        {
#if SCN_HAS_EXCEPTIONS
            std::basic_istringstream<CharT> ss(buf);
            ss.imbue(loc);
            ss >> std::setbase(base);

            try {
                T tmp;
                ss >> tmp;
                if (ss.bad()) {
                    return error(error::unrecoverable_internal_error,
                                 "Localized stringstream is bad");
                }
                if (ss.fail()) {
                    return read_num_check_range(tmp);
                }
                val = tmp;
            }
            catch (const std::ios_base::failure& f) {
                return error(error::invalid_scanned_value, f.what());
            }
            return ss.eof() ? static_cast<std::ptrdiff_t>(buf.size())
                            : static_cast<std::ptrdiff_t>(ss.tellg());
#else
            SCN_UNUSED(val);
            SCN_UNUSED(loc);
            SCN_UNUSED(buf);
            return error(error::exceptions_required,
                         "Localized number reading is only supported with "
                         "exceptions enabled");
#endif
        }

        template <typename CharT>
        template <typename T>
        expected<std::ptrdiff_t> basic_custom_locale_ref<CharT>::read_num(
            T& val,
            const string_type& buf,
            int b) const
        {
            return do_read_num<T, CharT>(val, to_locale(*this), buf, b);
        }

#if SCN_INCLUDE_SOURCE_DEFINITIONS

        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wpadded")
        SCN_CLANG_IGNORE("-Wweak-template-vtables")
        template class basic_custom_locale_ref<char>;
        template class basic_custom_locale_ref<wchar_t>;
        SCN_CLANG_POP

        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<char>::read_num<short>(short&,
                                                       const string_type&,
                                                       int) const;
        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<char>::read_num<int>(int&,
                                                     const string_type&,
                                                     int) const;
        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<char>::read_num<long>(long&,
                                                      const string_type&,
                                                      int) const;
        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<char>::read_num<long long>(long long&,
                                                           const string_type&,
                                                           int) const;
        template expected<std::ptrdiff_t> basic_custom_locale_ref<
            char>::read_num<unsigned short>(unsigned short&,
                                            const string_type&,
                                            int) const;
        template expected<std::ptrdiff_t> basic_custom_locale_ref<
            char>::read_num<unsigned int>(unsigned int&,
                                          const string_type&,
                                          int) const;
        template expected<std::ptrdiff_t> basic_custom_locale_ref<
            char>::read_num<unsigned long>(unsigned long&,
                                           const string_type&,
                                           int) const;
        template expected<std::ptrdiff_t> basic_custom_locale_ref<
            char>::read_num<unsigned long long>(unsigned long long&,
                                                const string_type&,
                                                int) const;
        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<char>::read_num<float>(float&,
                                                       const string_type&,
                                                       int) const;
        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<char>::read_num<double>(double&,
                                                        const string_type&,
                                                        int) const;
        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<char>::read_num<long double>(long double&,
                                                             const string_type&,
                                                             int) const;

        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<wchar_t>::read_num<short>(short&,
                                                          const string_type&,
                                                          int) const;
        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<wchar_t>::read_num<int>(int&,
                                                        const string_type&,
                                                        int) const;
        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<wchar_t>::read_num<long>(long&,
                                                         const string_type&,
                                                         int) const;
        template expected<std::ptrdiff_t> basic_custom_locale_ref<
            wchar_t>::read_num<long long>(long long&,
                                          const string_type&,
                                          int) const;
        template expected<std::ptrdiff_t> basic_custom_locale_ref<
            wchar_t>::read_num<unsigned short>(unsigned short&,
                                               const string_type&,
                                               int) const;
        template expected<std::ptrdiff_t> basic_custom_locale_ref<
            wchar_t>::read_num<unsigned int>(unsigned int&,
                                             const string_type&,
                                             int) const;
        template expected<std::ptrdiff_t> basic_custom_locale_ref<
            wchar_t>::read_num<unsigned long>(unsigned long&,
                                              const string_type&,
                                              int) const;
        template expected<std::ptrdiff_t> basic_custom_locale_ref<
            wchar_t>::read_num<unsigned long long>(unsigned long long&,
                                                   const string_type&,
                                                   int) const;
        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<wchar_t>::read_num<float>(float&,
                                                          const string_type&,
                                                          int) const;
        template expected<std::ptrdiff_t>
        basic_custom_locale_ref<wchar_t>::read_num<double>(double&,
                                                           const string_type&,
                                                           int) const;
        template expected<std::ptrdiff_t> basic_custom_locale_ref<
            wchar_t>::read_num<long double>(long double&,
                                            const string_type&,
                                            int) const;
#endif

    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
