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

#include <scn/impl/reader/reader.h>

#include <scn/impl/reader/bool_reader.h>
#include <scn/impl/reader/code_unit_and_point_reader.h>
#include <scn/impl/reader/float_reader.h>
#include <scn/impl/reader/integer_reader.h>
#include <scn/impl/reader/pointer_reader.h>
#include <scn/impl/reader/string_reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        namespace {
            template <typename Range>
            scan_expected<simple_borrowed_iterator_t<Range>>
            skip_ws_before_if_required(bool is_required,
                                       Range&& range,
                                       detail::locale_ref loc)
            {
                if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
                    return unexpected(e);
                }

                if (!is_required) {
                    return ranges::begin(range);
                }

                return skip_classic_whitespace(SCN_FWD(range));
            }

            template <typename T, typename CharT>
            constexpr auto make_reader()
            {
                if constexpr (std::is_same_v<T, bool>) {
                    return reader_impl_for_bool<CharT>{};
                }
                else if constexpr (std::is_same_v<T, char>) {
                    return reader_impl_for_char<CharT>{};
                }
                else if constexpr (std::is_same_v<T, wchar_t>) {
                    return reader_impl_for_wchar<CharT>{};
                }
                else if constexpr (std::is_same_v<T, char32_t>) {
                    return reader_impl_for_code_point<CharT>{};
                }
                else if constexpr (std::is_same_v<T, std::string_view> ||
                                   std::is_same_v<T, std::wstring_view>) {
                    return reader_impl_for_string<CharT>{};
                }
                else if constexpr (std::is_same_v<T, std::string> ||
                                   std::is_same_v<T, std::wstring>) {
                    return reader_impl_for_string<CharT>{};
                }
                else if constexpr (std::is_same_v<T, void*>) {
                    return reader_impl_for_voidptr<CharT>{};
                }
                else if constexpr (std::is_floating_point_v<T>) {
                    return reader_impl_for_float<CharT>{};
                }
                else if constexpr (std::is_integral_v<T> &&
                                   !std::is_same_v<T, char> &&
                                   !std::is_same_v<T, wchar_t> &&
                                   !std::is_same_v<T, char32_t> &&
                                   !std::is_same_v<T, bool>) {
                    return reader_impl_for_int<CharT>{};
                }
                else {
                    return reader_impl_for_monostate<CharT>{};
                }
            }

        }  // namespace

        template <typename Context>
        template <typename T>
        auto default_arg_reader<Context>::operator()(T& value)
            -> scan_expected<iterator>
        {
            if constexpr (!detail::is_type_disabled<T>) {
                auto rd = make_reader<T, char_type>();
                SCN_TRY(it, skip_ws_before_if_required(rd.skip_ws_before_read(),
                                                       range, loc));
                return rd.read_default(ranges::subrange{it, ranges::end(range)},
                                       value, loc);
            }
            else {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }
        }

        template <typename Context>
        auto default_arg_reader<Context>::operator()(
            typename basic_scan_arg<context_type>::handle h)
            -> scan_expected<iterator>
        {
            basic_scan_parse_context<char_type> parse_ctx{{}};
            context_type ctx{range, args, loc};
            if (auto e = h.scan(parse_ctx, ctx); !e) {
                return unexpected(e);
            }
            return {ctx.range().begin()};
        }

        template <typename Context>
        template <typename T>
        auto arg_reader<Context>::operator()(T& value)
            -> scan_expected<iterator>
        {
            if constexpr (!detail::is_type_disabled<T>) {
                auto rd = make_reader<T, char_type>();
                if (auto e = rd.check_specs(specs); !e) {
                    return unexpected(e);
                }

                auto it = skip_ws_before_if_required(rd.skip_ws_before_read(),
                                                     range, loc);
                if (SCN_UNLIKELY(!it)) {
                    return unexpected(it.error());
                }

                auto subr = ranges::subrange{*it, ranges::end(range)};
                if (specs.width != 0) {
                    SCN_TRY(w_it, rd.read_specs(take_width(subr, specs.width),
                                                specs, value, loc));
                    return w_it.base();
                }

                return rd.read_specs(subr, specs, value, loc);
            }
            else {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }
        }

        template <typename Context>
        auto custom_reader<Context>::operator()(
            typename basic_scan_arg<context_type>::handle h) const
            -> scan_expected<iterator>
        {
            if (auto e = h.scan(parse_ctx, ctx); !e) {
                return unexpected(e);
            }
            return {ctx.current()};
        }

