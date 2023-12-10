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

#pragma once

#include <scn/detail/args.h>
#include <scn/detail/format_string.h>
#include <scn/detail/xchar.h>

#include <scn/impl/reader/bool_reader.h>
#include <scn/impl/reader/code_unit_and_point_reader.h>
#include <scn/impl/reader/float_reader.h>
#include <scn/impl/reader/integer_reader.h>
#include <scn/impl/reader/pointer_reader.h>
#include <scn/impl/reader/regex_reader.h>
#include <scn/impl/reader/string_reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
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
            else if constexpr (std::is_same_v<T, regex_matches> ||
                               std::is_same_v<T, wregex_matches>) {
                return reader_impl_for_regex_matches<CharT>{};
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

        template <typename Context>
        struct default_arg_reader {
            using context_type = Context;
            using char_type = typename context_type::char_type;
            using args_type = basic_scan_args<context_type>;

            using range_type = typename context_type::range_type;
            using iterator = ranges::iterator_t<range_type>;

            template <typename T>
            scan_expected<iterator> operator()(T& value)
            {
                if constexpr (!detail::is_type_disabled<T>) {
                    auto rd = make_reader<T, char_type>();
                    auto& buffer = *range.begin().parent();

                    if (SCN_UNLIKELY(!buffer.is_contiguous())) {
                        SCN_TRY(it, skip_ws_before_if_required(
                                        rd.skip_ws_before_read(), range, loc));
                        return rd.read_default(
                            ranges::subrange{it, ranges::end(range)}, value,
                            loc);
                    }

                    auto crange =
                        detail::to_contiguous_buffer_segment<char_type>(range);
                    SCN_TRY(nows_it,
                            skip_ws_before_if_required(rd.skip_ws_before_read(),
                                                       crange, loc));
                    SCN_TRY(rd_it,
                            rd.read_default(
                                ranges::subrange{nows_it, ranges::end(crange)},
                                value, loc));
                    return range.begin().unsafe_advance(
                        ranges::distance(crange.begin(), rd_it));
                }
                else {
                    SCN_EXPECT(false);
                    SCN_UNREACHABLE;
                }
            }

            scan_expected<iterator> operator()(
                typename basic_scan_arg<context_type>::handle h)
            {
                if constexpr (!detail::is_type_disabled<void>) {
                    basic_scan_parse_context<char_type> parse_ctx{{}};
                    context_type ctx{range.begin(), args, loc};
                    if (auto e = h.scan(parse_ctx, ctx); !e) {
                        return unexpected(e);
                    }

                    return {ctx.begin()};
                }
                else {
                    SCN_EXPECT(false);
                    SCN_UNREACHABLE;
                }
            }

            range_type range;
            args_type args;
            detail::locale_ref loc;
        };

        template <typename Context>
        struct arg_reader {
            using context_type = Context;
            using char_type = typename context_type::char_type;

            using range_type = typename context_type::range_type;
            using iterator = ranges::iterator_t<range_type>;

            template <typename T>
            scan_expected<iterator> operator()(T& value)
            {
                if constexpr (!detail::is_type_disabled<T>) {
                    auto& buffer = *range.begin().parent();
                    auto rd = make_reader<T, char_type>();
                    if (auto e = rd.check_specs(specs); !e) {
                        return unexpected(e);
                    }

                    auto impl = [&](auto&& range)
                        -> scan_expected<ranges::iterator_t<decltype(range)>> {
                        auto it = skip_ws_before_if_required(
                            rd.skip_ws_before_read(), range, loc);
                        if (SCN_UNLIKELY(!it)) {
                            return unexpected(it.error());
                        }

                        auto subr = ranges::subrange{*it, ranges::end(range)};
                        if (specs.width != 0) {
                            SCN_TRY(w_it,
                                    rd.read_specs(take_width(subr, specs.width),
                                                  specs, value, loc));
                            return w_it.base();
                        }

                        return rd.read_specs(subr, specs, value, loc);
                    };

                    if (SCN_UNLIKELY(!buffer.is_contiguous())) {
                        return impl(range);
                    }

                    auto crange =
                        detail::to_contiguous_buffer_segment<char_type>(range);
                    SCN_TRY(it, impl(crange));
                    return range.begin().unsafe_advance(
                        ranges::distance(crange.begin(), it));
                }
                else {
                    SCN_EXPECT(false);
                    SCN_UNREACHABLE;
                }
            }

            scan_expected<iterator> operator()(
                typename basic_scan_arg<context_type>::handle)
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

            range_type range;
            const detail::basic_format_specs<char_type>& specs;
            detail::locale_ref loc;
        };

        template <typename Context>
        struct custom_reader {
            using context_type = Context;
            using char_type = typename context_type::char_type;
            using parse_context_type =
                typename context_type::parse_context_type;
            using iterator = typename context_type::iterator;

            template <typename T>
            scan_expected<iterator> operator()(T&) const
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

            scan_expected<iterator> operator()(
                typename basic_scan_arg<context_type>::handle h) const
            {
                if (auto e = h.scan(parse_ctx, ctx); !e) {
                    return unexpected(e);
                }
                return {ctx.begin()};
            }

            parse_context_type& parse_ctx;
            context_type& ctx;
        };

    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
