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

#ifndef SCN_OPTIONS_H
#define SCN_OPTIONS_H

#include "config.h"

#include <type_traits>

#ifndef SCN_DEFAULT_INT_SCANNING_METHOD
#define SCN_DEFAULT_INT_SCANNING_METHOD ::scn::method::custom
#endif

#ifndef SCN_DEFAULT_FLOAT_SCANNING_METHOD
#define SCN_DEFAULT_FLOAT_SCANNING_METHOD ::scn::method::strto
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    enum class method {
        sto,         // std::stoi, std::stod, etc.
        from_chars,  // std::from_chars
        strto,       // strtol, strtod, etc.
        custom       // custom hand-written
    };

    // std::from_chars availibility
    SCN_CONSTEXPR inline bool is_int_from_chars_available() noexcept
    {
        return SCN_HAS_INTEGER_CHARCONV;
    }
    SCN_CONSTEXPR inline bool is_float_from_chars_available() noexcept
    {
        return SCN_HAS_FLOAT_CHARCONV;
    }

    SCN_CONSTEXPR inline method int_from_chars_if_available(
        method fallback = SCN_DEFAULT_INT_SCANNING_METHOD) noexcept
    {
        return is_int_from_chars_available() ? method::from_chars : fallback;
    }
    SCN_CONSTEXPR inline method float_from_chars_if_available(
        method fallback = SCN_DEFAULT_FLOAT_SCANNING_METHOD) noexcept
    {
        return is_float_from_chars_available() ? method::from_chars : fallback;
    }

    template <typename T>
    SCN_CONSTEXPR auto from_chars_if_available(
        method fallback = SCN_DEFAULT_INT_SCANNING_METHOD) noexcept ->
        typename std::enable_if<std::is_integral<T>::value, method>::type
    {
        return int_from_chars_if_available(fallback);
    }
    template <typename T>
    SCN_CONSTEXPR auto from_chars_if_available(
        method fallback = SCN_DEFAULT_FLOAT_SCANNING_METHOD) noexcept ->
        typename std::enable_if<std::is_floating_point<T>::value, method>::type
    {
        return float_from_chars_if_available(fallback);
    }

    template <typename CharT>
    class basic_locale_ref;
    template <typename CharT>
    class basic_default_locale_ref;

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")

    struct options {
        struct builder;

        SCN_CONSTEXPR options() noexcept = default;
        SCN_CONSTEXPR options(builder b) noexcept;

        template <typename CharT>
        basic_locale_ref<CharT> get_locale_ref() const noexcept
        {
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
            if (!locale) {
                return basic_locale_ref<CharT>();
            }
            return basic_locale_ref<CharT>(locale);
            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
        }

        template <typename T>
        SCN_CONSTEXPR auto get_method_for() const noexcept ->
            typename std::enable_if<std::is_integral<T>::value, method>::type
        {
            return int_method;
        }
        template <typename T>
        SCN_CONSTEXPR auto get_method_for() const noexcept ->
            typename std::enable_if<std::is_floating_point<T>::value,
                                    method>::type
        {
            return float_method;
        }

        // Type-erase locale to avoid including <locale>
        const void* locale{nullptr};
        method int_method{SCN_DEFAULT_INT_SCANNING_METHOD};
        method float_method{SCN_DEFAULT_FLOAT_SCANNING_METHOD};
    };

    SCN_CLANG_POP

    struct options::builder {
        template <typename T>
        SCN_CONSTEXPR14 builder& locale(const T& l) noexcept
        {
            opt.locale = &l;
            return *this;
        }
        SCN_CONSTEXPR14 builder& int_method(method m) noexcept
        {
            opt.int_method = m;
            return *this;
        }
        SCN_CONSTEXPR14 builder& float_method(method m) noexcept
        {
            opt.float_method = m;
            return *this;
        }

        SCN_CONSTEXPR14 options make() noexcept
        {
            return static_cast<options&&>(opt);
        }

        options opt{};
    };

    inline SCN_CONSTEXPR options::options(options::builder b) noexcept
        : locale(b.opt.locale),
          int_method(b.opt.int_method),
          float_method(b.opt.float_method)
    {
    }

    struct default_options {
        template <typename CharT>
        SCN_CONSTEXPR basic_default_locale_ref<CharT> get_locale_ref() const
            noexcept
        {
            return basic_default_locale_ref<CharT>();
        }

        template <typename T>
        SCN_CONSTEXPR auto get_method_for() const noexcept ->
            typename std::enable_if<std::is_integral<T>::value, method>::type
        {
            return SCN_DEFAULT_INT_SCANNING_METHOD;
        }
        template <typename T>
        SCN_CONSTEXPR auto get_method_for() const noexcept ->
            typename std::enable_if<std::is_floating_point<T>::value,
                                    method>::type
        {
            return SCN_DEFAULT_FLOAT_SCANNING_METHOD;
        }
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_OPTIONS_H