#define SCN_DEFINE_READER_T(Ctx, T)                       \
    template auto default_arg_reader<Ctx>::operator()(T&) \
        -> scan_expected<iterator>;                       \
    template auto arg_reader<Ctx>::operator()(T&) -> scan_expected<iterator>;

#define SCN_DEFINE_READER_CUSTOM(Ctx)                        \
    template auto default_arg_reader<Ctx>::operator()(       \
        typename basic_scan_arg<context_type>::handle)       \
        -> scan_expected<iterator>;                          \
    template auto custom_reader<Ctx>::operator()(            \
        typename basic_scan_arg<context_type>::handle) const \
        -> scan_expected<iterator>;

        SCN_DEFINE_READER_CUSTOM(detail::scanner_scan_contexts::sv)
        SCN_DEFINE_READER_CUSTOM(detail::scanner_scan_contexts::wsv)
#if !SCN_DISABLE_ERASED_RANGE
        SCN_DEFINE_READER_CUSTOM(detail::scanner_scan_contexts::es)
        SCN_DEFINE_READER_CUSTOM(detail::scanner_scan_contexts::wes)
#endif
#if !SCN_DISABLE_IOSTREAM
        SCN_DEFINE_READER_CUSTOM(detail::scanner_scan_contexts::is)
        SCN_DEFINE_READER_CUSTOM(detail::scanner_scan_contexts::wis)
#endif

#define SCN_DEFINE_READER(Ctx)                   \
    SCN_DEFINE_READER_T(Ctx, signed char)        \
    SCN_DEFINE_READER_T(Ctx, short)              \
    SCN_DEFINE_READER_T(Ctx, int)                \
    SCN_DEFINE_READER_T(Ctx, long)               \
    SCN_DEFINE_READER_T(Ctx, long long)          \
    SCN_DEFINE_READER_T(Ctx, unsigned char)      \
    SCN_DEFINE_READER_T(Ctx, unsigned short)     \
    SCN_DEFINE_READER_T(Ctx, unsigned int)       \
    SCN_DEFINE_READER_T(Ctx, unsigned long)      \
    SCN_DEFINE_READER_T(Ctx, unsigned long long) \
    SCN_DEFINE_READER_T(Ctx, float)              \
    SCN_DEFINE_READER_T(Ctx, double)             \
    SCN_DEFINE_READER_T(Ctx, long double)        \
    SCN_DEFINE_READER_T(Ctx, char)               \
    SCN_DEFINE_READER_T(Ctx, wchar_t)            \
    SCN_DEFINE_READER_T(Ctx, char32_t)           \
    SCN_DEFINE_READER_T(Ctx, bool)               \
    SCN_DEFINE_READER_T(Ctx, void*)              \
    SCN_DEFINE_READER_T(Ctx, std::string)        \
    SCN_DEFINE_READER_T(Ctx, std::wstring)       \
    SCN_DEFINE_READER_T(Ctx, std::string_view)   \
    SCN_DEFINE_READER_T(Ctx, std::wstring_view)  \
    SCN_DEFINE_READER_T(Ctx, monostate)

        SCN_DEFINE_READER(detail::scanner_scan_contexts::sv)
        SCN_DEFINE_READER(detail::scanner_scan_contexts::wsv)
#if !SCN_DISABLE_ERASED_RANGE
        SCN_DEFINE_READER(detail::scanner_scan_contexts::es)
        SCN_DEFINE_READER(detail::scanner_scan_contexts::wes)
#endif
#if !SCN_DISABLE_IOSTREAM
        SCN_DEFINE_READER(detail::scanner_scan_contexts::is)
        SCN_DEFINE_READER(detail::scanner_scan_contexts::wis)
#endif

#undef SCN_DEFINE_READER_T
#undef SCN_DEFINE_READER_CUSTOM
#undef SCN_DEFINE_READER

    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
