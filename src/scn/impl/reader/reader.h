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

#include <scn/impl/reader/bool_reader.h>
#include <scn/impl/reader/code_unit_and_point_reader.h>
#include <scn/impl/reader/float/reader.h>
#include <scn/impl/reader/integer/reader.h>
#include <scn/impl/reader/pointer_reader.h>
#include <scn/impl/reader/string/reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename Reader, typename Range>
        scan_expected<Range> skip_ws_before_if_required(Reader& reader,
                                                        Range& range,
                                                        bool is_specs_localized,
                                                        detail::locale_ref loc)
        {
            if (auto e = eof_check(range); !e) {
                return unexpected(e);
            }

            if (!reader.skip_ws_before_read()) {
                return range;
            }

            if (is_specs_localized || loc) {
                return skip_localized_whitespace(range, loc);
            }

            return skip_classic_whitespace(range);
        }

        template <typename Context>
        struct default_arg_reader {
            using context_type = Context;
            using char_type = typename context_type::char_type;
            using args_type = basic_scan_args<context_type>;
            using subrange_type = typename context_type::subrange_type;
            using iterator = typename context_type::iterator;

            template <typename T>
            scan_expected<iterator> operator()(T& value)
            {
                auto rd = reader<T, char_type>{};
                return skip_ws_before_if_required(rd, range, false, loc)
                    .and_then([&](auto rng) {
                        return rd.read_value_default(rng, value, loc);
                    });
            }

            scan_expected<iterator> operator()(
                typename basic_scan_arg<context_type>::handle h)
            {
                basic_scan_parse_context<char_type> parse_ctx{{}};
                context_type ctx{detail::map_subrange_to_context_range_type(
                                     detail::tag_type<char_type>{}, range),
                                 args, loc};
                if (auto e = h.scan(parse_ctx, ctx); !e) {
                    return unexpected(e);
                }
                return {ctx.range().begin()};
            }

            subrange_type range;
            args_type args;
            detail::locale_ref loc;
        };

        template <typename Context>
        struct arg_reader {
            using context_type = Context;
            using char_type = typename context_type::char_type;
            using subrange_type = typename context_type::subrange_type;
            using iterator = typename context_type::iterator;

            template <typename T>
            scan_expected<iterator> operator()(T& value)
            {
                auto rd = reader<T, char_type>{};
                if (auto e = rd.check_specs(specs); !e) {
                    return unexpected(e);
                }

                return skip_ws_before_if_required(rd, range, specs.localized,
                                                  loc)
                    .and_then([&](auto rng) {
                        // force formatting
                        return rd.read_value_specs(rng, specs, value, loc);
                    });
            }

            scan_expected<iterator> operator()(
                typename basic_scan_arg<context_type>::handle)
            {
                // Handled separately, because parse context access needed
                return {ranges::begin(range)};
            }

            subrange_type range;
            const detail::basic_format_specs<char_type>& specs;
            detail::locale_ref loc;
        };

        template <typename Context>
        struct custom_reader {
            using context_type = Context;
            using parse_context_type =
                typename context_type::parse_context_type;
            using char_type = typename context_type::char_type;
            using iterator = typename context_type::iterator;

            scan_expected<iterator> operator()(
                typename basic_scan_arg<context_type>::handle h) const
            {
                if (auto e = h.scan(parse_ctx, ctx); !e) {
                    return unexpected(e);
                }
                return {ctx.current()};
            }

            template <typename T>
            scan_expected<iterator> operator()(T&) const
            {
                return {ctx.current()};
            }

            parse_context_type& parse_ctx;
            context_type& ctx;
        };
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
