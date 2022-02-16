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

#ifndef SCN_SCAN_LIST_H
#define SCN_SCAN_LIST_H

#include "common.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    /**
     * Adapts a `span` into a type that can be read into using \ref
     * scan_list. This way, potentially unnecessary dynamic memory
     * allocations can be avoided. To use as a parameter to \ref scan_list,
     * use \ref make_span_list_wrapper.
     *
     * \code{.cpp}
     * std::vector<int> buffer(8, 0);
     * scn::span<int> s = scn::make_span(buffer);
     *
     * auto wrapper = scn::span_list_wrapper<int>(s);
     * scn::scan_list("123 456", wrapper);
     * // s[0] == buffer[0] == 123
     * // s[1] == buffer[1] == 456
     * \endcode
     *
     * \see scan_list
     * \see make_span_list_wrapper
     */
    template <typename T>
    struct span_list_wrapper {
        using value_type = T;

        span_list_wrapper(span<T> s) : m_span(s) {}

        void push_back(T val)
        {
            SCN_EXPECT(n < max_size());
            m_span[n] = SCN_MOVE(val);
            ++n;
        }

        SCN_NODISCARD constexpr std::size_t size() const noexcept
        {
            return n;
        }
        SCN_NODISCARD constexpr std::size_t max_size() const noexcept
        {
            return m_span.size();
        }

        span<T> m_span;
        std::size_t n{0};
    };

    namespace detail {
        template <typename T>
        using span_list_wrapper_for =
            span_list_wrapper<typename decltype(make_span(
                SCN_DECLVAL(T&)))::value_type>;
    }

    /**
     * Adapts a contiguous buffer into a type containing a `span` that can
     * be read into using \ref scan_list.
     *
     * Example adapted from \ref span_list_wrapper:
     * \code{.cpp}
     * std::vector<int> buffer(8, 0);
     * scn::scan_list("123 456", scn::make_span_list_wrapper(buffer));
     * // s[0] == buffer[0] == 123
     * // s[1] == buffer[1] == 456
     * \endcode
     *
     * \see scan_list
     * \see span_list_wrapper
     */
    template <typename T>
    auto make_span_list_wrapper(T& s)
        -> temporary<detail::span_list_wrapper_for<T>>
    {
        auto _s = make_span(s);
        return temp(span_list_wrapper<typename decltype(_s)::value_type>(_s));
    }

    namespace detail {
        template <typename CharT>
        struct zero_value;
        template <>
        struct zero_value<char> : std::integral_constant<char, 0> {
        };
        template <>
        struct zero_value<wchar_t> : std::integral_constant<wchar_t, 0> {
        };

        template <typename WrappedRange, typename CharT>
        expected<CharT> read_single(WrappedRange& r, CharT)
        {
            return read_code_unit(r);
        }
        template <typename WrappedRange>
        expected<code_point> read_single(WrappedRange& r, code_point)
        {
            unsigned char buf[4] = {0};
            auto ret = read_code_point(r, make_span(buf, 4), true);
            if (!ret) {
                return ret.error();
            }
            return ret.value().cp;
        }
    }  // namespace detail

    /**
     * Reads values repeatedly from `r` and writes them into `c`.
     * The values read are of type `Container::value_type`, and they are
     * written into `c` using `c.push_back`.
     *
     * The values must be separated by separator
     * character `separator`, followed by whitespace. If `separator == 0`,
     * no separator character is expected.
     *
     * The range is read, until:
     *  - `c.max_size()` is reached, or
     *  - range `EOF` was reached, or
     *  - unexpected separator character was found between values.
     *
     * In all these cases, an error will not be returned, and the beginning
     * of the returned range will point to the first character after the
     * scanned list.
     *
     * To scan into `span`, use \ref span_list_wrapper.
     * \ref make_span_list_wrapper
     *
     * \code{.cpp}
     * std::vector<int> vec{};
     * auto result = scn::scan_list("123 456", vec);
     * // vec == [123, 456]
     * // result.empty() == true
     *
     * result = scn::scan_list("123, 456", vec, ',');
     * // vec == [123, 456]
     * // result.empty() == true
     * \endcode
     */
    template <typename Range,
              typename Container,
              typename Separator = typename detail::extract_char_type<
                  ranges::iterator_t<Range>>::type>
    SCN_NODISCARD auto scan_list(
        Range&& r,
        Container& c,
        Separator separator = detail::zero_value<Separator>::value)
        -> detail::scan_result_for_range<Range>
    {
        using value_type = typename Container::value_type;
        value_type value;

        auto range = wrap(SCN_FWD(r));
        using char_type = typename decltype(range)::char_type;

        auto args = make_args_for(range, 1, value);
        auto ctx = make_context(SCN_MOVE(range));
        auto pctx = make_parse_context(1, ctx.locale());
        auto cargs = basic_args<char_type>{args};

        while (true) {
            if (c.size() == c.max_size()) {
                break;
            }

            pctx.reset_args_left(1);
            auto err = visit(ctx, pctx, cargs);
            if (!err) {
                if (err == error::end_of_range) {
                    break;
                }
                return detail::wrap_result(wrapped_error{err},
                                           detail::range_tag<Range>{},
                                           SCN_MOVE(ctx.range()));
            }
            c.push_back(SCN_MOVE(value));

            if (separator != 0) {
                auto sep_ret = detail::read_single(ctx.range(), separator);
                if (!sep_ret) {
                    if (sep_ret.error() == scn::error::end_of_range) {
                        break;
                    }
                    return detail::wrap_result(wrapped_error{sep_ret.error()},
                                               detail::range_tag<Range>{},
                                               SCN_MOVE(ctx.range()));
                }
                if (sep_ret.value() == separator) {
                    continue;
                }
                else {
                    // Unexpected character, assuming end
                    break;
                }
            }
        }
        return detail::wrap_result(wrapped_error{}, detail::range_tag<Range>{},
                                   SCN_MOVE(ctx.range()));
    }

    /**
     * Otherwise equivalent to \ref scan_list, except with an additional
     * case of stopping scanning: if `until` is found where a separator was
     * expected.
     *
     * \see scan_list
     *
     * \code{.cpp}
     * std::vector<int> vec{};
     * auto result = scn::scan_list_until("123 456\n789", vec, '\n');
     * // vec == [123, 456]
     * // result.range() == "789"
     * \endcode
     */
    template <typename Range,
              typename Container,
              typename Separator = typename detail::extract_char_type<
                  ranges::iterator_t<Range>>::type>
    SCN_NODISCARD auto scan_list_until(
        Range&& r,
        Container& c,
        Separator until,
        Separator separator = detail::zero_value<Separator>::value)
        -> detail::scan_result_for_range<Range>
    {
        using value_type = typename Container::value_type;
        value_type value;

        auto range = wrap(SCN_FWD(r));
        using char_type = typename decltype(range)::char_type;

        auto args = make_args_for(range, 1, value);
        auto ctx = make_context(SCN_MOVE(range));

        bool scanning = true;
        while (scanning) {
            if (c.size() == c.max_size()) {
                break;
            }

            auto pctx = make_parse_context(1, ctx.locale());
            auto err = visit(ctx, pctx, basic_args<char_type>{args});
            if (!err) {
                if (err == error::end_of_range) {
                    break;
                }
                return detail::wrap_result(wrapped_error{err},
                                           detail::range_tag<Range>{},
                                           SCN_MOVE(ctx.range()));
            }
            c.push_back(SCN_MOVE(value));

            bool sep_found = false;
            while (true) {
                auto next = read_code_unit(ctx.range(), false);
                if (!next) {
                    if (next.error() == scn::error::end_of_range) {
                        scanning = false;
                        break;
                    }
                    return detail::wrap_result(wrapped_error{next.error()},
                                               detail::range_tag<Range>{},
                                               SCN_MOVE(ctx.range()));
                }

                if (next.value() == until) {
                    scanning = false;
                    break;
                }

                if (ctx.locale().get_static().is_space(next.value())) {
                    ctx.range().advance();
                    continue;
                }

                if (separator != 0) {
                    if (next.value() != separator || sep_found) {
                        break;
                    }
                    else {
                        ctx.range().advance();
                        sep_found = true;
                    }
                }
                else {
                    break;
                }
            }
        }
        return detail::wrap_result(wrapped_error{}, detail::range_tag<Range>{},
                                   SCN_MOVE(ctx.range()));
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif
