
.. _program_listing_file_include_scn_detail_scan.h:

Program Listing for File scan.h
===============================

|exhale_lsh| :ref:`Return to documentation for file <file_include_scn_detail_scan.h>` (``include/scn/detail/scan.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

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
   
   #ifndef SCN_DETAIL_SCAN_H
   #define SCN_DETAIL_SCAN_H
   
   #include <vector>
   
   #include "vscan.h"
   
   namespace scn {
       SCN_BEGIN_NAMESPACE
   
       namespace detail {
           template <typename Range, typename E = wrapped_error>
           struct scan_result_for_range {
               using type = scan_result<
                   typename detail::range_wrapper_for_t<Range>::return_type,
                   E>;
           };
           template <typename Range, typename E = wrapped_error>
           using scan_result_for_range_t =
               typename scan_result_for_range<Range, E>::type;
       }  // namespace detail
   
   
       // scan
   
       template <typename Range, typename Format, typename... Args>
       auto scan(Range&& r, const Format& f, Args&... a)
           -> detail::scan_result_for_range_t<Range>
       {
           static_assert(sizeof...(Args) > 0,
                         "Have to scan at least a single argument");
   
           using range_type = detail::range_wrapper_for_t<Range>;
           using context_type = basic_context<range_type>;
           using parse_context_type =
               basic_parse_context<typename context_type::locale_type>;
   
           auto args = make_args<context_type, parse_context_type>(a...);
           auto ctx = context_type(detail::wrap(std::forward<Range>(r)));
           auto pctx = parse_context_type(f, ctx);
           return vscan(ctx, pctx, {args});
       }
   
       // scan localized
   
       template <typename Locale,
                 typename Range,
                 typename Format,
                 typename... Args>
       auto scan_localized(const Locale& loc,
                           Range&& r,
                           const Format& f,
                           Args&... a) -> detail::scan_result_for_range_t<Range>
       {
           static_assert(sizeof...(Args) > 0,
                         "Have to scan at least a single argument");
   
           using range_type = detail::range_wrapper_for_t<Range>;
           using locale_type = basic_locale_ref<typename range_type::char_type>;
           using context_type = basic_context<range_type, locale_type>;
           using parse_context_type =
               basic_parse_context<typename context_type::locale_type>;
   
           auto args = make_args<context_type, parse_context_type>(a...);
           auto ctx = context_type(detail::wrap(std::forward<Range>(r)),
                                   {std::addressof(loc)});
           auto pctx = parse_context_type(f, ctx);
           return vscan(ctx, pctx, {args});
       }
   
       // default format
   
       template <typename Range, typename... Args>
       auto scan(Range&& r, detail::default_t, Args&... a)
           -> detail::scan_result_for_range_t<Range>
       {
           static_assert(sizeof...(Args) > 0,
                         "Have to scan at least a single argument");
   
           using range_type = detail::range_wrapper_for_t<Range>;
           using context_type = basic_context<range_type>;
           using parse_context_type =
               basic_empty_parse_context<typename context_type::locale_type>;
   
           auto args = make_args<context_type, parse_context_type>(a...);
           auto ctx = context_type(detail::wrap(std::forward<Range>(r)));
           auto pctx = parse_context_type(static_cast<int>(sizeof...(Args)), ctx);
           return vscan(ctx, pctx, {args});
       }
   
       // value
   
       template <typename T, typename Range>
       auto scan_value(Range&& r)
           -> detail::scan_result_for_range_t<Range, expected<T>>
       {
           using range_type = detail::range_wrapper_for_t<Range>;
           using context_type = basic_context<range_type>;
           using parse_context_type =
               basic_empty_parse_context<typename context_type::locale_type>;
   
           T value;
           auto args = make_args<context_type, parse_context_type>(value);
           auto ctx = context_type(detail::wrap(std::forward<Range>(r)));
   
   #if 0
           using char_type = typename context_type::char_type;
           auto e = skip_range_whitespace(ctx);
           if (!e) {
               ctx.range().reset_to_rollback_point();
               return {e, ctx.range().get_return()};
           }
   
           scanner<char_type, T> s{};
           e = s.scan(value, ctx);
           if (!e) {
               ctx.range().reset_to_rollback_point();
               return {e, ctx.range().get_return()};
           }
           return {std::move(value), ctx.range().get_return()};
   #else
           auto pctx = parse_context_type(1, ctx);
           auto ret = vscan(ctx, pctx, {args});
           if (!ret) {
               return {ret.error(), ret.range()};
           }
           return {value, ret.range()};
   #endif
       }
   
       // scanf
   
       template <typename Range, typename Format, typename... Args>
       auto scanf(Range&& r, const Format& f, Args&... a)
           -> detail::scan_result_for_range_t<Range>
       {
           static_assert(sizeof...(Args) > 0,
                         "Have to scan at least a single argument");
   
           using context_type = basic_context<detail::range_wrapper_for_t<Range>>;
           using parse_context_type =
               basic_scanf_parse_context<typename context_type::locale_type>;
   
           auto args = make_args<context_type, parse_context_type>(a...);
           auto ctx = context_type(detail::wrap(std::forward<Range>(r)));
           auto pctx = parse_context_type(f, ctx);
           return vscan(ctx, pctx, {args});
       }
   
       // input
   
       template <typename Format,
                 typename... Args,
                 typename CharT = detail::ranges::range_value_t<Format>>
       auto input(const Format& f, Args&... a) -> detail::scan_result_for_range_t<
           decltype(stdin_range<CharT>().lock())>
       {
           static_assert(sizeof...(Args) > 0,
                         "Have to scan at least a single argument");
   
           using context_type = basic_context<
               detail::range_wrapper_for_t<decltype(stdin_range<CharT>().lock())>>;
           using parse_context_type =
               basic_parse_context<typename context_type::locale_type>;
   
           auto args = make_args<context_type, parse_context_type>(a...);
           auto ctx = context_type(detail::wrap(stdin_range<CharT>().lock()));
           auto pctx = parse_context_type(f, ctx);
           return vscan(ctx, pctx, {args});
       }
   
       // prompt
   
       template <typename Format,
                 typename... Args,
                 typename CharT = detail::ranges::range_value_t<Format>>
       auto prompt(const CharT* p, const Format& f, Args&... a)
           -> detail::scan_result_for_range_t<
               decltype(stdin_range<CharT>().lock())>
       {
           static_assert(sizeof...(Args) > 0,
                         "Have to scan at least a single argument");
           SCN_EXPECT(p != nullptr);
   
           std::fputs(p, stdout);
   
           using context_type = basic_context<
               detail::range_wrapper_for_t<decltype(stdin_range<CharT>().lock())>>;
           using parse_context_type =
               basic_parse_context<typename context_type::locale_type>;
   
           auto args = make_args<context_type, parse_context_type>(a...);
           auto ctx = context_type(detail::wrap(stdin_range<CharT>().lock()));
           auto pctx = parse_context_type(f, ctx);
           return vscan(ctx, pctx, {args});
       }
   
       template <typename T, typename CharT>
       expected<const CharT*> parse_integer(basic_string_view<CharT> str,
                                            T& val,
                                            int base = 10)
       {
           SCN_EXPECT(!str.empty());
           auto s = scanner<CharT, T>{base};
           bool minus_sign = false;
           if (str[0] == detail::ascii_widen<CharT>('-')) {
               minus_sign = true;
           }
           auto ret = s._read_int(val, minus_sign,
                                  make_span(str.data(), str.size()).as_const(),
                                  detail::ascii_widen<CharT>('\0'));
           if (!ret) {
               return ret.error();
           }
           return {ret.value()};
       }
   
       // scanning api
   
       // getline
   
       namespace detail {
           template <typename WrappedRange, typename String, typename CharT>
           auto getline_impl(WrappedRange& r, String& str, CharT until)
               -> detail::scan_result_for_range_t<WrappedRange, wrapped_error>
           {
               auto until_pred = [until](CharT ch) { return ch == until; };
               auto s = read_until_space_zero_copy(r, until_pred, true);
               if (!s) {
                   return {std::move(s.error()), r.get_return()};
               }
               if (s.value().size() != 0) {
                   auto size = s.value().size();
                   if (until_pred(s.value()[size - 1])) {
                       --size;
                   }
                   str.clear();
                   str.resize(size);
                   std::copy(s.value().begin(), s.value().begin() + size,
                             str.begin());
                   return {{}, r.get_return()};
               }
   
               String tmp;
               auto out = std::back_inserter(tmp);
               auto e = read_until_space(r, out, until_pred, true);
               if (!e) {
                   return {std::move(e), r.get_return()};
               }
               if (until_pred(tmp.back())) {
                   tmp.pop_back();
               }
               str = std::move(tmp);
               return {{}, r.get_return()};
           }
           template <typename WrappedRange, typename CharT>
           auto getline_impl(WrappedRange& r,
                             basic_string_view<CharT>& str,
                             CharT until)
               -> detail::scan_result_for_range_t<WrappedRange, wrapped_error>
           {
               auto until_pred = [until](CharT ch) { return ch == until; };
               auto s = read_until_space_zero_copy(r, until_pred, true);
               if (!s) {
                   return {std::move(s.error()), r.get_return()};
               }
               if (s.value().size() != 0) {
                   auto size = s.value().size();
                   if (until_pred(s.value()[size - 1])) {
                       --size;
                   }
                   str = basic_string_view<CharT>{s.value().data(), size};
                   return {{}, r.get_return()};
               }
               // TODO: Compile-time error?
               return {
                   error(
                       error::invalid_operation,
                       "Cannot getline a string_view from a non-contiguous range"),
                   r.get_return()};
           }
   #if SCN_HAS_STRING_VIEW
           template <typename WrappedRange, typename CharT>
           auto getline_impl(WrappedRange& r,
                             std::basic_string_view<CharT>& str,
                             CharT until)
               -> detail::scan_result_for_range_t<WrappedRange, wrapped_error>
           {
               auto sv = ::scn::basic_string_view<CharT>{};
               auto ret = getline_impl(r, sv, until);
               str = ::std::basic_string_view<CharT>{sv.data(), sv.size()};
               return ret;
           }
   #endif
       }  // namespace detail
   
       template <typename Range, typename String, typename CharT>
       auto getline(Range&& r, String& str, CharT until)
           -> decltype(detail::getline_impl(
               std::declval<decltype(detail::wrap(std::forward<Range>(r)))&>(),
               str,
               until))
       {
           auto wrapped = detail::wrap(std::forward<Range>(r));
           return getline_impl(wrapped, str, until);
       }
   
       template <typename Range,
                 typename String,
                 typename CharT =
                     typename detail::extract_char_type<detail::ranges::iterator_t<
                         detail::range_wrapper_for_t<Range>>>::type>
       auto getline(Range&& r, String& str) -> decltype(
           getline(std::forward<Range>(r), str, detail::ascii_widen<CharT>('\n')))
       {
           return getline(std::forward<Range>(r), str,
                          detail::ascii_widen<CharT>('\n'));
       }
   
       // ignore
   
       namespace detail {
           template <typename CharT>
           struct ignore_iterator {
               using value_type = CharT;
               using pointer = value_type*;
               using reference = value_type&;
               using difference_type = std::ptrdiff_t;
               using iterator_category = std::output_iterator_tag;
   
               constexpr ignore_iterator() = default;
   
               constexpr const ignore_iterator& operator=(CharT) const noexcept
               {
                   return *this;
               }
   
               constexpr const ignore_iterator& operator*() const noexcept
               {
                   return *this;
               }
               constexpr const ignore_iterator& operator++() const noexcept
               {
                   return *this;
               }
           };
   
           template <typename CharT>
           struct ignore_iterator_n {
               using value_type = CharT;
               using pointer = value_type*;
               using reference = value_type&;
               using difference_type = std::ptrdiff_t;
               using iterator_category = std::output_iterator_tag;
   
               ignore_iterator_n() = default;
               ignore_iterator_n(difference_type n) : i(n) {}
   
               constexpr const ignore_iterator_n& operator=(CharT) const noexcept
               {
                   return *this;
               }
   
               constexpr const ignore_iterator_n& operator*() const noexcept
               {
                   return *this;
               }
   
               SCN_CONSTEXPR14 ignore_iterator_n& operator++() noexcept
               {
                   ++i;
                   return *this;
               }
   
               constexpr bool operator==(const ignore_iterator_n& o) const noexcept
               {
                   return i == o.i;
               }
               constexpr bool operator!=(const ignore_iterator_n& o) const noexcept
               {
                   return !(*this == o);
               }
   
               difference_type i{0};
           };
   
           template <typename WrappedRange,
                     typename CharT = typename detail::extract_char_type<
                         detail::range_wrapper_for_t<
                             typename WrappedRange::iterator>>::type>
           auto ignore_until_impl(WrappedRange& r, CharT until)
               -> scan_result<WrappedRange, wrapped_error>
           {
               auto until_pred = [until](CharT ch) { return ch == until; };
               ignore_iterator<CharT> it{};
               auto e = read_until_space(r, it, until_pred, false);
               if (!e) {
                   return {std::move(e), r.get_return()};
               }
               return {{}, r.get_return()};
           }
   
           template <typename WrappedRange,
                     typename CharT = typename detail::extract_char_type<
                         detail::range_wrapper_for_t<
                             typename WrappedRange::iterator>>::type>
           auto ignore_until_n_impl(WrappedRange& r,
                                    ranges::range_difference_t<WrappedRange> n,
                                    CharT until)
               -> scan_result<WrappedRange, wrapped_error>
           {
               auto until_pred = [until](CharT ch) { return ch == until; };
               ignore_iterator_n<CharT> begin{}, end{n};
               auto e = read_until_space_ranged(r, begin, end, until_pred, false);
               if (!e) {
                   return {std::move(e), r.get_return()};
               }
               return {{}, r.get_return()};
           }
       }  // namespace detail
   
       template <typename Range, typename CharT>
       auto ignore_until(Range&& r, CharT until)
           -> decltype(detail::ignore_until_impl(
               std::declval<decltype(detail::wrap(std::forward<Range>(r)))&>(),
               until))
       {
           auto wrapped = detail::wrap(std::forward<Range>(r));
           auto ret = detail::ignore_until_impl(wrapped, until);
           if (!ret) {
               auto e = wrapped.reset_to_rollback_point();
               if (!e) {
                   return {std::move(e), wrapped.get_return()};
               }
           }
           return ret;
       }
   
       template <typename Range, typename CharT>
       auto ignore_until_n(Range&& r,
                           detail::ranges::range_difference_t<Range> n,
                           CharT until)
           -> decltype(detail::ignore_until_n_impl(
               std::declval<decltype(detail::wrap(std::forward<Range>(r)))&>(),
               n,
               until))
       {
           auto wrapped = detail::wrap(std::forward<Range>(r));
           auto ret = detail::ignore_until_n_impl(wrapped, n, until);
           if (!ret) {
               auto e = wrapped.reset_to_rollback_point();
               if (!e) {
                   return {std::move(e), wrapped.get_return()};
               }
           }
           return ret;
       }
   
       template <typename T>
       struct span_list_wrapper {
           using value_type = T;
   
           span_list_wrapper(span<T> s) : m_span(s) {}
   
           void push_back(T val)
           {
               SCN_EXPECT(n < max_size());
               m_span[n] = std::move(val);
               ++n;
           }
   
           std::size_t size() const
           {
               return n;
           }
           std::size_t max_size() const
           {
               return m_span.size();
           }
   
           span<T> m_span;
           std::size_t n{0};
       };
       template <typename T>
       auto make_span_list_wrapper(T& s) -> temporary<
           span_list_wrapper<typename decltype(make_span(s))::value_type>>
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
       }  // namespace detail
   
       template <typename Range,
                 typename Container,
                 typename CharT = typename detail::extract_char_type<
                     detail::ranges::iterator_t<Range>>::type>
       auto scan_list(Range&& r,
                      Container& c,
                      CharT separator = detail::zero_value<CharT>::value)
           -> detail::scan_result_for_range_t<Range, wrapped_error>
       {
           using value_type = typename Container::value_type;
           using range_type = detail::range_wrapper_for_t<Range>;
           using context_type = basic_context<range_type>;
           using parse_context_type =
               basic_empty_parse_context<typename context_type::locale_type>;
   
           value_type value;
           auto args = make_args<context_type, parse_context_type>(value);
           auto ctx = context_type(detail::wrap(std::forward<Range>(r)));
   
           while (true) {
               if (c.size() == c.max_size()) {
                   break;
               }
   
               auto pctx = parse_context_type(1, ctx);
               auto ret = vscan(ctx, pctx, {args});
               if (!ret) {
                   if (ret.error() == error::end_of_range) {
                       break;
                   }
                   return {ret.error(), ctx.range().get_return()};
               }
               c.push_back(std::move(value));
   
               if (separator != 0) {
                   auto sep_ret = read_char(ctx.range());
                   if (!sep_ret) {
                       if (sep_ret.error() == scn::error::end_of_range) {
                           break;
                       }
                       return {sep_ret.error(), ctx.range().get_return()};
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
           return {{}, ctx.range().get_return()};
       }
   
       template <typename Range,
                 typename Container,
                 typename CharT = typename detail::extract_char_type<
                     detail::ranges::iterator_t<Range>>::type>
       auto scan_list_until(Range&& r,
                            Container& c,
                            CharT until,
                            CharT separator = detail::zero_value<CharT>::value)
           -> detail::scan_result_for_range_t<Range, wrapped_error>
       {
           using value_type = typename Container::value_type;
           using range_type = detail::range_wrapper_for_t<Range>;
           using context_type = basic_context<range_type>;
           using parse_context_type =
               basic_empty_parse_context<typename context_type::locale_type>;
   
           value_type value;
           auto args = make_args<context_type, parse_context_type>(value);
           auto ctx = context_type(detail::wrap(std::forward<Range>(r)));
   
           while (true) {
               if (c.size() == c.max_size()) {
                   break;
               }
   
               auto pctx = parse_context_type(1, ctx);
               auto ret = vscan(ctx, pctx, {args});
               if (!ret) {
                   if (ret.error() == error::end_of_range) {
                       break;
                   }
                   return {ret.error(), ctx.range().get_return()};
               }
               c.push_back(std::move(value));
   
               {
                   auto next = read_char(ctx.range());
                   if (!next) {
                       if (next.error() == scn::error::end_of_range) {
                           break;
                       }
                       return {next.error(), ctx.range().get_return()};
                   }
                   if (next.value() == until) {
                       break;
                   }
                   if (separator != 0) {
                       if (next.value() != separator) {
                           break;
                       }
                   } else {
                       if (!ctx.locale().is_space(next.value())) {
                           break;
                       }
                   }
                   next = read_char(ctx.range());
                   if (next.value() == until) {
                       break;
                   } else {
                       putback_n(ctx.range(), 1);
                   }
               }
           }
           return {{}, ctx.range().get_return()};
       }
   
       template <typename T>
       struct discard_type {
           discard_type() = default;
       };
   
       template <typename T>
       discard_type<T>& discard()
       {
           return temp(discard_type<T>{})();
       }
   
       template <typename CharT, typename T>
       struct scanner<CharT, discard_type<T>> : public scanner<CharT, T> {
           template <typename Context>
           error scan(discard_type<T>&, Context& ctx)
           {
               T tmp;
               return scanner<CharT, T>::scan(tmp, ctx);
           }
       };
   
       SCN_END_NAMESPACE
   }  // namespace scn
   
   #endif  // SCN_DETAIL_SCAN_H
